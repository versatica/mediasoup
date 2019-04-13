/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#define MS_CLASS "RTC::RembServer::RemoteBitrateEstimatorAbsSendTime"
// #define MS_LOG_DEV

#include "RTC/RembServer/RemoteBitrateEstimatorAbsSendTime.hpp"
#include "DepLibUV.hpp"
#include "Logger.hpp"
#include "RTC/RembServer/RemoteBitrateEstimator.hpp"
#include <algorithm>
#include <cmath>

namespace RTC
{
	namespace RembServer
	{
		static constexpr int TimestampGroupLengthMs{ 5 };
		static constexpr uint32_t AbsSendTimeFraction{ 18 };
		static constexpr uint32_t AbsSendTimeInterArrivalUpshift{ 8 };
		static constexpr uint32_t InterArrivalShift{ AbsSendTimeFraction +
			                                           AbsSendTimeInterArrivalUpshift };
		static constexpr uint32_t InitialProbingIntervalMs{ 2000 };
		static constexpr int MinClusterSize{ 4 };
		static constexpr size_t MaxProbePackets{ 15 };
		static constexpr size_t ExpectedNumberOfProbes{ 3 };
		static constexpr double TimestampToMs{ 1000.0 / static_cast<double>(1 << InterArrivalShift) };

		template<typename K, typename V>
		std::vector<K> Keys(const std::map<K, V>& map)
		{
			std::vector<K> keys;

			keys.reserve(map.size());

			auto it = map.begin();
			for (; it != map.end(); ++it)
			{
				keys.push_back(it->first);
			}

			return keys;
		}

		bool RemoteBitrateEstimatorAbsSendTime::IsWithinClusterBounds(
		  int sendDeltaMs, const Cluster& clusterAggregate)
		{
			MS_TRACE();

			if (clusterAggregate.count == 0)
				return true;

			float clusterMean = clusterAggregate.sendMeanMs / static_cast<float>(clusterAggregate.count);

			return std::fabs(static_cast<float>(sendDeltaMs) - clusterMean) < 2.5f;
		}

		void RemoteBitrateEstimatorAbsSendTime::AddCluster(std::list<Cluster>* clusters, Cluster* cluster)
		{
			MS_TRACE();

			cluster->sendMeanMs /= static_cast<float>(cluster->count);
			cluster->recvMeanMs /= static_cast<float>(cluster->count);
			cluster->meanSize /= cluster->count;

			clusters->push_back(*cluster);
		}

		void RemoteBitrateEstimatorAbsSendTime::ComputeClusters(std::list<Cluster>* clusters) const
		{
			MS_TRACE();

			Cluster current;
			int64_t prevSendTime{ -1 };
			int64_t prevRecvTime{ -1 };

			auto it = this->probes.begin();
			for (; it != this->probes.end(); ++it)
			{
				if (prevSendTime >= 0)
				{
					int sendDeltaMs = it->sendTimeMs - prevSendTime;
					int recvDeltaMs = it->recvTimeMs - prevRecvTime;

					if (sendDeltaMs >= 1 && recvDeltaMs >= 1)
						++current.numAboveMinDelta;

					if (!IsWithinClusterBounds(sendDeltaMs, current))
					{
						if (current.count >= MinClusterSize)
							AddCluster(clusters, &current);

						current = Cluster();
					}

					current.sendMeanMs += sendDeltaMs;
					current.recvMeanMs += recvDeltaMs;
					current.meanSize += it->payloadSize;
					++current.count;
				}

				prevSendTime = it->sendTimeMs;
				prevRecvTime = it->recvTimeMs;
			}

			if (current.count >= MinClusterSize)
				AddCluster(clusters, &current);
		}

		std::list<Cluster>::const_iterator RemoteBitrateEstimatorAbsSendTime::FindBestProbe(
		  const std::list<Cluster>& clusters) const
		{
			MS_TRACE();

			int highestProbeBitrateBps{ 0 };
			auto bestIt = clusters.end();

			auto it = clusters.begin();
			for (; it != clusters.end(); ++it)
			{
				if (it->sendMeanMs == 0 || it->recvMeanMs == 0)
					continue;

				if (
				  it->numAboveMinDelta > it->count / 2 &&
				  (it->recvMeanMs - it->sendMeanMs <= 2.0f && it->sendMeanMs - it->recvMeanMs <= 5.0f))
				{
					int probeBitrateBps = std::min(it->GetSendBitrateBps(), it->GetRecvBitrateBps());

					if (probeBitrateBps > highestProbeBitrateBps)
					{
						highestProbeBitrateBps = probeBitrateBps;
						bestIt                 = it;
					}
				}
				else
				{
#ifdef MS_LOG_DEV
					int sendBitrateBps = it->meanSize * 8 * 1000 / it->sendMeanMs;
					int recvBitrateBps = it->meanSize * 8 * 1000 / it->recvMeanMs;

					MS_WARN_DEV(
					  "probe failed, sent at %d bps, received at %d bps [mean "
					  "send delta:%fms, mean recv delta:%fms, num probes:%d]",
					  sendBitrateBps,
					  recvBitrateBps,
					  it->sendMeanMs,
					  it->recvMeanMs,
					  it->count);
#endif

					break;
				}
			}

			return bestIt;
		}

