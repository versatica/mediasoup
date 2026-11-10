#ifndef MS_RTC_RTCP_ANY_FEEDBACK_PACKET_HPP
#define MS_RTC_RTCP_ANY_FEEDBACK_PACKET_HPP

#include "common.hpp"
#include "RTC/RTCP/packet/Packet.hpp"
#include "Utils.hpp"

namespace RTC
{
	namespace RTCP
	{
		/**
		 * Base class for Transport Layer Feedback (RTPFB) and Payload-Specific
		 * Feedback (PSFB) packets.
		 *
		 * @see RFC 4585.
		 *
		 *  0                   1                   2                   3
		 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |V=2|P|   FMT   |       PT      |             Length            |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |                     SSRC of packet sender                     |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |                      SSRC of media source                     |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * :            Feedback Control Information (FCI)                 :
		 * :                                                               :
		 *
		 * - Feedback message type (FMT) (5 bits): The type of the feedback
		 *   message, interpreted relative to the packet type (PT).
		 * - Packet type (PT) (8 bits): 205 (RTPFB) or 206 (PSFB).
		 * - SSRC of packet sender (32 bits).
		 * - SSRC of media source (32 bits): The source of the media this
		 *   feedback refers to.
		 * - Feedback Control Information (FCI) (variable length): Its format
		 *   depends on the packet type and the feedback message type.
		 */
		class AnyFeedbackPacket : public Packet
		{
		public:
			/**
			 * Length (in bytes) of the common header of every feedback packet,
			 * RTCP Common Header included.
			 */
			static constexpr size_t FeedbackHeaderLength{ 12 };

		protected:
			AnyFeedbackPacket(uint8_t* buffer, size_t bufferLength);

		public:
			/**
			 * SSRC of packet sender.
			 */
			uint32_t GetSenderSsrc() const
			{
				return Utils::Byte::Get4Bytes(GetBuffer(), 4);
			}

			void SetSenderSsrc(uint32_t ssrc);

			/**
			 * SSRC of media source.
			 */
			uint32_t GetMediaSsrc() const
			{
				return Utils::Byte::Get4Bytes(GetBuffer(), 8);
			}

			void SetMediaSsrc(uint32_t ssrc);

		protected:
			/**
			 * Subclasses must invoke this method within their Dump() method.
			 */
			void DumpFeedbackCommon(int indentation) const;
		};
	} // namespace RTCP
} // namespace RTC

#endif
