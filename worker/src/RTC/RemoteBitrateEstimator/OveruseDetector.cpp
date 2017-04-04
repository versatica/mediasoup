/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#define MS_CLASS "OveruseDetector"
// #define MS_LOG_DEV

#include "RTC/RemoteBitrateEstimator/OveruseDetector.hpp"
#include "Logger.hpp"
#include <math.h>
#include <stdlib.h>
#include <algorithm>
#include <sstream>
#include <string>

namespace RTC
{
	constexpr double kMaxAdaptOffsetMs = 15.0;
	constexpr int kMinNumDeltas = 60;

	BandwidthUsage OveruseDetector::Detect(double offset, double tsDelta, int numOfDeltas, int64_t nowMs)
	{
		MS_TRACE();

		if (numOfDeltas < 2)
		{
			return kBwNormal;
		}
		const double T = std::min(numOfDeltas, kMinNumDeltas) * offset;
		if (T > this->threshold)
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
					this->timeOverUsing = 0;
					this->overuseCounter = 0;
					this->hypothesis = kBwOverusing;
				}
			}
		}
		else if (T < -this->threshold)
		{
			this->timeOverUsing = -1;
			this->overuseCounter = 0;
			this->hypothesis = kBwUnderusing;
		}
		else
		{
			this->timeOverUsing = -1;
			this->overuseCounter = 0;
			this->hypothesis = kBwNormal;
		}
		this->prevOffset = offset;

		UpdateThreshold(T, nowMs);

		return this->hypothesis;
	}

	void OveruseDetector::UpdateThreshold(double modifiedOffset, int64_t nowMs)
	{
		MS_TRACE();

		if (this->lastUpdateMs == -1)
			this->lastUpdateMs = nowMs;

		if (fabs(modifiedOffset) > this->threshold + kMaxAdaptOffsetMs)
		{
			// Avoid adapting the threshold to big latency spikes, caused e.g.,
			// by a sudden capacity drop.
			this->lastUpdateMs = nowMs;
			return;
		}

		const double k = fabs(modifiedOffset) < this->threshold ? this->kDown : this->kUp;
		const int64_t kMaxTimeDeltaMs = 100;
		int64_t timeDeltaMs = std::min(nowMs - this->lastUpdateMs, kMaxTimeDeltaMs);

		this->threshold += k * (fabs(modifiedOffset) - this->threshold) * timeDeltaMs;

		const double kMinThreshold = 6;
		const double kMaxThreshold = 600;

		this->threshold = std::min(std::max(this->threshold, kMinThreshold), kMaxThreshold);

		this->lastUpdateMs = nowMs;
	}
}
