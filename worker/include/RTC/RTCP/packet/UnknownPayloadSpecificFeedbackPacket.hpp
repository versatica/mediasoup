#ifndef MS_RTC_RTCP_UNKNOWN_PAYLOAD_SPECIFIC_FEEDBACK_PACKET_HPP
#define MS_RTC_RTCP_UNKNOWN_PAYLOAD_SPECIFIC_FEEDBACK_PACKET_HPP

#include "common.hpp"
#include "RTC/RTCP/packet/AnyPayloadSpecificFeedbackPacket.hpp"

namespace RTC
{
	namespace RTCP
	{
		/**
		 * Payload-Specific Feedback Packet (PSFB 206) with an unknown or not
		 * implemented message type.
		 *
		 * @see RFC 4585.
		 *
		 *  0                   1                   2                   3
		 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |V=2|P|   FMT   |  PT=PSFB=206  |             Length            |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |                     SSRC of packet sender                     |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |                      SSRC of media source                     |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * \                                                               \
		 * /                          Unknown Value                        /
		 * \                                                               \
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 *
		 * @remarks
		 * - An Application Layer Feedback (AFB) message whose application is not
		 *   implemented is also represented by this class.
		 */
		class UnknownPayloadSpecificFeedbackPacket : public AnyPayloadSpecificFeedbackPacket
		{
			// We need that compound packet calls protected and private methods in
			// this class.
			friend class CompoundPacket;

		public:
			/**
			 * Parse an UnknownPayloadSpecificFeedbackPacket.
			 *
			 * @remarks
			 * - `bufferLength` may exceed the exact length of the packet.
			 */
			static UnknownPayloadSpecificFeedbackPacket* Parse(const uint8_t* buffer, size_t bufferLength);

		private:
			/**
			 * Parse an UnknownPayloadSpecificFeedbackPacket.
			 *
			 * @remarks
			 * - To be used only by `CompoundPacket::Parse()`.
			 */
			static UnknownPayloadSpecificFeedbackPacket* ParseStrict(
			  const uint8_t* buffer, size_t bufferLength, size_t packetLength, uint8_t padding);

		private:
			/**
			 * Only used by ParseStrict(), Clone() and SoftClone() methods.
			 */
			UnknownPayloadSpecificFeedbackPacket(uint8_t* buffer, size_t bufferLength);

		public:
			~UnknownPayloadSpecificFeedbackPacket() override;

			void Dump(int indentation = 0) const final;

			UnknownPayloadSpecificFeedbackPacket* Clone(uint8_t* buffer, size_t bufferLength) const final;

			bool HasUnknownMessageType() const override
			{
				return true;
			}

			bool HasUnknownValue() const
			{
				return GetLength() - GetPaddingLength() > AnyFeedbackPacket::FeedbackHeaderLength;
			}

			const uint8_t* GetUnknownValue() const
			{
				if (!HasUnknownValue())
				{
					return nullptr;
				}

				return GetBuffer() + AnyFeedbackPacket::FeedbackHeaderLength;
			}

			size_t GetUnknownValueLength() const
			{
				return GetLength() - GetPaddingLength() - AnyFeedbackPacket::FeedbackHeaderLength;
			}

		private:
			UnknownPayloadSpecificFeedbackPacket* SoftClone(const uint8_t* buffer) const final;
		};
	} // namespace RTCP
} // namespace RTC

#endif
