#define MS_CLASS "RTC::BWE::OveruseEstimator"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/BWE/OveruseEstimator.hpp"
#include "Logger.hpp"
#include <cmath>

namespace RTC
{
	namespace BWE
	{
		/* Static. */

		// How many send deltas are kept to look for the shortest one.
		static constexpr size_t MinFramePeriodHistoryLength{ 60 };
		// Maximum value of the delta counter.
		static constexpr int64_t DeltaCounterMax{ 1000 };
		// Number of standard deviations beyond which a sample is taken as something
		// other than the network, and hence not allowed to move the noise estimate.
		static constexpr double MaxResidualDeviations{ 3.0 };
		// Weight of each sample in the noise estimate, and the same once enough
		// samples have gone by for it to have settled.
		static constexpr double NoiseAlpha{ 0.01 };
		static constexpr double SettledNoiseAlpha{ 0.002 };
		// Samples after which the weight above is the settled one.
		static constexpr int64_t SettledNumOfDeltas{ 10 * 30 };
		// Rate the weight above is expressed at, so that it doesn't depend on how
		// often packets come.
		static constexpr double NoiseAlphaFrameRate{ 30.0 };
		// Lowest variance of the noise that may be estimated (ms^2).
		static constexpr double MinVarNoiseMs2{ 1.0 };

		/* Instance methods. */

		void OveruseEstimator::Update(
		  double arrivalDeltaMs,
		  double sendDeltaMs,
		  int64_t sizeDelta,
		  Types::BandwidthUsage currentHypothesis)
		{
			MS_TRACE();

			const double minFramePeriodMs = UpdateMinFramePeriod(sendDeltaMs);
			// How much more the group took to arrive than it took to be sent, which is
			// what the model has to explain.
			const double deltaDiffMs   = arrivalDeltaMs - sendDeltaMs;
			const auto sizeDeltaDouble = static_cast<double>(sizeDelta);

			this->numOfDeltas = std::min(this->numOfDeltas + 1, DeltaCounterMax);

			// Let both values drift a little before looking at the sample, which is
			// what keeps the filter from locking onto a stale estimate.
			this->e[0][0] += this->processNoise[0];
			this->e[1][1] += this->processNoise[1];

			// And let the leftover delay drift much faster while the detector says the
			// network is changing in the direction the estimate isn't following, so
			// that it catches up instead of arguing with it.
			if (
			  (currentHypothesis == Types::BandwidthUsage::OVERUSING && this->offsetMs < this->prevOffsetMs) ||
			  (currentHypothesis == Types::BandwidthUsage::UNDERUSING &&
				 this->offsetMs > this->prevOffsetMs))
			{
				this->e[1][1] += 10 * this->processNoise[1];
			}

			// What the sample says about each of the two values.
			const double h[2]  = { sizeDeltaDouble, 1.0 };
			const double eh[2] = { (this->e[0][0] * h[0]) + (this->e[0][1] * h[1]),
			                       (this->e[1][0] * h[0]) + (this->e[1][1] * h[1]) };
			// How far the sample falls from what the model predicted.
			const double residual = deltaDiffMs - (this->slopeMsPerByte * h[0]) - this->offsetMs;

			const bool inStableState = currentHypothesis == Types::BandwidthUsage::NORMAL;
			const double maxResidual = MaxResidualDeviations * std::sqrt(this->varNoiseMs2);

			// A sample far beyond the noise is not noise: it's a key frame arriving
			// late, or something else that the model doesn't describe. It still counts,
			// but only up to that bound.
			if (std::fabs(residual) < maxResidual)
			{
				UpdateNoiseEstimate(residual, minFramePeriodMs, inStableState);
			}
			else
			{
				UpdateNoiseEstimate(
				  residual < 0 ? -maxResidual : maxResidual, minFramePeriodMs, inStableState);
			}

			// How much of the sample to believe, which is how certain the filter is of
			// itself compared to how noisy the samples are.
			const double denom = this->varNoiseMs2 + (h[0] * eh[0]) + (h[1] * eh[1]);
			const double k[2]  = { eh[0] / denom, eh[1] / denom };

			const double iKh[2][2] = {
				{ 1.0 - (k[0] * h[0]), -k[0] * h[1]        },
        { -k[1] * h[0],        1.0 - (k[1] * h[1]) }
			};
			const double e00 = this->e[0][0];
			const double e01 = this->e[0][1];

			this->e[0][0] = (e00 * iKh[0][0]) + (this->e[1][0] * iKh[0][1]);
			this->e[0][1] = (e01 * iKh[0][0]) + (this->e[1][1] * iKh[0][1]);
			this->e[1][0] = (e00 * iKh[1][0]) + (this->e[1][0] * iKh[1][1]);
			this->e[1][1] = (e01 * iKh[1][0]) + (this->e[1][1] * iKh[1][1]);

			// NOTE: A covariance matrix that isn't positive semi-definite means the
			// arithmetic above went wrong, which cannot depend on what arrives.
			MS_ASSERT(
			  this->e[0][0] + this->e[1][1] >= 0 &&
			    (this->e[0][0] * this->e[1][1]) - (this->e[0][1] * this->e[1][0]) >= 0 &&
			    this->e[0][0] >= 0,
			  "covariance matrix is no longer semi-definite");

			this->slopeMsPerByte = this->slopeMsPerByte + (k[0] * residual);
			this->prevOffsetMs   = this->offsetMs;
			this->offsetMs       = this->offsetMs + (k[1] * residual);
		}

		double OveruseEstimator::UpdateMinFramePeriod(double sendDeltaMs)
		{
			MS_TRACE();

			double minFramePeriodMs = sendDeltaMs;

			if (this->sendDeltaHistMs.size() >= MinFramePeriodHistoryLength)
			{
				this->sendDeltaHistMs.pop_front();
			}

			for (const double oldSendDeltaMs : this->sendDeltaHistMs)
			{
				minFramePeriodMs = std::min(oldSendDeltaMs, minFramePeriodMs);
			}

			this->sendDeltaHistMs.push_back(sendDeltaMs);

			return minFramePeriodMs;
		}

		void OveruseEstimator::UpdateNoiseEstimate(double residual, double minFramePeriodMs, bool stableState)
		{
			MS_TRACE();

			// While the network is not behaving, what the samples show is the network
			// misbehaving rather than the noise it has when it's fine.
			if (!stableState)
			{
				return;
			}

			// The filter runs faster at the start so that it reaches the jitter level
			// of the link quickly, and settles once it has seen enough.
			const double alpha = this->numOfDeltas > SettledNumOfDeltas ? SettledNoiseAlpha : NoiseAlpha;
			// The weight above is expressed per frame at a given rate, so it's scaled
			// by how far apart these samples actually came.
			const double beta = std::pow(1 - alpha, minFramePeriodMs * NoiseAlphaFrameRate / 1000.0);

			this->avgNoiseMs  = (beta * this->avgNoiseMs) + ((1 - beta) * residual);
			this->varNoiseMs2 = (beta * this->varNoiseMs2) + ((1 - beta) * (this->avgNoiseMs - residual) *
			                                                  (this->avgNoiseMs - residual));

			this->varNoiseMs2 = std::max(this->varNoiseMs2, MinVarNoiseMs2);
		}
	} // namespace BWE
} // namespace RTC