		RemoteBitrateEstimatorAbsSendTime::ProbeResult RemoteBitrateEstimatorAbsSendTime::ProcessClusters(
		  int64_t nowMs)
		{
			MS_TRACE();

			std::list<Cluster> clusters;
			ComputeClusters(&clusters);

			if (clusters.empty())
			{
				// If we reach the max number of probe packets and still have no clusters,
				// we will remove the oldest one.
				if (this->probes.size() >= MaxProbePackets)
					this->probes.pop_front();

				return ProbeResult::NO_UPDATE;
			}

			auto bestIt = FindBestProbe(clusters);
			if (bestIt != clusters.end())
			{
				int probeBitrateBps = std::min(bestIt->GetSendBitrateBps(), bestIt->GetRecvBitrateBps());

				// Make sure that a probe sent on a lower bitrate than our estimate can't
				// reduce the estimate.
				if (IsBitrateImproving(probeBitrateBps))
				{
					MS_DEBUG_DEV(
					  "probe successful, sent at %d bps, received at %d bps "
					  "[mean send delta:%fms, mean recv delta:%f ms, "
					  "num probes:%d",
					  bestIt->GetSendBitrateBps(),
					  bestIt->GetRecvBitrateBps(),
					  bestIt->sendMeanMs,
					  bestIt->recvMeanMs,
					  bestIt->count);

					this->remoteRate.SetEstimate(probeBitrateBps, nowMs);

					return ProbeResult::BITRATE_UPDATED;
				}
			}

			// Not probing and received non-probe packet, or finished with current set
			// of probes.
			if (clusters.size() >= ExpectedNumberOfProbes)
				this->probes.clear();

			return ProbeResult::NO_UPDATE;
		}

		bool RemoteBitrateEstimatorAbsSendTime::IsBitrateImproving(int newBitrateBps) const
		{
			MS_TRACE();

			bool initialProbe         = !this->remoteRate.ValidEstimate() && newBitrateBps > 0;
			bool bitrateAboveEstimate = this->remoteRate.ValidEstimate() &&
			                            newBitrateBps > static_cast<int>(this->remoteRate.LatestEstimate());

			return initialProbe || bitrateAboveEstimate;
		}

		void RemoteBitrateEstimatorAbsSendTime::IncomingPacket(
		  int64_t arrivalTimeMs, size_t payloadSize, const RTC::RtpPacket& packet, const uint32_t absSendTime)
		{
			MS_TRACE();

			IncomingPacketInfo(arrivalTimeMs, absSendTime, payloadSize, packet.GetSsrc());
		}

