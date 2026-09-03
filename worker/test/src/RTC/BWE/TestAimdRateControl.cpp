#include "common.hpp"
#include "RTC/BWE/AimdRateControl.hpp"
#include "RTC/BWE/BweTypes.hpp"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <cmath>

SCENARIO("BWE AimdRateControl", "[bwe][aimdratecontrol]")
{
	constexpr int64_t InitialTimeUs{ 123456 * 1000 };
	// Fraction of the measured throughput the bitrate is dropped to after an
	// overuse.
	constexpr double FractionAfterOveruse{ 0.85 };

	// Options of a rate control running in the sending endpoint.
	const RTC::BWE::AimdRateControl::AimdRateControlOptions sendSideOptions{ .sendSide = true };

	SECTION("the increase rate has a floor of 4 kbps per second")
	{
		RTC::BWE::AimdRateControl aimdRateControl;

		aimdRateControl.SetEstimate(30000, InitialTimeUs);

		REQUIRE(aimdRateControl.GetNearMaxIncreaseRateBpsPerSecond() == 4000);
	}

	SECTION("the increase rate is 5 kbps per second at 90 kbps and 200 ms of RTT")
	{
		RTC::BWE::AimdRateControl aimdRateControl;

		aimdRateControl.SetEstimate(90000, InitialTimeUs);

		REQUIRE(aimdRateControl.GetNearMaxIncreaseRateBpsPerSecond() == 5000);
	}

	SECTION("the increase rate is 5 kbps per second at 60 kbps and 100 ms of RTT")
	{
		RTC::BWE::AimdRateControl aimdRateControl;

		aimdRateControl.SetEstimate(60000, InitialTimeUs);
		aimdRateControl.SetRtt(100 * 1000);

		REQUIRE(aimdRateControl.GetNearMaxIncreaseRateBpsPerSecond() == 5000);
	}

	SECTION("the increase rate grows with the bitrate")
	{
		constexpr int64_t Bitrate{ 300000 };

		RTC::BWE::AimdRateControl aimdRateControl;

		aimdRateControl.SetEstimate(Bitrate, InitialTimeUs);
		aimdRateControl.Update(
		  { .bandwidthUsage = RTC::BWE::Types::BandwidthUsage::OVERUSING, .estimatedThroughput = Bitrate },
		  InitialTimeUs);

		REQUIRE_THAT(
		  aimdRateControl.GetNearMaxIncreaseRateBpsPerSecond(), Catch::Matchers::WithinAbs(14000, 1000));
	}

	SECTION("the bitrate is limited by the acknowledged one")
	{
		constexpr int64_t AckedBitrate{ 10000 };

		RTC::BWE::AimdRateControl aimdRateControl;

		int64_t nowUs{ InitialTimeUs };

		aimdRateControl.SetEstimate(AckedBitrate, nowUs);

		while (nowUs - InitialTimeUs < 20 * 1000 * 1000)
		{
			aimdRateControl.Update(
			  { .bandwidthUsage      = RTC::BWE::Types::BandwidthUsage::NORMAL,
				  .estimatedThroughput = AckedBitrate },
			  nowUs);

			nowUs += 100 * 1000;
		}

		REQUIRE(aimdRateControl.IsValidEstimate());
		REQUIRE(aimdRateControl.GetLatestEstimate() == (3 * AckedBitrate / 2) + 10000);
	}

	SECTION("a decreasing acknowledged bitrate doesn't drag the estimate down")
	{
		constexpr int64_t AckedBitrate{ 10000 };

		RTC::BWE::AimdRateControl aimdRateControl;

		int64_t nowUs{ InitialTimeUs };

		aimdRateControl.SetEstimate(AckedBitrate, nowUs);

		while (nowUs - InitialTimeUs < 20 * 1000 * 1000)
		{
			aimdRateControl.Update(
			  { .bandwidthUsage      = RTC::BWE::Types::BandwidthUsage::NORMAL,
				  .estimatedThroughput = AckedBitrate },
			  nowUs);

			nowUs += 100 * 1000;
		}

		REQUIRE(aimdRateControl.IsValidEstimate());

		// The estimate must not be reduced to 1.5x what is being acknowledged, but
		// it must not grow any further either.
		const int64_t prevEstimate = aimdRateControl.GetLatestEstimate();

		aimdRateControl.Update(
		  { .bandwidthUsage      = RTC::BWE::Types::BandwidthUsage::NORMAL,
			  .estimatedThroughput = AckedBitrate / 2 },
		  nowUs);

		REQUIRE(aimdRateControl.GetLatestEstimate() == prevEstimate);
		// And it's still the value it had grown to, rather than both being wrong.
		REQUIRE_THAT(
		  static_cast<double>(aimdRateControl.GetLatestEstimate()),
		  Catch::Matchers::WithinAbs((1.5 * AckedBitrate) + 10000, 2000));
	}

	SECTION("an overuse drops the bitrate to the backoff fraction of the throughput")
	{
		constexpr int64_t InitialBitrate{ 264000 };
		constexpr int64_t UpdatedBitrate{ 216000 };

		RTC::BWE::AimdRateControl aimdRateControl;

		const auto ackedBitrate =
		  static_cast<int64_t>(std::llround((UpdatedBitrate + 5000) / FractionAfterOveruse));

		int64_t nowUs{ InitialTimeUs };

		aimdRateControl.SetEstimate(InitialBitrate, nowUs);

		nowUs += 100 * 1000;

		aimdRateControl.Update(
		  { .bandwidthUsage      = RTC::BWE::Types::BandwidthUsage::OVERUSING,
			  .estimatedThroughput = ackedBitrate },
		  nowUs);

		REQUIRE(aimdRateControl.GetLatestEstimate() == UpdatedBitrate);
		// The increase rate at 216 kbps is 12 kbps per second.
		REQUIRE(aimdRateControl.GetNearMaxIncreaseRateBpsPerSecond() == 12000);
	}

	SECTION("the bitrate stays bounded while no throughput is measured")
	{
		constexpr int64_t InitialBitrate{ 123000 };

		RTC::BWE::AimdRateControl aimdRateControl;

		int64_t nowUs{ InitialTimeUs };

		aimdRateControl.Update(
		  { .bandwidthUsage      = RTC::BWE::Types::BandwidthUsage::NORMAL,
			  .estimatedThroughput = InitialBitrate },
		  nowUs);

		// The initial bitrate is taken from what has been measured for five seconds.
		nowUs += (5 * 1000 * 1000) + 1000;

		aimdRateControl.Update(
		  { .bandwidthUsage      = RTC::BWE::Types::BandwidthUsage::NORMAL,
			  .estimatedThroughput = InitialBitrate },
		  nowUs);

		for (int i{ 0 }; i < 100; ++i)
		{
			aimdRateControl.Update(
			  { .bandwidthUsage      = RTC::BWE::Types::BandwidthUsage::NORMAL,
				  .estimatedThroughput = std::nullopt },
			  nowUs);

			nowUs += 100 * 1000;
		}

		REQUIRE(aimdRateControl.GetLatestEstimate() <= (3 * InitialBitrate / 2) + 10000);
	}

	SECTION("the estimate doesn't increase in ALR when so configured")
	{
		constexpr int64_t InitialBitrate{ 123000 };

		// While in an application limited region the network gives no feedback that
		// could tell whether a higher estimate is correct, so it must not grow.
		auto options = sendSideOptions;

		options.noBitrateIncreaseInAlr = true;

		RTC::BWE::AimdRateControl aimdRateControl(options);

		int64_t nowUs{ InitialTimeUs };

		aimdRateControl.SetEstimate(InitialBitrate, nowUs);
		aimdRateControl.SetInApplicationLimitedRegion(true);
		aimdRateControl.Update(
		  { .bandwidthUsage      = RTC::BWE::Types::BandwidthUsage::NORMAL,
			  .estimatedThroughput = InitialBitrate },
		  nowUs);

		REQUIRE(aimdRateControl.GetLatestEstimate() == InitialBitrate);

		for (int i{ 0 }; i < 100; ++i)
		{
			aimdRateControl.Update(
			  { .bandwidthUsage      = RTC::BWE::Types::BandwidthUsage::NORMAL,
				  .estimatedThroughput = std::nullopt },
			  nowUs);

			nowUs += 100 * 1000;
		}

		REQUIRE(aimdRateControl.GetLatestEstimate() == InitialBitrate);
	}

	SECTION("the estimate can still be set from outside while in ALR")
	{
		constexpr int64_t InitialBitrate{ 123000 };

		auto options = sendSideOptions;

		options.noBitrateIncreaseInAlr = true;

		RTC::BWE::AimdRateControl aimdRateControl(options);

		aimdRateControl.SetEstimate(InitialBitrate, InitialTimeUs);
		aimdRateControl.SetInApplicationLimitedRegion(true);

		REQUIRE(aimdRateControl.GetLatestEstimate() == InitialBitrate);

		aimdRateControl.SetEstimate(2 * InitialBitrate, InitialTimeUs);

		REQUIRE(aimdRateControl.GetLatestEstimate() == 2 * InitialBitrate);
	}

	SECTION("the estimate increases while not in ALR")
	{
		constexpr int64_t InitialBitrate{ 123000 };

		// Not being in ALR must let the estimate grow, so that it cannot get stuck
		// at a given bitrate.
		auto options = sendSideOptions;

		options.noBitrateIncreaseInAlr = true;

		RTC::BWE::AimdRateControl aimdRateControl(options);

		int64_t nowUs{ InitialTimeUs };

		aimdRateControl.SetEstimate(InitialBitrate, nowUs);
		aimdRateControl.SetInApplicationLimitedRegion(false);
		aimdRateControl.Update(
		  { .bandwidthUsage      = RTC::BWE::Types::BandwidthUsage::NORMAL,
			  .estimatedThroughput = InitialBitrate },
		  nowUs);

		for (int i{ 0 }; i < 100; ++i)
		{
			aimdRateControl.Update(
			  { .bandwidthUsage      = RTC::BWE::Types::BandwidthUsage::NORMAL,
				  .estimatedThroughput = std::nullopt },
			  nowUs);

			nowUs += 100 * 1000;
		}

		REQUIRE(aimdRateControl.GetLatestEstimate() > InitialBitrate);
	}

	SECTION("the estimate is upper limited by the network state estimate")
	{
		constexpr int64_t LinkCapacityUpper{ 400000 };

		const RTC::BWE::Types::NetworkStateEstimate networkEstimate{ .linkCapacityUpper =
		                                                               LinkCapacityUpper };

		RTC::BWE::AimdRateControl aimdRateControl(sendSideOptions);

		aimdRateControl.SetEstimate(300000, InitialTimeUs);
		aimdRateControl.SetNetworkStateEstimate(networkEstimate);
		aimdRateControl.SetEstimate(500000, InitialTimeUs);

		REQUIRE(aimdRateControl.GetLatestEstimate() == LinkCapacityUpper);
	}

	SECTION("the current bitrate is the lowest possible upper limit")
	{
		constexpr int64_t CurrentBitrate{ 500000 };

		// Below the current bitrate, so it must not drag it down.
		const RTC::BWE::Types::NetworkStateEstimate networkEstimate{ .linkCapacityUpper = 300000 };

		RTC::BWE::AimdRateControl aimdRateControl(sendSideOptions);

		aimdRateControl.SetEstimate(CurrentBitrate, InitialTimeUs);

		REQUIRE(aimdRateControl.GetLatestEstimate() == CurrentBitrate);

		aimdRateControl.SetNetworkStateEstimate(networkEstimate);
		aimdRateControl.SetEstimate(700000, InitialTimeUs);

		REQUIRE(aimdRateControl.GetLatestEstimate() == CurrentBitrate);
	}

	SECTION("the current bitrate is not the lowest possible upper limit when so configured")
	{
		constexpr int64_t LinkCapacityUpper{ 300000 };

		const RTC::BWE::Types::NetworkStateEstimate networkEstimate{ .linkCapacityUpper =
		                                                               LinkCapacityUpper };

		auto options = sendSideOptions;

		options.useCurrentEstimateAsMinUpperBound = false;

		RTC::BWE::AimdRateControl aimdRateControl(options);

		aimdRateControl.SetEstimate(500000, InitialTimeUs);

		REQUIRE(aimdRateControl.GetLatestEstimate() == 500000);

		aimdRateControl.SetNetworkStateEstimate(networkEstimate);
		aimdRateControl.SetEstimate(700000, InitialTimeUs);

		REQUIRE(aimdRateControl.GetLatestEstimate() == LinkCapacityUpper);
	}

	SECTION("the estimate is lower limited by the network state estimate")
	{
		constexpr int64_t LinkCapacityLower{ 400000 };

		const RTC::BWE::Types::NetworkStateEstimate networkEstimate{ .linkCapacityLower =
		                                                               LinkCapacityLower };

		RTC::BWE::AimdRateControl aimdRateControl(sendSideOptions);

		aimdRateControl.SetNetworkStateEstimate(networkEstimate);
		aimdRateControl.SetEstimate(100000, InitialTimeUs);

		REQUIRE(
		  aimdRateControl.GetLatestEstimate() == std::llround(LinkCapacityLower * FractionAfterOveruse));
	}

	SECTION("setting an estimate below the network state estimate is ignored")
	{
		constexpr int64_t CurrentBitrate{ 200000 };

		const RTC::BWE::Types::NetworkStateEstimate networkEstimate{ .linkCapacityLower = 400000 };

		RTC::BWE::AimdRateControl aimdRateControl(sendSideOptions);

		aimdRateControl.SetEstimate(CurrentBitrate, InitialTimeUs);

		REQUIRE(aimdRateControl.GetLatestEstimate() == CurrentBitrate);

		aimdRateControl.SetNetworkStateEstimate(networkEstimate);

		// Ignored, since it's lower than the backoff fraction of the network state
		// estimate.
		aimdRateControl.SetEstimate(100000, InitialTimeUs);

		REQUIRE(aimdRateControl.GetLatestEstimate() == CurrentBitrate);
	}

	SECTION("the estimate is not limited by the network state estimate when so configured")
	{
		constexpr int64_t InitialBitrate{ 123000 };
		constexpr int64_t LinkCapacityUpper{ 150000 };

		const RTC::BWE::Types::NetworkStateEstimate networkEstimate{ .linkCapacityUpper =
		                                                               LinkCapacityUpper };

		auto options = sendSideOptions;

		options.estimateBoundedIncrease = false;

		RTC::BWE::AimdRateControl aimdRateControl(options);

		int64_t nowUs{ InitialTimeUs };

		aimdRateControl.SetEstimate(InitialBitrate, nowUs);
		aimdRateControl.SetInApplicationLimitedRegion(false);
		aimdRateControl.SetNetworkStateEstimate(networkEstimate);

		for (int i{ 0 }; i < 100; ++i)
		{
			aimdRateControl.Update(
			  { .bandwidthUsage      = RTC::BWE::Types::BandwidthUsage::NORMAL,
				  .estimatedThroughput = std::nullopt },
			  nowUs);

			nowUs += 100 * 1000;
		}

		REQUIRE(aimdRateControl.GetLatestEstimate() > LinkCapacityUpper);
	}
}
