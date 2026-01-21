#define MS_CLASS "RTC::ICE::StunPacket"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/ICE/StunPacket.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "Utils.hpp"
#include <cstdio>  // std::snprintf()
#include <cstring> // std::memcmp(), std::memcpy()

namespace RTC
{
	namespace ICE
	{
		/* Class variables. */

		const uint8_t StunPacket::MagicCookie[] = { 0x21, 0x12, 0xA4, 0x42 };

		/* Class methods. */

		bool StunPacket::IsStun(const uint8_t* buffer, size_t bufferLength)
		{
			// clang-format off
			return (
				// STUN headers are 20 bytes.
				(bufferLength >= StunPacket::FixedHeaderLength) &&
				// @see RFC 7983.
				(buffer[0] < 3) &&
				// Magic cookie must match.
				(buffer[4] == StunPacket::MagicCookie[0]) && (buffer[5] == StunPacket::MagicCookie[1]) &&
				(buffer[6] == StunPacket::MagicCookie[2]) && (buffer[7] == StunPacket::MagicCookie[3])
			);
			// clang-format on
		}

		StunPacket* StunPacket::Parse(const uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			if (!StunPacket::IsStun(buffer, bufferLength))
			{
				MS_WARN_TAG(ice, "not a STUN Packet");

				return nullptr;
			}

			auto* packet = new StunPacket(const_cast<uint8_t*>(buffer), bufferLength);

			// `bufferLength` must be the exact length of the STUN Packet, so let's
			// assign it immediately.
			packet->SetLength(bufferLength);

			if (!packet->Validate())
			{
				delete packet;
				return nullptr;
			}

			return packet;
		}

		/* Instance methods. */

		StunPacket::StunPacket(uint8_t* buffer, size_t bufferLength)
		  : Serializable(buffer, bufferLength)
		{
			MS_TRACE();

			SetLength(StunPacket::FixedHeaderLength);
		}

		StunPacket::~StunPacket()
		{
			MS_TRACE();
		}

		void StunPacket::Dump(int indentation) const
		{
			MS_TRACE();

			MS_DUMP_CLEAN(indentation, "<RTC::ICE::StunPacket>");

			MS_DUMP_CLEAN(indentation, "  length: %zu (buffer length: %zu)", GetLength(), GetBufferLength());

			std::string klass;

			switch (this->klass)
			{
				case Class::REQUEST:
				{
					klass = "request";
					break;
				}
				case Class::INDICATION:
				{
					klass = "indication";
					break;
				}
				case Class::SUCCESS_RESPONSE:
				{
					klass = "success response";
					break;
				}
				case Class::ERROR_RESPONSE:
				{
					klass = "error response";
					break;
				}
			}

			if (this->method == Method::BINDING)
			{
				MS_DUMP_CLEAN(indentation, "  method & class: binding %s", klass.c_str());
			}
			else
			{
				// This prints the unknown method number. Example: TURN Allocate => 0x003.
				MS_DUMP_CLEAN(
				  indentation,
				  "  method & class: %s with unknown method %#.3x",
				  klass.c_str(),
				  static_cast<uint16_t>(this->method));
			}

			auto transactionId1 = Utils::Byte::Get4Bytes(GetTransactionId(), 0);
			auto transactionId2 = Utils::Byte::Get8Bytes(GetTransactionId(), 4);

			MS_DUMP_CLEAN(indentation, "  transaction id (first 4 bytes): %" PRIu32, transactionId1);
			MS_DUMP_CLEAN(indentation, "  transaction id (last 8 bytes): %" PRIu64, transactionId2);

			if (HasUsername())
			{
				MS_DUMP_CLEAN(indentation, "  username: %s", GetUsername().c_str());
			}

			if (HasPriority())
			{
				MS_DUMP_CLEAN(indentation, "  priority: %" PRIu32, GetPriority());
			}

			if (HasIceControlling())
			{
				MS_DUMP_CLEAN(indentation, "  ice controlling: %" PRIu64, GetIceControlling());
			}

			if (HasIceControlled())
			{
				MS_DUMP_CLEAN(indentation, "  ice controlled: %" PRIu64, GetIceControlled());
			}

			if (HasUseCandidate())
			{
				MS_DUMP_CLEAN(indentation, "  use candidate: yes");
			}

			if (HasNomination())
			{
				MS_DUMP_CLEAN(indentation, "  nomination: %" PRIu32, GetNomination());
			}

			if (HasSoftware())
			{
				MS_DUMP_CLEAN(indentation, "  software: %s", GetSoftware().c_str());
			}

			if (HasXorMappedAddress())
			{
				int family;
				uint16_t port;
				std::string ip;

				Utils::IP::GetAddressInfo(this->xorMappedAddress, family, ip, port);

				MS_DUMP_CLEAN(indentation, "  xor mapped address: %s : %" PRIu16, ip.c_str(), port);
			}

			if (HasErrorCode())
			{
				MS_DUMP_CLEAN(indentation, "  error code: %" PRIu16, GetErrorCode());
			}

			if (HasMessageIntegrity())
			{
				char messageIntegrity[41];

				for (int i{ 0 }; i < 20; ++i)
				{
					std::snprintf(messageIntegrity + (i * 2), 3, "%.2x", this->messageIntegrity[i]);
				}

				MS_DUMP_CLEAN(indentation, "  message integrity: %s", messageIntegrity);
			}

			if (HasFingerprint())
			{
				MS_DUMP_CLEAN(indentation, "  fingerprint: yes");
			}

			MS_DUMP_CLEAN(indentation, "<RTC::ICE::StunPacket>");
		}