		void RemoteBitrateEstimatorAbsSendTime::IncomingPacketInfo(
		  int64_t arrivalTimeMs, uint32_t sendTime24bits, size_t payloadSize, uint32_t ssrc)
		{
			MS_TRACE();

			MS_ASSERT(sendTime24bits < (1ul << 24), "invalid sendTime24bits value");

			if (!this->umaRecorded)
				this->umaRecorded = true;

			// Shift up send time to use the full 32 bits that interArrival works with,
			// so wrapping works properly.
			uint32_t timestamp = sendTime24bits << AbsSendTimeInterArrivalUpshift;
			int64_t sendTimeMs = int64_t{ timestamp } * TimestampToMs;
			int64_t nowMs      = DepLibUV::GetTime();
			// TODO(holmer): SSRCs are only needed for REMB, should be broken out from
			// here.
			// Check if incoming bitrate estimate is valid, and if it needs to be reset.
			uint32_t incomingBitrate = this->incomingBitrate.GetRate(arrivalTimeMs);

			if (incomingBitrate != 0u)
			{
				this->incomingBitrateInitialized = true;
			}
			else if (this->incomingBitrateInitialized)
			{
				// Incoming bitrate had a previous valid value, but now not enough data
				// point are left within the current window. Reset incoming bitrate
				// estimator so that the window size will only contain new data points.
				this->incomingBitrate.Reset();
				this->incomingBitrateInitialized = false;
			}

			this->incomingBitrate.Update(payloadSize, arrivalTimeMs);

			if (this->firstPacketTimeMs == -1)
				this->firstPacketTimeMs = nowMs;

			uint32_t tsDelta{ 0 };
			int64_t tDelta{ 0 };
			int sizeDelta{ 0 };
			bool updateEstimate{ false };
			uint32_t targetBitrateBps{ 0 };
			std::vector<uint32_t> ssrcs;

			{
				TimeoutStreams(nowMs);

				// MS_ASSERT(this->interArrival.get());
				// MS_ASSERT(this->estimator.get());

				this->ssrcs[ssrc] = nowMs;

				// For now only try to detect probes while we don't have a valid estimate.
				// We currently assume that only packets larger than 200 bytes are paced by
				// the sender.
				const size_t minProbePacketSize{ 200 };

				if (
				  payloadSize > minProbePacketSize &&
				  (!this->remoteRate.ValidEstimate() ||
				   nowMs - this->firstPacketTimeMs < InitialProbingIntervalMs))
				{
#ifdef MS_LOG_DEV
					// TODO(holmer): Use a map instead to get correct order?
					if (this->totalProbesReceived < MaxProbePackets)
					{
						int sendDeltaMs{ -1 };
						int recvDeltaMs{ -1 };

						if (!this->probes.empty())
						{
							sendDeltaMs = sendTimeMs - this->probes.back().sendTimeMs;
							recvDeltaMs = arrivalTimeMs - this->probes.back().recvTimeMs;
						}

						MS_DEBUG_DEV(
						  "probe packet received [send time:%" PRId64
						  "ms, recv "
						  "time:%" PRId64 "ms, send delta:%dms, recv delta:%d ms]",
						  sendTimeMs,
						  arrivalTimeMs,
						  sendDeltaMs,
						  recvDeltaMs);
					}
#endif

					this->probes.emplace_back(sendTimeMs, arrivalTimeMs, payloadSize);
					++this->totalProbesReceived;

					// Make sure that a probe which updated the bitrate immediately has an
					// effect by calling the OnRembServerAvailableBitrate callback.
					if (ProcessClusters(nowMs) == ProbeResult::BITRATE_UPDATED)
						updateEstimate = true;
				}

				if (this->interArrival->ComputeDeltas(
				      timestamp, arrivalTimeMs, nowMs, payloadSize, &tsDelta, &tDelta, &sizeDelta))
				{
					double tsDeltaMs = (1000.0 * tsDelta) / (1 << InterArrivalShift);

					this->estimator->Update(tDelta, tsDeltaMs, sizeDelta, this->detector.State(), arrivalTimeMs);
					this->detector.Detect(
					  this->estimator->GetOffset(), tsDeltaMs, this->estimator->GetNumOfDeltas(), arrivalTimeMs);
				}

				if (!updateEstimate)
				{
					// Check if it's time for a periodic update or if we should update because
					// of an over-use.
					if (this->lastUpdateMs == -1 || nowMs - this->lastUpdateMs > this->remoteRate.GetFeedbackInterval())
					{
						updateEstimate = true;
					}
					else if (this->detector.State() == BW_OVERUSING)
					{
						uint32_t incomingRate = this->incomingBitrate.GetRate(arrivalTimeMs);

						if ((incomingRate != 0u) && this->remoteRate.TimeToReduceFurther(nowMs, incomingRate))
							updateEstimate = true;
					}
				}

				if (updateEstimate)
				{
					// The first overuse should immediately trigger a new estimate.
					// We also have to update the estimate immediately if we are overusing
					// and the target bitrate is too high compared to what we are receiving.
					const RateControlInput input(
					  this->detector.State(),
					  this->incomingBitrate.GetRate(arrivalTimeMs),
					  this->estimator->GetVarNoise());

					this->remoteRate.Update(&input, nowMs);
					targetBitrateBps = this->remoteRate.UpdateBandwidthEstimate(nowMs);
					updateEstimate   = this->remoteRate.ValidEstimate();
					ssrcs            = Keys(this->ssrcs);
				}
			}

			if (updateEstimate)
			{
				this->lastUpdateMs     = nowMs;
				this->availableBitrate = targetBitrateBps;

				this->observer->OnRembServerAvailableBitrate(this, ssrcs, targetBitrateBps);
			}
		}

		void RemoteBitrateEstimatorAbsSendTime::TimeoutStreams(int64_t nowMs)
		{
			MS_TRACE();

			for (auto it = this->ssrcs.begin(); it != this->ssrcs.end();)
			{
				if ((nowMs - it->second) > streamTimeOutMs)
					this->ssrcs.erase(it++);
				else
					++it;
			}

			if (this->ssrcs.empty())
			{
				// We can't update the estimate if we don't have any active streams.
				this->interArrival.reset(new InterArrival(
				  (TimestampGroupLengthMs << InterArrivalShift) / 1000, TimestampToMs, true));
				this->estimator.reset(new OveruseEstimator(OverUseDetectorOptions()));
				// We deliberately don't reset the this->firstPacketTimeMs here for now since
				// we only probe for bandwidth in the beginning of a call right now.
			}
		}

		bool RemoteBitrateEstimatorAbsSendTime::LatestEstimate(
		  std::vector<uint32_t>* ssrcs, uint32_t* bitrateBps) const
		{
			MS_TRACE();

			// MS_ASSERT(ssrcs);
			// MS_ASSERT(bitrateBps);

			if (!this->remoteRate.ValidEstimate())
				return false;

			*ssrcs = Keys(this->ssrcs);

			if (this->ssrcs.empty())
				*bitrateBps = 0;
			else
				*bitrateBps = this->remoteRate.LatestEstimate();

			return true;
		}
	} // namespace RembServer
} // namespace RTC
