#ifndef MS_RTC_BWE_PROBING_SCHEDULER_HPP
#define MS_RTC_BWE_PROBING_SCHEDULER_HPP

#include "common.hpp"
#include "handles/TimerHandleInterface.hpp"
#include "RTC/BWE/BitrateProber.hpp"
#include "RTC/BWE/BweTypes.hpp"
#include "RTC/BWE/ProbePacketGenerator.hpp"
#include "RTC/RTP/Packet.hpp"
#include "SharedInterface.hpp"

namespace RTC
{
	namespace BWE
	{
		/**
		 * Puts a probing burst on the wire at the pace it was asked for.
		 *
		 * Somebody else decides that a burst is worth sending and at what bitrate;
		 * this is what turns that decision into packets leaving at the right
		 * instants. It holds the burst, asks for the packets that carry the bytes
		 * due right now, hands them over one by one, and arms a timer for the next
		 * shot.
		 *
		 * The spacing is the whole point. A burst is a bitrate held for a few
		 * milliseconds, so letting its packets go as fast as the socket takes them
		 * measures the queue of whoever is in the way instead of the link. The
		 * instants come from the burst's own start and the bytes gone out so far, so
		 * a packet that leaves late is made up for by the ones behind it rather than
		 * dragging the whole burst below the bitrate asked for.
		 *
		 * @remarks
		 * - It is not a pacer. Nothing but the packets of a burst goes through here,
		 *   media is never queued, and there is no timer running while no burst is
		 *   being emitted.
		 */
		class ProbingScheduler : public ProbePacketGenerator::Listener,
		                         public TimerHandleInterface::Listener
		{
		public:
			class Listener
			{
			public:
				virtual ~Listener() = default;

			public:
				/**
				 * A packet of the burst is ready to go out.
				 *
				 * @returns Whether the rest of the burst is still wanted, so that a
				 * packet that couldn't be sent stops the ones behind it.
				 *
				 * @remarks
				 * - The packet must be sent before returning, since the next one is
				 *   built over the very same instance.
				 * - It carries room for the abs-send-time and for the transport wide
				 *   sequence number but no value for either, so both have to be written
				 *   here.
				 */
				virtual bool OnProbingSchedulerSendRtpPacket(
				  ProbingScheduler* probingScheduler,
				  RTC::RTP::Packet* packet,
				  const Types::ProbeCluster& probeCluster) = 0;
			};

		private:
			/**
			 * Holds what a shot needs while it is being emitted. The packets are
			 * handed over through a callback, so there is no scope of our own to keep
			 * it in, and this makes sure it doesn't outlive the shot.
			 */
			class ScopedShot
			{
			public:
				ScopedShot(ProbingScheduler& probingScheduler, const Types::ProbeCluster& probeCluster);

				~ScopedShot();

				ScopedShot(const ScopedShot&)            = delete;
				ScopedShot& operator=(const ScopedShot&) = delete;

			private:
				ProbingScheduler& probingScheduler;
			};

		public:
			ProbingScheduler(Listener* listener, SharedInterface* shared);

			~ProbingScheduler() override;

			// NOTE: It hands itself over as the listener of what it owns, so a copy
			// would leave those pointing at the instance it came from.
			ProbingScheduler(const ProbingScheduler&)            = delete;
			ProbingScheduler& operator=(const ProbingScheduler&) = delete;

			/**
			 * Take a burst that has been asked for.
			 *
			 * @remarks
			 * - Nothing goes out within this call. The burst begins on the turn of
			 *   the event loop that follows, so that the send path never runs inside
			 *   a call that may itself come from the send path.
			 * - The bursts asked for are emitted in the order they were asked for,
			 *   and one asked for long ago is dropped rather than sent late.
			 */
			void CreateProbeCluster(const Types::ProbeClusterConfig& clusterConfig);

			/**
			 * Bytes that every packet carries on top of its own length once it's on
			 * the network, so that a burst measures what the link really has to
			 * carry.
			 *
			 * @remarks
			 * - It depends on the path the packets take, so it has to be set again
			 *   whenever that changes.
			 */
			void SetPacketOverhead(size_t packetOverhead)
			{
				this->packetOverhead = packetOverhead;
			}

			/**
			 * Whether a burst is being emitted right now.
			 */
			bool IsProbing() const
			{
				return this->bitrateProber.IsProbing();
			}

			/* Pure virtual methods inherited from RTC::BWE::ProbePacketGenerator::Listener. */
		public:
			bool OnProbePacketGeneratorSendRtpPacket(
			  ProbePacketGenerator* probePacketGenerator, RTC::RTP::Packet* packet) override;

			/* Pure virtual methods inherited from TimerHandleInterface::Listener. */
		public:
			void OnTimer(TimerHandleInterface* timer) override;

		private:
			/**
			 * Emit what the current burst owes right now and arm the timer for its
			 * next shot, or stop it if there is nothing left to emit.
			 */
			void Process();

			/**
			 * Arm the timer for the given instant, or stop it if there is none.
			 */
			void ScheduleNextProbe(std::optional<int64_t> nextProbeTimeUs, int64_t nowUs);

		private:
			// Passed by argument.
			Listener* listener{ nullptr };
			SharedInterface* shared{ nullptr };
			// Others.
			// Decides which burst is being emitted and at what instant each of its
			// packets is due.
			BitrateProber bitrateProber;
			// Makes the packets that carry the bytes of a burst.
			ProbePacketGenerator probePacketGenerator;
			// Allocated by this.
			TimerHandleInterface* nextProbeTimer{ nullptr };
			// Bytes each packet carries on top of its own length once it's on the
			// network.
			size_t packetOverhead{ 0 };
			// The burst the current shot belongs to, which travels with each of its
			// packets. Only meaningful while a shot is being emitted, since it's the
			// generator that calls back and it knows nothing about bursts.
			Types::ProbeCluster shotProbeCluster;
			// Bytes of the current shot that have gone out, overhead included, which
			// is only meaningful while one is being emitted.
			size_t shotSentBytes{ 0 };
		};
	} // namespace BWE
} // namespace RTC

#endif
