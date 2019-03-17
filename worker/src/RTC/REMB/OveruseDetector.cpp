/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#define MS_CLASS "RTC::REMB::OveruseDetector"
// #define MS_LOG_DEV

#include "RTC/REMB/OveruseDetector.hpp"
#include "Logger.hpp"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <sstream>
#include <string>

namespace RTC
{
	namespace REMB
	{
		constexpr double MaxAdaptOffsetMs{ 15.0 };
		constexpr int MinNumDeltas{ 60 };

		BandwidthUsage OveruseDetector::Detect(double offset, double tsDelta, int numOfDeltas, int64_t nowMs)
		{
			MS_TRACE();

			if (numOfDeltas < 2)
			{
				return BW_NORMAL;
			}

			const double t = std::min(numOfDeltas, MinNumDeltas) * offset;

			if (t > this->threshold)
			{
				if (this->timeOverUsing == -1)
				{
					// Initialize the timer. Assume that we've been
					// over-using half of the time since the previous
					// sample.
					this->timeOverUsing = tsDelta / 2;
				}
				else
				{
					// Increment timer
					this->timeOverUsing += tsDelta;
				}

				this->overuseCounter++;

				if (this->timeOverUsing > this->overusingTimeThreshold && this->overuseCounter > 1)
				{
					if (offset >= this->prevOffset)
					{
						this->timeOverUsing  = 0;
						this->overuseCounter = 0;
						this->hypothesis     = BW_OVERUSING;
					}
				}
			}
			else if (t < -this->threshold)
			{
				this->timeOverUsing  = -1;
				this->overuseCounter = 0;
				this->hypothesis     = BW_UNDERUSING;
			}
			else
			{
				this->timeOverUsing  = -1;
				this->overuseCounter = 0;
				this->hypothesis     = BW_NORMAL;
			}

			this->prevOffset = offset;
			UpdateThreshold(t, nowMs);

			return this->hypothesis;
		}

		void OveruseDetector::UpdateThreshold(double modifiedOffset, int64_t nowMs)
		{
			MS_TRACE();

			if (this->lastUpdateMs == -1)
				this->lastUpdateMs = nowMs;

			if (fabs(modifiedOffset) > this->threshold + MaxAdaptOffsetMs)
			{
				// Avoid adapting the threshold to big latency spikes, caused e.g.,
				// by a sudden capacity drop.
				this->lastUpdateMs = nowMs;

				return;
			}

			const double k = fabs(modifiedOffset) < this->threshold ? this->down : this->up;
			const int64_t maxTimeDeltaMs{ 100 };
			int64_t timeDeltaMs = std::min(nowMs - this->lastUpdateMs, maxTimeDeltaMs);

			this->threshold += k * (fabs(modifiedOffset) - this->threshold) * timeDeltaMs;

			const double minThreshold{ 6 };
			const double maxThreshold{ 600 };

			this->threshold    = std::min(std::max(this->threshold, minThreshold), maxThreshold);
			this->lastUpdateMs = nowMs;
		}
	} // namespace REMB
} // namespace RTC
