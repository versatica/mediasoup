#define MS_CLASS "RTC::BWE::ProbeBitrateEstimator"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/BWE/ProbeBitrateEstimator.hpp"
#include "Logger.hpp"
#include "RTC/BWE/BitrateUtils.hpp"
#include <cmath> // std::llround()

namespace RTC
{
	namespace BWE
	{
		/* Static. */

		// Fraction of the packets of a burst that must come back for it to be
		// measurable at all.
		static constexpr double MinReceivedProbesRatio{ 0.80 };
		// The same for its bytes.
		static constexpr double MinReceivedBytesRatio{ 0.80 };
		// Highest ratio of received over sent rate that still describes a link. Above
		// it the burst arrived faster than it left, which measures the sender.
		static constexpr double MaxValidRatio{ 2.0 };
		// Ratio below which the burst is taken to have found the real capacity, since
		// it arrived clearly slower than it was sent.
		static constexpr double MinRatioForUnsaturatedLink{ 0.9 };
		// Fraction of the capacity found that is aimed for, so that acting on the
		// probe doesn't overuse the link right away.
		static constexpr double TargetUtilizationFraction{ 0.95 };
		// How long a burst is kept around waiting for the rest of its packets.
		static constexpr int64_t MaxClusterHistoryUs{ 1000 * 1000 };
		// Longest span of send or receive times a burst may cover. A longer one is
		// not a burst anymore.
		static constexpr int64_t MaxProbeIntervalUs{ 1000 * 1000 };

		/* Instance methods. */

