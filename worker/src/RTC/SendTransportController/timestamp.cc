/*
 *  Copyright (c) 2018 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "RTC/SendTransportController/timestamp.h"
#include <sstream>

// #include "api/array_view.h"
// #include "rtc_base/strings/string_builder.h"

namespace webrtc {

std::string ToString(Timestamp value) {
  std::ostringstream sb;
  if (value.IsPlusInfinity()) {
    sb << "+inf ms";
  } else if (value.IsMinusInfinity()) {
    sb << "-inf ms";
  } else {
    if (value.ms() % 1000 == 0)
      sb << value.seconds() << " s";
    else
      sb << value.ms() << " ms";
  }
  return sb.str();
}
}  // namespace webrtc
