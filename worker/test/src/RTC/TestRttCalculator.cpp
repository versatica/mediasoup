#include "common.hpp"
#include "RTC/RttCalculator.hpp"
#include "mocks/include/MockShared.hpp"
#include <catch2/catch_test_macros.hpp>
#include <string_view>
#include <vector>

SCENARIO("BWE RttCalculator", "[bwe][rttcalculator]")
{
	constexpr std::string_view TimerLabel{ "rtt-calculator-update" };
	constexpr int64_t BaseTimeUs{ 1000000000 };

	class TestRttCalculatorListener : public RTC::RttCalculator::Listener
	{
	public:
		void OnRttCalculatorRttUpdated(RTC::RttCalculator* /*rttCalculator*/, int64_t avgRttMs) override
		{
			this->avgRttsMs.push_back(avgRttMs);
		}

	public:
		std::vector<int64_t> avgRttsMs;
	};

	int64_t nowUs{ BaseTimeUs };

	mocks::MockShared shared(
	  [&nowUs]() -> int64_t
	  {
		  return nowUs;
	  });

	const auto advanceNowMs = [&nowUs](int64_t incrementMs) -> void
	{
		nowUs += incrementMs * 1000;
	};

	SECTION("nothing is said until something is measured")
	{
		TestRttCalculatorListener listener;
		RTC::RttCalculator rttCalculator(std::addressof(listener), std::addressof(shared));

		auto* timer = shared.GetTimer(TimerLabel);

		REQUIRE(timer);
		REQUIRE(timer->GetRepeatMs() == 1000);

		advanceNowMs(1000);

		REQUIRE(timer->EvaluateHasExpired());

		REQUIRE(listener.avgRttsMs.empty());
		REQUIRE(!rttCalculator.GetAvgRttMs().has_value());
	}

	SECTION("the first measurement is taken as it is")
	{
		TestRttCalculatorListener listener;
		RTC::RttCalculator rttCalculator(std::addressof(listener), std::addressof(shared));

		rttCalculator.AddRtt(100, shared.GetTimeMs());

		REQUIRE(listener.avgRttsMs == std::vector<int64_t>{ 100 });

		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		REQUIRE(rttCalculator.GetAvgRttMs().value() == 100);
	}

	SECTION("measurements age out of the window and the average follows them down")
	{
		TestRttCalculatorListener listener;
		RTC::RttCalculator rttCalculator(std::addressof(listener), std::addressof(shared));

		auto* timer = shared.GetTimer(TimerLabel);

		REQUIRE(timer);

		// Nothing to weigh it against, so this one is the average.
		rttCalculator.AddRtt(100, shared.GetTimeMs());

		advanceNowMs(1000);

		// Both are within the window: 100 * 0.7 + ((100 + 120) / 2) * 0.3.
		rttCalculator.AddRtt(120, shared.GetTimeMs());

		advanceNowMs(1000);

		// The first one is now older than the window, so it goes:
		// 103 * 0.7 + ((120 + 80) / 2) * 0.3.
		rttCalculator.AddRtt(80, shared.GetTimeMs());

		advanceNowMs(1000);

		// Nothing was measured, but the 120 ages out and that moves the average
		// just the same: 102 * 0.7 + 80 * 0.3.
		REQUIRE(timer->EvaluateHasExpired());

		REQUIRE(listener.avgRttsMs == std::vector<int64_t>{ 100, 103, 102, 95 });
	}

	SECTION("a tick with the window still full says the same thing again")
	{
		TestRttCalculatorListener listener;
		RTC::RttCalculator rttCalculator(std::addressof(listener), std::addressof(shared));

		auto* timer = shared.GetTimer(TimerLabel);

		REQUIRE(timer);

		rttCalculator.AddRtt(100, shared.GetTimeMs());

		advanceNowMs(1000);

		// The measurement is still within the window, so it is still the average.
		REQUIRE(timer->EvaluateHasExpired());

		REQUIRE(listener.avgRttsMs == std::vector<int64_t>{ 100, 100 });
	}

	SECTION("two measurements taken at the same instant both count")
	{
		TestRttCalculatorListener listener;
		RTC::RttCalculator rttCalculator(std::addressof(listener), std::addressof(shared));

		rttCalculator.AddRtt(10, shared.GetTimeMs());

		// 10 * 0.7 + ((10 + 30) / 2) * 0.3.
		rttCalculator.AddRtt(30, shared.GetTimeMs());

		advanceNowMs(1000);

		// All of them are within the window, so the third one averages over
		// {10, 30, 10} and the fourth one over {10, 30, 10, 30}.
		rttCalculator.AddRtt(10, shared.GetTimeMs());
		rttCalculator.AddRtt(30, shared.GetTimeMs());

		REQUIRE(listener.avgRttsMs == std::vector<int64_t>{ 10, 13, 13, 15 });
	}

	SECTION("the average is forgotten when every measurement ages out")
	{
		TestRttCalculatorListener listener;
		RTC::RttCalculator rttCalculator(std::addressof(listener), std::addressof(shared));

		auto* timer = shared.GetTimer(TimerLabel);

		REQUIRE(timer);

		rttCalculator.AddRtt(100, shared.GetTimeMs());

		advanceNowMs(2000);

		// The window is empty now, so there is nothing to tell the listener and
		// it keeps the last thing it was told.
		REQUIRE(timer->EvaluateHasExpired());

		REQUIRE(listener.avgRttsMs == std::vector<int64_t>{ 100 });
		REQUIRE(!rttCalculator.GetAvgRttMs().has_value());

		// And with the average forgotten, the next measurement is taken as it is
		// rather than weighed against a stale one.
		rttCalculator.AddRtt(40, shared.GetTimeMs());

		REQUIRE(listener.avgRttsMs == std::vector<int64_t>{ 100, 40 });
	}
}
