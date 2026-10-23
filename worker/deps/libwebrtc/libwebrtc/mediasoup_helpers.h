#ifndef LIBWEBRTC_MEDIASOUP_HELPERS_H
#define LIBWEBRTC_MEDIASOUP_HELPERS_H

#include "RTC/RTCP/FeedbackRtpTransport.hpp"
#include "modules/rtp_rtcp/source/rtp_packet/transport_feedback.h"
#include <cstdint>
#include <vector>

namespace mediasoup_helpers
{
	/**
	 * Helpers to retrieve necessary data from mediasoup FeedbackRtpTransportPacket.
	 */
	namespace FeedbackRtpTransport
	{
		const std::vector<webrtc::rtcp::ReceivedPacket> GetReceivedPackets(
		  const RTC::RTCP::FeedbackRtpTransportPacket* packet)
		{
			std::vector<webrtc::rtcp::ReceivedPacket> receivedPackets;

			for (auto& packetStatus : packet->GetPacketStatuses())
			{
				if (packetStatus.received)
				{
					receivedPackets.emplace_back(packetStatus.sequenceNumber, packetStatus.delta);
				}
			}

			return receivedPackets;
		}

		/**
		 * Get the reference time in microseconds.
		 */
		int64_t GetBaseTimeUs(const RTC::RTCP::FeedbackRtpTransportPacket* packet)
		{
			return packet->GetReferenceTimestampUs();
		}

		/**
		 * Get the unwrapped delta between current base time and |prev_timestamp_us|.
		 */
		int64_t GetBaseDeltaUs(const RTC::RTCP::FeedbackRtpTransportPacket* packet, int64_t prev_timestamp_us)
		{
			return packet->GetBaseDeltaUs(prev_timestamp_us);
		}
	} // namespace FeedbackRtpTransport
} // namespace mediasoup_helpers

#endif
