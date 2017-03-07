/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef MS_RTC_REMOTE_BITRATE_ESTIMATOR_BWE_DEFINES_HPP
#define MS_RTC_REMOTE_BITRATE_ESTIMATOR_BWE_DEFINES_HPP

#include "common.hpp"

namespace RTC {

namespace congestion_controller {
int GetMinBitrateBps();
}  // namespace congestion_controller

static const int64_t kBitrateWindowMs = 1000;

extern const char* kBweTypeHistogram;

enum BweNames {
  kReceiverNoExtension = 0,
  kReceiverTOffset = 1,
  kReceiverAbsSendTime = 2,
  kSendSideTransportSeqNum = 3,
  kBweNamesMax = 4
};

enum BandwidthUsage {
  kBwNormal = 0,
  kBwUnderusing = 1,
  kBwOverusing = 2,
};

enum RateControlState { kRcHold, kRcIncrease, kRcDecrease };

enum RateControlRegion { kRcNearMax, kRcAboveMax, kRcMaxUnknown };

struct RateControlInput {
  RateControlInput(BandwidthUsage bw_state,
                   const uint32_t incoming_bitrate,
                   double noise_var)
      : bw_state(bw_state),
        incoming_bitrate(incoming_bitrate),
        noise_var(noise_var) {}

  BandwidthUsage bw_state;
  uint32_t incoming_bitrate;
  double noise_var;
};
}  // namespace RTC

#endif  // MS_RTC_REMOTE_BITRATE_ESTIMATOR_INCLUDE_BWE_DEFINES_H_
