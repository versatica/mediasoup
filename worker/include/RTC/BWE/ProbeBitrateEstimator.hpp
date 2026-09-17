#ifndef MS_RTC_BWE_PROBE_BITRATE_ESTIMATOR_HPP
#define MS_RTC_BWE_PROBE_BITRATE_ESTIMATOR_HPP

#include "common.hpp"
#include "RTC/BWE/BweTypes.hpp"
#include <ankerl/unordered_dense.h>

namespace RTC
{
	namespace BWE
	{
		/**
		 * Measures what a probe actually achieved.
		 *
		 * A probe is a burst of packets sent on purpose above the current estimate,
		 * and the point of it is to learn the capacity of the link directly instead
		 * of inferring it from the delay. This class gathers the feedback of the
		 * packets of each burst and, once enough of the burst has come back, says at
		 * what bitrate it was delivered.
		 *
		 * The answer is the lower of the rate it was sent at and the rate it
		 * arrived at, because a burst that arrives faster than it was sent measures
		 * the sender and not the link. And when it arrives clearly slower than it
		 * was sent, that means the burst found the real capacity, so the answer is
		 * taken slightly below what arrived in order not to overuse it right away.
		 *
		 * A burst that too little of came back, that was sent or received over too
		 * long a span, or that arrived far faster than it was sent, is discarded
		 * rather than believed.
		 */
		class ProbeBitrateEstimator
		{
		private:
			/**
			 * What has been gathered so far about a burst.
			 */
			struct AggregatedCluster
			{
				int64_t numProbes{ 0 };
				std::optional<int64_t> firstSendTimeUs;
				std::optional<int64_t> lastSendTimeUs;
				std::optional<int64_t> firstReceiveTimeUs;
				std::optional<int64_t> lastReceiveTimeUs;
				// Size of the packet sent last, which the span of send times doesn't
				// cover.
				int64_t sizeLastSend{ 0 };
				// Size of the packet received first, which the span of receive times
				// doesn't cover.
				int64_t sizeFirstReceive{ 0 };
				int64_t sizeTotal{ 0 };
			};

		public:
			/**
			 * Feed the feedback of a packet that belongs to a probe.
			 *
			 * @returns The bitrate the probe achieved (bps), or no value while its
			 * burst cannot be trusted yet or at all.
			 */
			std::optional<int64_t> HandleProbeAndEstimateBitrate(const Types::PacketResult& packetResult);

			/**
			 * Take the latest measured bitrate (bps), leaving none behind, so that a
			 * probe is never acted upon twice.
			 */
			std::optional<int64_t> FetchAndResetLastEstimatedBitrate();

		private:
			/**
			 * Forget the bursts whose last packet arrived long enough ago that they
			 * cannot receive anything else.
			 */
			void EraseOldClusters(int64_t nowUs);

		private:
			// Bursts being gathered, by their id. Nothing looks at them in any
			// particular order.
			ankerl::unordered_dense::map<int64_t, AggregatedCluster> clusters;
			std::optional<int64_t> estimatedBitrate;
		};
	} // namespace BWE
} // namespace RTC

#endif
