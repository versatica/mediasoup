#ifndef RTCP_HELPERS_H_
#define RTCP_HELPERS_H_

#include "RTC/RTCP/FeedbackRtpTransport.hpp"
#include "modules/rtp_rtcp/source/rtp_packet/transport_feedback.h"

#include <cstdint>
#include <vector>

/*
 * Helpers to retrieve necessary data from mediasoup FeedbackRtpTransportPacket.
 */

const std::vector<ReceivedPacket> GetReceivedPackets(const RTC::RTCP::FeedbackRtpTransportPacket* packet);

// Get the reference time in microseconds, including any precision loss.
int64_t GetBaseTimeUs(const RTC::RTCP::FeedbackRtpTransportPacket* packet);

// Get the unwrapped delta between current base time and |prev_timestamp_us|.
int64_t GetBaseDeltaUs(const RTC::RTCP::FeedbackRtpTransportPacket* packet, int64_t prev_timestamp_us);

#endif

