/*
 *  Copyright (c) 2017 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#define MS_CLASS "webrtc::RtpTransportControllerSend"
// #define MS_LOG_DEV_LEVEL 3

#include "call/rtp_transport_controller_send.h"
#include "api/transport/network_types.h"
#include "api/units/data_rate.h"
#include "api/units/time_delta.h"
#include "api/units/timestamp.h"
#include "system_wrappers/source/field_trial.h"
#include "modules/congestion_controller/goog_cc/goog_cc_network_control.h"

#include "DepLibUV.hpp"
#include "Logger.hpp"
#include "RTC/RTCP/FeedbackRtpTransport.hpp"

#include <absl/memory/memory.h>
#include <absl/types/optional.h>
#include <utility>
#include <vector>

namespace webrtc {
namespace {
static const int64_t kRetransmitWindowSizeMs = 500;
static const size_t kMaxOverheadBytes = 500;

constexpr TimeDelta kPacerQueueUpdateInterval = TimeDelta::Millis<25>();

TargetRateConstraints ConvertConstraints(int min_bitrate_bps,
                                         int max_bitrate_bps,
                                         int start_bitrate_bps) {
  TargetRateConstraints msg;
  msg.at_time = Timestamp::ms(DepLibUV::GetTimeMsInt64());
  msg.min_data_rate =
      min_bitrate_bps >= 0 ? DataRate::bps(min_bitrate_bps) : DataRate::Zero();
  msg.max_data_rate = max_bitrate_bps > 0 ? DataRate::bps(max_bitrate_bps)
                                          : DataRate::Infinity();
  if (start_bitrate_bps > 0)
    msg.starting_rate = DataRate::bps(start_bitrate_bps);
  return msg;
}

TargetRateConstraints ConvertConstraints(const BitrateConstraints& contraints) {
  return ConvertConstraints(contraints.min_bitrate_bps,
                            contraints.max_bitrate_bps,
                            contraints.start_bitrate_bps);
}

bool IsEnabled(const WebRtcKeyValueConfig& trials, absl::string_view key) {
  return absl::StartsWith(trials.Lookup(key), "Enabled");
}

bool IsDisabled(const WebRtcKeyValueConfig& trials, absl::string_view key) {
  return absl::StartsWith(trials.Lookup(key), "Disabled");
}

}  // namespace

RtpTransportControllerSend::PacerSettings::PacerSettings(
    const WebRtcKeyValueConfig& trials)
    : holdback_window("holdback_window", TimeDelta::ms(5)),
      holdback_packets("holdback_packets", 3) {
  ParseFieldTrial({&holdback_window, &holdback_packets},
                  trials.Lookup("WebRTC-TaskQueuePacer"));
}

RtpTransportControllerSend::RtpTransportControllerSend(
    PacketRouter* packet_router,
    NetworkStatePredictorFactoryInterface* predictor_factory,
    NetworkControllerFactoryInterface* controller_factory,
    const BitrateConstraints& bitrate_config,
		const WebRtcKeyValueConfig& trials)
    : packet_router_(packet_router),
      pacer_(packet_router_, trials),
      observer_(nullptr),
      controller_factory_override_(controller_factory),
      process_interval_(controller_factory_override_->GetProcessInterval()),
      last_report_block_time_(Timestamp::ms(DepLibUV::GetTimeMsInt64())),
      send_side_bwe_with_overhead_(
          webrtc::field_trial::IsEnabled("WebRTC-SendSideBwe-WithOverhead")),
      transport_overhead_bytes_per_packet_(0),
      network_available_(false) {
  initial_config_.constraints = ConvertConstraints(bitrate_config);
  initial_config_.key_value_config = &trial_based_config_;

  // RTC_DCHECK(bitrate_config.start_bitrate_bps > 0);
  MS_ASSERT(bitrate_config.start_bitrate_bps > 0, "start bitrate must be > 0");

  pacer_.SetPacingRates(DataRate::bps(bitrate_config.start_bitrate_bps), DataRate::bps(0));
}

RtpTransportControllerSend::~RtpTransportControllerSend() {
}

void RtpTransportControllerSend::UpdateControlState() {
  absl::optional<TargetTransferRate> update = control_handler_->GetUpdate();
  if (!update)
    return;
  retransmission_rate_limiter_.SetMaxRate(update->target_rate.bps());
  // We won't create control_handler_ until we have an observers.
  // RTC_DCHECK(observer_ != nullptr);
  MS_ASSERT(observer_ != nullptr, "no observer");

  observer_->OnTargetTransferRate(*update);
}

void RtpTransportControllerSend::UpdateCongestedState() {
  bool congested = transport_feedback_adapter_.GetOutstandingData() >=
                   congestion_window_size_;
  if (congested != is_congested_) {
    is_congested_ = congested;
    pacer_.SetCongested(congested);
  }
}

PacketRouter* RtpTransportControllerSend::packet_router() {
  return this->packet_router_;
}

NetworkStateEstimateObserver*
RtpTransportControllerSend::network_state_estimate_observer() {
  return this->network_state_estimate_observer();
}

TransportFeedbackObserver*
RtpTransportControllerSend::transport_feedback_observer() {
  return this->transport_feedback_observer();
}

RtpPacketSender* RtpTransportControllerSend::packet_sender() {
  return &this->pacer_;
}

void RtpTransportControllerSend::SetAllocatedSendBitrateLimits(
    int min_send_bitrate_bps,
    int max_padding_bitrate_bps,
    int max_total_bitrate_bps) {
  streams_config_.min_total_allocated_bitrate =
      DataRate::bps(min_send_bitrate_bps);
  streams_config_.max_padding_rate = DataRate::bps(max_padding_bitrate_bps);
  streams_config_.max_total_allocated_bitrate =
      DataRate::bps(max_total_bitrate_bps);
  UpdateStreamsConfig();
}

void RtpTransportControllerSend::SetClientBitratePreferences(const TargetRateConstraints& constraints)
{
  controller_->OnTargetRateConstraints(constraints);
}

void RtpTransportControllerSend::SetPacingFactor(float pacing_factor) {
  streams_config_.pacing_factor = pacing_factor;
  UpdateStreamsConfig();
}
/*void RtpTransportControllerSend::SetQueueTimeLimit(int limit_ms) {
  pacer_.SetQueueTimeLimit(TimeDelta::ms(limit_ms));
}*/

