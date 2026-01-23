#include "Utils.hpp"
#include <cstdint>
#define MS_CLASS "RTC::ICE::StunPacket"
// #define MS_LOG_DEV_LEVEL 3

#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "RTC/ICE/StunPacket.hpp"
#include <cstdio>  // std::snprintf()
#include <cstring> // std::memcmp(), std::memcpy(), std::memset()
#include <string>

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

			// Get STUN Message Type field.
			const uint16_t typeField = Utils::Byte::Get2Bytes(buffer, 0);

			// Get STUN class.
			const auto klass =
			  static_cast<StunPacket::Class>(((buffer[0] & 0x01) << 1) | ((buffer[1] & 0x10) >> 4));

			// Get STUN method.
			const auto method = static_cast<StunPacket::Method>(
			  (typeField & 0x000f) | ((typeField & 0x00e0) >> 1) | ((typeField & 0x3E00) >> 2));

			packet->klass  = klass;
			packet->method = method;

			if (!packet->Validate(/*storeAttributes*/ true))
			{
				delete packet;
				return nullptr;
			}

			return packet;
		}

		StunPacket* StunPacket::Factory(
		  uint8_t* buffer, size_t bufferLength, StunPacket::Class klass, StunPacket::Method method)
		{
			MS_TRACE();

			if (bufferLength < StunPacket::FixedHeaderLength)
			{
				MS_THROW_TYPE_ERROR("no space for fixed header");
			}

			auto* packet = new StunPacket(buffer, bufferLength);

			std::memset(buffer, 0x00, packet->GetLength());

			packet->klass  = klass;
			packet->method = method;

			// Merge class and method fields into type.
			auto typeField = (static_cast<uint16_t>(method) & 0x0f80) << 2;

			typeField |= (static_cast<uint16_t>(method) & 0x0070) << 1;
			typeField |= (static_cast<uint16_t>(method) & 0x000f);
			typeField |= (static_cast<uint16_t>(klass) & 0x02) << 7;
			typeField |= (static_cast<uint16_t>(klass) & 0x01) << 4;

			// Set type field.
			Utils::Byte::Set2Bytes(buffer, 0, typeField);

			// NOTE: No need to write message length since it's already 0.

			// Set magic cookie.
			std::memcpy(buffer + 4, StunPacket::MagicCookie, 4);

			// Write a random TransactionId.
			Utils::Crypto::WriteRandomBytes(buffer + 8, StunPacket::TransactionIdLength);

			// No need to invoke SetLength() since constructor invoked it with
			// minimum Packet length.

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
					klass = "Request";
					break;
				}
				case Class::INDICATION:
				{
					klass = "Indication";
					break;
				}
				case Class::SUCCESS_RESPONSE:
				{
					klass = "Success Response";
					break;
				}
				case Class::ERROR_RESPONSE:
				{
					klass = "Error Response";
					break;
				}
			}

			if (this->method == Method::BINDING)
			{
				MS_DUMP_CLEAN(indentation, "  method & class: Binding %s", klass.c_str());
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

			char transactionId[12 * 2 + 1]; // 12 bytes × 2 hex chars + null terminator.

			for (uint8_t i{ 0 }; i < 12; ++i)
			{
				std::snprintf(transactionId + (i * 2), 3, "%02X", GetTransactionId()[i]);
			}

			MS_DUMP_CLEAN(indentation, "  transaction id: 0x%s", transactionId);

			MS_DUMP_CLEAN(indentation, "  attributes length: %zu", GetAttributesLength());

			MS_DUMP_CLEAN(indentation, "  <Attributes>");

			if (HasAttribute(StunPacket::AttributeType::USERNAME))
			{
				const auto username = GetUsername();

				MS_DUMP_CLEAN(
				  indentation + 1, "  username: %.*s", static_cast<int>(username.size()), username.data());
			}

			if (HasAttribute(StunPacket::AttributeType::PRIORITY))
			{
				MS_DUMP_CLEAN(indentation + 1, "  priority: %" PRIu32, GetPriority());
			}

			if (HasAttribute(StunPacket::AttributeType::ICE_CONTROLLING))
			{
				MS_DUMP_CLEAN(indentation + 1, "  ice controlling: %" PRIu64, GetIceControlling());
			}

			if (HasAttribute(StunPacket::AttributeType::ICE_CONTROLLED))
			{
				MS_DUMP_CLEAN(indentation + 1, "  ice controlled: %" PRIu64, GetIceControlled());
			}

			if (HasAttribute(StunPacket::AttributeType::USE_CANDIDATE))
			{
				MS_DUMP_CLEAN(indentation + 1, "  use candidate: yes");
			}

			if (HasAttribute(StunPacket::AttributeType::NOMINATION))
			{
				MS_DUMP_CLEAN(indentation + 1, "  nomination: %" PRIu32, GetNomination());
			}

			if (HasAttribute(StunPacket::AttributeType::SOFTWARE))
			{
				const auto software = GetSoftware();

				MS_DUMP_CLEAN(
				  indentation + 1, "  software: %.*s", static_cast<int>(software.size()), software.data());
			}

			if (HasAttribute(StunPacket::AttributeType::XOR_MAPPED_ADDRESS))
			{
				// TODO
			}

			if (HasAttribute(StunPacket::AttributeType::ERROR_CODE))
			{
				MS_DUMP_CLEAN(indentation + 1, "  error code: %" PRIu16, GetErrorCode());
			}

			if (HasAttribute(StunPacket::AttributeType::MESSAGE_INTEGRITY))
			{
				char messageIntegrity[41];

				for (uint8_t i{ 0 }; i < StunPacket::MessageIntegrityAttributeLength; ++i)
				{
					std::snprintf(messageIntegrity + (i * 2), 3, "%.2x", GetMessageIntegrity()[i]);
				}

				MS_DUMP_CLEAN(indentation + 1, "  message integrity: %s", messageIntegrity);
			}

			if (HasAttribute(StunPacket::AttributeType::FINGERPRINT))
			{
				MS_DUMP_CLEAN(indentation + 1, "  fingerprint: %" PRIu32, GetFingerprint());
			}

			MS_DUMP_CLEAN(indentation, "  </Attributes>");

			MS_DUMP_CLEAN(indentation, "<RTC::ICE::StunPacket>");
		}

		StunPacket* StunPacket::Clone(uint8_t* buffer, size_t bufferLength) const
		{
			MS_TRACE();

			auto* clonedPacket = new StunPacket(buffer, bufferLength);

			Serializable::CloneInto(clonedPacket);

			// Clone private members.
			clonedPacket->klass      = this->klass;
			clonedPacket->method     = this->method;
			clonedPacket->attributes = this->attributes;

			return clonedPacket;
		}

		bool StunPacket::Validate(bool storeAttributes)
		{
			MS_TRACE();

			const auto* fixedHeader = GetFixedHeaderPointer();

			// Get message length field.
			const uint16_t msgLength = Utils::Byte::Get2Bytes(fixedHeader, 2);

			// Message length field must be total length minus header's 20 bytes, and
			// must be multiple of 4 Bytes.
			// NOTE: Message length is effectively the total length of the Attributes
			// (with all paddings).
			if (static_cast<size_t>(msgLength) != GetAttributesLength() || !Utils::Byte::IsPaddedTo4Bytes(msgLength))
			{
				MS_WARN_TAG(
				  ice,
				  "invalid Packet, message length field (%" PRIu16
				  ") does not match given buffer length or it's not multiple of 4 bytes",
				  msgLength);

				return false;
			}

			if (!ParseAttributes(storeAttributes))
			{
				MS_WARN_TAG(rtp, "invalid Packet, invalid Attributes");

				return false;
			}

			// If it has FINGERPRINT Attribute then verify it.
			const auto* fingerprintAttr = GetAttribute(StunPacket::AttributeType::FINGERPRINT);

			if (fingerprintAttr)
			{
				// Compute the CRC32 of the received packet up to (but excluding) the
				// FINGERPRINT Attribute and XOR it with 0x5354554e.
				const auto computedFingerprint =
				  Utils::Crypto::GetCRC32(
				    fixedHeader, StunPacket::FixedHeaderLength + fingerprintAttr->offset) ^
				  0x5354554e;

				// Compare with the FINGERPRINT value in the packet.
				if (GetFingerprint() != computedFingerprint)
				{
					MS_WARN_TAG(
					  ice,
					  "invalid Packet, computed fingerprint value does not match the value in the FINGERPRINT attribute");

					return false;
				}
			}

			return true;
		}

		bool StunPacket::ParseAttributes(bool storeAttributes)
		{
			MS_TRACE();

			const uint8_t* attributesStart = GetAttributesPointer();
			const uint8_t* attributesEnd   = attributesStart + GetAttributesLength();
			uint8_t* ptr                   = const_cast<uint8_t*>(attributesStart);

			// Ensure there are at least 4 remaining bytes (Attribute with 0 length).
			while (ptr + 4 <= attributesEnd)
			{
				const auto* attribute = reinterpret_cast<StunPacket::Attribute*>(ptr);

				// Get the Attribute type.
				const auto attrType =
				  static_cast<StunPacket::AttributeType>(ntohs(static_cast<uint16_t>(attribute->type)));

				// Get the Attribute length.
				const auto attrLen = static_cast<uint16_t>(ntohs(attribute->len));

				// Offset of the Attribute from the start of the attributes.
				const auto attrOffset = static_cast<size_t>((ptr - attributesStart));

				// Ensure the Attribute length is not greater than the remaining length.
				if (ptr + 4 + attrLen > attributesEnd)
				{
					MS_WARN_TAG(
					  ice,
					  "invalid Packet, not enough space for the announced value of the Attribute with type %" PRIu16,
					  attrType);

					return false;
				}

				// FINGERPRINT must be the last Attribute.
				if (storeAttributes && HasAttribute(StunPacket::AttributeType::FINGERPRINT))
				{
					MS_WARN_TAG(ice, "invalid Packet, Attribute after FINGERPRINT is not allowed");

					return false;
				}

				// After a MESSAGE-INTEGRITY Attribute only FINGERPRINT is allowed.
				if (
				  storeAttributes && HasAttribute(StunPacket::AttributeType::MESSAGE_INTEGRITY) &&
				  attrType != StunPacket::AttributeType::FINGERPRINT)
				{
					MS_WARN_TAG(
					  ice,
					  "invalid Packet, Attribute after MESSAGE-INTEGRITY other than FINGERPRINT is not allowed");

					return false;
				}

				switch (attrType)
				{
					case StunPacket::AttributeType::USERNAME:
					{
						if (attrLen > StunPacket::UsernameAttributeMaxLength)
						{
							MS_WARN_TAG(
							  ice,
							  "invalid Packet, Attribute USERNAME must be at most %zu bytes",
							  StunPacket::UsernameAttributeMaxLength);

							return false;
						}

						if (storeAttributes && !StoreAttribute(attrType, attrLen, attrOffset))
						{
							return false;
						}

						break;
					}

					case StunPacket::AttributeType::PRIORITY:
					{
						if (attrLen != 4)
						{
							MS_WARN_TAG(ice, "invalid Packet, Attribute PRIORITY must be 4 bytes length");

							return false;
						}

						if (storeAttributes && !StoreAttribute(attrType, attrLen, attrOffset))
						{
							return false;
						}

						break;
					}

					case StunPacket::AttributeType::ICE_CONTROLLING:
					{
						if (attrLen != 8)
						{
							MS_WARN_TAG(ice, "invalid Packet, Attribute ICE-CONTROLLING must be 8 bytes length");

							return false;
						}

						if (storeAttributes && !StoreAttribute(attrType, attrLen, attrOffset))
						{
							return false;
						}

						break;
					}

					case StunPacket::AttributeType::ICE_CONTROLLED:
					{
						if (attrLen != 8)
						{
							MS_WARN_TAG(ice, "invalid Packet, Attribute ICE-CONTROLLED must be 8 bytes length");

							return false;
						}

						if (storeAttributes && !StoreAttribute(attrType, attrLen, attrOffset))
						{
							return false;
						}

						break;
					}

					case StunPacket::AttributeType::USE_CANDIDATE:
					{
						if (attrLen != 0)
						{
							MS_WARN_TAG(ice, "invalid Packet, Attribute USE-CANDIDATE must be 0 bytes length");

							return false;
						}

						if (storeAttributes && !StoreAttribute(attrType, attrLen, attrOffset))
						{
							return false;
						}

						break;
					}

					case StunPacket::AttributeType::NOMINATION:
					{
						if (attrLen != 4)
						{
							MS_WARN_TAG(ice, "invalid Packet, Attribute NOMINATION must be 4 bytes length");

							return false;
						}

						if (storeAttributes && !StoreAttribute(attrType, attrLen, attrOffset))
						{
							return false;
						}

						break;
					}

					case StunPacket::AttributeType::SOFTWARE:
					{
						if (attrLen > StunPacket::SoftwareAttributeMaxLength)
						{
							MS_WARN_TAG(
							  ice,
							  "invalid Packet, Attribute SOFTWARE must be at most %zu bytes length",
							  StunPacket::SoftwareAttributeMaxLength);

							return false;
						}

						if (storeAttributes && !StoreAttribute(attrType, attrLen, attrOffset))
						{
							return false;
						}

						break;
					}

					case StunPacket::AttributeType::ERROR_CODE:
					{
						if (attrLen < 4)
						{
							MS_WARN_TAG(ice, "invalid Packet, Attribute ERROR-CODE must be >= 4 bytes length");

							return false;
						}

						if (storeAttributes && !StoreAttribute(attrType, attrLen, attrOffset))
						{
							return false;
						}

						break;
					}

					case StunPacket::AttributeType::MESSAGE_INTEGRITY:
					{
						if (attrLen != StunPacket::MessageIntegrityAttributeLength)
						{
							MS_WARN_TAG(
							  ice,
							  "invalid Packet, Attribute MESSAGE-INTEGRITY must be %zu bytes length",
							  StunPacket::MessageIntegrityAttributeLength);

							return false;
						}

						if (storeAttributes && !StoreAttribute(attrType, attrLen, attrOffset))
						{
							return false;
						}

						break;
					}

					case StunPacket::AttributeType::FINGERPRINT:
					{
						if (attrLen != 4)
						{
							MS_WARN_TAG(ice, "invalid Packet, Attribute FINGERPRINT must be 4 bytes length");

							return false;
						}

						if (storeAttributes && !StoreAttribute(attrType, attrLen, attrOffset))
						{
							return false;
						}

						break;
					}

					default:
					{
						MS_DEBUG_DEV("unknown Attribute with type %" PRIu16, attrType);
					}
				}

				// Move to next Attribute.
				ptr += Utils::Byte::PadTo4Bytes(static_cast<size_t>(4 + attrLen));
			}

			// Ensure we read the Attributes length entirely.
			if (ptr - attributesStart != GetAttributesLength())
			{
				MS_WARN_TAG(
				  ice,
				  "invalid Packet, computed Attributes length (%zu) does not match announced length (%zu)",
				  ptr - attributesStart,
				  GetAttributesLength());

				return false;
			}

			return true;
		}

		bool StunPacket::StoreAttribute(AttributeType type, uint16_t len, size_t offset)
		{
			MS_TRACE();

			if (!this->attributes.try_emplace(type, type, len, offset).second)
			{
				MS_WARN_TAG(ice, "invalid Packet, duplicated %" PRIu16 " Attribute", type);

				return false;
			}

			return true;
		}

		void StunPacket::SetTransactionId(const uint8_t* transactionId)
		{
			MS_TRACE();

			std::memcpy(GetTransactionIdPointer(), transactionId, StunPacket::TransactionIdLength);
		}

		void StunPacket::SetUsername(const char* username, size_t len)
		{
			MS_TRACE();

			// TODO
		}

		void StunPacket::SetPriority(uint32_t priority)
		{
			MS_TRACE();

			// TODO
		}

		void StunPacket::SetIceControlling(uint64_t iceControlling)
		{
			MS_TRACE();

			// TODO
		}

		void StunPacket::SetIceControlled(uint64_t iceControlled)
		{
			MS_TRACE();

			// TODO
		}

		void StunPacket::EnableUseCandidate()
		{
			MS_TRACE();

			// TODO
		}

		void StunPacket::SetNomination(uint32_t nomination)
		{
			MS_TRACE();

			// TODO
		}

		void StunPacket::SetSoftware(const char* software, size_t len)
		{
			MS_TRACE();

			// TODO
		}

		void StunPacket::SetErrorCode(uint16_t errorCode)
		{
			MS_TRACE();

			// TODO
		}

		void StunPacket::SetXorMappedAddress(const struct sockaddr* xorMappedAddress)
		{
			MS_TRACE();

			// TODO
		}
	} // namespace ICE
} // namespace RTC
