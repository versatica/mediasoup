/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#define MS_CLASS "RTC::REMB::OveruseEstimator"
// #define MS_LOG_DEV

#include "RTC/REMB/OveruseEstimator.hpp"
#include "Logger.hpp"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>

namespace RTC
{
	namespace REMB
	{
		constexpr size_t MinFramePeriodHistoryLength{ 60 };
		constexpr uint16_t DeltaCounterMax{ 1000 };

		void OveruseEstimator::Update(
		  int64_t tDelta, double tsDelta, int sizeDelta, BandwidthUsage currentHypothesis, int64_t /*nowMs*/)
		{
			MS_TRACE();

			const double minFramePeriod = UpdateMinFramePeriod(tsDelta);
			const double tTsDelta       = tDelta - tsDelta;
			double fsDelta              = sizeDelta;

			++this->numOfDeltas;

			if (this->numOfDeltas > DeltaCounterMax)
				this->numOfDeltas = DeltaCounterMax;

			// Update the Kalman filter.
			this->e[0][0] += this->processNoise[0];
			this->e[1][1] += this->processNoise[1];

			if (
			  (currentHypothesis == BW_OVERUSING && this->offset < this->prevOffset) ||
			  (currentHypothesis == BW_UNDERUSING && this->offset > this->prevOffset))
			{
				this->e[1][1] += 10 * this->processNoise[1];
			}

			const double h[2]        = { fsDelta, 1.0 };
			const double eh[2]       = { this->e[0][0] * h[0] + this->e[0][1] * h[1],
                             this->e[1][0] * h[0] + this->e[1][1] * h[1] };
			const double residual    = tTsDelta - this->slope * h[0] - this->offset;
			const bool inStableState = (currentHypothesis == BW_NORMAL);
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

			const double denom     = this->varNoise + h[0] * eh[0] + h[1] * eh[1];
			const double k[2]      = { eh[0] / denom, eh[1] / denom };
			const double iKh[2][2] = { { 1.0 - k[0] * h[0], -k[0] * h[1] },
				                         { -k[1] * h[0], 1.0 - k[1] * h[1] } };
			const double e00       = this->e[0][0];
			const double e01       = this->e[0][1];

			// Update state.
			this->e[0][0] = e00 * iKh[0][0] + this->e[1][0] * iKh[0][1];
			this->e[0][1] = e01 * iKh[0][0] + this->e[1][1] * iKh[0][1];
			this->e[1][0] = e00 * iKh[1][0] + this->e[1][0] * iKh[1][1];
			this->e[1][1] = e01 * iKh[1][0] + this->e[1][1] * iKh[1][1];

			// The covariance matrix must be positive semi-definite.
			bool positiveSemiDefinite =
			  this->e[0][0] + this->e[1][1] >= 0 &&
			  this->e[0][0] * this->e[1][1] - this->e[0][1] * this->e[1][0] >= 0 && this->e[0][0] >= 0;

			MS_ASSERT(positiveSemiDefinite, "positiveSemiDefinite missing");

			if (!positiveSemiDefinite)
			{
				MS_ERROR("the over-use estimator's covariance matrix is no longer semi-definite");
			}

			this->slope      = this->slope + k[0] * residual;
			this->prevOffset = this->offset;
			this->offset     = this->offset + k[1] * residual;
		}

		double OveruseEstimator::UpdateMinFramePeriod(double tsDelta)
		{
			MS_TRACE();

			double minFramePeriod = tsDelta;

			if (this->tsDeltaHist.size() >= MinFramePeriodHistoryLength)
				this->tsDeltaHist.pop_front();

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
				return;

			// Faster filter during startup to faster adapt to the jitter level
			// of the network. |alpha| is tuned for 30 frames per second, but is scaled
			// according to |tsDelta|.
			double alpha{ 0.01 };

			if (this->numOfDeltas > 10 * 30)
				alpha = 0.002;

			// Only update the noise estimate if we're not over-using. |beta| is a
			// function of alpha and the time delta since the previous update.
			const double beta = pow(1 - alpha, tsDelta * 30.0 / 1000.0);

			this->avgNoise = beta * this->avgNoise + (1 - beta) * residual;
			this->varNoise = beta * this->varNoise +
			                 (1 - beta) * (this->avgNoise - residual) * (this->avgNoise - residual);
			if (this->varNoise < 1)
				this->varNoise = 1;
		}
	} // namespace REMB
} // namespace RTC
