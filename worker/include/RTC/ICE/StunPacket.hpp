#ifndef MS_RTC_ICE_STUN_PACKET_HPP
#define MS_RTC_ICE_STUN_PACKET_HPP

#include "common.hpp"
#include "Utils.hpp"
#include "RTC/Serializable.hpp"
#include <cstdint>
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
			static const size_t FixedHeaderLength{ 20 };
			static const size_t TransactionIdLength{ 12 };
			static const size_t UsernameAttributeMaxLength{ 513 };
			static const size_t XorMappedAddressIPv4Length{ 8 };
			static const size_t XorMappedAddressIPv6Length{ 20 };
			static const size_t SoftwareAttributeMaxLength{ 763 };
			static const size_t MessageIntegrityAttributeLength{ 20 };
			static const size_t FingerprintAttributeLength{ 4 };

			/**
			 * Whether given buffer could be a valid STUN Packet.
			 */
			static bool IsStun(const uint8_t* buffer, size_t bufferLength);

			/**
			 * Parse a STUN Packet.
			 *
			 * @remarks
			 * - `bufferLength` must be the exact length of the STUN Packet.
			 */
			static StunPacket* Parse(const uint8_t* buffer, size_t bufferLength);

			/**
			 * Create a STUN Packet.
			 *
			 * @remarks
			 * - `bufferLength` must be the exact length of the STUN Packet.
			 * - If `transactionId` is not given then a random Transaction ID is
			 *   generated.
			 */
			static StunPacket* Factory(
			  uint8_t* buffer,
			  size_t bufferLength,
			  StunPacket::Class klass,
			  StunPacket::Method method,
			  const uint8_t* transactionId);

			static StunPacket* Factory(
			  uint8_t* buffer, size_t bufferLength, StunPacket::Class klass, StunPacket::Method method);

		private:
			static const uint8_t MagicCookie[];

		private:
			/**
			 * Constructor is private because we only want to create STUN Packet
			 * instances via Parse() and Factory().
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

			StunPacket::Method GetMethod() const
			{
				return this->method;
			}

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

			void AddUsername(const std::string_view username);

			uint32_t GetPriority() const
			{
				const auto* attribute = GetAttribute(StunPacket::AttributeType::PRIORITY);

				if (!attribute)
				{
					return 0;
				}

				return Utils::Byte::Get4Bytes(GetAttributeValue(attribute), 0);
			}

			void AddPriority(uint32_t priority);

			uint64_t GetIceControlling() const
			{
				const auto* attribute = GetAttribute(StunPacket::AttributeType::ICE_CONTROLLING);

				if (!attribute)
				{
					return 0;
				}

				return Utils::Byte::Get8Bytes(GetAttributeValue(attribute), 0);
			}

			void AddIceControlling(uint64_t iceControlling);

			uint64_t GetIceControlled() const
			{
				const auto* attribute = GetAttribute(StunPacket::AttributeType::ICE_CONTROLLED);

				if (!attribute)
				{
					return 0;
				}

				return Utils::Byte::Get8Bytes(GetAttributeValue(attribute), 0);
			}

			void AddIceControlled(uint64_t iceControlled);

			void AddUseCandidate();

			uint32_t GetNomination() const
			{
				const auto* attribute = GetAttribute(StunPacket::AttributeType::NOMINATION);

				if (!attribute)
				{
					return 0;
				}

				return Utils::Byte::Get4Bytes(GetAttributeValue(attribute), 0);
			}

			void AddNomination(uint32_t nomination);

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

			void AddSoftware(const std::string_view software);

			bool GetXorMappedAddress(struct sockaddr_storage* xorMappedAddressStorage) const;

			void AddXorMappedAddress(const struct sockaddr* xorMappedAddress);

			uint16_t GetErrorCode(std::string_view& reasonPhrase) const
			{
				const auto* attribute = GetAttribute(StunPacket::AttributeType::ERROR_CODE);

				if (!attribute)
				{
					return 0;
				}

				const auto* attributeValue = GetAttributeValue(attribute);

				const uint8_t errorClass  = Utils::Byte::Get1Byte(attributeValue, 2) & 0b00000111;
				const uint8_t errorNumber = Utils::Byte::Get1Byte(attributeValue, 3);
				auto errorCode            = static_cast<uint16_t>((errorClass * 100) + errorNumber);

				// Reason Phrase comes after the first 4 bytes (it could be zero
				// length).
				if (attribute->len > 4)
				{
					reasonPhrase = std::string_view(
					  reinterpret_cast<const char*>(GetAttributeValue(attribute) + 4), attribute->len - 4);
				}
				else
				{
					reasonPhrase = {};
				}

				return errorCode;
			}

			void AddErrorCode(uint16_t errorCode, const std::string_view reasonPhrase);

			/**
			 * Check authentication of the STUN request or notification.
			 */
			StunPacket::AuthenticationResult CheckAuthentication(
			  const std::string_view usernameFragment1, const std::string_view& password) const;

			/**
			 * Check authentication of the STUN success response or error response.
			 */
			StunPacket::AuthenticationResult CheckAuthentication(std::string_view password) const;

			/**
			 * Whether the STUN Packet is protected, meaning that it has
			 * MESSAGE-INTEGRITY and/or FINGERPRINT Attributes.
			 */
			bool IsProtected() const
			{
				return (
				  HasAttribute(StunPacket::AttributeType::MESSAGE_INTEGRITY) ||
				  HasAttribute(StunPacket::AttributeType::FINGERPRINT));
			}

			/**
			 * Adds MESSAGE-INTEGRITY and FINGERPRINT to the STUN Packet.
			 *
			 * @remarks
			 * - MESSAGE-INTEGRITY is only added if given `password` is not empty.
			 * - The application MUST NOT add more Attributes into the STUN Packet
			 *   and MUST NOT modify any field of the STUN Packet after calling
			 *   this method.
			 *
			 * @throw MediaSoupTypeError - If there is no enough space in the buffer.
			 * @throw MediaSoupError - If the STUN Packet already has MESSAGE-INTEGRITY
			 *   or FINGERPRINT Attributes.
			 */
			void Protect(const std::string_view password);

			void Protect();

			/**
			 * Creates a STUN success response for the current STUN request.
			 *
			 * @throw MediaSoupError - If the STUN Packet is not a STUN request.
			 */
			StunPacket* CreateSuccessResponse(uint8_t* buffer, size_t bufferLength) const;

			/**
			 * Creates a STUN error response for the current STUN request. It uses
			 * given `errorCode` and `reasonPhrase` to add a ERROR-CODE Attribute.
			 *
			 * @throw MediaSoupError - If the STUN Packet is not a STUN request.
			 */
			StunPacket* CreateErrorResponse(
			  uint8_t* buffer,
			  size_t bufferLength,
			  uint16_t errorCode,
			  const std::string_view& reasonPhrase) const;

		private:
			uint8_t* GetFixedHeaderPointer() const
			{
				return const_cast<uint8_t*>(GetBuffer());
			}

			uint16_t GetMessageLength() const
			{
				return Utils::Byte::Get2Bytes(GetFixedHeaderPointer(), 2);
			}

			void SetMessageLength(uint16_t msgLength)
			{
				Utils::Byte::Set2Bytes(GetFixedHeaderPointer(), 2, msgLength);
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

			/**
			 * Stores the parsed Attribute data into the map of Attributes.
			 *
			 * @return `true` if the Attribute was stored and `false` if it couldn't
			 *   be stored because there was already an Attribute with same type in
			 *   the map.
			 */
			bool StoreParsedAttribute(StunPacket::AttributeType type, uint16_t len, size_t offset);

			/**
			 * Stores a new Attribute data into the map of Attributes.
			 *
			 * @throw MediaSoupError - If there is not enough space in the buffer for
			 *   the new Attribute or if the Attribute couldn't be stored because
			 *   there was already an Attribute with same type in the map.
			 */
			void StoreNewAttribute(StunPacket::AttributeType type, const void* data, uint16_t len);

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

			void AssertNotProtected() const;

		private:
			StunPacket::Class klass;
			StunPacket::Method method;
			// Map of STUN Attributes indexed by Attribute type.
			std::unordered_map<StunPacket::AttributeType, StunPacket::Attribute> attributes;
		};
	} // namespace ICE
} // namespace RTC

#endif
