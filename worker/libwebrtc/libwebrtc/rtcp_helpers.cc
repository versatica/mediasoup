
#include "rtcp_helpers.h"

#include <vector>

namespace webrtc {
namespace rtcp_helpers {

const std::vector<webrtc::rtcp::ReceivedPacket> GetReceivedPackets(const RTC::RTCP::FeedbackRtpTransportPacket* packet)
{
  std::vector<webrtc::rtcp::ReceivedPacket> receivedPackets;

  for (auto& packetResult : packet->GetPacketResults())
  {
    if (packetResult.received)
      receivedPackets.emplace_back(packetResult.sequenceNumber, packetResult.delta);
    else
      receivedPackets.emplace_back(packetResult.sequenceNumber);
  }

  return receivedPackets;
}

// Get the reference time in microseconds, including any precision loss.
int64_t GetBaseTimeUs(const RTC::RTCP::FeedbackRtpTransportPacket* packet)
{
  return packet->GetReferenceTimestamp() * 1000;
}

// Get the unwrapped delta between current base time and |prev_timestamp_us|.
int64_t GetBaseDeltaUs(const RTC::RTCP::FeedbackRtpTransportPacket* packet, int64_t prev_timestamp_us)
{
  return GetBaseTimeUs(packet) - prev_timestamp_us;
}

}  // namespace rtcp_helpers
}  // namespace webrtc
