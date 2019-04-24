/*
 *  Copyright (c) 2015 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef MS_RTC_REMB_SERVER_REMOTE_BITRATE_ESTIMATOR_ABS_SEND_TIME_HPP
#define MS_RTC_REMB_SERVER_REMOTE_BITRATE_ESTIMATOR_ABS_SEND_TIME_HPP

#include "common.hpp"
#include "RTC/RembServer/AimdRateControl.hpp"
#include "RTC/RembServer/InterArrival.hpp"
#include "RTC/RembServer/OveruseDetector.hpp"
#include "RTC/RembServer/OveruseEstimator.hpp"
#include "RTC/RembServer/RemoteBitrateEstimator.hpp"
#include "RTC/RtpDataCounter.hpp"
#include <cassert>
#include <list>
#include <map>
#include <memory>
#include <vector>

namespace RTC
{
	namespace RembServer
	{
		struct Probe
		{
			Probe(int64_t sendTimeMs, int64_t recvTimeMs, size_t payloadSize);

			int64_t sendTimeMs{ 0 };
			int64_t recvTimeMs{ 0 };
			size_t payloadSize{ 0 };
		};

		struct Cluster
		{
			int GetSendBitrateBps() const;
			int GetRecvBitrateBps() const;

			float sendMeanMs{ 0.0f };
			float recvMeanMs{ 0.0f };
			// TODO(holmer): Add some variance metric as well?
			size_t meanSize{ 0 };
			int count{ 0 };
			int numAboveMinDelta{ 0 };
		};

		class RemoteBitrateEstimatorAbsSendTime : public RemoteBitrateEstimator
		{
		private:
			static bool IsWithinClusterBounds(int sendDeltaMs, const Cluster& clusterAggregate);
			static void AddCluster(std::list<Cluster>* clusters, Cluster* cluster);

		public:
			explicit RemoteBitrateEstimatorAbsSendTime(Listener* listener);
			~RemoteBitrateEstimatorAbsSendTime() override = default;

			void IncomingPacket(
			  int64_t arrivalTimeMs, size_t payloadSize, const RtpPacket& packet, uint32_t absSendTime) override;
			// This class relies on Process() being called periodically (at least once
			// every other second) for streams to be timed out properly.
			void Process() override;
			void OnRttUpdate(int64_t avgRttMs, int64_t maxRttMs) override;
			void RemoveStream(uint32_t ssrc) override;
			bool LatestEstimate(std::vector<uint32_t>* ssrcs, uint32_t* bitrateBps) const override;
			void SetMinBitrate(int minBitrateBps) override;

		private:
			using Ssrcs = std::map<uint32_t, int64_t>;

			enum class ProbeResult
			{
				BITRATE_UPDATED,
				NO_UPDATE
			};

		private:
			void IncomingPacketInfo(
			  int64_t arrivalTimeMs, uint32_t sendTime24bits, size_t payloadSize, uint32_t ssrc);

			void ComputeClusters(std::list<Cluster>* clusters) const;
			std::list<Cluster>::const_iterator FindBestProbe(const std::list<Cluster>& clusters) const;
			// Returns true if a probe which changed the estimate was detected.
			ProbeResult ProcessClusters(int64_t nowMs);
			bool IsBitrateImproving(int newBitrateBps) const;
			void TimeoutStreams(int64_t nowMs);

		private:
			Listener* listener{ nullptr };
			std::unique_ptr<InterArrival> interArrival;
			std::unique_ptr<OveruseEstimator> estimator;
			OveruseDetector detector;
			RTC::RateCalculator incomingBitrate;
			bool incomingBitrateInitialized{ false };
			std::vector<int> recentPropagationDeltaMs;
			std::vector<int64_t> recentUpdateTimeMs;
			std::list<Probe> probes;
			size_t totalProbesReceived{ 0 };
			int64_t firstPacketTimeMs{ -1 };
			int64_t lastUpdateMs{ -1 };
			bool umaRecorded{ false };
			Ssrcs ssrcs;
			AimdRateControl remoteRate;
		};

		/* Inline Methods. */

		inline Probe::Probe(int64_t sendTimeMs, int64_t recvTimeMs, size_t payloadSize)
		  : sendTimeMs(sendTimeMs), recvTimeMs(recvTimeMs), payloadSize(payloadSize)
		{
		}

		inline int Cluster::GetSendBitrateBps() const
		{
			assert(sendMeanMs > 0.0f);

			return meanSize * 8 * 1000 / sendMeanMs;
		}

		inline int Cluster::GetRecvBitrateBps() const
		{
			assert(recvMeanMs > 0.0f);

			return meanSize * 8 * 1000 / recvMeanMs;
		}

		inline RemoteBitrateEstimatorAbsSendTime::RemoteBitrateEstimatorAbsSendTime(Listener* listener)
		  : listener(listener), interArrival(), estimator(), detector(), incomingBitrate()
		{
		}

		inline void RemoteBitrateEstimatorAbsSendTime::Process()
		{
		}

		inline void RemoteBitrateEstimatorAbsSendTime::OnRttUpdate(int64_t avgRttMs, int64_t /*maxRttMs*/)
		{
			this->remoteRate.SetRtt(avgRttMs);
		}

		inline void RemoteBitrateEstimatorAbsSendTime::RemoveStream(uint32_t ssrc)
		{
			this->ssrcs.erase(ssrc);
		}

		inline void RemoteBitrateEstimatorAbsSendTime::SetMinBitrate(int minBitrateBps)
		{
			this->remoteRate.SetMinBitrate(minBitrateBps);
		}
	} // namespace RembServer
} // namespace RTC

#endif
