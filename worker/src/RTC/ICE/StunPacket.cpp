#define MS_CLASS "RTC::ICE::StunPacket"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/ICE/StunPacket.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include <cstdio>  // std::snprintf()
#include <cstring> // std::memcmp(), std::memcpy(), std::memset()

namespace RTC
{
	namespace ICE
	{
		/* Static. */

		static constexpr size_t AttributeFactoryBufferLength{ 65536 };
		thread_local static uint8_t AttributeFactoryBuffer[AttributeFactoryBufferLength];

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
				// Magic Cookie must match.
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

			// Set magic Cookie.
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
				MS_DUMP_CLEAN(indentation, "  method and class: Binding %s", klass.c_str());
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
				  indentation + 1, "  username: \"%.*s\"", static_cast<int>(username.size()), username.data());
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
				  indentation + 1, "  software: \"%.*s\"", static_cast<int>(software.size()), software.data());
			}

			if (HasAttribute(StunPacket::AttributeType::XOR_MAPPED_ADDRESS))
			{
				struct sockaddr xorMappedAddress;

				if (GetXorMappedAddress(xorMappedAddress))
				{
					int family;
					uint16_t port;
					std::string ip;

					Utils::IP::GetAddressInfo(std::addressof(xorMappedAddress), family, ip, port);

					if (family == AF_INET)
					{
						MS_DUMP_CLEAN(indentation + 1, "  xor mapped address: IPv4 %s:%" PRIu16, ip.c_str(), port);
					}
					else if (family == AF_INET6)
					{
						MS_DUMP_CLEAN(
						  indentation + 1, "  xor mapped address: IPv6 [%s]:%" PRIu16, ip.c_str(), port);
					}
				}
			}

			if (HasAttribute(StunPacket::AttributeType::ERROR_CODE))
			{
				std::string_view reasonPhrase{};

				const auto errorCode = GetErrorCode(reasonPhrase);

				MS_DUMP_CLEAN(
				  indentation + 1,
				  "  error code: %" PRIu16 " (reason phrase: \"%.*s\")",
				  errorCode,
				  static_cast<int>(reasonPhrase.size()),
				  reasonPhrase.data());
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

		void StunPacket::SetTransactionId(const uint8_t* transactionId)
		{
			MS_TRACE();

			std::memcpy(GetTransactionIdPointer(), transactionId, StunPacket::TransactionIdLength);
		}

		void StunPacket::SetUsername(std::string& username)
		{
			MS_TRACE();

			if (username.length() > StunPacket::UsernameAttributeMaxLength)
			{
				MS_THROW_TYPE_ERROR(
				  "Attribute USERNAME must be at most %zu bytes", StunPacket::UsernameAttributeMaxLength);
			}

			StoreNewAttribute(StunPacket::AttributeType::USERNAME, username.c_str(), username.length());
		}

		void StunPacket::SetPriority(uint32_t priority)
		{
			MS_TRACE();

			Utils::Byte::Set4Bytes(AttributeFactoryBuffer, 0, priority);

			StoreNewAttribute(StunPacket::AttributeType::PRIORITY, AttributeFactoryBuffer, sizeof(priority));
		}

		void StunPacket::SetIceControlling(uint64_t iceControlling)
		{
			MS_TRACE();

			Utils::Byte::Set8Bytes(AttributeFactoryBuffer, 0, iceControlling);

			StoreNewAttribute(
			  StunPacket::AttributeType::ICE_CONTROLLING, AttributeFactoryBuffer, sizeof(iceControlling));
		}

		void StunPacket::SetIceControlled(uint64_t iceControlled)
		{
			MS_TRACE();

			Utils::Byte::Set8Bytes(AttributeFactoryBuffer, 0, iceControlled);

			StoreNewAttribute(
			  StunPacket::AttributeType::ICE_CONTROLLED, AttributeFactoryBuffer, sizeof(iceControlled));
		}

		void StunPacket::EnableUseCandidate()
		{
			MS_TRACE();

			StoreNewAttribute(StunPacket::AttributeType::USE_CANDIDATE, nullptr, 0);
		}

		void StunPacket::SetNomination(uint32_t nomination)
		{
			MS_TRACE();

			Utils::Byte::Set4Bytes(AttributeFactoryBuffer, 0, nomination);

			StoreNewAttribute(
			  StunPacket::AttributeType::NOMINATION, AttributeFactoryBuffer, sizeof(nomination));
		}

		void StunPacket::SetSoftware(std::string& software)
		{
			MS_TRACE();

			if (software.length() > StunPacket::SoftwareAttributeMaxLength)
			{
				MS_THROW_TYPE_ERROR(
				  "Attribute SOFTWARE must be at most %zu bytes", StunPacket::SoftwareAttributeMaxLength);
			}

			StoreNewAttribute(StunPacket::AttributeType::SOFTWARE, software.c_str(), software.length());
		}

		void StunPacket::SetErrorCode(uint16_t errorCode, std::string& reasonPhrase)
		{
			MS_TRACE();

			const auto codeClass  = static_cast<uint8_t>(errorCode / 100);
			const auto codeNumber = static_cast<uint8_t>(errorCode) - (codeClass * 100);

			Utils::Byte::Set2Bytes(AttributeFactoryBuffer, 0, 0);
			Utils::Byte::Set1Byte(AttributeFactoryBuffer, 2, codeClass);
			Utils::Byte::Set1Byte(AttributeFactoryBuffer, 3, codeNumber);

			std::memcpy(AttributeFactoryBuffer + 4, reasonPhrase.c_str(), reasonPhrase.length());

			StoreNewAttribute(
			  StunPacket::AttributeType::ERROR_CODE, AttributeFactoryBuffer, 4 + reasonPhrase.length());
		}

		bool StunPacket::GetXorMappedAddress(struct sockaddr& xorMappedAddress) const
		{
			MS_TRACE();

			std::memset(std::addressof(xorMappedAddress), 0x00, sizeof(xorMappedAddress));

			const auto* attribute = GetAttribute(StunPacket::AttributeType::XOR_MAPPED_ADDRESS);

			if (!attribute)
			{
				return false;
			}

			const auto* attributeValue = GetAttributeValue(attribute);
			uint8_t family             = attributeValue[1];
			uint16_t port;

			std::memcpy(std::addressof(port), attributeValue + 2, 2);

			// XOR with the first 2 bytes of the Magic Cookie.
			port = ntohs(port) ^ (StunPacket::MagicCookie[0] << 8 | StunPacket::MagicCookie[1]);

			// IPv4.
			if (family == 0x01)
			{
				if (attribute->len != StunPacket::XorMappedAddressIPv4Length)
				{
					MS_WARN_TAG(
					  ice,
					  "cannot get XOR_MAPPED_ADDRESS Attribute value, length of the Attribute is not %zu",
					  StunPacket::XorMappedAddressIPv4Length);

					return false;
				}

				auto* addr4 = reinterpret_cast<struct sockaddr_in*>(std::addressof(xorMappedAddress));

				addr4->sin_family = AF_INET;
				addr4->sin_port   = htons(port);

				uint32_t addr;
				std::memcpy(std::addressof(addr), attributeValue + 4, 4);

				// XOR with each byte of the Magic Cookie.
				uint8_t* addrBytes = reinterpret_cast<uint8_t*>(&addr);

				for (uint8_t i{ 0 }; i < sizeof(StunPacket::MagicCookie); ++i)
				{
					addrBytes[i] ^= StunPacket::MagicCookie[i];
				}

				addr4->sin_addr.s_addr = addr;

				return true;
			}
			// IPv6.
			else if (family == 0x02)
			{
				if (attribute->len != StunPacket::XorMappedAddressIPv6Length)
				{
					MS_WARN_TAG(
					  ice,
					  "cannot get XOR_MAPPED_ADDRESS Attribute value, length of the Attribute is not %zu",
					  StunPacket::XorMappedAddressIPv6Length);

					return false;
				}

				auto* addr6 = reinterpret_cast<struct sockaddr_in6*>(std::addressof(xorMappedAddress));

				addr6->sin6_family = AF_INET6;
				addr6->sin6_port   = htons(port);

				const auto transactionId = GetTransactionId();

				// XOR with first 4 bytes with each byte of the Magic Cookie.
				for (uint8_t i{ 0 }; i < sizeof(StunPacket::MagicCookie); ++i)
				{
					addr6->sin6_addr.s6_addr[i] =
					  attributeValue[sizeof(StunPacket::MagicCookie) + i] ^ StunPacket::MagicCookie[i];
				}

				// XOR other bytes with each byte of the Transaction id.
				for (uint8_t i{ 0 }; i < sizeof(StunPacket::MagicCookie) + StunPacket::TransactionIdLength;
				     ++i)
				{
					addr6->sin6_addr.s6_addr[i] = attributeValue[sizeof(StunPacket::MagicCookie) + i] ^
					                              transactionId[i - sizeof(StunPacket::MagicCookie)];
				}

				return true;
			}
			// Unknown family.
			else
			{
				MS_WARN_TAG(ice, "cannot get XOR_MAPPED_ADDRESS Attribute value, unknown family");

				return false;
			}
		}

		void StunPacket::SetXorMappedAddress(const struct sockaddr* xorMappedAddress)
		{
			MS_TRACE();

			switch (xorMappedAddress->sa_family)
			{
				case AF_INET:
				{
					// Set first byte to 0.
					AttributeFactoryBuffer[0] = 0;
					// Set inet family.
					AttributeFactoryBuffer[1] = 0x01;
					// Set port and XOR it.
					std::memcpy(
					  AttributeFactoryBuffer + 2,
					  &(reinterpret_cast<const sockaddr_in*>(xorMappedAddress))->sin_port,
					  2);
					AttributeFactoryBuffer[2] ^= StunPacket::MagicCookie[0];
					AttributeFactoryBuffer[3] ^= StunPacket::MagicCookie[1];
					// Set address and XOR it.
					std::memcpy(
					  AttributeFactoryBuffer + 4,
					  &(reinterpret_cast<const sockaddr_in*>(xorMappedAddress))->sin_addr.s_addr,
					  4);
					AttributeFactoryBuffer[4] ^= StunPacket::MagicCookie[0];
					AttributeFactoryBuffer[5] ^= StunPacket::MagicCookie[1];
					AttributeFactoryBuffer[6] ^= StunPacket::MagicCookie[2];
					AttributeFactoryBuffer[7] ^= StunPacket::MagicCookie[3];

					StoreNewAttribute(
					  StunPacket::AttributeType::XOR_MAPPED_ADDRESS,
					  AttributeFactoryBuffer,
					  StunPacket::XorMappedAddressIPv4Length);

					break;
				}

				case AF_INET6:
				{
					// Set first byte to 0.
					AttributeFactoryBuffer[0] = 0;
					// Set inet family.
					AttributeFactoryBuffer[1] = 0x02;
					// Set port and XOR it.
					std::memcpy(
					  AttributeFactoryBuffer + 2,
					  &(reinterpret_cast<const sockaddr_in6*>(xorMappedAddress))->sin6_port,
					  2);
					AttributeFactoryBuffer[2] ^= StunPacket::MagicCookie[0];
					AttributeFactoryBuffer[3] ^= StunPacket::MagicCookie[1];
					// Set address and XOR it.
					std::memcpy(
					  AttributeFactoryBuffer + 4,
					  &(reinterpret_cast<const sockaddr_in6*>(xorMappedAddress))->sin6_addr.s6_addr,
					  16);
					const auto* transactionId = GetTransactionId();

					AttributeFactoryBuffer[4] ^= StunPacket::MagicCookie[0];
					AttributeFactoryBuffer[5] ^= StunPacket::MagicCookie[1];
					AttributeFactoryBuffer[6] ^= StunPacket::MagicCookie[2];
					AttributeFactoryBuffer[7] ^= StunPacket::MagicCookie[3];
					AttributeFactoryBuffer[8] ^= transactionId[0];
					AttributeFactoryBuffer[9] ^= transactionId[1];
					AttributeFactoryBuffer[10] ^= transactionId[2];
					AttributeFactoryBuffer[11] ^= transactionId[3];
					AttributeFactoryBuffer[12] ^= transactionId[4];
					AttributeFactoryBuffer[13] ^= transactionId[5];
					AttributeFactoryBuffer[14] ^= transactionId[6];
					AttributeFactoryBuffer[15] ^= transactionId[7];
					AttributeFactoryBuffer[16] ^= transactionId[8];
					AttributeFactoryBuffer[17] ^= transactionId[9];
					AttributeFactoryBuffer[18] ^= transactionId[10];
					AttributeFactoryBuffer[19] ^= transactionId[11];

					StoreNewAttribute(
					  StunPacket::AttributeType::XOR_MAPPED_ADDRESS,
					  AttributeFactoryBuffer,
					  StunPacket::XorMappedAddressIPv6Length);

					break;
				}

				default:
				{
					MS_THROW_TYPE_ERROR("unknown IP family");
				}
			}
		}

		StunPacket::AuthenticationResult StunPacket::CheckAuthentication(
		  const std::string& usernameFragment1, const std::string& password) const
		{
			MS_TRACE();

			const auto* messageIntegrity = GetMessageIntegrity();
			const auto hasFingerprint    = HasAttribute(StunPacket::AttributeType::FINGERPRINT);

			switch (this->klass)
			{
				case StunPacket::Class::REQUEST:
				case StunPacket::Class::INDICATION:
				{
					// usernameFragment1 must not be empty.
					if (usernameFragment1.empty())
					{
						MS_WARN_TAG(
						  ice, "cannot authenticate request or indication, empty usernameFragment1 given");

						return StunPacket::AuthenticationResult::BAD_MESSAGE;
					}

					// USERNAME Attribute must be present.
					if (!HasAttribute(StunPacket::AttributeType::USERNAME))
					{
						MS_WARN_TAG(ice, "cannot authenticate request or indication, missing USERNAME Attribute");

						return StunPacket::AuthenticationResult::BAD_MESSAGE;
					}

					// MESSAGE-INTEGRITY Attribute must be present.
					if (!messageIntegrity)
					{
						MS_WARN_TAG(
						  ice, "cannot authenticate request or indication, missing MESSAGE-INTEGRITY Attribute");

						return StunPacket::AuthenticationResult::BAD_MESSAGE;
					}

					// Check that the USERNAME Attribute begins with the first username
					// fragment plus ":".
					const size_t usernameFragment1Len = usernameFragment1.length();
					const auto username               = GetUsername();

					if (
					  username.length() <= usernameFragment1Len || username.at(usernameFragment1Len) != ':' ||
					  username.compare(0, usernameFragment1Len, usernameFragment1) != 0)
					{
						return StunPacket::AuthenticationResult::UNAUTHORIZED;
					}

					break;
				}

				case StunPacket::Class::SUCCESS_RESPONSE:
				case StunPacket::Class::ERROR_RESPONSE:
				{
					// MESSAGE-INTEGRITY Attribute must be present.
					if (!messageIntegrity)
					{
						MS_WARN_TAG(
						  ice, "cannot authenticate request or indication, missing MESSAGE-INTEGRITY Attribute");

						return StunPacket::AuthenticationResult::BAD_MESSAGE;
					}

					break;
				}

				default:
				{
					MS_WARN_TAG(
					  ice,
					  "cannot authenticate packet, unknown STUN class %" PRIu16,
					  static_cast<uint16_t>(this->klass));

					return StunPacket::AuthenticationResult::BAD_MESSAGE;
				}
			}

			auto* fixedHeader = GetFixedHeaderPointer();

			// If there is FINGERPRINT it must be discarded for MESSAGE-INTEGRITY
			// calculation, so the message length field must be modified (and later
			// restored).
			if (hasFingerprint)
			{
				// Set the message length field by removing the length of the
				// FINGERPRINT Attribute (4 + 4).
				// NOTE: We cannot use SetMessageLength() because CheckAuthentication()
				// is marked as a `const` method.
				Utils::Byte::Set2Bytes(
				  fixedHeader, 2, static_cast<uint16_t>(GetAttributesLength() - 4 - 4));
			}

			// Calculate the HMAC-SHA1 of the message according to MESSAGE-INTEGRITY
			// rules, this is, by checking the bytes from 0 to the beginning of the
			// MESSAGE-INTEGRITY Attribute.
			const uint8_t* computedMessageIntegrity =
			  Utils::Crypto::GetHmacSha1(password, fixedHeader, (messageIntegrity - 4) - fixedHeader);

			StunPacket::AuthenticationResult result;

			// Compare the computed HMAC-SHA1 with the MESSAGE-INTEGRITY in the packet.
			if (std::memcmp(messageIntegrity, computedMessageIntegrity, StunPacket::FixedHeaderLength) == 0)
			{
				result = StunPacket::AuthenticationResult::OK;
			}
			else
			{
				result = StunPacket::AuthenticationResult::UNAUTHORIZED;
			}

			// Restore the message length field.
			// NOTE: We cannot use SetMessageLength() because CheckAuthentication()
			// is marked as a `const` method.
			if (hasFingerprint)
			{
				Utils::Byte::Set2Bytes(
				  fixedHeader, 2, static_cast<uint16_t>(GetAttributesLength()));
			}

			return result;
		}

		void StunPacket::Protect(std::string& password)
		{
			MS_TRACE();

			if (this->klass == StunPacket::Class::SUCCESS_RESPONSE)
			{
				MS_THROW_ERROR("cannot protect a STUN success response");
			}
			else if (this->klass == StunPacket::Class::ERROR_RESPONSE)
			{
				MS_THROW_ERROR("cannot protect a STUN error response");
			}
			else if (HasAttribute(StunPacket::AttributeType::MESSAGE_INTEGRITY))
			{
				MS_THROW_ERROR("cannot protect Packet, it already has MESSAGE-INTEGRITY Attribute");
			} else if (HasAttribute(StunPacket::AttributeType::FINGERPRINT))
			{
				MS_THROW_ERROR("cannot protect Packet, it already has FINGERPRINT Attribute");
			}

			// We need to add MESSAGE-INTEGRITY and FINGERPRINT Attributes so must
			// increase the length of the Packet.
			// NOTE: This may throw.
			SetLength(GetLength() + 4 + StunPacket::MessageIntegrityAttributeLength + 4 + 4);



			// TODO: Use SetMessageLength()
		}

		bool StunPacket::Validate(bool storeAttributes)
		{
			MS_TRACE();

			const auto* fixedHeader = GetFixedHeaderPointer();

			// Get message length field.
			const auto msgLength = GetMessageLength();

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
					  static_cast<uint16_t>(attrType));

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

						if (storeAttributes && !StoreParsedAttribute(attrType, attrLen, attrOffset))
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

						if (storeAttributes && !StoreParsedAttribute(attrType, attrLen, attrOffset))
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

						if (storeAttributes && !StoreParsedAttribute(attrType, attrLen, attrOffset))
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

						if (storeAttributes && !StoreParsedAttribute(attrType, attrLen, attrOffset))
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

						if (storeAttributes && !StoreParsedAttribute(attrType, attrLen, attrOffset))
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

						if (storeAttributes && !StoreParsedAttribute(attrType, attrLen, attrOffset))
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

						if (storeAttributes && !StoreParsedAttribute(attrType, attrLen, attrOffset))
						{
							return false;
						}

						break;
					}

					case StunPacket::AttributeType::XOR_MAPPED_ADDRESS:
					{
						if (attrLen != StunPacket::XorMappedAddressIPv4Length && attrLen != StunPacket::XorMappedAddressIPv6Length)
						{
							MS_WARN_TAG(
							  ice,
							  "invalid Packet, Attribute XOR_MAPPED_ADDRESS-CODE must be %zu or %zu bytes length",
							  StunPacket::XorMappedAddressIPv4Length,
							  StunPacket::XorMappedAddressIPv6Length);

							return false;
						}

						if (storeAttributes && !StoreParsedAttribute(attrType, attrLen, attrOffset))
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

						if (storeAttributes && !StoreParsedAttribute(attrType, attrLen, attrOffset))
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

						if (storeAttributes && !StoreParsedAttribute(attrType, attrLen, attrOffset))
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

						if (storeAttributes && !StoreParsedAttribute(attrType, attrLen, attrOffset))
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

		bool StunPacket::StoreParsedAttribute(AttributeType type, uint16_t len, size_t offset)
		{
			MS_TRACE();

			if (!this->attributes.try_emplace(type, type, len, offset).second)
			{
				MS_WARN_TAG(
				  ice,
				  "cannot store parsed Attribute with type %" PRIu16
				  ", there is an Attribute with same type already in the map",
				  static_cast<uint16_t>(type));

				return false;
			}

			return true;
		}

		void StunPacket::StoreNewAttribute(AttributeType type, const void* data, uint16_t len)
		{
			MS_TRACE();

			MS_ASSERT(
			  (data && len) || (!data && !len),
			  "data and len must either both have a value or both be empty/zero");

			if (this->attributes.find(type) != this->attributes.end())
			{
				MS_THROW_ERROR(
				  "cannot store new Attribute with type %" PRIu16
				  ", there is an Attribute with same type already in the map",
				  static_cast<uint16_t>(type));
			}

			// Add the Attribute at the end of the STUN Packet.

			const auto attrTotalPaddedLength = Utils::Byte::PadTo4Bytes(static_cast<size_t>(4 + len));

			// Get the pointer in which the new Attribute must be written.
			// NOTE: Do this before updating lengths.
			auto* attrPtr = GetAttributesPointer() + GetAttributesLength();

			// First update Packet length (it may throw).
			SetLength(GetLength() + attrTotalPaddedLength);

			// Also update the message length field.
			SetMessageLength(GetAttributesLength());

			Utils::Byte::Set2Bytes(attrPtr, 0, static_cast<uint16_t>(type));
			Utils::Byte::Set2Bytes(attrPtr, 2, len);

			if (data)
			{
				std::memcpy(attrPtr + 4, data, len);
				// Fill padding bytes with zeroes.
				std::memset(attrPtr + 4 + len, 0x00, attrTotalPaddedLength - len);
			}

			const auto [it, inserted] = this->attributes.try_emplace(type, type, len, 0);
			auto& attribute           = it->second;

			// Update stored Attribute's offset.
			attribute.offset = attrPtr - GetAttributesPointer();

			MS_ASSERT(inserted, "Attribute not inserted in the map (this shouldn't happen)");
		}
	} // namespace ICE
} // namespace RTC
