/*
 *  Copyright (c) 2018 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "RTC/SendTransportController/congestion_controller/rtp/control_handler.h"

#include <algorithm>
#include <vector>

#include "RTC/SendTransportController/data_rate.h"
#include "RTC/SendTransportController/safe_conversions.h"
#include "RTC/SendTransportController/safe_minmax.h"
#include "RTC/SendTransportController/field_trial.h"

#include "Logger.hpp"

#define MS_CLASS "ControlHandler"

namespace webrtc {

void CongestionControlHandler::SetTargetRate(
    TargetTransferRate new_target_rate) {
  last_incoming_ = new_target_rate;
}

void CongestionControlHandler::SetNetworkAvailability(bool network_available) {
  network_available_ = network_available;
}

absl::optional<TargetTransferRate> CongestionControlHandler::GetUpdate() {
  if (!last_incoming_.has_value())
    return absl::nullopt;
  TargetTransferRate new_outgoing = *last_incoming_;
  DataRate log_target_rate = new_outgoing.target_rate;
  bool pause_encoding = false;
  if (!network_available_)
    pause_encoding = true;
  if (pause_encoding)
    new_outgoing.target_rate = DataRate::Zero();
  if (!last_reported_ ||
      last_reported_->target_rate != new_outgoing.target_rate ||
      (!new_outgoing.target_rate.IsZero() &&
       (last_reported_->network_estimate.loss_rate_ratio !=
            new_outgoing.network_estimate.loss_rate_ratio ||
        last_reported_->network_estimate.round_trip_time !=
            new_outgoing.network_estimate.round_trip_time))) {
    if (encoder_paused_in_last_report_ != pause_encoding)
      MS_DEBUG_TAG(bwe, "Bitrate estimate state changed, BWE: %s",
                       ToString(log_target_rate).c_str());
    encoder_paused_in_last_report_ = pause_encoding;
    last_reported_ = new_outgoing;
    return new_outgoing;
  }
  return absl::nullopt;
}

}  // namespace webrtc
