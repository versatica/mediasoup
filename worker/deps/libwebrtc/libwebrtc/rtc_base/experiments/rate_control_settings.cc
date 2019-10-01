/*
 *  Copyright (c) 2019 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "rtc_base/experiments/rate_control_settings.h"
#include "api/transport/field_trial_based_config.h"
#include "rtc_base/experiments/field_trial_parser.h"
#include "rtc_base/numerics/safe_conversions.h"

#include <inttypes.h>
#include <stdio.h>
#include <string>

namespace webrtc {

namespace {

const int kDefaultAcceptedQueueMs = 250;

const int kDefaultMinPushbackTargetBitrateBps = 30000;

}  // namespace

RateControlSettings::RateControlSettings(
    const WebRtcKeyValueConfig* const key_value_config)
    : congestion_window_("QueueSize"),
      congestion_window_pushback_("MinBitrate"),
      pacing_factor_("pacing_factor"),
      alr_probing_("alr_probing", false),
      probe_max_allocation_("probe_max_allocation", true),
      bitrate_adjuster_("bitrate_adjuster", false),
      adjuster_use_headroom_("adjuster_use_headroom", false) {
  ParseFieldTrial({&congestion_window_, &congestion_window_pushback_},
                  key_value_config->Lookup("WebRTC-CongestionWindow"));
  ParseFieldTrial(
      {&pacing_factor_, &alr_probing_,
       &probe_max_allocation_, &bitrate_adjuster_, &adjuster_use_headroom_},
      key_value_config->Lookup("WebRTC-VideoRateControl"));
}

RateControlSettings::~RateControlSettings() = default;
RateControlSettings::RateControlSettings(RateControlSettings&&) = default;

RateControlSettings RateControlSettings::ParseFromFieldTrials() {
  FieldTrialBasedConfig field_trial_config;
  return RateControlSettings(&field_trial_config);
}

RateControlSettings RateControlSettings::ParseFromKeyValueConfig(
    const WebRtcKeyValueConfig* const key_value_config) {
  FieldTrialBasedConfig field_trial_config;
  return RateControlSettings(key_value_config ? key_value_config
                                              : &field_trial_config);
}

bool RateControlSettings::UseCongestionWindow() const {
  return static_cast<bool>(congestion_window_);
}

int64_t RateControlSettings::GetCongestionWindowAdditionalTimeMs() const {
  return congestion_window_.GetOptional().value_or(kDefaultAcceptedQueueMs);
}

bool RateControlSettings::UseCongestionWindowPushback() const {
  return congestion_window_ && congestion_window_pushback_;
}

uint32_t RateControlSettings::CongestionWindowMinPushbackTargetBitrateBps()
    const {
  return congestion_window_pushback_.GetOptional().value_or(
      kDefaultMinPushbackTargetBitrateBps);
}

absl::optional<double> RateControlSettings::GetPacingFactor() const {
  return pacing_factor_.GetOptional();
}

bool RateControlSettings::UseAlrProbing() const {
  return alr_probing_.Get();
}

bool RateControlSettings::TriggerProbeOnMaxAllocatedBitrateChange() const {
  return probe_max_allocation_.Get();
}

bool RateControlSettings::UseEncoderBitrateAdjuster() const {
  return bitrate_adjuster_.Get();
}

bool RateControlSettings::BitrateAdjusterCanUseNetworkHeadroom() const {
  return adjuster_use_headroom_.Get();
}

}  // namespace webrtc
