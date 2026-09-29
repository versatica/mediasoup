#define MS_CLASS "RTC::BWE::RemoteBitrateEstimatorAbsSendTime"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/BWE/RemoteBitrateEstimatorAbsSendTime.hpp"
#include "Logger.hpp"
#include <cstdlib> // std::abs()

namespace RTC
{
	namespace BWE
	{
		/* Static. */

		// Shortest gap between two probes for it to be taken as measurable rather
		// than as two packets that left together.
		static constexpr int64_t MinClusterDeltaUs{ 1000 };
		// How long after the first packet bursts are still looked for even though
		// there already is an estimation.
		static constexpr int64_t InitialProbingIntervalUs{ 2 * 1000 * 1000 };
		// Every packet whose send timestamp is within this many milliseconds of the
		// first one of a group belongs to that group.
		static constexpr int64_t TimestampGroupLengthMs{ 5 };
		// The 'abs-send-time' extension carries 24 bits with 18 of fraction, and it
		// is shifted up so that it fills the 32 bits the inter arrival works with and
		// wraps the way they do.
		static constexpr uint32_t AbsSendTimeFraction{ 18 };
		static constexpr uint32_t AbsSendTimeInterArrivalUpshift{ 8 };
		static constexpr uint32_t InterArrivalShift{ AbsSendTimeFraction +
		                                             AbsSendTimeInterArrivalUpshift };
		// Microseconds a single tick of those shifted timestamps is worth.
		static constexpr double TimestampToUs{ 1000000.0 / static_cast<double>(1 << InterArrivalShift) };
		// Fewest probes a run must hold to be taken as a burst.
		static constexpr int MinClusterSize{ 4 };
		// Most probes kept while looking for a burst.
		static constexpr size_t MaxProbePackets{ 15 };
		// How many runs are enough to give up on the probes seen so far.
		static constexpr size_t ExpectedNumberOfProbes{ 3 };
		// Smallest packet taken as paced by the sender, since anything smaller is
		// likely audio, which is not.
		static constexpr size_t MinProbePacketSize{ 200 };
		// How long a stream may be silent before it stops counting.
		static constexpr int64_t StreamTimeOutUs{ 2 * 1000 * 1000 };
		// Window of the meter of incoming data.
		static constexpr int64_t IncomingBitrateWindowMs{ 1000 };
		// Widest gap between a send delta and the mean of the run it is compared
		// against for both to be taken as the same burst.
		static constexpr int64_t MaxClusterDeviationUs{ 2500 };
		// How much longer the arrival deltas of a burst may be than its send deltas,
		// and how much shorter, for the burst to say anything about the link.
		static constexpr int64_t MaxReceiveDeltaExcessUs{ 2000 };
		static constexpr int64_t MaxSendDeltaExcessUs{ 5000 };

		/* Instance methods. */

		RemoteBitrateEstimatorAbsSendTime::RemoteBitrateEstimatorAbsSendTime(Listener* listener)
		  : listener(listener), incomingRateCalculator(IncomingBitrateWindowMs)
		{
			MS_TRACE();
		}

