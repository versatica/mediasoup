#ifndef MS_RTC_RTCP_UNKNOWN_TRANSPORT_LAYER_FEEDBACK_PACKET_HPP
#define MS_RTC_RTCP_UNKNOWN_TRANSPORT_LAYER_FEEDBACK_PACKET_HPP

#include "common.hpp"
#include "RTC/RTCP/packet/AnyTransportLayerFeedbackPacket.hpp"

namespace RTC
{
	namespace RTCP
	{
		/**
		 * Transport Layer Feedback Packet (RTPFB 205) with an unknown or not
		 * implemented message type.
		 *
		 * @see RFC 4585.
		 *
		 *  0                   1                   2                   3
		 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |V=2|P|   FMT   |  PT=RTPFB=205 |             Length            |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |                     SSRC of packet sender                     |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |                      SSRC of media source                     |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * \                                                               \
		 * /                          Unknown Value                        /
		 * \                                                               \
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 */
		class UnknownTransportLayerFeedbackPacket : public AnyTransportLayerFeedbackPacket
		{
			// We need that compound packet calls protected and private methods in
			// this class.
			friend class CompoundPacket;

		public:
			/**
			 * Parse an UnknownTransportLayerFeedbackPacket.
			 *
			 * @remarks
			 * - `bufferLength` may exceed the exact length of the packet.
			 */
			static UnknownTransportLayerFeedbackPacket* Parse(const uint8_t* buffer, size_t bufferLength);

		private:
			/**
			 * Parse an UnknownTransportLayerFeedbackPacket.
			 *
			 * @remarks
			 * - To be used only by `CompoundPacket::Parse()`.
			 */
			static UnknownTransportLayerFeedbackPacket* ParseStrict(
			  const uint8_t* buffer, size_t bufferLength, size_t packetLength, uint8_t padding);

		private:
			/**
			 * Only used by ParseStrict(), Clone() and SoftClone() methods.
			 */
			UnknownTransportLayerFeedbackPacket(uint8_t* buffer, size_t bufferLength);

		public:
			~UnknownTransportLayerFeedbackPacket() override;

			void Dump(int indentation = 0) const final;

			UnknownTransportLayerFeedbackPacket* Clone(uint8_t* buffer, size_t bufferLength) const final;

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
			UnknownTransportLayerFeedbackPacket* SoftClone(const uint8_t* buffer) const final;
		};
	} // namespace RTCP
} // namespace RTC

#endif
