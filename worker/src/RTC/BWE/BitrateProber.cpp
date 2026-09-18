#define MS_CLASS "RTC::BWE::BitrateProber"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/BWE/BitrateProber.hpp"
#include "Logger.hpp"
#include "RTC/BWE/Utils.hpp"

namespace RTC
{
	namespace BWE
	{
		/* Static. */

		// How long a burst that was asked for but never started is worth keeping.
		static constexpr int64_t ProbeClusterTimeoutUs{ 5 * 1000 * 1000 };
		// How many bursts may be waiting at once.
		static constexpr size_t MaxPendingProbeClusters{ 5 };

		/* Instance methods. */

		BitrateProber::BitrateProber() : BitrateProber(BitrateProberOptions{})
		{
			MS_TRACE();
		}

		BitrateProber::BitrateProber(BitrateProberOptions options) : options(options)
		{
			MS_TRACE();

			SetEnabled(true);
		}

		void BitrateProber::SetEnabled(bool enabled)
		{
			MS_TRACE();

			if (enabled)
			{
				if (this->state == State::DISABLED)
				{
					this->state = State::INACTIVE;

					MS_DEBUG_DEV("probing enabled");
				}
			}
			else
			{
				this->state = State::DISABLED;

				MS_DEBUG_DEV("probing disabled");
			}
		}

		void BitrateProber::SetAllowProbeWithoutMediaPacket(bool allow)
		{
			MS_TRACE();

			this->options.allowStartProbingImmediately = allow;

			MaybeSetActiveState(/*packetSize*/ 0);
		}

		void BitrateProber::OnIncomingPacket(size_t packetSize)
		{
			MS_TRACE();

			MaybeSetActiveState(packetSize);
		}

		void BitrateProber::CreateProbeCluster(const Types::ProbeClusterConfig& clusterConfig)
		{
			MS_TRACE();

			MS_ASSERT(this->state != State::DISABLED, "probing is disabled");
			MS_ASSERT(clusterConfig.minProbeDeltaUs > 0, "no time between packets of the burst");

			// Whatever was asked for long ago is not worth sending anymore, and only so
			// many may be waiting at once.
			while (!this->clusters.empty() &&
			       (clusterConfig.atUs - this->clusters.front().requestedAtUs > ProbeClusterTimeoutUs ||
			        this->clusters.size() > MaxPendingProbeClusters))
			{
				this->clusters.pop();
			}

			Cluster cluster;

			cluster.probeCluster.id        = clusterConfig.id;
			cluster.probeCluster.minProbes = clusterConfig.targetProbeCount;
			// The bytes the burst is meant to carry are its bitrate held for as long as
			// it is meant to last.
			cluster.probeCluster.minBytes = Utils::ApplyBitrateFactor(
			  clusterConfig.targetBitrate,
			  static_cast<double>(clusterConfig.targetDurationUs) / (8 * 1000000));
			cluster.sendBitrate     = clusterConfig.targetBitrate;
			cluster.minProbeDeltaUs = clusterConfig.minProbeDeltaUs;
			cluster.requestedAtUs   = clusterConfig.atUs;

			MS_ASSERT(cluster.probeCluster.minBytes >= 0, "burst of a negative count of bytes");

			this->clusters.push(cluster);

			MaybeSetActiveState(/*packetSize*/ 0);

			MS_DEBUG_DEV(
			  "probe cluster created [id:%" PRIi64 ", bitrate:%" PRIi64 ", minBytes:%" PRIi64
			  ", minProbes:%" PRIi64 ", active:%s]",
			  cluster.probeCluster.id,
			  cluster.sendBitrate,
			  cluster.probeCluster.minBytes,
			  cluster.probeCluster.minProbes,
			  this->state == State::ACTIVE ? "true" : "false");
		}

		std::optional<int64_t> BitrateProber::GetNextProbeTimeUs(int64_t nowUs) const
		{
			MS_TRACE();

			// Nothing is being emitted, or everything asked for is already done.
			if (this->state != State::ACTIVE || this->clusters.empty())
			{
				return std::nullopt;
			}

			// A burst that hasn't started goes out at the first chance there is.
			return this->nextProbeTimeUs.value_or(nowUs);
		}

