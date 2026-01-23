#ifndef MS_RTC_ICE_STUN_PACKET_HPP
#define MS_RTC_ICE_STUN_PACKET_HPP

#include "common.hpp"
#include "Utils.hpp"
#include "RTC/Serializable.hpp"
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

namespace RTC
{
	namespace ICE
	{
		/**
		 * STUN Packet.
		 *
		 * @see RFC 5389.
		 * @see RFC 8445.
		 */
		class StunPacket : public Serializable
		{
		public:
			/**
			 * STUN Message Header.
			 *
			 * @see RFC 5389 section 6.
			 *
			 *  0                   1                   2                   3
			 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
			 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
			 * |0 0|     STUN Message Type     |         Message Length        |
			 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
			 * |                         Magic Cookie                          |
			 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
			 * |                                                               |
			 * |                     Transaction ID (96 bits)                  |
			 * |                                                               |
			 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
			 */

			/**
			 * The STUN Message Type field is decomposed further into the following
			 * structure:
			 *
			 *  0                 1
			 *  2  3  4 5 6 7 8 9 0 1 2 3 4 5
			 * +--+--+-+-+-+-+-+-+-+-+-+-+-+-+
			 * |M |M |M|M|M|C|M|M|M|C|M|M|M|M|
			 * |11|10|9|8|7|1|6|5|4|0|3|2|1|0|
			 * +--+--+-+-+-+-+-+-+-+-+-+-+-+-+
			 *
			 * Here the bits in the message type field are shown as most significant
			 * (M11) through least significant (M0).  M11 through M0 represent a
			 * 12-bit encoding of the method.  C1 and C0 represent a 2-bit encoding
			 * of the class.
			 */

			/**
			 * STUN Attributes.
			 *
			 * After the STUN Message Header are zero or more Attributes. Each
			 * Attribute MUST be TLV encoded, with a 16-bit type, 16-bit length, and
			 * value. Each STUN Attribute MUST end on a 32-bit boundary.  All fields
			 * in an Attribute are transmitted most significant bit first.
			 *
			 *  0                   1                   2                   3
			 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
			 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
			 * |         Type                  |            Length             |
			 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
			 * |                         Value (variable)                ....
			 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
			 */

			/**
			 * STUN message class.
			 */
			enum class Class : uint8_t
			{
				REQUEST          = 0,
				INDICATION       = 1,
				SUCCESS_RESPONSE = 2,
				ERROR_RESPONSE   = 3
			};

			/**
			 * STUN message method.
			 */
			enum class Method : uint16_t
			{
				BINDING = 1
			};

			/**
			 * STUN Attribute type.
			 */
			enum class AttributeType : uint16_t
			{
				MAPPED_ADDRESS     = 0x0001,
				USERNAME           = 0x0006,
				MESSAGE_INTEGRITY  = 0x0008,
				ERROR_CODE         = 0x0009,
				UNKNOWN_ATTRIBUTES = 0x000A,
				REALM              = 0x0014,
				NONCE              = 0x0015,
				XOR_MAPPED_ADDRESS = 0x0020,
				PRIORITY           = 0x0024,
				USE_CANDIDATE      = 0x0025,
				SOFTWARE           = 0x8022,
				ALTERNATE_SERVER   = 0x8023,
				FINGERPRINT        = 0x8028,
				ICE_CONTROLLED     = 0x8029,
				ICE_CONTROLLING    = 0x802A,
				NOMINATION         = 0xC001
			};

			// Authentication result.
			enum class AuthenticationResult : uint8_t
			{
				OK           = 0,
				UNAUTHORIZED = 1,
				BAD_MESSAGE  = 2
			};

		private:
			struct Attribute
			{
				Attribute(AttributeType type, uint16_t len, size_t offset)
				  : type(type), len(len), offset(offset) {};

				/**
				 * Attribute type.
				 */
				AttributeType type;
				/**
				 * Length of the value (not padded).
				 */
				uint16_t len;
				/**
				 * Offset of the Attribute from the start of the Attributes.
				 */
				size_t offset;
			};

		public:
			/**
			 * Whether given buffer could be a valid STUN Packet.
			 */
			static bool IsStun(const uint8_t* buffer, size_t bufferLength);

			/**
			 * Parse a STUN Packet.
			 *
			 * @remarks
			 * - `bufferLength` must be the exact length of the Packet.
			 */
			static StunPacket* Parse(const uint8_t* buffer, size_t bufferLength);

		private:
			static const size_t FixedHeaderLength{ 20 };
			static const uint8_t MagicCookie[];
			static const size_t TransactionIdLength{ 12 };
			static const size_t UsernameAttributeMaxLength{ 513 };
			static const size_t SoftwareAttributeMaxLength{ 763 };
			static const size_t MessageIntegrityAttributeLength{ 20 };

		private:
			/**
			 * Constructor is private because we only want to create Packet instances
			 * via Parse().
			 */
			StunPacket(uint8_t* buffer, size_t bufferLength);

		public:
			~StunPacket() override;

			void Dump(int indentation = 0) const final;

			StunPacket* Clone(uint8_t* buffer, size_t bufferLength) const final;

			StunPacket::Class GetClass() const
			{
				return this->klass;
			}

			void SetClass(StunPacket::Class klass);

			StunPacket::Method GetMethod() const
			{
				return this->method;
			}

			void SetMethod(StunPacket::Method method);

			const uint8_t* GetTransactionId() const
			{
				return GetTransactionIdPointer();
			}

			void SetTransactionId(const uint8_t* transactionId);

			bool HasAttribute(StunPacket::AttributeType type) const
			{
				return this->attributes.find(type) != this->attributes.end();
			}

