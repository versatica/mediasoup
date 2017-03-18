/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#define MS_CLASS "RemoteBitrateEstimatorSingleStream"
// #define MS_LOG_DEV

#include "RTC/RemoteBitrateEstimator/RemoteBitrateEstimatorSingleStream.hpp"
#include "RTC/RemoteBitrateEstimator/AimdRateControl.hpp"
#include "RTC/RemoteBitrateEstimator/InterArrival.hpp"
#include "RTC/RemoteBitrateEstimator/OveruseDetector.hpp"
#include "RTC/RemoteBitrateEstimator/OveruseEstimator.hpp"
#include "DepLibUV.hpp"
#include "Utils.hpp" // Byte::Get3Bytes
#include "Logger.hpp"
#include <utility> // std::make_pair

namespace RTC {

enum { kTimestampGroupLengthMs = 5 };
static const double kTimestampToMs = 1.0 / 90.0;

struct RemoteBitrateEstimatorSingleStream::Detector {
  explicit Detector(int64_t last_packet_time_ms,
                    const OverUseDetectorOptions& options,
                    bool enable_burst_grouping)
      : last_packet_time_ms(last_packet_time_ms),
        inter_arrival(90 * kTimestampGroupLengthMs,
                      kTimestampToMs,
                      enable_burst_grouping),
        estimator(options),
        detector() {}
  int64_t last_packet_time_ms;
  InterArrival inter_arrival;
  OveruseEstimator estimator;
  OveruseDetector detector;
};

RemoteBitrateEstimatorSingleStream::RemoteBitrateEstimatorSingleStream(
    Listener* observer)
    : incomingBitrate(),
      lastValidIncomingBitrate(0),
      remoteRate(new AimdRateControl()),
      observer(observer),
      lastProcessTime(-1),
      processIntervalMs(kProcessIntervalMs),
      umaRecorded(false) {
  MS_ASSERT(this->observer, "'this->observer' missing");
}

RemoteBitrateEstimatorSingleStream::~RemoteBitrateEstimatorSingleStream() {
  while (!this->overuseDetectors.empty()) {
    SsrcOveruseEstimatorMap::iterator it = this->overuseDetectors.begin();
    delete it->second;
    this->overuseDetectors.erase(it);
  }
}

void RemoteBitrateEstimatorSingleStream::IncomingPacket(
    int64_t arrival_time_ms,
    size_t payload_size,
    const RtpPacket& packet,
    const uint8_t* transmissionTimeOffset
    ) {
  if (!this->umaRecorded) {
    this->umaRecorded = true;
  }
  uint32_t ssrc = packet.GetSsrc();
  uint32_t rtp_timestamp = packet.GetTimestamp() +
      Utils::Byte::Get3Bytes(transmissionTimeOffset, 0);
  int64_t now_ms = DepLibUV::GetTime();
  SsrcOveruseEstimatorMap::iterator it = this->overuseDetectors.find(ssrc);
  if (it == this->overuseDetectors.end()) {
    // This is a new SSRC. Adding to map.
    // TODO(holmer): If the channel changes SSRC the old SSRC will still be
    // around in this map until the channel is deleted. This is OK since the
    // callback will no longer be called for the old SSRC. This will be
    // automatically cleaned up when we have one RemoteBitrateEstimator per REMB
    // group.
    std::pair<SsrcOveruseEstimatorMap::iterator, bool> insert_result =
        this->overuseDetectors.insert(std::make_pair(
            ssrc, new Detector(now_ms, OverUseDetectorOptions(), true)));
    it = insert_result.first;
  }
  Detector* estimator = it->second;
  estimator->last_packet_time_ms = now_ms;

  // Check if incoming bitrate estimate is valid, and if it needs to be reset.
  uint32_t incoming_bitrate = this->incomingBitrate.GetRate(now_ms);
  if (incoming_bitrate) {
    this->lastValidIncomingBitrate = incoming_bitrate;
  } else if (this->lastValidIncomingBitrate > 0) {
    // Incoming bitrate had a previous valid value, but now not enough data
    // point are left within the current window. Reset incoming bitrate
    // estimator so that the window size will only contain new data points.
    this->incomingBitrate.Reset();
    this->lastValidIncomingBitrate = 0;
  }
  this->incomingBitrate.Update(payload_size, now_ms);

  const BandwidthUsage prior_state = estimator->detector.State();
  uint32_t timestamp_delta = 0;
  int64_t time_delta = 0;
  int size_delta = 0;
  if (estimator->inter_arrival.ComputeDeltas(
          rtp_timestamp, arrival_time_ms, now_ms, payload_size,
          &timestamp_delta, &time_delta, &size_delta)) {
    double timestamp_delta_ms = timestamp_delta * kTimestampToMs;
    estimator->estimator.Update(time_delta, timestamp_delta_ms, size_delta,
                                estimator->detector.State(), now_ms);
    estimator->detector.Detect(estimator->estimator.GetOffset(),
                               timestamp_delta_ms,
                               estimator->estimator.num_of_deltas(), now_ms);
  }
  if (estimator->detector.State() == kBwOverusing) {
    uint32_t incoming_bitrate_bps = this->incomingBitrate.GetRate(now_ms);
    if (incoming_bitrate_bps &&
        (prior_state != kBwOverusing ||
         GetRemoteRate()->TimeToReduceFurther(now_ms, incoming_bitrate_bps))) {
      // The first overuse should immediately trigger a new estimate.
      // We also have to update the estimate immediately if we are overusing
      // and the target bitrate is too high compared to what we are receiving.
      UpdateEstimate(now_ms);
    }
  }
}

void RemoteBitrateEstimatorSingleStream::Process() {
  {
    UpdateEstimate(DepLibUV::GetTime());
  }
  this->lastProcessTime = DepLibUV::GetTime();
}

int64_t RemoteBitrateEstimatorSingleStream::TimeUntilNextProcess() {
  if (this->lastProcessTime < 0) {
    return 0;
  }
  //MS_DASSERT(this->processIntervalMs > 0);
  return this->lastProcessTime + this->processIntervalMs - DepLibUV::GetTime();
}

void RemoteBitrateEstimatorSingleStream::UpdateEstimate(int64_t now_ms) {
  BandwidthUsage bw_state = kBwNormal;
  double sum_var_noise = 0.0;
  SsrcOveruseEstimatorMap::iterator it = this->overuseDetectors.begin();
  while (it != this->overuseDetectors.end()) {
    const int64_t time_of_last_received_packet =
        it->second->last_packet_time_ms;
    if (time_of_last_received_packet >= 0 &&
        now_ms - time_of_last_received_packet > kStreamTimeOutMs) {
      // This over-use detector hasn't received packets for |kStreamTimeOutMs|
      // milliseconds and is considered stale.
      delete it->second;
      this->overuseDetectors.erase(it++);
    } else {
      sum_var_noise += it->second->estimator.var_noise();
      // Make sure that we trigger an over-use if any of the over-use detectors
      // is detecting over-use.
      if (it->second->detector.State() > bw_state) {
        bw_state = it->second->detector.State();
      }
      ++it;
    }
  }
  // We can't update the estimate if we don't have any active streams.
  if (this->overuseDetectors.empty()) {
    return;
  }
  AimdRateControl* remote_rate = GetRemoteRate();

  double mean_noise_var = sum_var_noise /
      static_cast<double>(this->overuseDetectors.size());
  const RateControlInput input(bw_state,
                               this->incomingBitrate.GetRate(now_ms),
                               mean_noise_var);
  remote_rate->Update(&input, now_ms);
  uint32_t target_bitrate = remote_rate->UpdateBandwidthEstimate(now_ms);
  if (remote_rate->ValidEstimate()) {
    this->processIntervalMs = remote_rate->GetFeedbackInterval();
    //MS_DASSERT(this->processIntervalMs > 0);
    std::vector<uint32_t> ssrcs;
    GetSsrcs(&ssrcs);
    this->observer->onReceiveBitrateChanged(ssrcs, target_bitrate);
  }
}

void RemoteBitrateEstimatorSingleStream::OnRttUpdate(int64_t avg_rtt_ms,
                                                     int64_t max_rtt_ms) {
  (void)max_rtt_ms;
  GetRemoteRate()->SetRtt(avg_rtt_ms);
}

void RemoteBitrateEstimatorSingleStream::RemoveStream(unsigned int ssrc) {
  SsrcOveruseEstimatorMap::iterator it = this->overuseDetectors.find(ssrc);
  if (it != this->overuseDetectors.end()) {
    delete it->second;
    this->overuseDetectors.erase(it);
  }
}

bool RemoteBitrateEstimatorSingleStream::LatestEstimate(
    std::vector<uint32_t>* ssrcs,
    uint32_t* bitrate_bps) const {
  MS_ASSERT(bitrate_bps, "'bitrate_bps' missing");
  if (!this->remoteRate->ValidEstimate()) {
    return false;
  }
  GetSsrcs(ssrcs);
  if (ssrcs->empty())
    *bitrate_bps = 0;
  else
    *bitrate_bps = this->remoteRate->LatestEstimate();
  return true;
}

void RemoteBitrateEstimatorSingleStream::GetSsrcs(
    std::vector<uint32_t>* ssrcs) const {
  MS_ASSERT(ssrcs, "'ssrcs' missing");
  ssrcs->resize(this->overuseDetectors.size());
  int i = 0;
  for (SsrcOveruseEstimatorMap::const_iterator it = this->overuseDetectors.begin();
      it != this->overuseDetectors.end(); ++it, ++i) {
    (*ssrcs)[i] = it->first;
  }
}

AimdRateControl* RemoteBitrateEstimatorSingleStream::GetRemoteRate() {
  if (!this->remoteRate)
    this->remoteRate.reset(new AimdRateControl());
  return this->remoteRate.get();
}

void RemoteBitrateEstimatorSingleStream::SetMinBitrate(int min_bitrate_bps) {
  this->remoteRate->SetMinBitrate(min_bitrate_bps);
}

}  // namespace RTC