		std::optional<int64_t> ProbeBitrateEstimator::HandleProbeAndEstimateBitrate(
		  const Types::PacketResult& packetResult)
		{
			MS_TRACE();

			// NOTE: Safe since only the packets that belong to a probe and were
			// received reach this method, as the caller tells them apart and iterates
			// over the ones that arrived.
			MS_ASSERT(packetResult.sentPacket.probeCluster.has_value(), "packet is not a probe");
			MS_ASSERT(packetResult.IsReceived(), "packet was not received");

			const auto& probeCluster = packetResult.sentPacket.probeCluster.value();
			// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
			const int64_t receiveTimeUs = packetResult.receiveTimeUs.value();
			const int64_t sendTimeUs    = packetResult.sentPacket.sendTimeUs;
			const auto size             = static_cast<int64_t>(packetResult.sentPacket.size);

			EraseOldClusters(receiveTimeUs);

			auto& cluster = this->clusters[probeCluster.id];

			if (!cluster.firstSendTimeUs.has_value() || sendTimeUs < cluster.firstSendTimeUs.value())
			{
				cluster.firstSendTimeUs = sendTimeUs;
			}

			if (!cluster.lastSendTimeUs.has_value() || sendTimeUs > cluster.lastSendTimeUs.value())
			{
				cluster.lastSendTimeUs = sendTimeUs;
				cluster.sizeLastSend   = size;
			}

			if (!cluster.firstReceiveTimeUs.has_value() || receiveTimeUs < cluster.firstReceiveTimeUs.value())
			{
				cluster.firstReceiveTimeUs = receiveTimeUs;
				cluster.sizeFirstReceive   = size;
			}

			if (!cluster.lastReceiveTimeUs.has_value() || receiveTimeUs > cluster.lastReceiveTimeUs.value())
			{
				cluster.lastReceiveTimeUs = receiveTimeUs;
			}

			cluster.sizeTotal += size;
			cluster.numProbes += 1;

			// NOTE: Both come from the cluster this side asked for, so a burst that
			// expects nothing is a programming error.
			MS_ASSERT(probeCluster.minProbes > 0, "probe cluster expects no packets");
			MS_ASSERT(probeCluster.minBytes > 0, "probe cluster expects no bytes");

			// NOTE: The count of packets is truncated and the count of bytes is
			// rounded, which is not a slip: a size is a magnitude that gets rounded
			// wherever it is derived, while a number of packets is just an integer
			// scaled down.
			const auto minProbes =
			  static_cast<int64_t>(static_cast<double>(probeCluster.minProbes) * MinReceivedProbesRatio);
			const auto minSize =
			  std::llround(static_cast<double>(probeCluster.minBytes) * MinReceivedBytesRatio);

			// Too little of the burst has come back to tell anything yet.
			if (cluster.numProbes < minProbes || cluster.sizeTotal < minSize)
			{
				return std::nullopt;
			}

			// NOTE: The four of them were just given a value above, since every packet
			// that gets here is both sent and received.
			// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
			const int64_t sendIntervalUs = cluster.lastSendTimeUs.value() - cluster.firstSendTimeUs.value();
			// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
			const int64_t receiveIntervalUs =
			  cluster.lastReceiveTimeUs.value() - cluster.firstReceiveTimeUs.value();

			// A burst that covers no span at all, or one so long that it isn't a burst
			// anymore, measures nothing.
			if (sendIntervalUs <= 0 || sendIntervalUs > MaxProbeIntervalUs || receiveIntervalUs <= 0 || receiveIntervalUs > MaxProbeIntervalUs)
			{
				MS_DEBUG_DEV(
				  "probe unsuccessful, invalid send or receive interval [clusterId:%" PRIi64
				  ", sendIntervalUs:%" PRIi64 ", receiveIntervalUs:%" PRIi64 "]",
				  probeCluster.id,
				  sendIntervalUs,
				  receiveIntervalUs);

				return std::nullopt;
			}

			// NOTE: A span of send times covering more than one instant means more than
			// one packet, and no packet is empty, so the totals are strictly above the
			// sizes left out below.
			MS_ASSERT(cluster.sizeTotal > cluster.sizeLastSend, "total size doesn't exceed the last sent");
			MS_ASSERT(
			  cluster.sizeTotal > cluster.sizeFirstReceive, "total size doesn't exceed the first received");

			// The span of send times doesn't cover the time it took to send the last
			// packet, so its size is left out of the rate it was sent at. Same for the
			// first one received.
			//
			// NOTE: Bytes over microseconds into bits over seconds, in integer
			// arithmetic. A burst is a handful of packets, so the product stays far
			// from what an int64_t holds.
			const int64_t sendBitrate =
			  ((cluster.sizeTotal - cluster.sizeLastSend) * 8 * 1000000) / sendIntervalUs;
			const int64_t receiveBitrate =
			  ((cluster.sizeTotal - cluster.sizeFirstReceive) * 8 * 1000000) / receiveIntervalUs;

			// NOTE: The span above is at most a second and the size at least one byte,
			// so the rate it was sent at is never zero.
			const double ratio = static_cast<double>(receiveBitrate) / static_cast<double>(sendBitrate);

			if (ratio > MaxValidRatio)
			{
				MS_DEBUG_DEV(
				  "probe unsuccessful, received over sent ratio too high [clusterId:%" PRIi64
				  ", sendBitrate:%" PRIi64 ", receiveBitrate:%" PRIi64 ", ratio:%f]",
				  probeCluster.id,
				  sendBitrate,
				  receiveBitrate,
				  ratio);

				return std::nullopt;
			}

			int64_t bitrate = std::min(sendBitrate, receiveBitrate);

			// Arriving clearly slower than it was sent means the burst found the real
			// capacity of the link, so aim slightly below it.
			if (receiveBitrate < BitrateUtils::ApplyBitrateFactor(sendBitrate, MinRatioForUnsaturatedLink))
			{
				// NOTE: Nine tenths of a rate never come out above the rate itself, so
				// being below that fraction means being below the rate.
				MS_ASSERT(
				  sendBitrate > receiveBitrate,
				  "burst received faster than it was sent [sendBitrate:%" PRIi64 ", receiveBitrate:%" PRIi64
				  "]",
				  sendBitrate,
				  receiveBitrate);

				bitrate = BitrateUtils::ApplyBitrateFactor(receiveBitrate, TargetUtilizationFraction);
			}

			MS_DEBUG_DEV(
			  "probe successful [clusterId:%" PRIi64 ", sendBitrate:%" PRIi64 ", receiveBitrate:%" PRIi64
			  ", bitrate:%" PRIi64 "]",
			  probeCluster.id,
			  sendBitrate,
			  receiveBitrate,
			  bitrate);

			this->estimatedBitrate = bitrate;

			return this->estimatedBitrate;
		}

		std::optional<int64_t> ProbeBitrateEstimator::FetchAndResetLastEstimatedBitrate()
		{
			MS_TRACE();

			const std::optional<int64_t> estimatedBitrate = this->estimatedBitrate;

			this->estimatedBitrate.reset();

			return estimatedBitrate;
		}

		void ProbeBitrateEstimator::EraseOldClusters(int64_t nowUs)
		{
			MS_TRACE();

			for (auto it = this->clusters.begin(); it != this->clusters.end();)
			{
				const auto& cluster = it->second;

				if (cluster.lastReceiveTimeUs.has_value() && cluster.lastReceiveTimeUs.value() + MaxClusterHistoryUs < nowUs)
				{
					it = this->clusters.erase(it);
				}
				else
				{
					++it;
				}
			}
		}
	} // namespace BWE
} // namespace RTC
