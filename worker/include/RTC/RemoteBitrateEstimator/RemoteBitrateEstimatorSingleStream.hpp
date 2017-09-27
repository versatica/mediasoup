/*
 *  Copyright (c) 2015 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef MS_RTC_REMOTE_BITRATE_ESTIMATOR_SINGLE_STREAM_HPP
#define MS_RTC_REMOTE_BITRATE_ESTIMATOR_SINGLE_STREAM_HPP

#include "common.hpp"
#include "RTC/RemoteBitrateEstimator/AimdRateControl.hpp"
#include "RTC/RemoteBitrateEstimator/InterArrival.hpp"
#include "RTC/RemoteBitrateEstimator/OveruseEstimator.hpp"
#include "RTC/RemoteBitrateEstimator/RemoteBitrateEstimator.hpp"
#include "RTC/RtpDataCounter.hpp"
#include <cassert>
#include <map>
#include <memory>
#include <vector>

namespace RTC
{
	class RemoteBitrateEstimatorSingleStream : public RemoteBitrateEstimator
	{
	private:
		static constexpr double TimestampToMs{ 1.0 / 90.0 };

	public:
		explicit RemoteBitrateEstimatorSingleStream(Listener* observer);
		~RemoteBitrateEstimatorSingleStream() override;

		void IncomingPacket(
		  int64_t arrivalTimeMs,
		  size_t payloadSize,
		  const RtpPacket& packet,
		  uint32_t transmissionTimeOffset) override;
		void Process() override;
		void OnRttUpdate(int64_t avgRttMs, int64_t maxRttMs) override;
		void RemoveStream(uint32_t ssrc) override;
		bool LatestEstimate(std::vector<uint32_t>* ssrcs, uint32_t* bitrateBps) const override;
		void SetMinBitrate(int minBitrateBps) override;

	private:
		struct Detector
		{
			enum
			{
				TIMESTAMP_GROUP_LENGTH_MS = 5
			};
			explicit Detector(
			  int64_t lastPacketTimeMs, const OverUseDetectorOptions& options, bool enableBurstGrouping);

			int64_t lastPacketTimeMs;
			InterArrival interArrival;
			OveruseEstimator estimator;
			OveruseDetector detector;
		};

	private:
		using SsrcOveruseEstimatorMap = std::map<uint32_t, Detector*>;

	private:
		// Triggers a new estimate calculation.
		void UpdateEstimate(int64_t nowMs);
		void GetSsrcs(std::vector<uint32_t>* ssrcs) const;

		// Returns |remoteRate| if the pointed to object exists,
		// otherwise creates it.
		AimdRateControl* GetRemoteRate();

	private:
		SsrcOveruseEstimatorMap overuseDetectors;
		RateCalculator incomingBitrate;
		uint32_t lastValidIncomingBitrate{ 0 };
		std::unique_ptr<AimdRateControl> remoteRate;
		Listener* observer{ nullptr };
		int64_t lastProcessTime{ -1 };
		int64_t processIntervalMs{ 500 };
		bool umaRecorded{ false };
	};

	/* Inline Methods. */

	inline RemoteBitrateEstimatorSingleStream::Detector::Detector(
	  int64_t lastPacketTimeMs, const OverUseDetectorOptions& options, bool enableBurstGrouping)
	  : lastPacketTimeMs(lastPacketTimeMs),
	    interArrival(90 * TIMESTAMP_GROUP_LENGTH_MS, TimestampToMs, enableBurstGrouping),
	    estimator(options), detector()
	{
	}

	inline RemoteBitrateEstimatorSingleStream::RemoteBitrateEstimatorSingleStream(Listener* observer)
	  : incomingBitrate(), remoteRate(new AimdRateControl()), observer(observer)
	{
		// assert(this->observer);
	}

	inline RemoteBitrateEstimatorSingleStream::~RemoteBitrateEstimatorSingleStream()
	{
		while (!this->overuseDetectors.empty())
		{
			auto it = this->overuseDetectors.begin();

			delete it->second;
			this->overuseDetectors.erase(it);
		}
	}

	inline void RemoteBitrateEstimatorSingleStream::Process()
	{
		UpdateEstimate(DepLibUV::GetTime());
		this->lastProcessTime = DepLibUV::GetTime();
	}

	inline void RemoteBitrateEstimatorSingleStream::OnRttUpdate(int64_t avgRttMs, int64_t /*maxRttMs*/)
	{
		GetRemoteRate()->SetRtt(avgRttMs);
	}

	inline void RemoteBitrateEstimatorSingleStream::RemoveStream(unsigned int ssrc)
	{
		auto it = this->overuseDetectors.find(ssrc);

		if (it != this->overuseDetectors.end())
		{
			delete it->second;
			this->overuseDetectors.erase(it);
		}
	}

	inline AimdRateControl* RemoteBitrateEstimatorSingleStream::GetRemoteRate()
	{
		if (!this->remoteRate)
			this->remoteRate.reset(new AimdRateControl());
		return this->remoteRate.get();
	}

	inline void RemoteBitrateEstimatorSingleStream::SetMinBitrate(int minBitrateBps)
	{
		this->remoteRate->SetMinBitrate(minBitrateBps);
	}
} // namespace RTC

#endif
