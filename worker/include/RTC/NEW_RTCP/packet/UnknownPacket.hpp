#ifndef MS_RTC_NEW_RTCP_UNKNOWN_PACKET_HPP
#define MS_RTC_NEW_RTCP_UNKNOWN_PACKET_HPP

#include "common.hpp"
#include "RTC/NEW_RTCP/packet/Packet.hpp"

namespace RTC
{
	namespace NEW_RTCP
	{
		/**
		 * RTCP Unknown Packet
		 *
		 * @see RFC 3550.
		 *
		 *        0                   1                   2                   3
		 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |V=2|P|    SC   |       PT      |             Length            |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * \                                                               \
		 * /                          Unknown Value                        /
		 * \                                                               \
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 */

		// Forward declaration.
		class Packet;

		class UnknownPacket : public Packet
		{
			// We need that compound packet calls protected and private methods in
			// this class.
			friend class CompoundPacket;

		public:
			/**
			 * Parse a UnknownPacket.
			 *
			 * @remarks
			 * `bufferLength` may exceed the exact length of the packet.
			 */
			static UnknownPacket* Parse(const uint8_t* buffer, size_t bufferLength);

		private:
			/**
			 * Parse a UnknownPacket.
			 *
			 * @remarks
			 * To be used only by `Packet::Parse()`.
			 */
			static UnknownPacket* ParseStrict(const uint8_t* buffer, size_t bufferLength, size_t packetLength);

		private:
			/**
			 * Only used by Parse() and ParseStrict() static methods.
			 */
			UnknownPacket(uint8_t* buffer, size_t bufferLength);

		public:
			~UnknownPacket() override;

			void Dump(int indentation = 0) const final;

			UnknownPacket* Clone(uint8_t* buffer, size_t bufferLength) const final;

			bool HasUnknownType() const override
			{
				return true;
			}

			bool HasUnknownValue() const
			{
				return HasVariableLengthValue();
			}

			const uint8_t* GetUnknownValue() const
			{
				return GetVariableLengthValue();
			}

			uint16_t GetUnknownValueLength() const
			{
				return GetVariableLengthValueLength();
			}

		protected:
			UnknownPacket* SoftClone(const uint8_t* buffer) const final;
		};
	} // namespace NEW_RTCP
} // namespace RTC

#endif
