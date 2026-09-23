#define MS_CLASS "RTC::BWE::ProbingScheduler"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/BWE/ProbingScheduler.hpp"
#include "Logger.hpp"

namespace RTC
{
	namespace BWE
	{
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

			this->shotSentBytes += packet->GetLength();

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

			// The timer only has millisecond resolution while the bursts are spaced in
			// microseconds, so a shot may come up before it is due. It waits instead
			// of going out early, since a burst emitted faster than it was meant to
			// measures a bitrate nobody asked for.
			if (nowUs < nextProbeTimeUs)
			{
				ScheduleNextProbe(nextProbeTimeUs, nowUs);

				return;
			}

			this->shotSentBytes = 0;

			this->probePacketGenerator.GeneratePackets(this->bitrateProber.GetRecommendedMinProbeSize());

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
			// this is also what counts the shots a burst is made of.
			this->bitrateProber.ProbeSent(nowUs, this->shotSentBytes);

			ScheduleNextProbe(this->bitrateProber.GetNextProbeTimeUs(nowUs), nowUs);
		}

		void ProbingScheduler::ScheduleNextProbe(std::optional<int64_t> nextProbeTimeUs, int64_t nowUs)
		{
			MS_TRACE();

			if (!nextProbeTimeUs.has_value())
			{
				this->nextProbeTimer->Stop();

				return;
			}

			// Rounded up so that the shot is never emitted before it is due, and never
			// negative for one that is already overdue.
			const int64_t timeoutMs = std::max<int64_t>(nextProbeTimeUs.value() - nowUs + 999, 0) / 1000;

			this->nextProbeTimer->Restart(timeoutMs);
		}
	} // namespace BWE
} // namespace RTC