void RtpTransportControllerSend::RegisterTargetTransferRateObserver(
    TargetTransferRateObserver* observer) {

    // RTC_DCHECK(observer_ == nullptr);
    MS_ASSERT(observer_ == nullptr, "observer already set");

    observer_ = observer;
    observer_->OnStartRateUpdate(*initial_config_.constraints.starting_rate);
    MaybeCreateControllers();
}

void RtpTransportControllerSend::OnNetworkAvailability(bool network_available) {
  MS_DEBUG_DEV("<<<<< network_available:%s", network_available ? "true" : "false");

  NetworkAvailability msg;
  msg.at_time = Timestamp::ms(DepLibUV::GetTimeMsInt64());
  msg.network_available = network_available;

  if (network_available_ == msg.network_available)
    return;
  network_available_ = msg.network_available;
  if (network_available_) {
    pacer_.Resume();
  } else {
    pacer_.Pause();
  }
	is_congested_ = false;
	pacer_.SetCongested(false);

  control_handler_->SetNetworkAvailability(network_available_);
  PostUpdates(controller_->OnNetworkAvailability(msg));
  UpdateControlState();
}

/*RtcpBandwidthObserver* RtpTransportControllerSend::GetBandwidthObserver() {
  return this;
}*/

void RtpTransportControllerSend::EnablePeriodicAlrProbing(bool enable) {
	streams_config_.requests_alr_probing = enable;
  UpdateStreamsConfig();
}

void RtpTransportControllerSend::OnSentPacket(
    const rtc::SentPacket& sent_packet, size_t size)
{
	MS_DEBUG_DEV("<<<<< size:%zu", size);

	absl::optional<SentPacket> packet_msg = transport_feedback_adapter_.ProcessSentPacket(sent_packet);
	if (packet_msg)
	{
		// Only update outstanding data if:
		// 1. Packet feadback is used.
		// 2. The packet has not yet received an acknowledgement.
		// 3. It is not a retransmission of an earlier packet.
		UpdateCongestedState();
		if (controller_)
			PostUpdates(controller_->OnSentPacket(*packet_msg));
	}
}