		StunPacket* StunPacket::Clone(uint8_t* buffer, size_t bufferLength) const
		{
			MS_TRACE();

			auto* clonedPacket = new StunPacket(buffer, bufferLength);

			Serializable::CloneInto(clonedPacket);

			// Clone private members.
			clonedPacket->klass            = this->klass;
			clonedPacket->method           = this->method;
			clonedPacket->username         = this->username;
			clonedPacket->priority         = this->priority;
			clonedPacket->iceControlling   = this->iceControlling;
			clonedPacket->iceControlled    = this->iceControlled;
			clonedPacket->hasUseCandidate  = this->hasUseCandidate;
			clonedPacket->nomination       = this->nomination;
			clonedPacket->software         = this->software;
			clonedPacket->xorMappedAddress = this->xorMappedAddress;
			clonedPacket->errorCode        = this->errorCode;
			clonedPacket->messageIntegrity = this->messageIntegrity;
			clonedPacket->hasFingerprint   = this->hasFingerprint;
			clonedPacket->password         = this->password;

			return clonedPacket;
		}

		bool StunPacket::Validate()
		{
			MS_TRACE();

			const auto* fixedHeader = GetFixedHeaderPointer();

			// Get STUN Message Type field.
			const uint16_t msgType = Utils::Byte::Get2Bytes(fixedHeader, 0);

			// Get Message Length field.
			const uint16_t msgLength = Utils::Byte::Get2Bytes(fixedHeader, 2);

			// Message Length field must be total length minus header's 20 bytes, and
			// must be multiple of 4 Bytes.
			// NOTE: Message Length is effectively the total length of the attributes
			// (with all paddings).
			if (
			  static_cast<size_t>(msgLength) != GetLength() - StunPacket::FixedHeaderLength ||
			  !Utils::Byte::IsPaddedTo4Bytes(msgLength))
			{
				MS_WARN_TAG(
				  ice,
				  "invalid Packet, Message Length field (%zu) does not match given buffer length or it's not multiple of 4 bytes",
				  StunPacket::FixedHeaderLength);

				return false;
			}

			// Get STUN class.
			const auto msgClass = static_cast<StunPacket::Class>(
			  ((fixedHeader[0] & 0x01) << 1) | ((fixedHeader[1] & 0x10) >> 4));

			// Get STUN method.
			const auto msgMethod = static_cast<StunPacket::Method>(
			  (msgType & 0x000f) | ((msgType & 0x00e0) >> 1) | ((msgType & 0x3E00) >> 2));

			SetClass(msgClass);
			SetMethod(msgMethod);

			/* Start looking for attributes. */

			const uint8_t* attributesStart = GetAttributesPointer();
			const uint8_t* attributesEnd   = attributesStart + msgLength;
			uint8_t* ptr                   = const_cast<uint8_t*>(attributesStart);

			// Offset to the beginning of the FINGERPRINT attribute (to its `type`
			// field) computed from the beginning of the Packet.
			size_t fingerprintAttrOffset{ 0 };
			// Holds the value of the FINGERPRINT attribute.
			uint32_t fingerprint{ 0 };

			// Ensure there are at least 4 remaining bytes (attribute with 0 length).
			while (ptr + 4 <= attributesEnd)
			{
				const auto* attribute = reinterpret_cast<StunPacket::Attribute*>(ptr);

				// Get the attribute type.
				const auto attrType =
				  static_cast<StunPacket::AttributeType>(ntohs(static_cast<uint16_t>(attribute->type)));

				// Get the attribute length.
				const auto attrLen = static_cast<uint16_t>(ntohs(attribute->len));

				// Pointer to the attribute value (if any).
				const uint8_t* attrValue = ptr + 4;

				// Ensure the attribute length is not greater than the remaining length.
				if (ptr + 4 + attrLen > attributesEnd)
				{
					MS_WARN_TAG(
					  ice,
					  "invalid Packet, not enough space for the announced value of the attribute with type %" PRIu16,
					  attrType);

					return false;
				}

				// FINGERPRINT must be the last attribute.
				if (HasFingerprint())
				{
					MS_WARN_TAG(ice, "invalid Packet, attribute after FINGERPRINT is not allowed");

					return false;
				}

				// After a MESSAGE-INTEGRITY attribute just FINGERPRINT is allowed.
				if (HasMessageIntegrity() && attrType != StunPacket::AttributeType::FINGERPRINT)
				{
					MS_WARN_TAG(
					  ice,
					  "invalid Packet, attribute after MESSAGE-INTEGRITY other than FINGERPRINT is not allowed");

					return false;
				}

				switch (attrType)
				{
					case StunPacket::AttributeType::USERNAME:
					{
						SetUsername(reinterpret_cast<const char*>(attrValue), static_cast<size_t>(attrLen));

						break;
					}

					case StunPacket::AttributeType::PRIORITY:
					{
						// Ensure attribute length is 4 bytes.
						if (attrLen != 4)
						{
							MS_WARN_TAG(ice, "invalid Packet, attribute PRIORITY must be 4 bytes length");

							return false;
						}

						SetPriority(Utils::Byte::Get4Bytes(attrValue, 0));

						break;
					}

					case StunPacket::AttributeType::ICE_CONTROLLING:
					{
						// Ensure attribute length is 8 bytes.
						if (attrLen != 8)
						{
							MS_WARN_TAG(ice, "invalid Packet, attribute ICE-CONTROLLING must be 8 bytes length");

							return false;
						}

						SetIceControlling(Utils::Byte::Get8Bytes(attrValue, 0));

						break;
					}

					case StunPacket::AttributeType::ICE_CONTROLLED:
					{
						// Ensure attribute length is 8 bytes.
						if (attrLen != 8)
						{
							MS_WARN_TAG(ice, "invalid Packet, attribute ICE-CONTROLLED must be 8 bytes length");

							return false;
						}

						SetIceControlled(Utils::Byte::Get8Bytes(attrValue, 0));

						break;
					}

					case StunPacket::AttributeType::USE_CANDIDATE:
					{
						// Ensure attribute length is 0 bytes.
						if (attrLen != 0)
						{
							MS_WARN_TAG(ice, "invalid Packet, attribute USE-CANDIDATE must be 0 bytes length");

							return false;
						}

						EnableUseCandidate();

						break;
					}

					case StunPacket::AttributeType::NOMINATION:
					{
						// Ensure attribute length is 4 bytes.
						if (attrLen != 4)
						{
							MS_WARN_TAG(ice, "invalid Packet, attribute NOMINATION must be 4 bytes length");

							return false;
						}

						SetNomination(Utils::Byte::Get4Bytes(attrValue, 0));

						break;
					}

					case StunPacket::AttributeType::SOFTWARE:
					{
						// Ensure attribute length is less than 763 bytes.
						if (attrLen >= 763)
						{
							MS_WARN_TAG(
							  ice, "invalid Packet, attribute SOFTWARE must be less than 763 bytes length");

							return false;
						}

						SetSoftware(reinterpret_cast<const char*>(attrValue), static_cast<size_t>(attrLen));

						break;
					}

					case StunPacket::AttributeType::ERROR_CODE:
					{
						// Ensure attribute length >= 4bytes.
						if (attrLen < 4)
						{
							MS_WARN_TAG(ice, "invalid Packet, attribute ERROR-CODE must be >= 4 bytes length");

							return false;
						}

						const uint8_t errorClass  = Utils::Byte::Get1Byte(attrValue, 2);
						const uint8_t errorNumber = Utils::Byte::Get1Byte(attrValue, 3);
						auto errorCode            = static_cast<uint16_t>((errorClass * 100) + errorNumber);

						SetErrorCode(errorCode);

						break;
					}

					case StunPacket::AttributeType::MESSAGE_INTEGRITY:
					{
						// Ensure attribute length is 20 bytes.
						if (attrLen != 20)
						{
							MS_WARN_TAG(ice, "invalid Packet, attribute MESSAGE-INTEGRITY must be 20 bytes length");

							return false;
						}

						SetMessageIntegrity(attrValue);

						break;
					}

					case StunPacket::AttributeType::FINGERPRINT:
					{
						// Ensure attribute length is 4 bytes.
						if (attrLen != 4)
						{
							MS_WARN_TAG(ice, "invalid Packet, attribute FINGERPRINT must be 4 bytes length");

							return false;
						}

						fingerprintAttrOffset = ptr - fixedHeader;
						fingerprint           = Utils::Byte::Get4Bytes(attrValue, 0);

						EnableFingerprint();

						break;
					}

					default:;
				}

				// Move to next attribute.
				ptr += Utils::Byte::PadTo4Bytes(static_cast<size_t>(4 + attrLen));
			}

			// Ensure we read the entire Packet length.
			if (ptr != fixedHeader + GetLength())
			{
				MS_WARN_TAG(
				  ice,
				  "invalid Packet, computed length (%zu) does not match announced length (%zu)",
				  ptr - fixedHeader,
				  GetLength());

				return false;
			}

			// If it has FINGERPRINT attribute then verify it.
			if (HasFingerprint())
			{
				// Compute the CRC32 of the received packet up to (but excluding) the
				// FINGERPRINT attribute and XOR it with 0x5354554e.
				const auto computedFingerprint =
				  Utils::Crypto::GetCRC32(fixedHeader, fingerprintAttrOffset) ^ 0x5354554e;

				// Compare with the FINGERPRINT value in the packet.
				if (fingerprint != computedFingerprint)
				{
					MS_WARN_TAG(
					  ice, "invalid Packet, computed FINGERPRINT value does not match the value in the packet");

					return false;
				}
			}

			return true;
		}

