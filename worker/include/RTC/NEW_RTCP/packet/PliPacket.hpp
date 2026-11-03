#ifndef MS_RTC_NEW_RTCP_PLI_PACKET_HPP
#define MS_RTC_NEW_RTCP_PLI_PACKET_HPP

#include "common.hpp"
#include "RTC/NEW_RTCP/packet/AnyPayloadSpecificFeedbackPacket.hpp"

namespace RTC
{
	namespace NEW_RTCP
	{
		/**
		 * Picture Loss Indication Payload-Specific Feedback Packet (PSFB 206, FMT
		 * 1).
		 *
		 * @see RFC 4585.
		 *
		 *  0                   1                   2                   3
		 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |V=2|P|  FMT=1  |  PT=PSFB=206  |           Length=2            |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |                     SSRC of packet sender                     |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |                      SSRC of media source                     |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 *
		 * @remarks
		 * - PLI has no FCI, so the Length field must be 2.
		 */
		class PliPacket : public AnyPayloadSpecificFeedbackPacket
		{
			// We need that compound packet calls protected and private methods in
			// this class.
			friend class CompoundPacket;

		public:
			/**
			 * Parse a PliPacket.
			 *
			 * @remarks
			 * - `bufferLength` may exceed the exact length of the packet.
			 */
			static PliPacket* Parse(const uint8_t* buffer, size_t bufferLength);

			/**
			 * Create a PliPacket.
			 *
			 * @remarks
			 * - `bufferLength` could be greater than the packet real length.
			 *
			 * @throw MediaSoupTypeError - If given `bufferLength` is lower than
			 *   `FeedbackHeaderLength`.
			 */
			static PliPacket* Factory(uint8_t* buffer, size_t bufferLength);

		private:
			/**
			 * Parse a PliPacket.
			 *
			 * @remarks
			 * - To be used only by `CompoundPacket::Parse()`.
			 */
			static PliPacket* ParseStrict(
			  const uint8_t* buffer, size_t bufferLength, size_t packetLength, uint8_t padding);

		private:
			/**
			 * Only used by ParseStrict(), Factory(), Clone() and SoftClone() methods.
			 */
			PliPacket(uint8_t* buffer, size_t bufferLength);

		public:
			~PliPacket() override;

			void Dump(int indentation = 0) const final;

			PliPacket* Clone(uint8_t* buffer, size_t bufferLength) const final;

		private:
			PliPacket* SoftClone(const uint8_t* buffer) const final;
		};
	} // namespace NEW_RTCP
} // namespace RTC

#endif