void RtpTransportControllerSend::UpdateBitrateConstraints(
    const BitrateConstraints& updated) {
  TargetRateConstraints msg = ConvertConstraints(updated);
      PostUpdates(controller_->OnTargetRateConstraints(msg));
  };

/*void RtpTransportControllerSend::SetSdpBitrateParameters(
    const BitrateConstraints& constraints) {
  absl::optional<BitrateConstraints> updated =
      bitrate_configurator_.UpdateWithSdpParameters(constraints);
  if (updated.has_value()) {
    UpdateBitrateConstraints(*updated);
  } else {
    RTC_LOG(LS_VERBOSE)
        << "WebRTC.RtpTransportControllerSend.SetSdpBitrateParameters: "
           "nothing to update";
  }
}

void RtpTransportControllerSend::SetClientBitratePreferences(
    const BitrateSettings& preferences) {
  absl::optional<BitrateConstraints> updated =
      bitrate_configurator_.UpdateWithClientPreferences(preferences);
  if (updated.has_value()) {
    UpdateBitrateConstraints(*updated);
  } else {
    RTC_LOG(LS_VERBOSE)
        << "WebRTC.RtpTransportControllerSend.SetClientBitratePreferences: "
           "nothing to update";
  }
}*/

void RtpTransportControllerSend::OnTransportOverheadChanged(
    size_t transport_overhead_bytes_per_packet) {
  MS_DEBUG_DEV("<<<<< transport_overhead_bytes_per_packet:%zu", transport_overhead_bytes_per_packet);

  if (transport_overhead_bytes_per_packet >= kMaxOverheadBytes) {
    MS_ERROR("transport overhead exceeds: %zu", kMaxOverheadBytes);
    return;
  }

  pacer_.SetTransportOverhead(
      DataSize::bytes(transport_overhead_bytes_per_packet));
}

void RtpTransportControllerSend::OnReceivedEstimatedBitrate(uint32_t bitrate) {
  MS_DEBUG_DEV("<<<<< bitrate:%zu", bitrate);

  RemoteBitrateReport msg;
  msg.receive_time = Timestamp::ms(DepLibUV::GetTimeMsInt64());
  msg.bandwidth = DataRate::bps(bitrate);

  PostUpdates(controller_->OnRemoteBitrateReport(msg));
}

void RtpTransportControllerSend::OnReceivedRtcpReceiverReport(
    const ReportBlockList& report_blocks,
    int64_t rtt_ms,
    int64_t now_ms) {
  MS_DEBUG_DEV("<<<<< rtt_ms:%" PRIi64, rtt_ms);

  OnReceivedRtcpReceiverReportBlocks(report_blocks, now_ms);

  RoundTripTimeUpdate report;
  report.receive_time = Timestamp::ms(now_ms);
  report.round_trip_time = TimeDelta::ms(rtt_ms);
  report.smoothed = false;
  if (!report.round_trip_time.IsZero())
    PostUpdates(controller_->OnRoundTripTimeUpdate(report));
}

void RtpTransportControllerSend::OnAddPacket(
    const RtpPacketSendInfo& packet_info) {
  transport_feedback_adapter_.AddPacket(
      packet_info,
      send_side_bwe_with_overhead_ ? transport_overhead_bytes_per_packet_
                                   : 0,
      Timestamp::ms(DepLibUV::GetTimeMsInt64()));
}

void RtpTransportControllerSend::OnTransportFeedback(
    const RTC::RTCP::FeedbackRtpTransportPacket& feedback) {
  MS_DEBUG_DEV("<<<<<");

  absl::optional<TransportPacketsFeedback> feedback_msg =
      transport_feedback_adapter_.ProcessTransportFeedback(
          feedback, Timestamp::ms(DepLibUV::GetTimeMsInt64()));
  if (feedback_msg)
    PostUpdates(controller_->OnTransportPacketsFeedback(*feedback_msg));

	UpdateCongestedState();
}

void RtpTransportControllerSend::OnRemoteNetworkEstimate(
    NetworkStateEstimate estimate) {
  estimate.update_time = Timestamp::ms(DepLibUV::GetTimeMsInt64());
	PostUpdates(controller_->OnNetworkStateEstimate(estimate));
}

