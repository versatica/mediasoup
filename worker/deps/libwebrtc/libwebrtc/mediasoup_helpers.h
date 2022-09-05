#ifndef LIBWEBRTC_MEDIASOUP_HELPERS_H
#define LIBWEBRTC_MEDIASOUP_HELPERS_H

#include "modules/rtp_rtcp/source/rtp_packet/transport_feedback.h"

#include "RTC/RTCP/FeedbackRtpTransport.hpp"

#include <cstdint>
#include <vector>

namespace mediasoup_helpers
{
	/**
	 * Helpers to retrieve necessary data from mediasoup FeedbackRtpTransportPacket.
	 */
	namespace FeedbackRtpTransport
	{
		constexpr int64_t kTimeWrapPeriodUs = (1ll << 24) * 64 * 1000;
		
		const std::vector<webrtc::rtcp::ReceivedPacket> GetReceivedPackets(
			const RTC::RTCP::FeedbackRtpTransportPacket* packet)
		{
			std::vector<webrtc::rtcp::ReceivedPacket> receivedPackets;

			for (auto& packetResult : packet->GetPacketResults())
			{
			  if (packetResult.received)
			    receivedPackets.emplace_back(packetResult.sequenceNumber, packetResult.delta);
			}

			return receivedPackets;
		}

		// Get the reference time in microseconds, including any precision loss.
		int64_t GetBaseTimeUs(const RTC::RTCP::FeedbackRtpTransportPacket* packet)
		{
			return packet->GetReferenceTimestamp() * 1000;
		}

		// Get the wrapped delta between current base time and |prev_timestamp_us|.
		int64_t GetBaseDeltaUs(
		  const RTC::RTCP::FeedbackRtpTransportPacket* packet, int64_t prev_timestamp_us)
		{
			int64_t delta = GetBaseTimeUs(packet) - prev_timestamp_us;
			
			if (std::abs(delta - kTimeWrapPeriodUs) < std::abs(delta))
			{
				delta -= kTimeWrapPeriodUs;
			}
			else if (std::abs(delta + kTimeWrapPeriodUs) < std::abs(delta))
			{
				delta += kTimeWrapPeriodUs;
			}

			return delta;
		}
	} // namespace FeedbackRtpTransport
} // namespace mediasoup_helpers

#endif
