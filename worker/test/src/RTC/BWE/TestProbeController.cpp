#include "common.hpp"
#include "RTC/BWE/ProbeController.hpp"
#include <catch2/catch_test_macros.hpp>

SCENARIO("BWE ProbeController", "[bwe][probecontroller]")
{
	// The clock starts well away from zero so that a mistake taking a time for a
	// duration doesn't go unnoticed.
	constexpr int64_t InitialTimeUs{ 100000000 };
	constexpr int64_t MinBitrate{ 100 };
	constexpr int64_t StartBitrate{ 300 };
	constexpr int64_t MaxBitrate{ 10000 };
	// Long enough for the climbing that follows the first burst to give up.
	constexpr int64_t ExponentialProbingTimeoutUs{ 5 * 1000 * 1000 };

	int64_t nowUs{ InitialTimeUs };

	SECTION("the first bursts go out once there is a bitrate to start from")
	{
		RTC::BWE::ProbeController probeController;

		REQUIRE(probeController.OnNetworkAvailability(/*networkAvailable*/ true, nowUs).empty());

		const auto probes = probeController.SetBitrates(MinBitrate, StartBitrate, MaxBitrate, nowUs);

		REQUIRE(probes.size() >= 2);
	}

	SECTION("and they wait for the network if it isn't there yet")
	{
		RTC::BWE::ProbeController probeController;

		REQUIRE(probeController.SetBitrates(MinBitrate, StartBitrate, MaxBitrate, nowUs).empty());

		const auto probes = probeController.OnNetworkAvailability(/*networkAvailable*/ true, nowUs);

		REQUIRE(probes.size() >= 2);
	}

	SECTION("a burst lasts and carries what the options say")
	{
		RTC::BWE::ProbeController probeController;

		REQUIRE(probeController.OnNetworkAvailability(/*networkAvailable*/ true, nowUs).empty());

		const auto probes = probeController.SetBitrates(MinBitrate, StartBitrate, MaxBitrate, nowUs);

		REQUIRE(probes.size() >= 2);
		REQUIRE(probes[0].targetDurationUs == 15 * 1000);
		REQUIRE(probes[0].targetProbeCount == 5);
	}

	SECTION("and both can be configured")
	{
		const RTC::BWE::ProbeController::ProbeControllerOptions options{ .minProbePacketsSent = 2,
		                                                                 .minProbeDurationUs =
		                                                                   123 * 1000 };

		RTC::BWE::ProbeController probeController(options);

		REQUIRE(probeController.OnNetworkAvailability(/*networkAvailable*/ true, nowUs).empty());

		const auto probes = probeController.SetBitrates(MinBitrate, StartBitrate, MaxBitrate, nowUs);

		REQUIRE(probes.size() >= 2);
		REQUIRE(probes[0].targetDurationUs == 123 * 1000);
		REQUIRE(probes[0].targetProbeCount == 2);
	}

	SECTION("nothing is probed while the network is down")
	{
		RTC::BWE::ProbeController probeController;

		auto probes = probeController.OnNetworkAvailability(/*networkAvailable*/ false, nowUs);

		probes = probeController.SetBitrates(MinBitrate, StartBitrate, MaxBitrate, nowUs);

		REQUIRE(probes.empty());

		probes = probeController.OnNetworkAvailability(/*networkAvailable*/ true, nowUs);

		REQUIRE(probes.size() >= 2);
	}

	SECTION("the factors of the first bursts can be configured")
	{
		const RTC::BWE::ProbeController::ProbeControllerOptions options{
			.firstExponentialProbeScale = 2.0, .secondExponentialProbeScale = 3.0
		};

		RTC::BWE::ProbeController probeController(options);

		REQUIRE(probeController.OnNetworkAvailability(/*networkAvailable*/ true, nowUs).empty());

		const auto probes = probeController.SetBitrates(MinBitrate, StartBitrate, MaxBitrate, nowUs);

		REQUIRE(probes.size() == 2);
		REQUIRE(probes[0].targetBitrate == StartBitrate * 2);
		REQUIRE(probes[1].targetBitrate == StartBitrate * 3);
	}

	SECTION("and a factor of zero leaves just the first one")
	{
		const RTC::BWE::ProbeController::ProbeControllerOptions options{
			.firstExponentialProbeScale = 2.0, .secondExponentialProbeScale = 0.0
		};

		RTC::BWE::ProbeController probeController(options);

		REQUIRE(probeController.OnNetworkAvailability(/*networkAvailable*/ true, nowUs).empty());

		const auto probes = probeController.SetBitrates(MinBitrate, StartBitrate, MaxBitrate, nowUs);

		REQUIRE(probes.size() == 1);
		REQUIRE(probes[0].targetBitrate == StartBitrate * 2);
	}

	SECTION("raising the maximum is room that hasn't been looked for yet")
	{
		RTC::BWE::ProbeController probeController;

		REQUIRE(probeController.OnNetworkAvailability(/*networkAvailable*/ true, nowUs).empty());

		auto probes = probeController.SetBitrates(MinBitrate, StartBitrate, MaxBitrate, nowUs);

		nowUs += ExponentialProbingTimeoutUs;

		probes = probeController.SetEstimatedBitrate(
		  StartBitrate, RTC::BWE::ProbeController::BandwidthLimitedCause::DELAY_BASED_LIMITED, nowUs);
		probes = probeController.Process(nowUs);
		probes = probeController.SetBitrates(MinBitrate, StartBitrate, MaxBitrate + 100, nowUs);

		REQUIRE(probes.size() == 1);
		REQUIRE(probes[0].targetBitrate == MaxBitrate + 100);
	}

	SECTION("asking for more is probed while not filling the link even when told not to otherwise")
	{
		const RTC::BWE::ProbeController::ProbeControllerOptions options{
			.probeOnMaxAllocatedBitrateChangeWithoutAlr = false
		};

		RTC::BWE::ProbeController probeController(options);

		REQUIRE(probeController.OnNetworkAvailability(/*networkAvailable*/ true, nowUs).empty());

		auto probes = probeController.SetBitrates(MinBitrate, StartBitrate, MaxBitrate, nowUs);

		probes = probeController.SetEstimatedBitrate(
		  MaxBitrate - 1, RTC::BWE::ProbeController::BandwidthLimitedCause::DELAY_BASED_LIMITED, nowUs);

		nowUs += ExponentialProbingTimeoutUs;

		probes = probeController.Process(nowUs);

		REQUIRE(probes.empty());

		probeController.SetAlrStartTimeUs(nowUs);

		probes = probeController.OnMaxTotalAllocatedBitrate(MaxBitrate + 1, nowUs);

		REQUIRE(probes.size() == 2);
		REQUIRE(probes.at(0).targetBitrate == MaxBitrate);
	}

	SECTION("and while filling it too, by default")
	{
		constexpr int64_t TransportMaxBitrate{ 10000000 };
		constexpr int64_t EstimatedAfterInitialProbing{ 300000 };
		constexpr int64_t ScreenShareMaxBitrate{ 3500000 };

		RTC::BWE::ProbeController probeController;

		REQUIRE(probeController.OnNetworkAvailability(/*networkAvailable*/ true, nowUs).empty());

		auto probes = probeController.SetBitrates(MinBitrate, StartBitrate, TransportMaxBitrate, nowUs);

		probes = probeController.SetEstimatedBitrate(
		  EstimatedAfterInitialProbing,
		  RTC::BWE::ProbeController::BandwidthLimitedCause::DELAY_BASED_LIMITED,
		  nowUs);

		nowUs += ExponentialProbingTimeoutUs;

		probes = probeController.Process(nowUs);

		REQUIRE(probes.empty());

		probeController.SetAlrStartTimeUs(std::nullopt);

		probes = probeController.OnMaxTotalAllocatedBitrate(ScreenShareMaxBitrate, nowUs);

		REQUIRE(probes.size() == 1);
		REQUIRE(probes.at(0).targetBitrate == 2 * EstimatedAfterInitialProbing);
	}

	SECTION("unless it is told not to")
	{
		constexpr int64_t TransportMaxBitrate{ 10000000 };
		constexpr int64_t EstimatedAfterInitialProbing{ 300000 };
		constexpr int64_t ScreenShareMaxBitrate{ 3500000 };

		const RTC::BWE::ProbeController::ProbeControllerOptions options{
			.probeOnMaxAllocatedBitrateChangeWithoutAlr = false
		};

		RTC::BWE::ProbeController probeController(options);

		REQUIRE(probeController.OnNetworkAvailability(/*networkAvailable*/ true, nowUs).empty());

		auto probes = probeController.SetBitrates(MinBitrate, StartBitrate, TransportMaxBitrate, nowUs);

		probes = probeController.SetEstimatedBitrate(
		  EstimatedAfterInitialProbing,
		  RTC::BWE::ProbeController::BandwidthLimitedCause::DELAY_BASED_LIMITED,
		  nowUs);

		nowUs += ExponentialProbingTimeoutUs;

		probes = probeController.Process(nowUs);

		REQUIRE(probes.empty());

		probeController.SetAlrStartTimeUs(std::nullopt);

		REQUIRE(probeController.OnMaxTotalAllocatedBitrate(ScreenShareMaxBitrate, nowUs).empty());
	}

	SECTION("asking for less is not probed, but asking for more again is")
	{
		RTC::BWE::ProbeController probeController;

		REQUIRE(probeController.OnNetworkAvailability(/*networkAvailable*/ true, nowUs).empty());

		auto probes = probeController.SetBitrates(MinBitrate, StartBitrate, MaxBitrate, nowUs);

		REQUIRE(!probes.empty());

		// The first allocation arrives while the initial bursts are still going.
		REQUIRE(probeController.OnMaxTotalAllocatedBitrate(MaxBitrate, nowUs).empty());

		probes = probeController.SetEstimatedBitrate(
		  StartBitrate, RTC::BWE::ProbeController::BandwidthLimitedCause::DELAY_BASED_LIMITED, nowUs);

		nowUs += ExponentialProbingTimeoutUs;

		REQUIRE(probeController.Process(nowUs).empty());

		// Asking for less moves the limit without looking for anything.
		REQUIRE(probeController.OnMaxTotalAllocatedBitrate(MaxBitrate / 2, nowUs).empty());

		probes = probeController.OnMaxTotalAllocatedBitrate(MaxBitrate * 3 / 4, nowUs);

		REQUIRE(!probes.empty());
	}

	SECTION("what is asked for is bounded by the current estimate")
	{
		RTC::BWE::ProbeController probeController;

		REQUIRE(MaxBitrate > 1.5 * StartBitrate);
		REQUIRE(probeController.OnNetworkAvailability(/*networkAvailable*/ true, nowUs).empty());

		auto probes = probeController.SetBitrates(MinBitrate, StartBitrate, MaxBitrate, nowUs);

		probes = probeController.SetEstimatedBitrate(
		  StartBitrate, RTC::BWE::ProbeController::BandwidthLimitedCause::DELAY_BASED_LIMITED, nowUs);

		nowUs += ExponentialProbingTimeoutUs;

		probes = probeController.Process(nowUs);

		REQUIRE(probes.empty());

		probeController.SetAlrStartTimeUs(nowUs);

		probes = probeController.OnMaxTotalAllocatedBitrate(MaxBitrate, nowUs);

		REQUIRE(probes.size() == 1);
		REQUIRE(probes.at(0).targetBitrate == 2 * StartBitrate);

		// A burst that worked out leads to another one.
		probes = probeController.SetEstimatedBitrate(
		  (3 * StartBitrate) / 2,
		  RTC::BWE::ProbeController::BandwidthLimitedCause::DELAY_BASED_LIMITED,
		  nowUs);

		REQUIRE(probes.size() == 1);
		REQUIRE(probes.at(0).targetBitrate > (3 * StartBitrate) / 2);
	}

	SECTION("probing what is asked for can be turned off")
	{
		const RTC::BWE::ProbeController::ProbeControllerOptions options{
			.probeOnMaxAllocatedBitrateChange = false
		};

		RTC::BWE::ProbeController probeController(options);

		REQUIRE(probeController.OnNetworkAvailability(/*networkAvailable*/ true, nowUs).empty());

		auto probes = probeController.SetBitrates(MinBitrate, StartBitrate, MaxBitrate, nowUs);

		probes = probeController.SetEstimatedBitrate(
		  MaxBitrate - 1, RTC::BWE::ProbeController::BandwidthLimitedCause::DELAY_BASED_LIMITED, nowUs);

		nowUs += ExponentialProbingTimeoutUs;

		probes = probeController.Process(nowUs);

		REQUIRE(probes.empty());

		probeController.SetAlrStartTimeUs(nowUs);

		probes = probeController.OnMaxTotalAllocatedBitrate(MaxBitrate + 1, nowUs);

		REQUIRE(probes.empty());
	}

	SECTION("raising the maximum is probed even while sitting at the previous one")
	{
		RTC::BWE::ProbeController probeController;

		REQUIRE(probeController.OnNetworkAvailability(/*networkAvailable*/ true, nowUs).empty());

		auto probes = probeController.SetBitrates(MinBitrate, StartBitrate, MaxBitrate, nowUs);

		nowUs += ExponentialProbingTimeoutUs;

		probes = probeController.SetEstimatedBitrate(
		  StartBitrate, RTC::BWE::ProbeController::BandwidthLimitedCause::DELAY_BASED_LIMITED, nowUs);
		probes = probeController.Process(nowUs);
		probes = probeController.SetEstimatedBitrate(
		  MaxBitrate, RTC::BWE::ProbeController::BandwidthLimitedCause::DELAY_BASED_LIMITED, nowUs);
		probes = probeController.SetBitrates(MinBitrate, StartBitrate, MaxBitrate + 100, nowUs);

		REQUIRE(probes.size() == 1);
		REQUIRE(probes[0].targetBitrate == MaxBitrate + 100);
	}

	SECTION("each burst leads to the next one only once its result is good enough")
	{
		RTC::BWE::ProbeController probeController;

		REQUIRE(probeController.OnNetworkAvailability(/*networkAvailable*/ true, nowUs).empty());

		auto probes = probeController.SetBitrates(MinBitrate, StartBitrate, MaxBitrate, nowUs);

		// Another one only follows once the estimate climbs above 0.7 of what the
		// last one aimed for, which is 0.7 * 6 * 300.
		probes = probeController.SetEstimatedBitrate(
		  1000, RTC::BWE::ProbeController::BandwidthLimitedCause::DELAY_BASED_LIMITED, nowUs);

		REQUIRE(probes.empty());

		probes = probeController.SetEstimatedBitrate(
		  1800, RTC::BWE::ProbeController::BandwidthLimitedCause::DELAY_BASED_LIMITED, nowUs);

		REQUIRE(probes.size() == 1);
		REQUIRE(probes[0].targetBitrate == 2 * 1800);
	}

	SECTION("and stops climbing once the maximum is already reached")
	{
		const RTC::BWE::ProbeController::ProbeControllerOptions options{
			.abortFurtherProbeIfMaxLowerThanCurrent = true
		};

		RTC::BWE::ProbeController probeController(options);

		REQUIRE(probeController.OnNetworkAvailability(/*networkAvailable*/ true, nowUs).empty());

		auto probes = probeController.SetBitrates(MinBitrate, StartBitrate, MaxBitrate, nowUs);

		REQUIRE(!probes.empty());

		probes = probeController.SetBitrates(MinBitrate, StartBitrate, StartBitrate, nowUs);

		REQUIRE(probes.empty());

		probes = probeController.SetEstimatedBitrate(
		  1800, RTC::BWE::ProbeController::BandwidthLimitedCause::DELAY_BASED_LIMITED, nowUs);

		REQUIRE(probes.empty());
	}

	SECTION("or once what is asked for is already reached")
	{
		const RTC::BWE::ProbeController::ProbeControllerOptions options{
			.abortFurtherProbeIfMaxLowerThanCurrent = true
		};

		RTC::BWE::ProbeController probeController(options);

		REQUIRE(probeController.OnNetworkAvailability(/*networkAvailable*/ true, nowUs).empty());

		auto probes = probeController.SetBitrates(MinBitrate, StartBitrate, MaxBitrate, nowUs);

		REQUIRE(!probes.empty());

		probes = probeController.OnMaxTotalAllocatedBitrate(StartBitrate, nowUs);

		REQUIRE(probes.empty());

		probes = probeController.SetEstimatedBitrate(
		  1800, RTC::BWE::ProbeController::BandwidthLimitedCause::DELAY_BASED_LIMITED, nowUs);

		REQUIRE(probes.empty());
	}

	SECTION("without that option the next burst aims at twice what is asked for")
	{
		RTC::BWE::ProbeController probeController;

		REQUIRE(probeController.OnNetworkAvailability(/*networkAvailable*/ true, nowUs).empty());

		auto probes = probeController.SetBitrates(MinBitrate, StartBitrate, MaxBitrate, nowUs);

		REQUIRE(!probes.empty());

		probes = probeController.OnMaxTotalAllocatedBitrate(StartBitrate, nowUs);

		REQUIRE(probes.empty());

		probes = probeController.SetEstimatedBitrate(
		  1800, RTC::BWE::ProbeController::BandwidthLimitedCause::DELAY_BASED_LIMITED, nowUs);

		REQUIRE(probes.size() == 1);
		REQUIRE(probes[0].targetBitrate == 2 * StartBitrate);
	}

	SECTION("a burst whose result never arrives is given up on")
	{
		RTC::BWE::ProbeController probeController;

		REQUIRE(probeController.OnNetworkAvailability(/*networkAvailable*/ true, nowUs).empty());

		auto probes = probeController.SetBitrates(MinBitrate, StartBitrate, MaxBitrate, nowUs);

		REQUIRE(!probes.empty());

		nowUs += ExponentialProbingTimeoutUs;

		probes = probeController.Process(nowUs);

		REQUIRE(probes.empty());

		probes = probeController.SetEstimatedBitrate(
		  1800, RTC::BWE::ProbeController::BandwidthLimitedCause::DELAY_BASED_LIMITED, nowUs);

		REQUIRE(probes.empty());
	}

	SECTION("the initial bursts are repeated for a while when told to")
	{
		RTC::BWE::ProbeController probeController;

		probeController.EnableRepeatedInitialProbing(true);

		REQUIRE(probeController.OnNetworkAvailability(/*networkAvailable*/ true, nowUs).empty());

		auto probes = probeController.SetBitrates(MinBitrate, StartBitrate, MaxBitrate, nowUs);

		REQUIRE(!probes.empty());

		const int64_t startTimeUs = nowUs;
		int64_t lastProbeAtUs     = nowUs;

		while (nowUs < startTimeUs + (5 * 1000 * 1000))
		{
			nowUs += 100 * 1000;

			probes = probeController.Process(nowUs);

			if (!probes.empty())
			{
				// One every second, noticed at the granularity this is asked at.
				REQUIRE(nowUs - lastProbeAtUs == 1100 * 1000);
				REQUIRE(probes[0].minProbeDeltaUs == 20 * 1000);
				REQUIRE(probes[0].targetDurationUs == 100 * 1000);

				lastProbeAtUs = nowUs;
			}
			else
			{
				REQUIRE(nowUs - lastProbeAtUs < 1100 * 1000);
			}
		}

		nowUs += 1000 * 1000;

		REQUIRE(probeController.Process(nowUs).empty());
	}

	SECTION("and stop as soon as there is traffic of its own to measure")
	{
		RTC::BWE::ProbeController probeController;

		probeController.EnableRepeatedInitialProbing(true);

		REQUIRE(probeController.OnNetworkAvailability(/*networkAvailable*/ true, nowUs).empty());

		auto probes = probeController.SetBitrates(MinBitrate, StartBitrate, MaxBitrate, nowUs);

		REQUIRE(!probes.empty());

		nowUs += 1100 * 1000;

		probes = probeController.Process(nowUs);

		REQUIRE(probes.size() == 1);

		probes = probeController.OnMaxTotalAllocatedBitrate(MinBitrate, nowUs);

		nowUs += 1100 * 1000;

		probes = probeController.Process(nowUs);

		REQUIRE(probes.empty());
	}

	SECTION("a large drop is checked with one burst while not filling the link")
	{
		RTC::BWE::ProbeController probeController;

		REQUIRE(probeController.OnNetworkAvailability(/*networkAvailable*/ true, nowUs).empty());

		auto probes = probeController.SetBitrates(MinBitrate, StartBitrate, MaxBitrate, nowUs);

		REQUIRE(probes.size() >= 2);

		probes = probeController.SetEstimatedBitrate(
		  500, RTC::BWE::ProbeController::BandwidthLimitedCause::DELAY_BASED_LIMITED, nowUs);

		probeController.SetAlrStartTimeUs(nowUs);

		nowUs += (5 * 1000 * 1000) + 1000;

		probes = probeController.Process(nowUs);
		probes = probeController.SetEstimatedBitrate(
		  250, RTC::BWE::ProbeController::BandwidthLimitedCause::DELAY_BASED_LIMITED, nowUs);
		probes = probeController.RequestProbe(nowUs);

		// 0.85 of what the estimate was worth before the drop.
		REQUIRE(probes.size() == 1);
		REQUIRE(probes[0].targetBitrate == 425);
	}

	SECTION("and also just after it starts filling it again")
	{
		RTC::BWE::ProbeController probeController;

		REQUIRE(probeController.OnNetworkAvailability(/*networkAvailable*/ true, nowUs).empty());

		auto probes = probeController.SetBitrates(MinBitrate, StartBitrate, MaxBitrate, nowUs);

		REQUIRE(probes.size() == 2);

		probes = probeController.SetEstimatedBitrate(
		  500, RTC::BWE::ProbeController::BandwidthLimitedCause::DELAY_BASED_LIMITED, nowUs);

		probeController.SetAlrStartTimeUs(std::nullopt);

		nowUs += (5 * 1000 * 1000) + 1000;

		probes = probeController.Process(nowUs);
		probes = probeController.SetEstimatedBitrate(
		  250, RTC::BWE::ProbeController::BandwidthLimitedCause::DELAY_BASED_LIMITED, nowUs);

		probeController.SetAlrEndedTimeUs(nowUs);

		nowUs += (3 * 1000 * 1000) - 1000;

		probes = probeController.RequestProbe(nowUs);

		REQUIRE(probes.size() == 1);
		REQUIRE(probes[0].targetBitrate == 425);
	}

	SECTION("but not once it has been filling it for a while")
	{
		RTC::BWE::ProbeController probeController;

		REQUIRE(probeController.OnNetworkAvailability(/*networkAvailable*/ true, nowUs).empty());

		auto probes = probeController.SetBitrates(MinBitrate, StartBitrate, MaxBitrate, nowUs);

		REQUIRE(probes.size() == 2);

		probes = probeController.SetEstimatedBitrate(
		  500, RTC::BWE::ProbeController::BandwidthLimitedCause::DELAY_BASED_LIMITED, nowUs);

		probeController.SetAlrStartTimeUs(std::nullopt);

		nowUs += (5 * 1000 * 1000) + 1000;

		probes = probeController.Process(nowUs);
		probes = probeController.SetEstimatedBitrate(
		  250, RTC::BWE::ProbeController::BandwidthLimitedCause::DELAY_BASED_LIMITED, nowUs);

		probeController.SetAlrEndedTimeUs(nowUs);

		nowUs += (3 * 1000 * 1000) + 1000;

		probes = probeController.RequestProbe(nowUs);

		REQUIRE(probes.empty());
	}

	SECTION("nor long after the drop happened")
	{
		RTC::BWE::ProbeController probeController;

		REQUIRE(probeController.OnNetworkAvailability(/*networkAvailable*/ true, nowUs).empty());

		auto probes = probeController.SetBitrates(MinBitrate, StartBitrate, MaxBitrate, nowUs);

		REQUIRE(probes.size() == 2);

		probes = probeController.SetEstimatedBitrate(
		  500, RTC::BWE::ProbeController::BandwidthLimitedCause::DELAY_BASED_LIMITED, nowUs);

		probeController.SetAlrStartTimeUs(nowUs);

		nowUs += (5 * 1000 * 1000) + 1000;

		probes = probeController.Process(nowUs);
		probes = probeController.SetEstimatedBitrate(
		  250, RTC::BWE::ProbeController::BandwidthLimitedCause::DELAY_BASED_LIMITED, nowUs);

		nowUs += (5 * 1000 * 1000) + 1000;

		probes = probeController.RequestProbe(nowUs);

		REQUIRE(probes.empty());
	}

	SECTION("while not filling the link a burst goes out every five seconds")
	{
		RTC::BWE::ProbeController probeController;

		REQUIRE(probeController.OnNetworkAvailability(/*networkAvailable*/ true, nowUs).empty());

		probeController.EnablePeriodicAlrProbing(true);

		auto probes = probeController.SetBitrates(MinBitrate, StartBitrate, MaxBitrate, nowUs);

		REQUIRE(probes.size() == 2);

		probes = probeController.SetEstimatedBitrate(
		  500, RTC::BWE::ProbeController::BandwidthLimitedCause::DELAY_BASED_LIMITED, nowUs);

		const int64_t startTimeUs = nowUs;

		probeController.SetAlrStartTimeUs(startTimeUs);

		nowUs += 5 * 1000 * 1000;

		probes = probeController.Process(nowUs);

		REQUIRE(probes.size() == 1);
		REQUIRE(probes[0].targetBitrate == 1000);

		probes = probeController.SetEstimatedBitrate(
		  500, RTC::BWE::ProbeController::BandwidthLimitedCause::DELAY_BASED_LIMITED, nowUs);

		// The next one is due ten seconds into the state, not five after this one.
		probeController.SetAlrStartTimeUs(startTimeUs);

		nowUs += 4 * 1000 * 1000;

		probes = probeController.Process(nowUs);
		probes = probeController.SetEstimatedBitrate(
		  500, RTC::BWE::ProbeController::BandwidthLimitedCause::DELAY_BASED_LIMITED, nowUs);

		REQUIRE(probes.empty());

		probeController.SetAlrStartTimeUs(startTimeUs);

		nowUs += 1000 * 1000;

		probes = probeController.Process(nowUs);

		REQUIRE(probes.size() == 1);

		probes = probeController.SetEstimatedBitrate(
		  500, RTC::BWE::ProbeController::BandwidthLimitedCause::DELAY_BASED_LIMITED, nowUs);

		REQUIRE(probes.empty());
	}

	SECTION("and starting over leaves nothing to probe until there are bitrates again")
	{
		RTC::BWE::ProbeController probeController;

		REQUIRE(probeController.OnNetworkAvailability(/*networkAvailable*/ true, nowUs).empty());

		probeController.SetAlrStartTimeUs(nowUs);
		probeController.EnablePeriodicAlrProbing(true);

		auto probes = probeController.SetBitrates(MinBitrate, StartBitrate, MaxBitrate, nowUs);

		probeController.Reset(nowUs);

		nowUs += 10 * 1000 * 1000;

		probes = probeController.Process(nowUs);

		REQUIRE(probes.empty());

		probes = probeController.SetBitrates(MinBitrate, StartBitrate, MaxBitrate, nowUs);

		REQUIRE(probes.size() == 2);

		// The bitrate to start from is what stands in for the estimate until one
		// is fed.
		nowUs += 10 * 1000 * 1000;

		probes = probeController.Process(nowUs);

		REQUIRE(probes.size() == 1);
		REQUIRE(probes[0].targetBitrate == StartBitrate * 2);
	}

	SECTION("nothing is probed while nothing can be sent")
	{
		RTC::BWE::ProbeController probeController;

		probeController.EnablePeriodicAlrProbing(true);

		REQUIRE(probeController.OnNetworkAvailability(/*networkAvailable*/ false, nowUs).empty());
		REQUIRE(probeController.SetBitrates(MinBitrate, StartBitrate, MaxBitrate, nowUs).empty());

		nowUs += 10 * 1000 * 1000;

		REQUIRE(probeController.Process(nowUs).empty());

		// Starting over after a route change doesn't make it probe either, since
		// nothing can be sent until the handshake is done, even though the bounds
		// may well change meanwhile.
		probeController.Reset(nowUs);

		REQUIRE(
		  probeController.SetBitrates(2 * MinBitrate, 2 * StartBitrate, 2 * MaxBitrate, nowUs).empty());

		nowUs += 10 * 1000 * 1000;

		REQUIRE(probeController.Process(nowUs).empty());
	}

	SECTION("no burst ever aims above what the application allows")
	{
		RTC::BWE::ProbeController probeController;

		REQUIRE(probeController.OnNetworkAvailability(/*networkAvailable*/ true, nowUs).empty());

		auto probes = probeController.SetBitrates(MinBitrate, 10000000, 100000000, nowUs);

		probes = probeController.SetEstimatedBitrate(
		  60000000, RTC::BWE::ProbeController::BandwidthLimitedCause::DELAY_BASED_LIMITED, nowUs);

		REQUIRE(probes.size() == 1);
		REQUIRE(probes[0].targetBitrate == 100000000);

		// And once there, there is nothing left to climb towards.
		probes = probeController.SetEstimatedBitrate(
		  100000000, RTC::BWE::ProbeController::BandwidthLimitedCause::DELAY_BASED_LIMITED, nowUs);

		REQUIRE(probes.empty());
	}

	SECTION("nor above twice what the application wants to send")
	{
		RTC::BWE::ProbeController probeController;

		REQUIRE(probeController.OnNetworkAvailability(/*networkAvailable*/ true, nowUs).empty());

		auto probes = probeController.SetBitrates(MinBitrate, 10000000, 100000000, nowUs);

		probeController.EnablePeriodicAlrProbing(true);
		probeController.SetAlrStartTimeUs(nowUs);

		probes = probeController.SetEstimatedBitrate(
		  10000000, RTC::BWE::ProbeController::BandwidthLimitedCause::DELAY_BASED_LIMITED, nowUs);

		// Asking for less than is already being sent is nothing to probe about.
		probes = probeController.OnMaxTotalAllocatedBitrate(9000000, nowUs);

		REQUIRE(probes.empty());

		nowUs += 5 * 1000 * 1000;

		probes = probeController.Process(nowUs);

		REQUIRE(probes.size() == 1);
		REQUIRE(probes[0].targetBitrate == 2 * 9000000);

		// With nothing asked for, the estimate is what bounds the burst again.
		REQUIRE(probeController.OnMaxTotalAllocatedBitrate(0, nowUs).empty());

		nowUs += 5 * 1000 * 1000;

		probes = probeController.Process(nowUs);

		REQUIRE(probes.size() == 1);
		REQUIRE(probes[0].targetBitrate == 2 * 10000000);
	}

	SECTION("every factor and count that shapes a burst can be set")
	{
		const RTC::BWE::ProbeController::ProbeControllerOptions options{
			.firstExponentialProbeScale         = 2.0,
			.secondExponentialProbeScale        = 5.0,
			.furtherExponentialProbeScale       = 3.0,
			.furtherProbeThreshold              = 0.8,
			.firstAllocationProbeScale          = 2.0,
			.secondAllocationProbeScale         = std::nullopt,
			.allocationProbeLimitByCurrentScale = 1000.0,
			.minProbePacketsSent                = 2
		};

		RTC::BWE::ProbeController probeController(options);

		REQUIRE(probeController.OnNetworkAvailability(/*networkAvailable*/ true, nowUs).empty());

		auto probes = probeController.SetBitrates(MinBitrate, StartBitrate, 5000000, nowUs);

		REQUIRE(probes.size() == 2);
		REQUIRE(probes[0].targetBitrate == 600);
		REQUIRE(probes[0].targetProbeCount == 2);
		REQUIRE(probes[1].targetBitrate == 1500);
		REQUIRE(probes[1].targetProbeCount == 2);

		// Another burst only follows one whose result got above 0.8 of what it
		// aimed for.
		probes = probeController.SetEstimatedBitrate(
		  1100, RTC::BWE::ProbeController::BandwidthLimitedCause::DELAY_BASED_LIMITED, nowUs);

		REQUIRE(probes.empty());

		probes = probeController.SetEstimatedBitrate(
		  1250, RTC::BWE::ProbeController::BandwidthLimitedCause::DELAY_BASED_LIMITED, nowUs);

		REQUIRE(probes.size() == 1);
		REQUIRE(probes[0].targetBitrate == 3 * 1250);

		nowUs += 5 * 1000 * 1000;

		probes = probeController.Process(nowUs);

		probeController.SetAlrStartTimeUs(nowUs);

		probes = probeController.OnMaxTotalAllocatedBitrate(200000, nowUs);

		REQUIRE(probes.size() == 1);
		REQUIRE(probes[0].targetBitrate == 400000);
	}

	SECTION("while loss is what holds the estimate back a burst barely goes above it")
	{
		RTC::BWE::ProbeController probeController;

		REQUIRE(probeController.OnNetworkAvailability(/*networkAvailable*/ true, nowUs).empty());

		probeController.EnablePeriodicAlrProbing(true);

		auto probes = probeController.SetBitrates(MinBitrate, StartBitrate, MaxBitrate, nowUs);

		probes = probeController.SetEstimatedBitrate(
		  500, RTC::BWE::ProbeController::BandwidthLimitedCause::DELAY_BASED_LIMITED, nowUs);

		probeController.SetAlrStartTimeUs(nowUs);

		nowUs += 5 * 1000 * 1000;

		probes = probeController.Process(nowUs);

		REQUIRE(probes.size() == 1);

		probes = probeController.SetEstimatedBitrate(
		  500, RTC::BWE::ProbeController::BandwidthLimitedCause::LOSS_LIMITED_BWE_INCREASING, nowUs);

		nowUs += 6 * 1000 * 1000;

		probes = probeController.Process(nowUs);

		REQUIRE(probes.size() == 1);
		REQUIRE(probes[0].targetBitrate == 750);

		// And once loss is no longer what holds it back, the usual factor applies
		// again.
		probes = probeController.SetEstimatedBitrate(
		  750, RTC::BWE::ProbeController::BandwidthLimitedCause::DELAY_BASED_LIMITED, nowUs);

		nowUs += 6 * 1000 * 1000;

		probes = probeController.Process(nowUs);

		REQUIRE(!probes.empty());
		REQUIRE(probes[0].targetBitrate > 1125);
	}

	SECTION("a known upper bound of the link capacity is probed periodically")
	{
		const RTC::BWE::ProbeController::ProbeControllerOptions options{
			.networkStateEstimateProbingIntervalUs = 5 * 1000 * 1000
		};

		RTC::BWE::ProbeController probeController(options);

		REQUIRE(probeController.OnNetworkAvailability(/*networkAvailable*/ true, nowUs).empty());

		auto probes = probeController.SetBitrates(MinBitrate, StartBitrate, MaxBitrate, nowUs);

		probes = probeController.SetEstimatedBitrate(
		  5000, RTC::BWE::ProbeController::BandwidthLimitedCause::DELAY_BASED_LIMITED, nowUs);

		probeController.SetLinkCapacityUpperBound(6000);

		nowUs += 5 * 1000 * 1000;

		probes = probeController.Process(nowUs);

		REQUIRE(probes.size() == 1);
		REQUIRE(probes[0].targetBitrate == 6000);

		nowUs += 5 * 1000 * 1000;

		probes = probeController.Process(nowUs);

		REQUIRE(probes.size() == 1);
		REQUIRE(probes[0].targetBitrate == 6000);
	}

	SECTION("and it's what bounds the burst when loss holds the estimate back")
	{
		const RTC::BWE::ProbeController::ProbeControllerOptions options{
			.networkStateEstimateProbingIntervalUs = 5 * 1000 * 1000
		};

		RTC::BWE::ProbeController probeController(options);

		REQUIRE(probeController.OnNetworkAvailability(/*networkAvailable*/ true, nowUs).empty());

		auto probes = probeController.SetBitrates(MinBitrate, StartBitrate, MaxBitrate, nowUs);

		probes = probeController.SetEstimatedBitrate(
		  500, RTC::BWE::ProbeController::BandwidthLimitedCause::DELAY_BASED_LIMITED, nowUs);

		probeController.SetLinkCapacityUpperBound(700);

		nowUs += 5 * 1000 * 1000;

		probes = probeController.Process(nowUs);

		REQUIRE(probes.size() == 1);

		probes = probeController.SetEstimatedBitrate(
		  500, RTC::BWE::ProbeController::BandwidthLimitedCause::LOSS_LIMITED_BWE_INCREASING, nowUs);

		nowUs += 5 * 1000 * 1000;

		probes = probeController.Process(nowUs);

		REQUIRE(!probes.empty());
		REQUIRE(probes[0].targetBitrate == 700);
	}

	SECTION("and it bounds the periodic bursts as soon as there is one")
	{
		const RTC::BWE::ProbeController::ProbeControllerOptions options{
			.networkStateEstimateProbingIntervalUs = 5 * 1000 * 1000
		};

		RTC::BWE::ProbeController probeController(options);

		REQUIRE(probeController.OnNetworkAvailability(/*networkAvailable*/ true, nowUs).empty());

		probeController.EnablePeriodicAlrProbing(true);

		auto probes = probeController.SetBitrates(MinBitrate, StartBitrate, MaxBitrate, nowUs);

		probes = probeController.SetEstimatedBitrate(
		  6000, RTC::BWE::ProbeController::BandwidthLimitedCause::DELAY_BASED_LIMITED, nowUs);

		probeController.SetAlrStartTimeUs(nowUs);

		// Twice the estimate is above the maximum, so the maximum is what it aims
		// for while nothing measured the link.
		nowUs += 5 * 1000 * 1000;

		probes = probeController.Process(nowUs);

		REQUIRE(probes.size() == 1);
		REQUIRE(probes[0].targetBitrate == MaxBitrate);

		probeController.SetLinkCapacityUpperBound(8000);

		nowUs += 5 * 1000 * 1000;

		probes = probeController.Process(nowUs);

		REQUIRE(probes.size() == 1);
		REQUIRE(probes[0].targetBitrate == 8000);
	}

	SECTION("and those bursts may be told to last longer than the usual ones")
	{
		const RTC::BWE::ProbeController::ProbeControllerOptions options{
			.networkStateEstimateProbingIntervalUs = 5 * 1000 * 1000, .networkStateProbeDurationUs = 100 * 1000
		};

		RTC::BWE::ProbeController probeController(options);

		REQUIRE(probeController.OnNetworkAvailability(/*networkAvailable*/ true, nowUs).empty());

		auto probes = probeController.SetBitrates(MinBitrate, StartBitrate, MaxBitrate, nowUs);

		probes = probeController.SetEstimatedBitrate(
		  5000, RTC::BWE::ProbeController::BandwidthLimitedCause::DELAY_BASED_LIMITED, nowUs);

		REQUIRE(!probes.empty());
		REQUIRE(probes[0].targetDurationUs < 100 * 1000);

		probeController.SetLinkCapacityUpperBound(6000);

		nowUs += 5 * 1000 * 1000;

		probes = probeController.Process(nowUs);

		REQUIRE(probes.size() == 1);
		REQUIRE(probes[0].targetDurationUs == 100 * 1000);
	}

	SECTION("loss growing back while not filling the link is worth a burst")
	{
		RTC::BWE::ProbeController probeController;

		REQUIRE(probeController.OnNetworkAvailability(/*networkAvailable*/ true, nowUs).empty());

		auto probes = probeController.SetBitrates(MinBitrate, StartBitrate, MaxBitrate, nowUs);

		probeController.EnablePeriodicAlrProbing(true);

		probes = probeController.SetEstimatedBitrate(
		  StartBitrate,
		  RTC::BWE::ProbeController::BandwidthLimitedCause::LOSS_LIMITED_BWE_INCREASING,
		  nowUs);

		nowUs += ExponentialProbingTimeoutUs;

		probes = probeController.Process(nowUs);

		REQUIRE(probes.empty());

		probeController.SetAlrStartTimeUs(nowUs);

		nowUs += (5 * 1000 * 1000) + 1000;

		probes = probeController.Process(nowUs);

		REQUIRE(probes.size() == 1);
		REQUIRE(probes.at(0).targetBitrate == 450);
	}

	SECTION("but loss that isn't growing back is not")
	{
		RTC::BWE::ProbeController probeController;

		REQUIRE(probeController.OnNetworkAvailability(/*networkAvailable*/ true, nowUs).empty());

		auto probes = probeController.SetBitrates(MinBitrate, StartBitrate, MaxBitrate, nowUs);

		probeController.EnablePeriodicAlrProbing(true);

		probes = probeController.SetEstimatedBitrate(
		  StartBitrate, RTC::BWE::ProbeController::BandwidthLimitedCause::LOSS_LIMITED_BWE, nowUs);

		nowUs += ExponentialProbingTimeoutUs;

		probes = probeController.Process(nowUs);

		REQUIRE(probes.empty());

		probeController.SetAlrStartTimeUs(nowUs);

		nowUs += (5 * 1000 * 1000) + 1000;

		probes = probeController.Process(nowUs);

		REQUIRE(probes.empty());
	}

	SECTION("nor is it worth one while the link is being filled")
	{
		RTC::BWE::ProbeController probeController;

		REQUIRE(probeController.OnNetworkAvailability(/*networkAvailable*/ true, nowUs).empty());

		auto probes = probeController.SetBitrates(MinBitrate, StartBitrate, MaxBitrate, nowUs);

		probeController.EnablePeriodicAlrProbing(true);

		probes = probeController.SetEstimatedBitrate(
		  StartBitrate,
		  RTC::BWE::ProbeController::BandwidthLimitedCause::LOSS_LIMITED_BWE_INCREASING,
		  nowUs);

		nowUs += ExponentialProbingTimeoutUs;

		probes = probeController.Process(nowUs);

		REQUIRE(probes.empty());

		probeController.SetAlrStartTimeUs(std::nullopt);

		nowUs += (5 * 1000 * 1000) + 1000;

		probes = probeController.Process(nowUs);

		REQUIRE(probes.empty());
	}

	SECTION("a burst that stays below the measured link capacity leads to another one")
	{
		const RTC::BWE::ProbeController::ProbeControllerOptions options{
			.networkStateEstimateProbingIntervalUs = 5 * 1000 * 1000
		};

		RTC::BWE::ProbeController probeController(options);

		REQUIRE(probeController.OnNetworkAvailability(/*networkAvailable*/ true, nowUs).empty());

		auto probes = probeController.SetBitrates(MinBitrate, StartBitrate, MaxBitrate, nowUs);

		REQUIRE(!probes.empty());

		// A second has to pass before another burst can be asked for.
		nowUs += 1100 * 1000;

		probes = probeController.Process(nowUs);

		REQUIRE(probes.empty());

		probeController.SetLinkCapacityUpperBound(5 * StartBitrate);

		nowUs += 5 * 1000 * 1000;

		probes = probeController.Process(nowUs);

		REQUIRE(!probes.empty());
		REQUIRE(probes[0].targetBitrate < 5 * StartBitrate);

		const auto probeTargetBitrate = probes[0].targetBitrate;

		// The estimate reaching exactly what the burst aimed for is a result good
		// enough to keep climbing.
		probes = probeController.SetEstimatedBitrate(
		  probeTargetBitrate, RTC::BWE::ProbeController::BandwidthLimitedCause::DELAY_BASED_LIMITED, nowUs);

		REQUIRE(!probes.empty());
		REQUIRE(probes[0].targetBitrate == 2 * probeTargetBitrate);
	}

	SECTION("an estimate far below the measured link capacity is probed more often")
	{
		const RTC::BWE::ProbeController::ProbeControllerOptions options{
			.probeIfEstimateLowerThanNetworkStateEstimateRatio      = 0.5,
			.estimateLowerThanNetworkStateEstimateProbingIntervalUs = 1000 * 1000
		};

		RTC::BWE::ProbeController probeController(options);

		REQUIRE(probeController.OnNetworkAvailability(/*networkAvailable*/ true, nowUs).empty());

		auto probes = probeController.SetBitrates(MinBitrate, StartBitrate, MaxBitrate, nowUs);

		probes = probeController.SetEstimatedBitrate(
		  StartBitrate, RTC::BWE::ProbeController::BandwidthLimitedCause::DELAY_BASED_LIMITED, nowUs);

		// A second has to pass before another burst can be asked for.
		nowUs += 1100 * 1000;

		probes = probeController.Process(nowUs);

		REQUIRE(probes.empty());

		// The estimate is not below half of what was measured, so there is nothing
		// to look for.
		probeController.SetLinkCapacityUpperBound(StartBitrate);

		probes = probeController.Process(nowUs);

		REQUIRE(probes.empty());

		probeController.SetLinkCapacityUpperBound(StartBitrate * 3);

		probes = probeController.Process(nowUs);

		REQUIRE(probes.size() == 1);
		REQUIRE(probes[0].targetBitrate > StartBitrate);

		// And it keeps going while the estimate stays that low.
		nowUs += 1100 * 1000;

		probes = probeController.Process(nowUs);

		REQUIRE(!probes.empty());

		// Once the estimate comes up, the periodic bursts stop, even though the
		// result of this one may still lead to another.
		probes = probeController.SetEstimatedBitrate(
		  2 * StartBitrate, RTC::BWE::ProbeController::BandwidthLimitedCause::DELAY_BASED_LIMITED, nowUs);

		nowUs += 1100 * 1000;

		probes = probeController.Process(nowUs);

		REQUIRE(probes.empty());
	}

	SECTION("a good result leads to another burst only while loss isn't the limit")
	{
		const RTC::BWE::ProbeController::ProbeControllerOptions options{
			.networkStateEstimateProbingIntervalUs = 5 * 1000 * 1000
		};

		RTC::BWE::ProbeController probeController(options);

		REQUIRE(probeController.OnNetworkAvailability(/*networkAvailable*/ true, nowUs).empty());

		auto probes = probeController.SetBitrates(MinBitrate, StartBitrate, MaxBitrate, nowUs);

		REQUIRE(!probes.empty());

		// A second has to pass before another burst can be asked for.
		nowUs += 1100 * 1000;

		probes = probeController.Process(nowUs);

		REQUIRE(probes.empty());

		probeController.SetLinkCapacityUpperBound(3 * StartBitrate);

		nowUs += 5 * 1000 * 1000;

		probes = probeController.Process(nowUs);

		REQUIRE(!probes.empty());
		REQUIRE(probes[0].targetBitrate < 3 * StartBitrate);

		probes = probeController.SetEstimatedBitrate(
		  probes[0].targetBitrate,
		  RTC::BWE::ProbeController::BandwidthLimitedCause::LOSS_LIMITED_BWE,
		  nowUs);

		REQUIRE(probes.empty());
	}

	SECTION("and while the delay is, it climbs up to what was measured")
	{
		const RTC::BWE::ProbeController::ProbeControllerOptions options{
			.networkStateEstimateProbingIntervalUs = 5 * 1000 * 1000
		};

		RTC::BWE::ProbeController probeController(options);

		REQUIRE(probeController.OnNetworkAvailability(/*networkAvailable*/ true, nowUs).empty());

		auto probes = probeController.SetBitrates(MinBitrate, StartBitrate, MaxBitrate, nowUs);

		REQUIRE(!probes.empty());

		// A second has to pass before another burst can be asked for.
		nowUs += 1100 * 1000;

		probes = probeController.Process(nowUs);

		REQUIRE(probes.empty());

		probeController.SetLinkCapacityUpperBound(3 * StartBitrate);

		nowUs += 5 * 1000 * 1000;

		probes = probeController.Process(nowUs);

		REQUIRE(!probes.empty());
		REQUIRE(probes[0].targetBitrate < 3 * StartBitrate);

		probes = probeController.SetEstimatedBitrate(
		  probes[0].targetBitrate,
		  RTC::BWE::ProbeController::BandwidthLimitedCause::DELAY_BASED_LIMITED,
		  nowUs);

		REQUIRE(!probes.empty());
		REQUIRE(probes[0].targetBitrate == 3 * StartBitrate);
	}

	SECTION("room appearing while a burst is in flight is looked for once it times out")
	{
		const RTC::BWE::ProbeController::ProbeControllerOptions options{
			.networkStateEstimateProbingIntervalUs                  = 5 * 1000 * 1000,
			.probeIfEstimateLowerThanNetworkStateEstimateRatio      = 0.7,
			.estimateLowerThanNetworkStateEstimateProbingIntervalUs = 3 * 1000 * 1000
		};

		RTC::BWE::ProbeController probeController(options);

		REQUIRE(probeController.OnNetworkAvailability(/*networkAvailable*/ true, nowUs).empty());

		auto probes = probeController.SetBitrates(MinBitrate, StartBitrate, MaxBitrate, nowUs);

		REQUIRE(!probes.empty());

		const auto firstProbeBitrate = probes[0].targetBitrate;

		probeController.SetLinkCapacityUpperBound((firstProbeBitrate * 12 / 10) / 2);

		// Half of what the burst aimed for is too poor a result to keep climbing
		// right away.
		probes = probeController.SetEstimatedBitrate(
		  firstProbeBitrate / 2,
		  RTC::BWE::ProbeController::BandwidthLimitedCause::DELAY_BASED_LIMITED,
		  nowUs);

		REQUIRE(probes.empty());

		nowUs += 5 * 1000 * 1000;

		probes = probeController.Process(nowUs);

		REQUIRE(!probes.empty());
		REQUIRE(probes[0].targetBitrate <= (firstProbeBitrate * 12 / 10) / 2);

		// The link turning out to be wider than thought is worth looking into even
		// before this burst's result arrives, once its own interval has passed.
		probeController.SetLinkCapacityUpperBound(3 * StartBitrate);

		probes = probeController.SetEstimatedBitrate(
		  probes[0].targetBitrate,
		  RTC::BWE::ProbeController::BandwidthLimitedCause::DELAY_BASED_LIMITED,
		  nowUs);

		REQUIRE(probes.empty());

		nowUs += 3 * 1000 * 1000;

		probes = probeController.Process(nowUs);

		REQUIRE(!probes.empty());

		// And once the estimate is close to what was measured there is nothing
		// left to look for.
		probes = probeController.SetEstimatedBitrate(
		  (3 * StartBitrate * 9) / 10,
		  RTC::BWE::ProbeController::BandwidthLimitedCause::DELAY_BASED_LIMITED,
		  nowUs);

		REQUIRE(probes.empty());
	}

	SECTION("having already aimed at the maximum, another burst waits its turn")
	{
		const RTC::BWE::ProbeController::ProbeControllerOptions options{
			.networkStateEstimateProbingIntervalUs = 2 * 1000 * 1000,
			.skipIfEstimateLargerThanFractionOfMax = 0.9
		};

		RTC::BWE::ProbeController probeController(options);

		REQUIRE(probeController.OnNetworkAvailability(/*networkAvailable*/ true, nowUs).empty());

		auto probes = probeController.SetBitrates(MinBitrate, StartBitrate, MaxBitrate, nowUs);

		REQUIRE(!probes.empty());

		probeController.SetLinkCapacityUpperBound(2 * MaxBitrate);

		probes = probeController.SetEstimatedBitrate(
		  (MaxBitrate * 8) / 10,
		  RTC::BWE::ProbeController::BandwidthLimitedCause::DELAY_BASED_LIMITED,
		  nowUs);

		REQUIRE(!probes.empty());
		REQUIRE(probes[0].targetBitrate == MaxBitrate);

		// Its result arriving changes nothing, since there is nothing above the
		// maximum to aim at.
		probes = probeController.SetEstimatedBitrate(
		  (MaxBitrate * 8) / 10,
		  RTC::BWE::ProbeController::BandwidthLimitedCause::DELAY_BASED_LIMITED,
		  nowUs);

		REQUIRE(probes.empty());

		nowUs += 1000 * 1000;

		probes = probeController.Process(nowUs);

		REQUIRE(probes.empty());

		nowUs += 1000 * 1000;

		probes = probeController.Process(nowUs);

		REQUIRE(!probes.empty());
	}

	SECTION("what the application wants to send survives starting over")
	{
		RTC::BWE::ProbeController probeController;

		REQUIRE(probeController.OnNetworkAvailability(/*networkAvailable*/ true, nowUs).empty());

		auto probes = probeController.SetBitrates(MinBitrate, StartBitrate, MaxBitrate, nowUs);

		REQUIRE(!probes.empty());

		probes = probeController.OnMaxTotalAllocatedBitrate(StartBitrate / 4, nowUs);

		probeController.Reset(nowUs);

		probes = probeController.SetBitrates(MinBitrate, StartBitrate, MaxBitrate, nowUs);

		REQUIRE(!probes.empty());
		REQUIRE(probes[0].targetBitrate == StartBitrate / 4 * 2);
	}

	SECTION("an estimate already close to the maximum is not worth a burst")
	{
		const RTC::BWE::ProbeController::ProbeControllerOptions options{
			.skipIfEstimateLargerThanFractionOfMax = 0.9
		};

		RTC::BWE::ProbeController probeController(options);

		REQUIRE(probeController.OnNetworkAvailability(/*networkAvailable*/ true, nowUs).empty());

		probeController.EnablePeriodicAlrProbing(true);

		auto probes = probeController.SetBitrates(MinBitrate, StartBitrate, MaxBitrate, nowUs);

		REQUIRE(!probes.empty());

		probes = probeController.SetEstimatedBitrate(
		  MaxBitrate, RTC::BWE::ProbeController::BandwidthLimitedCause::DELAY_BASED_LIMITED, nowUs);

		REQUIRE(probes.empty());

		probeController.SetAlrStartTimeUs(nowUs);

		nowUs += 10 * 1000 * 1000;

		probes = probeController.Process(nowUs);

		REQUIRE(probes.empty());

		// Until the maximum goes up and there is room above it again.
		probes = probeController.SetBitrates(MinBitrate, StartBitrate, 2 * MaxBitrate, nowUs);

		REQUIRE(!probes.empty());
	}

	SECTION("and neither is one already close to what the application wants to send")
	{
		const RTC::BWE::ProbeController::ProbeControllerOptions options{
			.skipIfEstimateLargerThanFractionOfMax = 1.0
		};

		RTC::BWE::ProbeController probeController(options);

		REQUIRE(probeController.OnNetworkAvailability(/*networkAvailable*/ true, nowUs).empty());

		probeController.EnablePeriodicAlrProbing(true);

		auto probes = probeController.SetBitrates(MinBitrate, StartBitrate, MaxBitrate, nowUs);

		REQUIRE(!probes.empty());

		probes = probeController.SetEstimatedBitrate(
		  MaxBitrate / 2, RTC::BWE::ProbeController::BandwidthLimitedCause::DELAY_BASED_LIMITED, nowUs);

		nowUs += 10 * 1000 * 1000;

		probeController.SetAlrStartTimeUs(nowUs);

		probes = probeController.OnMaxTotalAllocatedBitrate(MaxBitrate / 2, nowUs);

		REQUIRE(probes.empty());

		nowUs += 2 * 1000 * 1000;

		probes = probeController.Process(nowUs);

		REQUIRE(probes.empty());

		// Until it wants a single bit more than is already being sent.
		probes = probeController.OnMaxTotalAllocatedBitrate((MaxBitrate / 2) + 1, nowUs);

		REQUIRE(!probes.empty());
	}

	SECTION("nor is the burst that a measured link capacity would be worth")
	{
		const RTC::BWE::ProbeController::ProbeControllerOptions options{
			.networkStateEstimateProbingIntervalUs = 2 * 1000 * 1000,
			.skipIfEstimateLargerThanFractionOfMax = 0.9
		};

		RTC::BWE::ProbeController probeController(options);

		REQUIRE(probeController.OnNetworkAvailability(/*networkAvailable*/ true, nowUs).empty());

		auto probes = probeController.SetBitrates(MinBitrate, StartBitrate, MaxBitrate, nowUs);

		REQUIRE(!probes.empty());

		probeController.SetLinkCapacityUpperBound(2 * MaxBitrate);

		probes = probeController.SetEstimatedBitrate(
		  MaxBitrate, RTC::BWE::ProbeController::BandwidthLimitedCause::DELAY_BASED_LIMITED, nowUs);

		REQUIRE(probes.empty());

		nowUs += 10 * 1000 * 1000;

		probes = probeController.Process(nowUs);

		REQUIRE(probes.empty());
	}

	SECTION("but one that stays under the measured link capacity is, and lasts longer")
	{
		const RTC::BWE::ProbeController::ProbeControllerOptions options{
			.networkStateEstimateProbingIntervalUs = 2 * 1000 * 1000,
			.networkStateProbeDurationUs           = 100 * 1000,
			.networkStateMinProbeDeltaUs           = 20 * 1000,
			.skipIfEstimateLargerThanFractionOfMax = 0.9
		};

		RTC::BWE::ProbeController probeController(options);

		REQUIRE(probeController.OnNetworkAvailability(/*networkAvailable*/ true, nowUs).empty());

		auto probes = probeController.SetBitrates(MinBitrate, StartBitrate, MaxBitrate, nowUs);

		REQUIRE(!probes.empty());

		probeController.SetLinkCapacityUpperBound(2 * MaxBitrate);

		probes = probeController.SetEstimatedBitrate(
		  MaxBitrate, RTC::BWE::ProbeController::BandwidthLimitedCause::DELAY_BASED_LIMITED, nowUs);

		REQUIRE(probes.empty());

		// Two seconds have to pass before another burst can be asked for.
		nowUs += 2100 * 1000;

		probes = probeController.SetEstimatedBitrate(
		  StartBitrate, RTC::BWE::ProbeController::BandwidthLimitedCause::DELAY_BASED_LIMITED, nowUs);

		REQUIRE(probes.empty());

		probeController.SetLinkCapacityUpperBound(2 * StartBitrate);

		probes = probeController.Process(nowUs);

		REQUIRE(!probes.empty());
		REQUIRE(probes[0].targetBitrate <= 2 * StartBitrate);
		REQUIRE(probes[0].minProbeDeltaUs == 20 * 1000);
		REQUIRE(probes[0].targetDurationUs == 100 * 1000);
	}

	SECTION("a measured link capacity below the estimate doesn't bring the burst down")
	{
		const RTC::BWE::ProbeController::ProbeControllerOptions options{
			.networkStateEstimateProbingIntervalUs = 5 * 1000 * 1000,
			.networkStateProbeDurationUs           = 100 * 1000,
			.networkStateMinProbeDeltaUs           = 20 * 1000
		};

		RTC::BWE::ProbeController probeController(options);

		REQUIRE(probeController.OnNetworkAvailability(/*networkAvailable*/ true, nowUs).empty());

		probeController.EnablePeriodicAlrProbing(true);

		auto probes = probeController.SetBitrates(MinBitrate, StartBitrate, MaxBitrate, nowUs);

		probes = probeController.SetEstimatedBitrate(
		  StartBitrate, RTC::BWE::ProbeController::BandwidthLimitedCause::DELAY_BASED_LIMITED, nowUs);

		probeController.SetLinkCapacityUpperBound(StartBitrate);

		// A second has to pass before another burst can be asked for.
		nowUs += 1100 * 1000;

		probes = probeController.Process(nowUs);

		REQUIRE(probes.empty());

		probeController.SetAlrStartTimeUs(nowUs);
		probeController.SetLinkCapacityUpperBound(StartBitrate / 2);

		nowUs += 6 * 1000 * 1000;

		probes = probeController.Process(nowUs);

		REQUIRE(!probes.empty());
		REQUIRE(probes[0].targetBitrate == StartBitrate);

		// And since it isn't what bounds the burst, neither are its durations.
		REQUIRE(probes[0].minProbeDeltaUs == 2 * 1000);
		REQUIRE(probes[0].targetDurationUs == 15 * 1000);
	}

	SECTION("a delay that is growing right now is no moment to probe")
	{
		RTC::BWE::ProbeController probeController;

		REQUIRE(probeController.OnNetworkAvailability(/*networkAvailable*/ true, nowUs).empty());

		auto probes = probeController.SetBitrates(MinBitrate, StartBitrate, MaxBitrate, nowUs);

		REQUIRE(!probes.empty());

		// A second has to pass before another burst can be asked for.
		nowUs += 1100 * 1000;

		probes = probeController.Process(nowUs);

		REQUIRE(probes.empty());

		probeController.SetLinkCapacityUpperBound(3 * StartBitrate);

		probes = probeController.SetEstimatedBitrate(
		  StartBitrate,
		  RTC::BWE::ProbeController::BandwidthLimitedCause::DELAY_BASED_LIMITED_DELAY_INCREASED,
		  nowUs);

		REQUIRE(probes.empty());

		nowUs += 5 * 1000 * 1000;

		probes = probeController.Process(nowUs);

		REQUIRE(probes.empty());
	}

	SECTION("and neither is a round trip time long enough to mean full queues")
	{
		RTC::BWE::ProbeController probeController;

		REQUIRE(probeController.OnNetworkAvailability(/*networkAvailable*/ true, nowUs).empty());

		auto probes = probeController.SetBitrates(MinBitrate, StartBitrate, MaxBitrate, nowUs);

		REQUIRE(!probes.empty());

		// A second has to pass before another burst can be asked for.
		nowUs += 1100 * 1000;

		probes = probeController.Process(nowUs);

		REQUIRE(probes.empty());

		probeController.SetLinkCapacityUpperBound(3 * StartBitrate);

		probes = probeController.SetEstimatedBitrate(
		  StartBitrate,
		  RTC::BWE::ProbeController::BandwidthLimitedCause::RTT_BASED_BACK_OFF_HIGH_RTT,
		  nowUs);

		REQUIRE(probes.empty());

		nowUs += 5 * 1000 * 1000;

		probes = probeController.Process(nowUs);

		REQUIRE(probes.empty());
	}
}