			const std::string_view GetUsername() const
			{
				const auto* attribute = GetAttribute(StunPacket::AttributeType::USERNAME);

				if (!attribute)
				{
					return {};
				}

				return std::string_view(
				  reinterpret_cast<const char*>(GetAttributeValue(attribute)), attribute->len);
			}

			void SetUsername(const char* username, size_t len);

			uint32_t GetPriority() const
			{
				const auto* attribute = GetAttribute(StunPacket::AttributeType::PRIORITY);

				if (!attribute)
				{
					return 0;
				}

				return Utils::Byte::Get4Bytes(GetAttributeValue(attribute), 0);
			}

			void SetPriority(uint32_t priority);

			uint64_t GetIceControlling() const
			{
				const auto* attribute = GetAttribute(StunPacket::AttributeType::ICE_CONTROLLING);

				if (!attribute)
				{
					return 0;
				}

				return Utils::Byte::Get8Bytes(GetAttributeValue(attribute), 0);
			}

			void SetIceControlling(uint64_t iceControlling);

			uint64_t GetIceControlled() const
			{
				const auto* attribute = GetAttribute(StunPacket::AttributeType::ICE_CONTROLLED);

				if (!attribute)
				{
					return 0;
				}

				return Utils::Byte::Get8Bytes(GetAttributeValue(attribute), 0);
			}

			void SetIceControlled(uint64_t iceControlled);

			void EnableUseCandidate();

			uint32_t GetNomination() const
			{
				const auto* attribute = GetAttribute(StunPacket::AttributeType::ICE_CONTROLLED);

				if (!attribute)
				{
					return 0;
				}

				return Utils::Byte::Get4Bytes(GetAttributeValue(attribute), 0);
			}

			void SetNomination(uint32_t nomination);

			const std::string_view GetSoftware() const
			{
				const auto* attribute = GetAttribute(StunPacket::AttributeType::SOFTWARE);

				if (!attribute)
				{
					return {};
				}

				return std::string_view(
				  reinterpret_cast<const char*>(GetAttributeValue(attribute)), attribute->len);
			}

			void SetSoftware(const char* software, size_t len);

			uint16_t GetErrorCode() const
			{
				const auto* attribute = GetAttribute(StunPacket::AttributeType::ERROR_CODE);

				if (!attribute)
				{
					return 0;
				}

				const auto* attributeValue = GetAttributeValue(attribute);
				const uint8_t errorClass   = Utils::Byte::Get1Byte(attributeValue, 2);
				const uint8_t errorNumber  = Utils::Byte::Get1Byte(attributeValue, 3);
				auto errorCode             = static_cast<uint16_t>((errorClass * 100) + errorNumber);

				return errorCode;
			}

			void SetXorMappedAddress(const struct sockaddr* xorMappedAddress);

			void SetErrorCode(uint16_t errorCode);

			void EnableFingerprint();

			void SetPassword(const std::string& password);

		private:
			uint8_t* GetFixedHeaderPointer() const
			{
				return const_cast<uint8_t*>(GetBuffer());
			}

			uint8_t* GetTransactionIdPointer() const
			{
				return GetFixedHeaderPointer() + 8;
			}

			uint8_t* GetAttributesPointer() const
			{
				return GetFixedHeaderPointer() + StunPacket::FixedHeaderLength;
			}

			size_t GetAttributesLength() const
			{
				return GetLength() - StunPacket::FixedHeaderLength;
			}

			/**
			 * Validates whether the STUN Packet is valid. It also stores internal
			 * offsets pointing to relevant STUN Attributes if `storeAttributes` is
			 * `true`.
			 */
#ifdef MS_TEST
		public:
#endif
			bool Validate(bool storeAttributes);
#ifdef MS_TEST
		private:
#endif

			/**
			 * Parses Attributes. Returns `true` if they are valid. It also stores
			 * internal containers holding Attributes if `storeAttributes` is `true`.
			 */
			bool ParseAttributes(bool storeAttributes);

			bool StoreAttribute(StunPacket::AttributeType type, uint16_t len, size_t offset);

			const StunPacket::Attribute* GetAttribute(StunPacket::AttributeType type) const
			{
				auto it = this->attributes.find(type);

				if (it != this->attributes.end())
				{
					return std::addressof(it->second);
				}

				return nullptr;
			}

			const uint8_t* GetAttributeValue(const StunPacket::Attribute* attribute) const
			{
				return GetAttributesPointer() + attribute->offset + 4;
			}

#ifdef MS_TEST
		public:
#endif
			const uint8_t* GetMessageIntegrity() const
			{
				const auto* attribute = GetAttribute(StunPacket::AttributeType::MESSAGE_INTEGRITY);

				if (!attribute)
				{
					return nullptr;
				}

				return GetAttributeValue(attribute);
			}
#ifdef MS_TEST
		private:
#endif

			void SetMessageIntegrity(const uint8_t* messageIntegrity);

#ifdef MS_TEST
		public:
#endif
			uint32_t GetFingerprint() const
			{
				const auto* attribute = GetAttribute(StunPacket::AttributeType::FINGERPRINT);

				if (!attribute)
				{
					return 0;
				}

				return Utils::Byte::Get4Bytes(GetAttributeValue(attribute), 0);
			}
#ifdef MS_TEST
		private:
#endif

		private:
			StunPacket::Class klass;
			StunPacket::Method method;
			// Map of STUN Attributes indexed by Attribute type.
			std::unordered_map<StunPacket::AttributeType, StunPacket::Attribute> attributes;
			const struct sockaddr* xorMappedAddress{ nullptr };
			std::string password;
		};
	} // namespace ICE
} // namespace RTC

#endif