		void RemoteBitrateEstimatorAbsSendTime::IncomingPacket(
		  const RTC::RTP::Packet* packet, int64_t arrivalTimeUs, int64_t nowUs)
		{
			MS_TRACE();

			uint32_t sendTime24bits{ 0 };

			if (!packet->ReadAbsSendTime(sendTime24bits))
			{
				MS_WARN_DEV("packet is missing the abs-send-time extension, ignoring it");

				return;
			}

			const size_t payloadSize = packet->GetPayloadLength() + packet->GetPaddingLength();

			// Shift up the send time to use the full 32 bits the inter arrival works
			// with, so that wrapping works properly.
			const uint32_t timestamp = sendTime24bits << AbsSendTimeInterArrivalUpshift;

			// NOTE: Truncated rather than rounded, which is not a slip. A tick of this
			// field is 3.8 us, so a burst whose packets left exactly a millisecond
			// apart gives deltas that land a hair under or over it. Truncating leaves
			// enough of them at or above the millisecond for the burst to be taken as
			// one, and rounding does not.
			const auto sendTimeUs = static_cast<int64_t>(static_cast<double>(timestamp) * TimestampToUs);

			// Tell whether the meter of incoming data still has something to say, and
			// start it over when it doesn't.
			const auto incomingBitrateValue = this->incomingRateCalculator.GetRate(arrivalTimeUs / 1000);

			if (incomingBitrateValue.has_value())
			{
				this->incomingBitrateInitialized = true;
			}
			else if (this->incomingBitrateInitialized)
			{
				// It had a valid value before, but there are no longer enough samples
				// within its window. Start it over so that the window only holds new
				// data.
				this->incomingRateCalculator.Reset();

				this->incomingBitrateInitialized = false;
			}

			this->incomingRateCalculator.Update(payloadSize, arrivalTimeUs / 1000);

			if (!this->firstPacketTimeUs.has_value())
			{
				this->firstPacketTimeUs = nowUs;
			}

			bool updateEstimate{ false };
			int64_t targetBitrate{ 0 };

			TimeoutStreams(nowUs);

			// NOTE: Guaranteed by the call right above, which creates both whenever no
			// stream is left. The very first call always finds none, since the only
			// place a stream is put into the map is below this point, and no path ever
			// gives either of them up afterwards.
			MS_ASSERT(this->interArrival, "no inter arrival");
			MS_ASSERT(this->overuseEstimator, "no overuse estimator");

			this->ssrcs.insert_or_assign(packet->GetSsrc(), nowUs);

			// For now only try to detect bursts while there is no valid estimation. It
			// is assumed that only packets larger than 200 bytes are paced by the
			// sender.
			if (
			  payloadSize > MinProbePacketSize &&
			  (!this->remoteRateControl.IsValidEstimate() ||
				 // NOTE: It was given a value right above if it had none, which the
			   // checker cannot tell.
				 // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
				 nowUs - this->firstPacketTimeUs.value() < InitialProbingIntervalUs))
			{
				this->probes.push_back(
				  Probe{
				    .sendTimeUs = sendTimeUs, .receiveTimeUs = arrivalTimeUs, .payloadSize = payloadSize });

				// Make sure that a burst which updated the estimation right away has an
				// effect, by telling the listener about it.
				if (ProcessClusters(nowUs) == ProbeResult::BITRATE_UPDATED)
				{
					updateEstimate = true;
				}
			}

			const auto deltas =
			  this->interArrival->ComputeDeltas(timestamp, arrivalTimeUs, nowUs, payloadSize);

			if (deltas.has_value())
			{
				const auto sendDeltaMs    = (1000.0 * static_cast<double>(deltas.value().timestampDelta)) /
				                            static_cast<double>(1 << InterArrivalShift);
				const auto arrivalDeltaMs = static_cast<double>(deltas.value().arrivalDeltaUs) / 1000.0;

				this->overuseEstimator->Update(
				  arrivalDeltaMs, sendDeltaMs, deltas.value().sizeDelta, this->overuseDetector.GetState());

				this->overuseDetector.Detect(
				  this->overuseEstimator->GetOffsetMs(),
				  sendDeltaMs,
				  this->overuseEstimator->GetNumOfDeltas(),
				  arrivalTimeUs);
			}

			if (!updateEstimate)
			{
				// Tell whether it's time for a periodic update, or whether there is an
				// overuse to react to right away.
				if (
				  !this->lastUpdateUs.has_value() ||
				  nowUs - this->lastUpdateUs.value() > this->remoteRateControl.GetFeedbackIntervalUs())
				{
					updateEstimate = true;
				}
				else if (this->overuseDetector.GetState() == Types::BandwidthUsage::OVERUSING)
				{
					const auto incomingRate = this->incomingRateCalculator.GetRate(arrivalTimeUs / 1000);

					if (
					  incomingRate.has_value() &&
					  this->remoteRateControl.IsTimeToReduceFurther(nowUs, incomingRate.value()))
					{
						updateEstimate = true;
					}
				}
			}

			if (updateEstimate)
			{
				// The first overuse triggers a new estimation right away, and so does
				// being in overuse with an estimation far above what is coming in.
				const Types::RateControlInput input{
					.bandwidthUsage      = this->overuseDetector.GetState(),
					.estimatedThroughput = this->incomingRateCalculator.GetRate(arrivalTimeUs / 1000)
				};

				targetBitrate  = this->remoteRateControl.Update(input, nowUs);
				updateEstimate = this->remoteRateControl.IsValidEstimate();
			}

			if (updateEstimate)
			{
				this->lastUpdateUs = nowUs;

				std::vector<uint32_t> ssrcs;

				ssrcs.reserve(this->ssrcs.size());

				for (const auto& kv : this->ssrcs)
				{
					const uint32_t ssrc = kv.first;

					ssrcs.push_back(ssrc);
				}

				this->listener->OnRemoteBitrateEstimatorAbsSendTimeBitrateChanged(this, ssrcs, targetBitrate);
			}
		}

