#ifndef MS_RTC_BWE_REMOTE_BITRATE_ESTIMATOR_ABS_SEND_TIME_HPP
#define MS_RTC_BWE_REMOTE_BITRATE_ESTIMATOR_ABS_SEND_TIME_HPP

#include "common.hpp"
#include "RTC/BWE/AimdRateControl.hpp"
#include "RTC/BWE/InterArrival.hpp"
#include "RTC/BWE/OveruseDetector.hpp"
#include "RTC/BWE/OveruseEstimator.hpp"
#include "RTC/RTP/Packet.hpp"
#include "RTC/RateCalculator.hpp"
#include <list>
#include <map>
#include <vector>

namespace RTC
{
	namespace BWE
	{
		/**
		 * Estimates the bandwidth of the incoming link out of the `abs-send-time` RTP
		 * header extension, for senders that do not negotiate transport-cc.
		 *
		 * There is no feedback to work with here: all the information available is
		 * the instant each packet says it left the sender and the instant it arrived,
		 * so the estimation is the delay based detection over the whole group of
		 * incoming streams. It also detects the bursts a sender emits at the
		 * beginning of a call and takes their rate as a shortcut to an estimate,
		 * which is what makes the first seconds of a call usable.
		 *
		 * The result travels back to the sender as a REMB, so whoever owns this class
		 * gets both the estimate and the streams it applies to.
		 */
		class RemoteBitrateEstimatorAbsSendTime
		{
		public:
			class Listener
			{
			public:
				virtual ~Listener() = default;

			public:
				/**
				 * The estimation of the incoming link changed, so the remote senders of
				 * the given streams have to be told about it.
				 *
				 * @param ssrcs - Streams the estimation applies to, which are those that
				 *   have sent something recently.
				 * @param bitrate - Estimated bitrate of the incoming link (bps).
				 */
				virtual void OnRemoteBitrateEstimatorAbsSendTimeBitrateChanged(
				  RemoteBitrateEstimatorAbsSendTime* remoteBitrateEstimator,
				  const std::vector<uint32_t>& ssrcs,
				  int64_t bitrate) = 0;
			};

		private:
			/**
			 * A packet that may belong to a burst emitted by the sender.
			 */
			struct Probe
			{
				/**
				 * Instant the packet says it left the sender, in the sender's own
				 * timeline (us).
				 */
				int64_t sendTimeUs;
				/**
				 * Instant the packet arrived (us).
				 */
				int64_t receiveTimeUs;
				/**
				 * Size of the packet (bytes).
				 */
				size_t payloadSize;
			};

			/**
			 * A run of consecutive probes whose send deltas are close enough to each
			 * other to look like a single burst.
			 *
			 * @remarks
			 * - While a run is being accumulated the fields below hold sums rather than
			 *   means, and they are divided by `count` when the run is closed.
			 */
			struct Cluster
			{
				/**
				 * Rate at which the burst was sent (bps).
				 */
				int64_t GetSendBitrate() const
				{
					return (static_cast<int64_t>(this->meanSize) * 8 * 1000000) / this->sendMeanUs;
				}

				/**
				 * Rate at which the burst was received (bps).
				 */
				int64_t GetReceiveBitrate() const
				{
					return (static_cast<int64_t>(this->meanSize) * 8 * 1000000) / this->receiveMeanUs;
				}

				/**
				 * Mean time between the send instants of two consecutive probes (us).
				 */
				int64_t sendMeanUs{ 0 };
				/**
				 * Mean time between the arrival instants of two consecutive probes (us).
				 */
				int64_t receiveMeanUs{ 0 };
				/**
				 * Mean size of the probes (bytes).
				 */
				size_t meanSize{ 0 };
				/**
				 * Number of probes in the run.
				 */
				int count{ 0 };
				/**
				 * How many of those probes came far enough apart from the previous one
				 * for the gap to be measurable.
				 */
				int numAboveMinDelta{ 0 };
			};