void RtpTransportControllerSend::Process()
{
  // TODO (ibc): Must really check if we need this ugly periodic timer which is called
  // every 5ms.
  // NOTE: Yes, otherwise the pssss scenario does not work:
  // https://bitbucket.org/versatica/mediasoup/issues/12/no-probation-if-no-real-media
	UpdateControllerWithTimeInterval();
}

void RtpTransportControllerSend::MaybeCreateControllers() {
  // RTC_DCHECK(!controller_);
  // RTC_DCHECK(!control_handler_);
  MS_ASSERT(!controller_, "controller already set");
  MS_ASSERT(!control_handler_, "controller handler already set");

  control_handler_ = absl::make_unique<CongestionControlHandler>();

  initial_config_.constraints.at_time =
      Timestamp::ms(DepLibUV::GetTimeMsInt64());

  controller_ = controller_factory_override_->Create(initial_config_);
  process_interval_ = controller_factory_override_->GetProcessInterval();

  UpdateControllerWithTimeInterval();
}

void RtpTransportControllerSend::UpdateControllerWithTimeInterval() {
  MS_DEBUG_DEV("<<<<<");

  // RTC_DCHECK(controller_);
  MS_ASSERT(controller_, "controller not set");

  ProcessInterval msg;
  msg.at_time = Timestamp::ms(DepLibUV::GetTimeMsInt64());

  PostUpdates(controller_->OnProcessInterval(msg));
}

void RtpTransportControllerSend::UpdateStreamsConfig() {
  MS_DEBUG_DEV("<<<<<");

  streams_config_.at_time = Timestamp::ms(DepLibUV::GetTimeMsInt64());
  if (controller_)
    PostUpdates(controller_->OnStreamsConfig(streams_config_));
}

void RtpTransportControllerSend::PostUpdates(NetworkControlUpdate update) {
	if (update.congestion_window) {
		congestion_window_size_ = *update.congestion_window;
		UpdateCongestedState();
	}
	if (update.pacer_config) {
		pacer_.SetPacingRates(update.pacer_config->data_rate(),
			                    update.pacer_config->pad_rate());
	}
	if (!update.probe_cluster_configs.empty()) {
		pacer_.CreateProbeClusters(update.probe_cluster_configs);
	}
	if (update.target_rate) {
		control_handler_->SetTargetRate(*update.target_rate);
		UpdateControlState();
	}
}

void RtpTransportControllerSend::OnReceivedRtcpReceiverReportBlocks(
    const ReportBlockList& report_blocks,
    int64_t now_ms) {
  if (report_blocks.empty())
    return;

  int total_packets_lost_delta = 0;
  int total_packets_delta = 0;

  // Compute the packet loss from all report blocks.
  for (const RTCPReportBlock& report_block : report_blocks) {
    auto it = last_report_blocks_.find(report_block.source_ssrc);
    if (it != last_report_blocks_.end()) {
      auto number_of_packets = report_block.extended_highest_sequence_number -
                               it->second.extended_highest_sequence_number;
      total_packets_delta += number_of_packets;
      auto lost_delta = report_block.packets_lost - it->second.packets_lost;
      total_packets_lost_delta += lost_delta;
    }
    last_report_blocks_[report_block.source_ssrc] = report_block;
  }
  // Can only compute delta if there has been previous blocks to compare to. If
  // not, total_packets_delta will be unchanged and there's nothing more to do.
  if (!total_packets_delta)
    return;
  int packets_received_delta = total_packets_delta - total_packets_lost_delta;
  // To detect lost packets, at least one packet has to be received. This check
  // is needed to avoid bandwith detection update in
  // VideoSendStreamTest.SuspendBelowMinBitrate

  if (packets_received_delta < 1)
    return;
  Timestamp now = Timestamp::ms(now_ms);
  TransportLossReport msg;
  msg.packets_lost_delta = total_packets_lost_delta;
  msg.packets_received_delta = packets_received_delta;
  msg.receive_time = now;
  msg.start_time = last_report_block_time_;
  msg.end_time = now;

  PostUpdates(controller_->OnTransportLossReport(msg));
  last_report_block_time_ = now;
}

}  // namespace webrtc