		void StunPacket::SetClass(StunPacket::Class klass)
		{
			MS_TRACE();

			this->klass = klass;
		}

		void StunPacket::SetMethod(StunPacket::Method method)
		{
			MS_TRACE();

			this->method = method;
		}

		void StunPacket::SetTransactionId(const uint8_t* transactionId)
		{
			MS_TRACE();

			// Set TransactionId field.
			std::memcpy(GetTransactionIdPointer(), transactionId, StunPacket::TransactionIdLength);
		}

		void StunPacket::SetUsername(const char* username, size_t len)
		{
			MS_TRACE();

			this->username.assign(username, len);
		}

		void StunPacket::SetPriority(uint32_t priority)
		{
			MS_TRACE();

			this->priority = priority;
		}

		void StunPacket::SetIceControlling(uint64_t iceControlling)
		{
			MS_TRACE();

			this->iceControlling = iceControlling;
		}

		void StunPacket::SetIceControlled(uint64_t iceControlled)
		{
			MS_TRACE();

			this->iceControlled = iceControlled;
		}

		void StunPacket::EnableUseCandidate()
		{
			MS_TRACE();

			this->hasUseCandidate = true;
		}

		void StunPacket::SetNomination(uint32_t nomination)
		{
			MS_TRACE();

			this->nomination = nomination;
		}

		void StunPacket::SetSoftware(const char* software, size_t len)
		{
			MS_TRACE();

			this->software.assign(software, len);
		}

		void StunPacket::SetErrorCode(uint16_t errorCode)
		{
			MS_TRACE();

			this->errorCode = errorCode;
		}

		void StunPacket::SetMessageIntegrity(const uint8_t* messageIntegrity)
		{
			MS_TRACE();

			this->messageIntegrity = messageIntegrity;
		}

		void StunPacket::EnableFingerprint()
		{
			MS_TRACE();

			this->hasFingerprint = true;
		}

		void StunPacket::SetXorMappedAddress(const struct sockaddr* xorMappedAddress)
		{
			MS_TRACE();

			this->xorMappedAddress = xorMappedAddress;
		}

		void StunPacket::SetPassword(const std::string& password)
		{
			MS_TRACE();

			// Just for request, indication and success response messages.
			if (this->klass == Class::ERROR_RESPONSE)
			{
				MS_THROW_ERROR("cannot set password in an STUN error response");
			}

			this->password = password;
		}
	} // namespace ICE
} // namespace RTC
