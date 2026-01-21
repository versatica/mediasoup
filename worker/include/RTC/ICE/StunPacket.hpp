#ifndef MS_RTC_ICE_STUN_PACKET_HPP
#define MS_RTC_ICE_STUN_PACKET_HPP

#include "common.hpp"
#include "RTC/Serializable.hpp"
#include <string>

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
			 * After the STUN Message Header are zero or more attributes. Each
			 * attribute MUST be TLV encoded, with a 16-bit type, 16-bit length, and
			 * value. Each STUN attribute MUST end on a 32-bit boundary.  All fields
			 * in an attribute are transmitted most significant bit first.
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
			 * STUN attribute type.
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
				AttributeType type;
				uint16_t len;
				uint8_t value[];
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

			StunPacket::Method GetMethod() const
			{
				return this->method;
			}

			const uint8_t* GetTransactionId() const
			{
				return GetTransactionIdPointer();
			}

			bool HasUsername() const
			{
				return !this->username.empty();
			}

			const std::string& GetUsername() const
			{
				return this->username;
			}

			bool HasPriority() const
			{
				return this->priority.has_value();
			}

			uint32_t GetPriority() const
			{
				return this->priority.value_or(0);
			}

			bool HasIceControlling() const
			{
				return this->iceControlling.has_value();
			}

			uint64_t GetIceControlling() const
			{
				return this->iceControlling.value_or(0);
			}

			bool HasIceControlled() const
			{
				return this->iceControlled.has_value();
			}

			uint64_t GetIceControlled() const
			{
				return this->iceControlled.value_or(0);
			}

			bool HasNomination() const
			{
				return this->nomination.has_value();
			}

			bool HasUseCandidate() const
			{
				return this->hasUseCandidate;
			}

			uint32_t GetNomination() const
			{
				return this->nomination.value_or(0);
			}

			bool HasSoftware() const
			{
				return !this->software.empty();
			}

			const std::string& GetSoftware() const
			{
				return this->software;
			}

			bool HasXorMappedAddress() const
			{
				return this->xorMappedAddress != nullptr;
			}

			bool HasErrorCode() const
			{
				return this->errorCode.has_value();
			}

			uint16_t GetErrorCode() const
			{
				return this->errorCode.value_or(0);
			}

			bool HasMessageIntegrity() const
			{
				return this->messageIntegrity != nullptr;
			}

			bool HasFingerprint() const
			{
				return this->hasFingerprint;
			}

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

			/**
			 * Validates whether the STUN Packet is valid. It also sets internal
			 * offsets pointing to relevant STUN attributes.
			 */
#ifdef MS_TEST
		public:
#endif
			bool Validate();
#ifdef MS_TEST
		private:
#endif

			void SetClass(StunPacket::Class klass);

			void SetMethod(StunPacket::Method method);

			void SetTransactionId(const uint8_t* transactionId);

			void SetUsername(const char* username, size_t len);

			void SetPriority(uint32_t priority);

			void SetIceControlling(uint64_t iceControlling);

			void SetIceControlled(uint64_t iceControlled);

			void EnableUseCandidate();

			void SetNomination(uint32_t nomination);

			void SetSoftware(const char* software, size_t len);

			void SetXorMappedAddress(const struct sockaddr* xorMappedAddress);

			void SetErrorCode(uint16_t errorCode);

			/**
			 * @remarks
			 * - The given pointer and memory must remain accesible during the
			 *   lifetime of the Packet.
			 */
			void SetMessageIntegrity(const uint8_t* messageIntegrity);

			void EnableFingerprint();

			void SetPassword(const std::string& password);

		private:
			StunPacket::Class klass;
			StunPacket::Method method;
			// STUN attributes.
			std::string username; // Less than 513 bytes.
			std::optional<uint32_t> priority;
			std::optional<uint64_t> iceControlling;
			std::optional<uint64_t> iceControlled;
			bool hasUseCandidate{ false };
			std::optional<uint32_t> nomination;
			std::string software; // Less than 763 bytes.
			const struct sockaddr* xorMappedAddress{ nullptr };
			std::optional<uint16_t> errorCode;
			const uint8_t* messageIntegrity{ nullptr };
			bool hasFingerprint{ false };
			// Others.
			std::string password;
		};
	} // namespace ICE
} // namespace RTC

#endif
