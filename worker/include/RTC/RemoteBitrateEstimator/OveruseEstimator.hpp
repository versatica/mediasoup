/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */
#ifndef MS_RTC_REMOTE_BITRATE_ESTIMATOR_OVERUSE_ESTIMATOR_HPP
#define MS_RTC_REMOTE_BITRATE_ESTIMATOR_OVERUSE_ESTIMATOR_HPP

#include "common.hpp"
#include "RTC/RemoteBitrateEstimator/BandwidthUsage.hpp"
#include <cstring> // std::memcpy()
#include <deque>
#include <utility>

namespace RTC
{
	// (jmillan) borrowed from webrtc/common_types.h
	//
	// Bandwidth over-use detector options.
	// These are used to drive experimentation with bandwidth estimation parameters.
	// See modules/remote_bitrate_estimator/overuse_detector.h
	struct OverUseDetectorOptions
	{
		OverUseDetectorOptions();

		double initialSlope;
		double initialOffset;
		double initialE[2][2];
		double initialProcessNoise[2];
		double initialAvgNoise;
		double initialVarNoise;
	};

	class OveruseEstimator
	{
	public:
		explicit OveruseEstimator(OverUseDetectorOptions options);
		~OveruseEstimator();

		// Update the estimator with a new sample. The deltas should represent deltas
		// between timestamp groups as defined by the InterArrival class.
		// |currentHypothesis| should be the hypothesis of the over-use detector at
		// this time.
		void Update(
		    int64_t tDelta, double tsDelta, int sizeDelta, BandwidthUsage currentHypothesis, int64_t nowMs);
		// Returns the estimated noise/jitter variance in ms^2.
		double GetVarNoise() const;
		// Returns the estimated inter-arrival time delta offset in ms.
		double GetOffset() const;
		// Returns the number of deltas which the current over-use estimator state is
		// based on.
		unsigned int GetNumOfDeltas() const;

	private:
		double UpdateMinFramePeriod(double tsDelta);
		void UpdateNoiseEstimate(double residual, double tsDelta, bool stableState);

	private:
		// Must be first member variable. Cannot be const because we need to be copyable.
		OverUseDetectorOptions options;
		uint16_t numOfDeltas{ 0 };
		double slope{ 0 };
		double offset{ 0 };
		double prevOffset{ 0 };
		double e[2][2];
		double processNoise[2];
		double avgNoise{ 0 };
		double varNoise{ 0 };
		std::deque<double> tsDeltaHist;
	};

	/* Inline methods. */

	inline OverUseDetectorOptions::OverUseDetectorOptions()
	    : initialSlope(8.0 / 512.0), initialOffset(0), initialE(), initialProcessNoise(),
	      initialAvgNoise(0.0), initialVarNoise(50)
	{
		initialE[0][0] = 100;
		initialE[1][1] = 1e-1;
		initialE[0][1] = initialE[1][0] = 0;
		initialProcessNoise[0]          = 1e-13;
		initialProcessNoise[1]          = 1e-3;
	}

	inline OveruseEstimator::OveruseEstimator(OverUseDetectorOptions options)
	    : options(std::move(options)), numOfDeltas(0), slope(this->options.initialSlope),
	      offset(this->options.initialOffset), prevOffset(this->options.initialOffset), e(),
	      processNoise(), avgNoise(this->options.initialAvgNoise),
	      varNoise(this->options.initialVarNoise), tsDeltaHist()
	{
		std::memcpy(this->e, this->options.initialE, sizeof(this->e));
		std::memcpy(this->processNoise, this->options.initialProcessNoise, sizeof(this->processNoise));
	}

	inline OveruseEstimator::~OveruseEstimator()
	{
		this->tsDeltaHist.clear();
	}

	inline double OveruseEstimator::GetVarNoise() const
	{
		return this->varNoise;
	}

	inline double OveruseEstimator::GetOffset() const
	{
		return this->offset;
	}

	inline unsigned int OveruseEstimator::GetNumOfDeltas() const
	{
		return this->numOfDeltas;
	}
} // namespace RTC

#endif