		void RemoteBitrateEstimatorAbsSendTime::OnRttUpdate(int64_t avgRttUs)
		{
			MS_TRACE();

			this->remoteRateControl.SetRttUs(avgRttUs);
		}

		void RemoteBitrateEstimatorAbsSendTime::RemoveStream(uint32_t ssrc)
		{
			MS_TRACE();

			this->ssrcs.erase(ssrc);
		}

		int64_t RemoteBitrateEstimatorAbsSendTime::GetLatestEstimate() const
		{
			MS_TRACE();

			if (!this->remoteRateControl.IsValidEstimate() || this->ssrcs.empty())
			{
				return 0;
			}

			return this->remoteRateControl.GetLatestEstimate();
		}

		bool RemoteBitrateEstimatorAbsSendTime::IsWithinClusterBounds(
		  int64_t sendDeltaUs, const Cluster& clusterAggregate)
		{
			MS_TRACE();

			if (clusterAggregate.count == 0)
			{
				return true;
			}

			const int64_t clusterMeanUs = clusterAggregate.sendMeanUs / clusterAggregate.count;

			return std::abs(sendDeltaUs - clusterMeanUs) < MaxClusterDeviationUs;
		}

		void RemoteBitrateEstimatorAbsSendTime::MaybeAddCluster(
		  const Cluster& clusterAggregate, std::list<Cluster>& clusters)
		{
			MS_TRACE();

			if (
			  clusterAggregate.count < MinClusterSize || clusterAggregate.sendMeanUs <= 0 ||
			  clusterAggregate.receiveMeanUs <= 0)
			{
				return;
			}

			clusters.push_back(
			  Cluster{ .sendMeanUs       = clusterAggregate.sendMeanUs / clusterAggregate.count,
				         .receiveMeanUs    = clusterAggregate.receiveMeanUs / clusterAggregate.count,
				         .meanSize         = clusterAggregate.meanSize / clusterAggregate.count,
				         .count            = clusterAggregate.count,
				         .numAboveMinDelta = clusterAggregate.numAboveMinDelta });
		}

		std::list<RemoteBitrateEstimatorAbsSendTime::Cluster> RemoteBitrateEstimatorAbsSendTime::ComputeClusters() const
		{
			MS_TRACE();

			std::list<Cluster> clusters;
			Cluster clusterAggregate;
			// NOTE: The two instants a delta is taken against always come from the
			// very same probe, so they are held as one rather than as two values that
			// could tell a different story from each other.
			const Probe* prevProbe{ nullptr };

			for (const auto& probe : this->probes)
			{
				if (prevProbe)
				{
					const int64_t sendDeltaUs    = probe.sendTimeUs - prevProbe->sendTimeUs;
					const int64_t receiveDeltaUs = probe.receiveTimeUs - prevProbe->receiveTimeUs;

					if (sendDeltaUs >= MinClusterDeltaUs && receiveDeltaUs >= MinClusterDeltaUs)
					{
						clusterAggregate.numAboveMinDelta++;
					}

					if (!IsWithinClusterBounds(sendDeltaUs, clusterAggregate))
					{
						MaybeAddCluster(clusterAggregate, clusters);

						clusterAggregate = Cluster();
					}

					clusterAggregate.sendMeanUs += sendDeltaUs;
					clusterAggregate.receiveMeanUs += receiveDeltaUs;
					clusterAggregate.meanSize += probe.payloadSize;
					clusterAggregate.count++;
				}

				prevProbe = std::addressof(probe);
			}

			MaybeAddCluster(clusterAggregate, clusters);

			return clusters;
		}

