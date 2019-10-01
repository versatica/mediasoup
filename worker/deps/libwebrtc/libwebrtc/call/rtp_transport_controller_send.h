/*
 *  Copyright (c) 2017 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef CALL_RTP_TRANSPORT_CONTROLLER_SEND_H_
#define CALL_RTP_TRANSPORT_CONTROLLER_SEND_H_

#include "api/network_state_predictor.h"
#include "api/transport/network_control.h"
#include "api/transport/field_trial_based_config.h"
// #include "call/rtp_bitrate_configurator.h"
#include "call/rtp_transport_controller_send_interface.h"
#include "modules/congestion_controller/rtp/control_handler.h"
#include "modules/congestion_controller/rtp/transport_feedback_adapter.h"
#include "rtc_base/constructor_magic.h"
#include "modules/pacing/packet_router.h"
#include "modules/pacing/paced_sender.h"

#include <atomic>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace webrtc {

// TODO(nisse): When we get the underlying transports here, we should
// have one object implementing RtpTransportControllerSendInterface
// per transport, sharing the same congestion controller.
class RtpTransportControllerSend final
    : public RtpTransportControllerSendInterface,
      public RtcpBandwidthObserver,
      public TransportFeedbackObserver,
      public NetworkStateEstimateObserver {
 public:
  RtpTransportControllerSend(
      PacketRouter* packet_router,
      NetworkStatePredictorFactoryInterface* predictor_factory,
      NetworkControllerFactoryInterface* controller_factory,
      const BitrateConstraints& bitrate_config);
  ~RtpTransportControllerSend() override;

  PacketRouter* packet_router() override;

  NetworkStateEstimateObserver* network_state_estimate_observer() override;
  TransportFeedbackObserver* transport_feedback_observer() override;
  PacedSender* packet_sender() override;

  void SetAllocatedSendBitrateLimits(int min_send_bitrate_bps,
                                     int max_padding_bitrate_bps,
                                     int max_total_bitrate_bps) override;

  void SetClientBitratePreferences(const TargetRateConstraints& constraints);

  void SetPacingFactor(float pacing_factor) override;
  void RegisterTargetTransferRateObserver(
      TargetTransferRateObserver* observer) override;
  void OnNetworkAvailability(bool network_available) override;
  RtcpBandwidthObserver* GetBandwidthObserver() override;
  void EnablePeriodicAlrProbing(bool enable) override;
  void OnSentPacket(const rtc::SentPacket& sent_packet, size_t size) override;

  void OnTransportOverheadChanged(
      size_t transport_overhead_per_packet) override;

  // Implements RtcpBandwidthObserver interface
  void OnReceivedEstimatedBitrate(uint32_t bitrate) override;
  void OnReceivedRtcpReceiverReport(const ReportBlockList& report_blocks,
                                    int64_t rtt,
                                    int64_t now_ms) override;

  // Implements TransportFeedbackObserver interface
  void OnAddPacket(const RtpPacketSendInfo& packet_info) override;
  void OnTransportFeedback(const RTC::RTCP::FeedbackRtpTransportPacket& feedback) override;

  // Implements NetworkStateEstimateObserver interface
  void OnRemoteNetworkEstimate(NetworkStateEstimate estimate) override;

  void Process();

 private:
  void MaybeCreateControllers();

  void UpdateControllerWithTimeInterval();

  void UpdateStreamsConfig();
  void OnReceivedRtcpReceiverReportBlocks(const ReportBlockList& report_blocks,
                                          int64_t now_ms);
  void PostUpdates(NetworkControlUpdate update);
  void UpdateControlState();

  const FieldTrialBasedConfig trial_based_config_;

  PacketRouter* packet_router_;
  PacedSender pacer_;

  TargetTransferRateObserver* observer_;

  NetworkControllerFactoryInterface* const controller_factory_override_;

  TransportFeedbackAdapter transport_feedback_adapter_;

  std::unique_ptr<CongestionControlHandler> control_handler_;

  std::unique_ptr<NetworkControllerInterface> controller_ ;

  TimeDelta process_interval_;

  std::map<uint32_t, RTCPReportBlock> last_report_blocks_;
  Timestamp last_report_block_time_;

  NetworkControllerConfig initial_config_;
	StreamsConfig streams_config_;

  const bool send_side_bwe_with_overhead_;
  // MS_NOTE: not used.
  // const bool add_pacing_to_cwin_;
  // Transport overhead is written by OnNetworkRouteChanged and read by
  // AddPacket.
  // TODO(srte): Remove atomic when feedback adapter runs on task queue.
  std::atomic<size_t> transport_overhead_bytes_per_packet_;

  bool network_available_;

  // TODO(perkj): |task_queue_| is supposed to replace |process_thread_|.
  // |task_queue_| is defined last to ensure all pending tasks are cancelled
  // and deleted before any other members.
  RTC_DISALLOW_COPY_AND_ASSIGN(RtpTransportControllerSend);
};

}  // namespace webrtc

#endif  // CALL_RTP_TRANSPORT_CONTROLLER_SEND_H_
