// TODO: Replace it with the real original libwebrtc file.

#ifndef RTCP_HELPERS_H_
#define RTCP_HELPERS_H_

#include "RTC/RTCP/FeedbackRtpTransport.hpp"

#include <cstdint>
#include <vector>

/*
 * Helpers to retrieve necessary data from mediasoup FeedbackRtpTransportPacket.
 */

// Convert to multiples of 0.25ms.
static constexpr int kDeltaScaleFactor = 250;

class ReceivedPacket {
 public:
  ReceivedPacket(uint16_t sequence_number, int16_t delta_ticks)
      : sequence_number_(sequence_number),
        delta_ticks_(delta_ticks),
        received_(true) {}
  explicit ReceivedPacket(uint16_t sequence_number)
      : sequence_number_(sequence_number), received_(false) {}
  ReceivedPacket(const ReceivedPacket&) = default;
  ReceivedPacket& operator=(const ReceivedPacket&) = default;

  uint16_t sequence_number() const { return sequence_number_; }
  int16_t delta_ticks() const { return delta_ticks_; }
  int32_t delta_us() const { return delta_ticks_ * kDeltaScaleFactor; }
  bool received() const { return received_; }

 private:
  uint16_t sequence_number_;
  int16_t delta_ticks_;
  bool received_;
};

const std::vector<ReceivedPacket> GetReceivedPackets(const RTC::RTCP::FeedbackRtpTransportPacket* packet);

// Get the reference time in microseconds, including any precision loss.
int64_t GetBaseTimeUs(const RTC::RTCP::FeedbackRtpTransportPacket* packet);

// Get the unwrapped delta between current base time and |prev_timestamp_us|.
int64_t GetBaseDeltaUs(const RTC::RTCP::FeedbackRtpTransportPacket* packet, int64_t prev_timestamp_us);

#endif

