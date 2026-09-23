#include "common.hpp"
#include "RTC/BWE/BweTypes.hpp"
#include "RTC/BWE/ProbingScheduler.hpp"
#include "mocks/include/MockShared.hpp"
#include <catch2/catch_test_macros.hpp>
#include <vector>

SCENARIO("BWE ProbingScheduler", "[bwe][probingscheduler]")
{
	// The clock starts well away from zero so that a mistake taking a time for a
	// duration doesn't go unnoticed.
	constexpr int64_t InitialTimeUs{ 100000000 };
	constexpr std::string_view TimerLabel{ "probing-scheduler-next-probe" };

	int64_t nowUs{ InitialTimeUs };

	mocks::MockShared shared(/*getTimeUs*/
	                         [&nowUs]() -> int64_t
	                         {
		                         return nowUs;
	                         });

	class TestProbingSchedulerListener : public RTC::BWE::ProbingScheduler::Listener
	{
	public:
		bool OnProbingSchedulerSendRtpPacket(
		  RTC::BWE::ProbingScheduler* /*probingScheduler*/,
		  RTC::RTP::Packet* packet,
		  const RTC::BWE::Types::ProbeCluster& probeCluster) override
		{
			if (!this->sendPackets)
			{
				return false;
			}

			this->sentLengths.push_back(packet->GetLength());
			this->sentClusterIds.push_back(probeCluster.id);

			return true;
		}

		size_t GetSentBytes() const
		{
			size_t sentBytes{ 0 };

			for (const auto length : this->sentLengths)
			{
				sentBytes += length;
			}

			return sentBytes;
		}

	public:
		std::vector<size_t> sentLengths;
		// The burst each of those packets was handed over as part of.
		std::vector<int64_t> sentClusterIds;
		bool sendPackets{ true };
	};

	TestProbingSchedulerListener listener;

	const auto makeClusterConfig = [](
	                                 int64_t id,
	                                 int64_t atUs,
	                                 int64_t targetBitrate,
	                                 int64_t minProbeDeltaUs) -> RTC::BWE::Types::ProbeClusterConfig
	{
		return RTC::BWE::Types::ProbeClusterConfig{ .id               = id,
		                                            .atUs             = atUs,
		                                            .targetBitrate    = targetBitrate,
		                                            .targetDurationUs = 15 * 1000,
		                                            .minProbeDeltaUs  = minProbeDeltaUs,
		                                            .targetProbeCount = 5 };
	};

	// Move the clock to when the next shot is due and let it out. Answers whether
	// there was one to begin with.
	const auto emitNextShot = [&nowUs, &shared, TimerLabel]() -> bool
	{
		auto* timer = shared.GetTimer(TimerLabel);

		if (!timer->IsActive())
		{
			return false;
		}

		nowUs = std::max(nowUs, timer->GetExpiresAtMs() * 1000);

		return timer->EvaluateHasExpired();
	};

	SECTION("a burst is emitted on the turn of the loop that follows asking for it")
	{
		RTC::BWE::ProbingScheduler probingScheduler(std::addressof(listener), std::addressof(shared));

		REQUIRE(!probingScheduler.IsProbing());
		REQUIRE(listener.sentLengths.empty());

		probingScheduler.CreateProbeClusters({ makeClusterConfig(0, nowUs, 900000, 2 * 1000) });

		auto* timer = shared.GetTimer(TimerLabel);

		REQUIRE(timer != nullptr);
		// Asking for it doesn't put the send path in the caller's stack.
		REQUIRE(listener.sentLengths.empty());
		// But nothing is waited for either: it's due right away.
		REQUIRE(timer->IsActive());
		REQUIRE(timer->GetTimeoutMs() == 0);

		REQUIRE(emitNextShot());

		REQUIRE(!listener.sentLengths.empty());
		REQUIRE(probingScheduler.IsProbing());
		// Every packet is handed over as part of the burst it belongs to.
		REQUIRE(listener.sentClusterIds.front() == 0);
	}

	SECTION("a burst goes out in as many shots as it was asked for")
	{
		constexpr int64_t TestBitrate{ 900000 };
		constexpr int64_t MinProbeDeltaUs{ 2 * 1000 };
		// What the burst is meant to carry, which is its bitrate held for the time
		// it is meant to last, rounded to the nearest byte.
		constexpr size_t MinBytes{ ((TestBitrate * (15 * 1000)) + 4000000) / (8 * 1000000) };

		RTC::BWE::ProbingScheduler probingScheduler(std::addressof(listener), std::addressof(shared));

		probingScheduler.CreateProbeClusters(
		  { makeClusterConfig(0, nowUs, TestBitrate, MinProbeDeltaUs) });

		size_t shots{ 0 };

		while (emitNextShot())
		{
			++shots;
		}

		// Both counts have to be reached, since a burst of too few packets says
		// nothing however many bytes it carried, and the other way round.
		REQUIRE(shots >= 5);
		REQUIRE(listener.GetSentBytes() >= MinBytes);

		// And once it is done there is nothing left running.
		REQUIRE(!probingScheduler.IsProbing());
		REQUIRE(!shared.GetTimer(TimerLabel)->IsActive());
	}

	SECTION("the shots that piled up while the loop was held up go out together")
	{
		constexpr int64_t TestBitrate{ 900000 };
		constexpr int64_t MinProbeDeltaUs{ 2 * 1000 };

		RTC::BWE::ProbingScheduler probingScheduler(std::addressof(listener), std::addressof(shared));

		probingScheduler.CreateProbeClusters(
		  { makeClusterConfig(0, nowUs, TestBitrate, MinProbeDeltaUs) });

		REQUIRE(emitNextShot());

		const size_t packetsAfterFirstShot = listener.sentLengths.size();

		REQUIRE(packetsAfterFirstShot > 0);

		// The loop gives no sign of life for the time of several shots.
		//
		// NOTE: The span has to sit between two bounds, which is the only window
		// where piling up happens at all. Below the time between two shots nothing
		// piles up, and above `BitrateProberOptions::maxProbeDelayUs`, which
		// defaults to 10 ms, the burst is given up on rather than emitted late, so
		// there would be nothing left to send. Six milliseconds is inside both.
		nowUs += 3 * MinProbeDeltaUs;

		auto* timer = shared.GetTimer(TimerLabel);

		REQUIRE(timer->IsActive());
		REQUIRE(timer->EvaluateHasExpired());

		// All of them went out on that single tick instead of one per turn of the
		// loop, which would have stretched the burst well below its bitrate.
		REQUIRE(listener.sentLengths.size() > packetsAfterFirstShot + 1);
	}

	SECTION("the shots of a burst are spaced by the time it asked for")
	{
		constexpr int64_t TestBitrate{ 900000 };
		constexpr int64_t MinProbeDeltaUs{ 10 * 1000 };

		RTC::BWE::ProbingScheduler probingScheduler(std::addressof(listener), std::addressof(shared));

		probingScheduler.CreateProbeClusters(
		  { makeClusterConfig(0, nowUs, TestBitrate, MinProbeDeltaUs) });

		REQUIRE(emitNextShot());

		// The burst is measured from when its first packet went out.
		const int64_t startTimeUs = nowUs;
		auto* timer               = shared.GetTimer(TimerLabel);

		// The next shot is due once the bytes already gone out have been carried at
		// the bitrate asked for, measured from the start of the burst.
		const int64_t expectedNextUs =
		  startTimeUs + ((static_cast<int64_t>(listener.GetSentBytes()) * 8 * 1000000) / TestBitrate);

		REQUIRE(timer->IsActive());
		// The tick lands just before the shot is due rather than just after, since
		// what is left of the wait by then is under a millisecond and goes out
		// anyway. A burst whose shots all left late would measure a bitrate below
		// the one it was asked for.
		REQUIRE(timer->GetExpiresAtMs() * 1000 <= expectedNextUs);
		REQUIRE(expectedNextUs - (timer->GetExpiresAtMs() * 1000) < 1000);
	}

	SECTION("a listener that cannot send stops the burst")
	{
		RTC::BWE::ProbingScheduler probingScheduler(std::addressof(listener), std::addressof(shared));

		listener.sendPackets = false;

		probingScheduler.CreateProbeClusters({ makeClusterConfig(0, nowUs, 900000, 2 * 1000) });

		REQUIRE(emitNextShot());
		REQUIRE(listener.sentLengths.empty());

		// Nothing went out, so there is no reason to believe that trying again
		// would do any better.
		REQUIRE(!shared.GetTimer(TimerLabel)->IsActive());
	}

	SECTION("a burst asked for while another one is being emitted waits for it")
	{
		constexpr int64_t TestBitrate{ 900000 };

		RTC::BWE::ProbingScheduler probingScheduler(std::addressof(listener), std::addressof(shared));

		// Both at once, which is what the controller does when it asks for the two
		// bursts the connection opens with.
		probingScheduler.CreateProbeClusters(
		  { makeClusterConfig(0, nowUs, TestBitrate, 2 * 1000),
			  makeClusterConfig(1, nowUs, TestBitrate, 2 * 1000) });

		while (emitNextShot())
		{
			// Nothing to do, the shots go out on their own.
		}

		// Two whole bursts went out, not one.
		constexpr size_t MinBytes{ ((TestBitrate * (15 * 1000)) + 4000000) / (8 * 1000000) };

		REQUIRE(listener.GetSentBytes() >= 2 * MinBytes);
		REQUIRE(!probingScheduler.IsProbing());

		// And each of them was handed over as its own, in the order they were asked
		// for and without mixing their packets.
		REQUIRE(listener.sentClusterIds.front() == 0);
		REQUIRE(listener.sentClusterIds.back() == 1);
		REQUIRE(std::ranges::is_sorted(listener.sentClusterIds));
	}
}
