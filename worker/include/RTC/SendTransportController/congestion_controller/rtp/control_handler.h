/*
 *  Copyright (c) 2018 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef MODULES_CONGESTION_CONTROLLER_RTP_CONTROL_HANDLER_H_
#define MODULES_CONGESTION_CONTROLLER_RTP_CONTROL_HANDLER_H_

#include <stdint.h>

#include "absl/types/optional.h"
#include "RTC/SendTransportController/network_types.h"
#include "RTC/SendTransportController/data_size.h"
#include "RTC/SendTransportController/time_delta.h"
#include "RTC/SendTransportController/constructor_magic.h"

namespace webrtc {
// This is used to observe the network controller state and route calls to
// the proper handler. It also keeps cached values for safe asynchronous use.
// This makes sure that things running on the worker queue can't access state
// in RtpTransportControllerSend, which would risk causing data race on
// destruction unless members are properly ordered.
class CongestionControlHandler {
 public:
  CongestionControlHandler() = default;
  ~CongestionControlHandler() = default;

  void SetTargetRate(TargetTransferRate new_target_rate);
  void SetNetworkAvailability(bool network_available);
  absl::optional<TargetTransferRate> GetUpdate();

 private:
  absl::optional<TargetTransferRate> last_incoming_;
  absl::optional<TargetTransferRate> last_reported_;
  bool network_available_ = true;
  bool encoder_paused_in_last_report_ = false;

  RTC_DISALLOW_COPY_AND_ASSIGN(CongestionControlHandler);
};
}  // namespace webrtc
#endif  // MODULES_CONGESTION_CONTROLLER_RTP_CONTROL_HANDLER_H_
