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
		  RTC::BWE::ProbingScheduler* /*probingScheduler*/, RTC::RTP::Packet* packet) override
		{
			if (!this->sendPackets)
			{
				return false;
			}

			this->sentLengths.push_back(packet->GetLength());

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

	SECTION("a burst begins as soon as it is asked for")
	{
		RTC::BWE::ProbingScheduler probingScheduler(std::addressof(listener), std::addressof(shared));

		REQUIRE(!probingScheduler.IsProbing());
		REQUIRE(listener.sentLengths.empty());

		probingScheduler.CreateProbeCluster(makeClusterConfig(0, nowUs, 900000, 2 * 1000));

		// It doesn't wait for a tick that isn't running yet.
		REQUIRE(!listener.sentLengths.empty());
		REQUIRE(probingScheduler.IsProbing());

		auto* timer = shared.GetTimer(TimerLabel);

		REQUIRE(timer != nullptr);
		REQUIRE(timer->IsActive());
	}

	SECTION("a burst goes out in as many shots as it was asked for")
	{
		constexpr int64_t TestBitrate{ 900000 };
		constexpr int64_t MinProbeDeltaUs{ 2 * 1000 };
		// What the burst is meant to carry, which is its bitrate held for the time
		// it is meant to last, rounded to the nearest byte.
		constexpr size_t MinBytes{ ((TestBitrate * (15 * 1000)) + 4000000) / (8 * 1000000) };

		RTC::BWE::ProbingScheduler probingScheduler(std::addressof(listener), std::addressof(shared));

		probingScheduler.CreateProbeCluster(makeClusterConfig(0, nowUs, TestBitrate, MinProbeDeltaUs));

		auto* timer = shared.GetTimer(TimerLabel);
		size_t shots{ 1 };

		while (timer->IsActive())
		{
			nowUs = std::max(nowUs, timer->GetExpiresAtMs() * 1000);

			REQUIRE(timer->EvaluateHasExpired());

			++shots;
		}

		// Both counts have to be reached, since a burst of too few packets says
		// nothing however many bytes it carried, and the other way round.
		REQUIRE(shots >= 5);
		REQUIRE(listener.GetSentBytes() >= MinBytes);

		// And once it is done there is nothing left running.
		REQUIRE(!probingScheduler.IsProbing());
		REQUIRE(!timer->IsActive());
	}

	SECTION("the shots of a burst are spaced by the time it asked for")
	{
		constexpr int64_t TestBitrate{ 900000 };
		constexpr int64_t MinProbeDeltaUs{ 10 * 1000 };

		RTC::BWE::ProbingScheduler probingScheduler(std::addressof(listener), std::addressof(shared));

		const int64_t startTimeUs = nowUs;

		probingScheduler.CreateProbeCluster(makeClusterConfig(0, nowUs, TestBitrate, MinProbeDeltaUs));

		auto* timer = shared.GetTimer(TimerLabel);

		// The next shot is due once the bytes already gone out have been carried at
		// the bitrate asked for, measured from the start of the burst.
		const int64_t expectedNextUs =
		  startTimeUs + ((static_cast<int64_t>(listener.GetSentBytes()) * 8 * 1000000) / TestBitrate);

		REQUIRE(timer->IsActive());
		// Rounded up, since a shot emitted early would measure a bitrate nobody
		// asked for.
		REQUIRE(timer->GetExpiresAtMs() * 1000 >= expectedNextUs);
		REQUIRE((timer->GetExpiresAtMs() * 1000) - expectedNextUs < 1000);
	}

	SECTION("a listener that cannot send stops the burst")
	{
		RTC::BWE::ProbingScheduler probingScheduler(std::addressof(listener), std::addressof(shared));

		listener.sendPackets = false;

		probingScheduler.CreateProbeCluster(makeClusterConfig(0, nowUs, 900000, 2 * 1000));

		REQUIRE(listener.sentLengths.empty());

		auto* timer = shared.GetTimer(TimerLabel);

		// Nothing went out, so there is no reason to believe that trying again
		// would do any better.
		REQUIRE(!timer->IsActive());
	}

	SECTION("a burst asked for while another one is being emitted waits for it")
	{
		constexpr int64_t TestBitrate{ 900000 };

		RTC::BWE::ProbingScheduler probingScheduler(std::addressof(listener), std::addressof(shared));

		probingScheduler.CreateProbeCluster(makeClusterConfig(0, nowUs, TestBitrate, 2 * 1000));
		probingScheduler.CreateProbeCluster(makeClusterConfig(1, nowUs, TestBitrate, 2 * 1000));

		auto* timer = shared.GetTimer(TimerLabel);

		while (timer->IsActive())
		{
			nowUs = std::max(nowUs, timer->GetExpiresAtMs() * 1000);

			REQUIRE(timer->EvaluateHasExpired());
		}

		// Two whole bursts went out, not one.
		constexpr size_t MinBytes{ ((TestBitrate * (15 * 1000)) + 4000000) / (8 * 1000000) };

		REQUIRE(listener.GetSentBytes() >= 2 * MinBytes);
		REQUIRE(!probingScheduler.IsProbing());
	}
}
