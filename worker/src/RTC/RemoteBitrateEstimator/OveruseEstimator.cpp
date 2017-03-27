/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#define MS_CLASS "OveruseEstimator"
// #define MS_LOG_DEV

#include "RTC/RemoteBitrateEstimator/OveruseEstimator.hpp"
#include "Logger.hpp"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <algorithm>

namespace RTC
{
	constexpr size_t kMinFramePeriodHistoryLength = 60;
	constexpr uint16_t kDeltaCounterMax = 1000;

	void OveruseEstimator::Update(int64_t tDelta, double tsDelta, int sizeDelta, BandwidthUsage currentHypothesis, int64_t nowMs)
	{
		MS_TRACE();

		(void) nowMs;
		const double minFramePeriod = UpdateMinFramePeriod(tsDelta);
		const double tTsDelta = tDelta - tsDelta;
		double fsDelta = sizeDelta;

		++this->numOfDeltas;
		if (this->numOfDeltas > kDeltaCounterMax)
		{
			this->numOfDeltas = kDeltaCounterMax;
		}

		// Update the Kalman filter.
		this->E[0][0] += this->processNoise[0];
		this->E[1][1] += this->processNoise[1];

		if ((currentHypothesis == kBwOverusing && this->offset < this->prevOffset) ||
		    (currentHypothesis == kBwUnderusing && this->offset > this->prevOffset))
		{
			this->E[1][1] += 10 * this->processNoise[1];
		}

		const double h[2] = {fsDelta, 1.0};
		const double Eh[2] = {this->E[0][0] * h[0] + this->E[0][1] * h[1], this->E[1][0] * h[0] + this->E[1][1] * h[1]};

		const double residual = tTsDelta - this->slope * h[0] - this->offset;

		const bool inStableState = (currentHypothesis == kBwNormal);
		const double maxResidual = 3.0 * sqrt(this->varNoise);
		// We try to filter out very late frames. For instance periodic key
		// frames doesn't fit the Gaussian model well.
		if (fabs(residual) < maxResidual)
		{
			UpdateNoiseEstimate(residual, minFramePeriod, inStableState);
		}
		else
		{
			UpdateNoiseEstimate(residual < 0 ? -maxResidual : maxResidual, minFramePeriod, inStableState);
		}

		const double denom = this->varNoise + h[0] * Eh[0] + h[1] * Eh[1];

		const double K[2] = {Eh[0] / denom, Eh[1] / denom};

		const double IKh[2][2] = {{1.0 - K[0] * h[0], -K[0] * h[1]}, {-K[1] * h[0], 1.0 - K[1] * h[1]}};
		const double e00 = this->E[0][0];
		const double e01 = this->E[0][1];

		// Update state.
		this->E[0][0] = e00 * IKh[0][0] + this->E[1][0] * IKh[0][1];
		this->E[0][1] = e01 * IKh[0][0] + this->E[1][1] * IKh[0][1];
		this->E[1][0] = e00 * IKh[1][0] + this->E[1][0] * IKh[1][1];
		this->E[1][1] = e01 * IKh[1][0] + this->E[1][1] * IKh[1][1];

		// The covariance matrix must be positive semi-definite.
		bool positiveSemiDefinite =
		    this->E[0][0] + this->E[1][1] >= 0 && this->E[0][0] * this->E[1][1] - this->E[0][1] * this->E[1][0] >= 0 && this->E[0][0] >= 0;

		MS_ASSERT(positiveSemiDefinite, "positiveSemiDefinite missing");

		if (!positiveSemiDefinite)
		{
			MS_ERROR("the over-use estimator's covariance matrix is no longer semi-definite");
		}

		this->slope = this->slope + K[0] * residual;
		this->prevOffset = this->offset;
		this->offset = this->offset + K[1] * residual;
	}

	double OveruseEstimator::UpdateMinFramePeriod(double tsDelta)
	{
		MS_TRACE();

		double minFramePeriod = tsDelta;

		if (this->tsDeltaHist.size() >= kMinFramePeriodHistoryLength)
		{
			this->tsDeltaHist.pop_front();
		}
		for (const double oldTsDelta : this->tsDeltaHist)
		{
			minFramePeriod = std::min(oldTsDelta, minFramePeriod);
		}
		this->tsDeltaHist.push_back(tsDelta);
		return minFramePeriod;
	}

	void OveruseEstimator::UpdateNoiseEstimate(double residual, double tsDelta, bool stableState)
	{
		MS_TRACE();

		if (!stableState)
		{
			return;
		}
		// Faster filter during startup to faster adapt to the jitter level
		// of the network. |alpha| is tuned for 30 frames per second, but is scaled
		// according to |tsDelta|.
		double alpha = 0.01;

		if (this->numOfDeltas > 10 * 30)
		{
			alpha = 0.002;
		}

		// Only update the noise estimate if we're not over-using. |beta| is a
		// function of alpha and the time delta since the previous update.
		const double beta = pow(1 - alpha, tsDelta * 30.0 / 1000.0);

		this->avgNoise = beta * this->avgNoise + (1 - beta) * residual;
		this->varNoise = beta * this->varNoise + (1 - beta) * (this->avgNoise - residual) * (this->avgNoise - residual);
		if (this->varNoise < 1)
		{
			this->varNoise = 1;
		}
	}
}
