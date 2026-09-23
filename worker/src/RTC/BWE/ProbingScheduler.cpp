#define MS_CLASS "RTC::BWE::ProbingScheduler"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/BWE/ProbingScheduler.hpp"
#include "Logger.hpp"

namespace RTC
{
	namespace BWE
	{
		/* Static. */

		// How long before it is due a shot may go out. The instants of a burst are
		// microseconds apart while the timer only counts milliseconds, so without
		// this every shot would leave slightly late and the whole burst would
		// measure a bitrate below the one it was asked for.
		static constexpr int64_t MaxEarlyProbeProcessingUs{ 1000 };

		/* Instance methods. */

		ProbingScheduler::ProbingScheduler(Listener* listener, SharedInterface* shared)
		  : listener(listener),
		    shared(shared),
		    // A burst is made of packets this fabricates, so there is no traffic of
		    // somebody else's to wait for before one may begin.
		    bitrateProber(BitrateProber::BitrateProberOptions{ .allowStartProbingImmediately = true }),
		    probePacketGenerator(this)
		{
			MS_TRACE();

			this->nextProbeTimer = this->shared->CreateTimer(this, "probing-scheduler-next-probe");
		}

		ProbingScheduler::~ProbingScheduler()
		{
			MS_TRACE();

			delete this->nextProbeTimer;
		}

		void ProbingScheduler::CreateProbeCluster(const Types::ProbeClusterConfig& clusterConfig)
		{
			MS_TRACE();

			this->bitrateProber.CreateProbeCluster(clusterConfig);

			// The burst may begin right away, so this doesn't wait for a tick that
			// isn't running yet.
			Process();
		}

		bool ProbingScheduler::OnProbePacketGeneratorSendRtpPacket(
		  ProbePacketGenerator* /*probePacketGenerator*/, RTC::RTP::Packet* packet)
		{
			MS_TRACE();

			if (!this->listener->OnProbingSchedulerSendRtpPacket(this, packet))
			{
				return false;
			}

			// What the burst is measured against is what the link has to carry, which
			// is more than what the packet itself is worth.
			this->shotSentBytes += packet->GetLength() + this->packetOverhead;

			return true;
		}

		void ProbingScheduler::OnTimer(TimerHandleInterface* timer)
		{
			MS_TRACE();

			if (timer == this->nextProbeTimer)
			{
				Process();
			}
		}

		void ProbingScheduler::Process()
		{
			MS_TRACE();

			const int64_t nowUs = this->shared->GetTimeUs();

			// NOTE: This may give up on a burst that is due too far in the past, so
			// it's what decides whether there is anything to emit at all.
			const auto currentCluster = this->bitrateProber.GetCurrentCluster(nowUs);

			if (!currentCluster.has_value())
			{
				ScheduleNextProbe(std::nullopt, nowUs);

				return;
			}

			const int64_t nextProbeTimeUs = this->bitrateProber.GetNextProbeTimeUs(nowUs).value_or(nowUs);

			// A shot that is almost due goes out now rather than waiting for another
			// tick of a timer that cannot count in microseconds.
			if (nowUs + MaxEarlyProbeProcessingUs < nextProbeTimeUs)
			{
				ScheduleNextProbe(nextProbeTimeUs, nowUs);

				return;
			}

			this->shotSentBytes = 0;

			const size_t recommendedSize = this->bitrateProber.GetRecommendedMinProbeSize();

			// A burst whose bitrate doesn't amount to a single byte over the time
			// between two of its shots cannot be emitted at all.
			if (recommendedSize == 0)
			{
				MS_DEBUG_DEV("the burst carries no bytes between shots, giving up on it");

				ScheduleNextProbe(std::nullopt, nowUs);

				return;
			}

			// The burst opens with the smallest packet there is. What measures it
			// leaves out the size of the first packet to arrive, so a big one there
			// throws away much of what the burst carried.
			if (currentCluster.value().sentBytes == 0)
			{
				this->probePacketGenerator.GeneratePackets(this->probePacketGenerator.GetMinPacketLength());
			}

			if (this->shotSentBytes < recommendedSize)
			{
				this->probePacketGenerator.GeneratePackets(recommendedSize - this->shotSentBytes);
			}

			// Nothing went out, so there is nothing to report and no reason to believe
			// that trying again would do any better. A burst asked for later starts
			// everything over.
			if (this->shotSentBytes == 0)
			{
				MS_DEBUG_DEV("no packet of the burst could be sent, giving up on it");

				ScheduleNextProbe(std::nullopt, nowUs);

				return;
			}

			// NOTE: Reported once for the whole shot rather than per packet, since
			// this is also what counts the shots a burst is made of. The clock is
			// read again because handing the packets over took time of its own, and
			// this instant is what the rest of the burst is measured from.
			const int64_t sentAtUs = this->shared->GetTimeUs();

			this->bitrateProber.ProbeSent(sentAtUs, this->shotSentBytes);

			ScheduleNextProbe(this->bitrateProber.GetNextProbeTimeUs(sentAtUs), sentAtUs);
		}

		void ProbingScheduler::ScheduleNextProbe(std::optional<int64_t> nextProbeTimeUs, int64_t nowUs)
		{
			MS_TRACE();

			if (!nextProbeTimeUs.has_value())
			{
				this->nextProbeTimer->Stop();

				return;
			}

			// Truncated rather than rounded up, so that the tick lands just before
			// the shot is due instead of just after. What is left of the wait by then
			// is under a millisecond, which is what the margin above is for.
			const auto timeoutMs = std::max<int64_t>(nextProbeTimeUs.value() - nowUs, 0) / 1000;

			this->nextProbeTimer->Restart(timeoutMs);
		}
	} // namespace BWE
} // namespace RTC
