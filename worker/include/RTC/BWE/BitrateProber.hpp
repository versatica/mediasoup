#ifndef MS_RTC_BWE_BITRATE_PROBER_HPP
#define MS_RTC_BWE_BITRATE_PROBER_HPP

#include "common.hpp"
#include "RTC/BWE/BweTypes.hpp"
#include <queue>

namespace RTC
{
	namespace BWE
	{
		/**
		 * Paces out the bursts that somebody else decided to send.
		 *
		 * It takes the bursts asked for, keeps them in the order they were asked for,
		 * and answers a single question over and over: at what instant may the next
		 * packet of the current one go out, and how many bytes should it carry. It
		 * sends nothing itself.
		 *
		 * The spacing is what makes a burst worth anything. A burst is a bitrate held
		 * for a few milliseconds, so letting the packets go as fast as the socket
		 * takes them measures the queue of whoever is in the way rather than the link.
		 * So each instant is computed from the start of the burst and the bytes gone
		 * out so far, which keeps the whole of it close to the bitrate asked for even
		 * when a packet leaves late.
		 */
		class BitrateProber
		{
		private:
			/**
			 * Whether bursts are being emitted, and why not when they are not.
			 */
			enum class State : uint8_t
			{
				/**
				 * Nothing is going to be emitted at all.
				 */
				DISABLED,
				/**
				 * There may be bursts waiting, but none of them has started.
				 */
				INACTIVE,
				/**
				 * A burst is being emitted.
				 */
				ACTIVE
			};

			/**
			 * A burst that was asked for, and how much of it has gone out.
			 */
			struct Cluster
			{
				/**
				 * What travels with each of its packets, and what makes its result
				 * trustworthy.
				 */
				Types::ProbeCluster probeCluster;
				/**
				 * Bitrate it is meant to be sent at (bps).
				 */
				int64_t sendBitrate{ 0 };
				/**
				 * Time between two consecutive bursts of packets within it.
				 */
				int64_t minProbeDeltaUs{ 0 };
				/**
				 * Instant at which it was asked for, which is what makes it go stale.
				 */
				int64_t requestedAtUs{ 0 };
				/**
				 * Instant at which its first packet went out, or no value while none
				 * has.
				 */
				std::optional<int64_t> startedAtUs;
				int64_t sentProbes{ 0 };
				int64_t sentBytes{ 0 };
			};

		public:
			struct BitrateProberOptions
			{
				/**
				 * How late a packet of a burst may go out before the whole burst is
				 * given up on, since one sent too slowly no longer measures the bitrate
				 * it was meant to.
				 */
				int64_t maxProbeDelayUs{ 10 * 1000 };
				/**
				 * Size a packet has to reach for a burst to start on it (bytes).
				 *
				 * @remarks
				 * - It only applies while the bursts are made of traffic that somebody
				 *   else produces, since a burst that begins on a tiny packet starts
				 *   measuring from an instant that says nothing.
				 */
				size_t minPacketSize{ 200 };
				/**
				 * Whether a burst may start without waiting for a packet of that size.
				 */
				bool allowStartProbingImmediately{ false };
			};

		public:
			BitrateProber();

			explicit BitrateProber(BitrateProberOptions options);

			~BitrateProber() = default;

			/**
			 * Whether a burst is being emitted right now.
			 */
			bool IsProbing() const
			{
				return this->state == State::ACTIVE;
			}

			void SetEnabled(bool enabled);

			/**
			 * Whether a burst may start without waiting for a packet big enough.
			 */
			void SetAllowProbeWithoutMediaPacket(bool allow);

			/**
			 * Feed a packet that somebody else is sending, which is what may start a
			 * burst that is waiting for one.
			 */
			void OnIncomingPacket(size_t packetSize);

			/**
			 * Take a burst that has been asked for.
			 */
			void CreateProbeCluster(const Types::ProbeClusterConfig& clusterConfig);

			/**
			 * Instant at which the next packet of the current burst may go out, or no
			 * value if there is nothing to send.
			 *
			 * @remarks
			 * - The instant given is `nowUs` itself when the packet may go out right
			 *   away, so that the answer is always an instant that can be operated
			 *   with rather than a value meaning "as soon as possible".
			 */
			[[nodiscard]] std::optional<int64_t> GetNextProbeTimeUs(int64_t nowUs) const;

			/**
			 * The burst being emitted, or no value if there is none.
			 *
			 * @remarks
			 * - It gives up on the burst, and may leave none being emitted, when the
			 *   instant it was due at is too far behind.
			 */
			[[nodiscard]] std::optional<Types::ProbeCluster> GetCurrentCluster(int64_t nowUs);

			/**
			 * Bytes the next packet of the current burst should carry, or zero if there
			 * is no burst.
			 *
			 * @remarks
			 * - Packets are allowed to be grouped into a single shot as long as they
			 *   add up to this, which is what keeps a burst from needing a packet every
			 *   couple of milliseconds.
			 */
			size_t GetRecommendedMinProbeSize() const;

			/**
			 * Report that the given bytes of the current burst have gone out.
			 */
			void ProbeSent(int64_t nowUs, size_t size);

		private:
			/**
			 * Start emitting if there is a burst waiting and the given packet is enough
			 * to begin on.
			 */
			void MaybeSetActiveState(size_t packetSize);

			bool IsReadyToSetActiveState(size_t packetSize) const;

			/**
			 * Instant at which the next packet of the given burst is due, computed from
			 * the start of the burst so that it stays close to its bitrate.
			 */
			int64_t CalculateNextProbeTimeUs(const Cluster& cluster) const;

		private:
			// Passed by argument.
			BitrateProberOptions options;

			// Others.
			State state{ State::DISABLED };
			// The bursts asked for, the first one being the one being emitted.
			std::queue<Cluster> clusters;
			// Instant at which the next packet is due, or no value while it is due
			// right away.
			std::optional<int64_t> nextProbeTimeUs;
		};
	} // namespace BWE
} // namespace RTC

#endif