		std::optional<Types::ProbeCluster> BitrateProber::GetCurrentCluster(int64_t nowUs)
		{
			MS_TRACE();

			if (this->clusters.empty() || this->state != State::ACTIVE)
			{
				return std::nullopt;
			}

			// A packet that goes out long after it was due no longer measures the
			// bitrate the burst was meant to be sent at, so the whole burst is dropped
			// rather than believed.
			if (this->nextProbeTimeUs.has_value() && nowUs - this->nextProbeTimeUs.value() > this->options.maxProbeDelayUs)
			{
				MS_DEBUG_DEV(
				  "probe delayed too much, discarding the cluster [dueAt:%" PRIi64 ", now:%" PRIi64 "]",
				  this->nextProbeTimeUs.value(),
				  nowUs);

				this->clusters.pop();

				if (this->clusters.empty())
				{
					this->state = State::INACTIVE;

					return std::nullopt;
				}
			}

			return this->clusters.front().probeCluster;
		}

		size_t BitrateProber::GetRecommendedMinProbeSize() const
		{
			MS_TRACE();

			if (this->clusters.empty())
			{
				return 0;
			}

			const auto& cluster = this->clusters.front();

			// What the burst's bitrate carries over the time between two of its shots.
			return static_cast<size_t>(Utils::ApplyBitrateFactor(
			  cluster.sendBitrate, static_cast<double>(cluster.minProbeDeltaUs) / (8 * 1000000)));
		}

		void BitrateProber::ProbeSent(int64_t nowUs, size_t size)
		{
			MS_TRACE();

			MS_ASSERT(this->state == State::ACTIVE, "no burst is being emitted");
			MS_ASSERT(size != 0, "a packet of no bytes was sent");

			if (this->clusters.empty())
			{
				return;
			}

			auto& cluster = this->clusters.front();

			if (cluster.sentProbes == 0)
			{
				MS_ASSERT(!cluster.startedAtUs.has_value(), "burst already started");

				cluster.startedAtUs = nowUs;
			}

			cluster.sentBytes += static_cast<int64_t>(size);
			cluster.sentProbes += 1;

			this->nextProbeTimeUs = CalculateNextProbeTimeUs(cluster);

			// Both counts have to be reached, since a burst of too few packets says
			// nothing however many bytes it carried, and the other way round.
			if (
			  cluster.sentBytes >= cluster.probeCluster.minBytes &&
			  cluster.sentProbes >= cluster.probeCluster.minProbes)
			{
				this->clusters.pop();
			}

			if (this->clusters.empty())
			{
				this->state = State::INACTIVE;
			}
		}

		void BitrateProber::MaybeSetActiveState(size_t packetSize)
		{
			MS_TRACE();

			if (IsReadyToSetActiveState(packetSize))
			{
				this->nextProbeTimeUs.reset();
				this->state = State::ACTIVE;
			}
		}

		bool BitrateProber::IsReadyToSetActiveState(size_t packetSize) const
		{
			MS_TRACE();

			if (this->clusters.empty())
			{
				MS_ASSERT(
				  this->state == State::DISABLED || this->state == State::INACTIVE,
				  "emitting a burst with none to emit");

				return false;
			}

			switch (this->state)
			{
				case State::DISABLED:
				case State::ACTIVE:
				{
					return false;
				}

				case State::INACTIVE:
				{
					if (this->options.allowStartProbingImmediately)
					{
						return true;
					}

					// A burst that begins on a tiny packet starts measuring from an instant
					// that says nothing, so a big enough one has to go out first. What
					// counts as big enough is never more than a whole shot of the burst.
					return packetSize >= std::min(GetRecommendedMinProbeSize(), this->options.minPacketSize);
				}
			}

			return false;
		}

		int64_t BitrateProber::CalculateNextProbeTimeUs(const Cluster& cluster) const
		{
			MS_TRACE();

			MS_ASSERT(cluster.sendBitrate > 0, "burst at no bitrate");
			MS_ASSERT(cluster.startedAtUs.has_value(), "burst has not started");

			// Measured from the start of the burst rather than from the previous packet,
			// so that a packet going out late is made up for by the ones after it
			// instead of dragging the whole burst below the bitrate asked for.
			const int64_t deltaUs = (cluster.sentBytes * 8 * 1000000) / cluster.sendBitrate;

			// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
			return cluster.startedAtUs.value() + deltaUs;
		}
	} // namespace BWE
} // namespace RTC
