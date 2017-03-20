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
#include <math.h>
#include <stdlib.h>
#include <algorithm>
#include <sstream>
#include <string>

namespace RTC
{
	// (jmillan) disable the experiment until we know how to use the threshold values.
	/*
		 const char kAdaptiveThresholdExperiment[] = "WebRTC-AdaptiveBweThreshold";
		 const char kEnabledPrefix[] = "Enabled";
		 const size_t kEnabledPrefixLength = sizeof(kEnabledPrefix) - 1;
		 const char kDisabledPrefix[] = "Disabled";
		 const size_t kDisabledPrefixLength = sizeof(kDisabledPrefix) - 1;
		 */

	const double kMaxAdaptOffsetMs = 15.0;
	const double kOverUsingTimeThreshold = 10;
	const int kMinNumDeltas = 60;

	// (jmillan) disable the experiment until we know how to use the threshold values.
	bool AdaptiveThresholdExperimentIsDisabled()
	{
		return true;
	}

	bool ReadExperimentConstants(double* kUp, double* kDown)
	{
		(void) kUp;
		(void) kDown;
		return false;
	}

	/*
		 bool AdaptiveThresholdExperimentIsDisabled() {
		 std::string experimentString =
		 webrtc::fieldTrial::FindFullName(kAdaptiveThresholdExperiment);
		 const size_t kMinExperimentLength = kDisabledPrefixLength;
		 if (experimentString.length() < kMinExperimentLength)
		 return false;
		 return experimentString.substr(0, kDisabledPrefixLength) == kDisabledPrefix;
		 }
		 */

	// Gets thresholds from the experiment name following the format
	// "WebRTC-AdaptiveBweThreshold/Enabled-0.5,0.002/".
	/*
		 bool ReadExperimentConstants(double* kUp, double* kDown) {
		 std::string experimentString =
		 webrtc::fieldTrial::FindFullName(kAdaptiveThresholdExperiment);
		 const size_t kMinExperimentLength = kEnabledPrefixLength + 3;
		 if (experimentString.length() < kMinExperimentLength ||
		 experimentString.substr(0, kEnabledPrefixLength) != kEnabledPrefix)
		 return false;
		 return sscanf(experimentString.substr(kEnabledPrefixLength + 1).c_str(),
		 "%lf,%lf", kUp, kDown) == 2;
		 }
		 */

	BandwidthUsage OveruseDetector::Detect(double offset, double tsDelta, int numOfDeltas, int64_t nowMs)
	{
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
		if (!this->inExperiment)
			return;

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

	void OveruseDetector::InitializeExperiment()
	{
		//MS_DASSERT(this->inExperiment);
		double kUp = 0.0;
		double kDown = 0.0;
		this->overusingTimeThreshold = kOverUsingTimeThreshold;
		if (ReadExperimentConstants(&kUp, &kDown))
		{
			this->kUp = kUp;
			this->kDown = kDown;
		}
	}
}
