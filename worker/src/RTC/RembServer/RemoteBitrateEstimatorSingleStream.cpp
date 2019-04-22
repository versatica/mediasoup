/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#define MS_CLASS "RTC::RembServer::RemoteBitrateEstimatorSingleStream"
// #define MS_LOG_DEV

#include "RTC/RembServer/RemoteBitrateEstimatorSingleStream.hpp"
#include "DepLibUV.hpp"
#include "Logger.hpp"
#include "RTC/RembServer/AimdRateControl.hpp"
#include "RTC/RembServer/InterArrival.hpp"
#include "RTC/RembServer/OveruseDetector.hpp"
#include "RTC/RembServer/OveruseEstimator.hpp"
#include <utility> // std::make_pair()

namespace RTC
{
	namespace RembServer
	{
		void RemoteBitrateEstimatorSingleStream::IncomingPacket(
		  int64_t arrivalTimeMs,
		  size_t payloadSize,
		  const RTC::RtpPacket& packet,
		  const uint32_t transmissionTimeOffset)
		{
			MS_TRACE();

			if (!this->umaRecorded)
				this->umaRecorded = true;

			uint32_t ssrc         = packet.GetSsrc();
			uint32_t rtpTimestamp = packet.GetTimestamp() + transmissionTimeOffset;
			int64_t nowMs         = DepLibUV::GetTime();

			auto it = this->overuseDetectors.find(ssrc);
			if (it == this->overuseDetectors.end())
			{
				// This is a new SSRC. Adding to map.
				// TODO(holmer): If the channel changes SSRC the old SSRC will still be
				// around in this map until the channel is deleted. This is OK since the
				// callback will no longer be called for the old SSRC. This will be
				// automatically cleaned up when we have one RemoteBitrateEstimator per REMB
				// group.
				std::pair<SsrcOveruseEstimatorMap::iterator, bool> insertResult =
				  this->overuseDetectors.insert(
				    std::make_pair(ssrc, new Detector(nowMs, OverUseDetectorOptions(), true)));
				it = insertResult.first;
			}

			Detector* estimator         = it->second;
			estimator->lastPacketTimeMs = nowMs;

			// Check if incoming bitrate estimate is valid, and if it needs to be reset.
			uint32_t incomingBitrate = this->incomingBitrate.GetRate(nowMs);

			if (incomingBitrate != 0u)
			{
				this->lastValidIncomingBitrate = incomingBitrate;
			}
			else if (this->lastValidIncomingBitrate > 0)
			{
				// Incoming bitrate had a previous valid value, but now not enough data
				// point are left within the current window. Reset incoming bitrate
				// estimator so that the window size will only contain new data points.
				this->incomingBitrate.Reset();
				this->lastValidIncomingBitrate = 0;
			}

			this->incomingBitrate.Update(payloadSize, nowMs);

			const BandwidthUsage priorState = estimator->detector.State();
			uint32_t timestampDelta{ 0 };
			int64_t timeDelta{ 0 };
			int sizeDelta{ 0 };

			if (estimator->interArrival.ComputeDeltas(
			      rtpTimestamp, arrivalTimeMs, nowMs, payloadSize, &timestampDelta, &timeDelta, &sizeDelta))
			{
				double timestampDeltaMs = timestampDelta * TimestampToMs;

				estimator->estimator.Update(
				  timeDelta, timestampDeltaMs, sizeDelta, estimator->detector.State(), nowMs);

				estimator->detector.Detect(
				  estimator->estimator.GetOffset(),
				  timestampDeltaMs,
				  estimator->estimator.GetNumOfDeltas(),
				  nowMs);
			}

			if (estimator->detector.State() == BW_OVERUSING)
			{
				uint32_t incomingBitrateBps = this->incomingBitrate.GetRate(nowMs);

				if (
				  (incomingBitrateBps != 0u) &&
				  (priorState != BW_OVERUSING ||
				   GetRemoteRate()->TimeToReduceFurther(nowMs, incomingBitrateBps)))
				{
					// The first overuse should immediately trigger a new estimate.
					// We also have to update the estimate immediately if we are overusing
					// and the target bitrate is too high compared to what we are receiving.
					UpdateEstimate(nowMs);
				}
			}
		}

		void RemoteBitrateEstimatorSingleStream::UpdateEstimate(int64_t nowMs)
		{
			MS_TRACE();

			BandwidthUsage bwState{ BW_NORMAL };
			double sumVarNoise{ 0.0 };
			auto it = this->overuseDetectors.begin();

			while (it != this->overuseDetectors.end())
			{
				const int64_t timeOfLastReceivedPacket = it->second->lastPacketTimeMs;

				if (timeOfLastReceivedPacket >= 0 && nowMs - timeOfLastReceivedPacket > streamTimeOutMs)
				{
					// This over-use detector hasn't received packets for |kStreamTimeOutMs|
					// milliseconds and is considered stale.
					delete it->second;
					this->overuseDetectors.erase(it++);
				}
				else
				{
					sumVarNoise += it->second->estimator.GetVarNoise();
					// Make sure that we trigger an over-use if any of the over-use detectors
					// is detecting over-use.
					if (it->second->detector.State() > bwState)
						bwState = it->second->detector.State();

					++it;
				}
			}
			// We can't update the estimate if we don't have any active streams.
			if (this->overuseDetectors.empty())
				return;

			AimdRateControl* remoteRate = GetRemoteRate();
			double meanNoiseVar = sumVarNoise / static_cast<double>(this->overuseDetectors.size());
			const RateControlInput input(bwState, this->incomingBitrate.GetRate(nowMs), meanNoiseVar);

			remoteRate->Update(&input, nowMs);

			uint32_t targetBitrate = remoteRate->UpdateBandwidthEstimate(nowMs);

			if (remoteRate->ValidEstimate())
			{
				this->processIntervalMs = remoteRate->GetFeedbackInterval();

				// MS_ASSERT(this->processIntervalMs > 0);

				this->availableBitrate = targetBitrate;

				std::vector<uint32_t> ssrcs;

				GetSsrcs(&ssrcs);
				this->listener->OnRembServerAvailableBitrate(this, ssrcs, targetBitrate);
			}
		}

		bool RemoteBitrateEstimatorSingleStream::LatestEstimate(
		  std::vector<uint32_t>* ssrcs, uint32_t* bitrateBps) const
		{
			MS_TRACE();

			MS_ASSERT(bitrateBps, "bitrateBps missing");

			if (!this->remoteRate->ValidEstimate())
				return false;

			GetSsrcs(ssrcs);

			if (ssrcs->empty())
				*bitrateBps = 0;
			else
				*bitrateBps = this->remoteRate->LatestEstimate();

			return true;
		}

		void RemoteBitrateEstimatorSingleStream::GetSsrcs(std::vector<uint32_t>* ssrcs) const
		{
			MS_TRACE();

			MS_ASSERT(ssrcs, "ssrcs missing");

			ssrcs->resize(this->overuseDetectors.size());

			int i{ 0 };
			auto it = this->overuseDetectors.begin();

			for (; it != this->overuseDetectors.end(); ++it, ++i)
			{
				(*ssrcs)[i] = it->first;
			}
		}
	} // namespace RembServer
} // namespace RTC