		const RemoteBitrateEstimatorAbsSendTime::Cluster* RemoteBitrateEstimatorAbsSendTime::FindBestProbe(
		  const std::list<Cluster>& clusters) const
		{
			MS_TRACE();

			int64_t highestProbeBitrate{ 0 };
			const Cluster* best{ nullptr };

			for (const auto& cluster : clusters)
			{
				// NOTE: This is not redundant with the check MaybeAddCluster() does.
				// That one requires the sums to be positive, but what is stored is
				// their integer division by the number of probes, and a small enough
				// sum over four probes or more gives a mean of zero. This is what keeps
				// the rates below from dividing by it.
				if (cluster.sendMeanUs == 0 || cluster.receiveMeanUs == 0)
				{
					continue;
				}

				if (
				  cluster.numAboveMinDelta > cluster.count / 2 &&
				  (cluster.receiveMeanUs - cluster.sendMeanUs <= MaxReceiveDeltaExcessUs &&
					 cluster.sendMeanUs - cluster.receiveMeanUs <= MaxSendDeltaExcessUs))
				{
					const int64_t probeBitrate =
					  std::min(cluster.GetSendBitrate(), cluster.GetReceiveBitrate());

					if (probeBitrate > highestProbeBitrate)
					{
						highestProbeBitrate = probeBitrate;
						best                = std::addressof(cluster);
					}
				}
				else
				{
					MS_DEBUG_DEV(
					  "burst failed [sendBitrate:%" PRIi64 " bps, receiveBitrate:%" PRIi64
					  " bps, sendMean:%" PRIi64 " us, receiveMean:%" PRIi64 " us, numProbes:%d]",
					  cluster.GetSendBitrate(),
					  cluster.GetReceiveBitrate(),
					  cluster.sendMeanUs,
					  cluster.receiveMeanUs,
					  cluster.count);

					break;
				}
			}

			return best;
		}

		RemoteBitrateEstimatorAbsSendTime::ProbeResult RemoteBitrateEstimatorAbsSendTime::ProcessClusters(
		  int64_t nowUs)
		{
			MS_TRACE();

			const std::list<Cluster> clusters = ComputeClusters();

			if (clusters.empty())
			{
				// The most probes that are kept have been reached and still no run came
				// out of them, so the oldest one is dropped.
				if (this->probes.size() >= MaxProbePackets)
				{
					this->probes.pop_front();
				}

				return ProbeResult::NO_UPDATE;
			}

			const auto* best = FindBestProbe(clusters);

			if (best)
			{
				const int64_t probeBitrate = std::min(best->GetSendBitrate(), best->GetReceiveBitrate());

				// Make sure that a burst sent at a lower rate than the estimation cannot
				// bring it down.
				if (IsBitrateImproving(probeBitrate))
				{
					MS_DEBUG_DEV(
					  "burst successful [sendBitrate:%" PRIi64 " bps, receiveBitrate:%" PRIi64
					  " bps, sendMean:%" PRIi64 " us, receiveMean:%" PRIi64 " us, numProbes:%d]",
					  best->GetSendBitrate(),
					  best->GetReceiveBitrate(),
					  best->sendMeanUs,
					  best->receiveMeanUs,
					  best->count);

					this->remoteRateControl.SetEstimate(probeBitrate, nowUs);

					return ProbeResult::BITRATE_UPDATED;
				}
			}

			// Either not bursting and a regular packet came in, or done with the
			// current set of probes.
			if (clusters.size() >= ExpectedNumberOfProbes)
			{
				this->probes.clear();
			}

			return ProbeResult::NO_UPDATE;
		}

		bool RemoteBitrateEstimatorAbsSendTime::IsBitrateImproving(int64_t probeBitrate) const
		{
			MS_TRACE();

			const bool initialProbe = !this->remoteRateControl.IsValidEstimate() && probeBitrate > 0;
			const bool bitrateAboveEstimate = this->remoteRateControl.IsValidEstimate() &&
			                                  probeBitrate > this->remoteRateControl.GetLatestEstimate();

			return initialProbe || bitrateAboveEstimate;
		}

		void RemoteBitrateEstimatorAbsSendTime::TimeoutStreams(int64_t nowUs)
		{
			MS_TRACE();

			for (auto it = this->ssrcs.begin(); it != this->ssrcs.end();)
			{
				const int64_t lastSeenUs = it->second;

				if (nowUs - lastSeenUs > StreamTimeOutUs)
				{
					it = this->ssrcs.erase(it);
				}
				else
				{
					++it;
				}
			}

			if (this->ssrcs.empty())
			{
				// The estimation cannot be updated without an active stream, so the
				// delay based detection starts over.
				//
				// NOTE: The instant of the first packet is deliberately not reset, since
				// bursts are only looked for at the beginning of a call.
				this->interArrival = std::make_unique<InterArrival>(
				  static_cast<uint32_t>((TimestampGroupLengthMs << InterArrivalShift) / 1000), TimestampToUs);
				this->overuseEstimator = std::make_unique<OveruseEstimator>();
			}
		}
	} // namespace BWE
} // namespace RTC
