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
			enum class Attribute : uint16_t
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
			enum class Authentication : uint8_t
			{
				OK           = 0,
				UNAUTHORIZED = 1,
				BAD_MESSAGE  = 2
			};

		public:
			/**
			 * Message Header fixed length.
			 */
			static const size_t FixedHeaderLength{ 20 };

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
			static const uint8_t MagicCookie[];

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

		private:
			void SetClass(StunPacket::Class klass);

			void SetMethod(StunPacket::Method method);

		private:
			StunPacket::Class klass;
			StunPacket::Method method;
		};
	} // namespace ICE
} // namespace RTC

#endif