			enum class ProbeResult : uint8_t
			{
				BITRATE_UPDATED,
				NO_UPDATE
			};

		public:
			explicit RemoteBitrateEstimatorAbsSendTime(Listener* listener);

			RemoteBitrateEstimatorAbsSendTime(const RemoteBitrateEstimatorAbsSendTime&) = delete;
			RemoteBitrateEstimatorAbsSendTime& operator=(const RemoteBitrateEstimatorAbsSendTime&) = delete;

			/**
			 * Feed a received RTP packet.
			 *
			 * @param packet - Received packet, which is ignored unless it carries the
			 *   `abs-send-time` extension.
			 * @param arrivalTimeUs - Instant the packet arrived.
			 * @param nowUs - Current instant, which is not the one above when packets
			 *   are processed in batches.
			 */
			void IncomingPacket(RTC::RTP::Packet* packet, int64_t arrivalTimeUs, int64_t nowUs);

			/**
			 * Feed the round trip time towards the senders, which bounds how often the
			 * estimation may be reduced.
			 */
			void OnRttUpdate(int64_t avgRttUs);

			/**
			 * Forget a stream, so that it no longer takes part in the estimation nor in
			 * the streams reported to the listener.
			 */
			void RemoveStream(uint32_t ssrc);

			/**
			 * Latest estimation of the incoming link (bps), or zero while there is
			 * none or no stream is active.
			 */
			int64_t GetLatestEstimate() const;

		private:
			/**
			 * Whether the given send delta is close enough to the mean of the run being
			 * accumulated to belong to it.
			 */
			static bool IsWithinClusterBounds(int64_t sendDeltaUs, const Cluster& clusterAggregate);

			/**
			 * Close the run being accumulated and keep it, unless it is too short or
			 * covers no time at all.
			 */
			static void MaybeAddCluster(const Cluster& clusterAggregate, std::list<Cluster>& clusters);

			/**
			 * Split the probes seen so far into runs of similar send deltas.
			 */
			std::list<Cluster> ComputeClusters() const;

			/**
			 * The fastest run that looks like a burst that made it through, or none if
			 * no run does.
			 */
			const Cluster* FindBestProbe(const std::list<Cluster>& clusters) const;

			/**
			 * Look at the probes seen so far and take the rate of the best burst as the
			 * estimation, if it beats what is already known.
			 */
			ProbeResult ProcessClusters(int64_t nowUs);

			/**
			 * Whether the given rate says something that the current estimation does
			 * not already say.
			 */
			bool IsBitrateImproving(int64_t probeBitrate) const;

			/**
			 * Forget the streams that have been silent for too long, and start the
			 * delay based detection over when none is left.
			 */
			void TimeoutStreams(int64_t nowUs);

		private:
			Listener* listener{ nullptr };
			// Recreated whenever every stream times out, since the groups of a stream
			// that is gone say nothing about the ones of whatever comes next.
			std::unique_ptr<InterArrival> interArrival;
			std::unique_ptr<OveruseEstimator> overuseEstimator;
			OveruseDetector overuseDetector;
			// Rate at which data is coming in, which is what bounds the estimation.
			RTC::RateCalculator incomingRateCalculator;
			// Whether the meter above has reported a rate at least once, so that a
			// stream falling silent can be told apart from never having measured
			// anything.
			bool incomingBitrateInitialized{ false };
			// Packets big enough to have been paced by the sender, kept while a burst
			// is still being looked for.
			std::list<Probe> probes;
			// Instant of the first packet ever fed, which bounds how long bursts are
			// looked for.
			std::optional<int64_t> firstPacketTimeUs;
			// Instant the listener was told about the estimation for the last time.
			std::optional<int64_t> lastUpdateUs;
			// Instant each stream was last seen at.
			std::map<uint32_t, int64_t> ssrcs;
			AimdRateControl remoteRateControl;
		};
	} // namespace BWE
} // namespace RTC

#endif
