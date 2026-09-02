#include "common.hpp"
#include "RTC/BWE/BweTypes.hpp"
#include "RTC/BWE/LinkCapacityEstimator.hpp"
#include <catch2/catch_test_macros.hpp>

SCENARIO("BWE LinkCapacityEstimator", "[bwe][linkcapacityestimator]")
{
	SECTION("there is no estimate until a sample is fed")
	{
		const RTC::BWE::LinkCapacityEstimator linkCapacityEstimator;

		REQUIRE(!linkCapacityEstimator.GetEstimate().has_value());
		// Without an estimate there is no interval either, so the bounds must not
		// constrain anything.
		REQUIRE(linkCapacityEstimator.GetUpperBound() == RTC::BWE::Types::BitrateInfinite);
		REQUIRE(linkCapacityEstimator.GetLowerBound() == 0);
	}

	SECTION("the first sample is taken as is")
	{
		RTC::BWE::LinkCapacityEstimator linkCapacityEstimator;

		linkCapacityEstimator.OnOveruseDetected(500000);

		REQUIRE(linkCapacityEstimator.GetEstimate() == 500000);
	}

	SECTION("an overuse sample moves the estimate by 5%")
	{
		RTC::BWE::LinkCapacityEstimator linkCapacityEstimator;

		linkCapacityEstimator.OnOveruseDetected(500000);
		linkCapacityEstimator.OnOveruseDetected(1000000);

		// 0.95 * 500 kbps + 0.05 * 1000 kbps.
		REQUIRE(linkCapacityEstimator.GetEstimate() == 525000);
	}

	SECTION("a probe sample moves the estimate by 50%")
	{
		RTC::BWE::LinkCapacityEstimator linkCapacityEstimator;

		linkCapacityEstimator.OnOveruseDetected(500000);
		// A probe measures the capacity directly, so it's trusted far more than an
		// overuse, which only tells that the capacity was exceeded.
		linkCapacityEstimator.OnProbeRate(1000000);

		// 0.5 * 500 kbps + 0.5 * 1000 kbps.
		REQUIRE(linkCapacityEstimator.GetEstimate() == 750000);
	}

	SECTION("samples are taken with a resolution of 1 kbps")
	{
		RTC::BWE::LinkCapacityEstimator roundedDown;
		RTC::BWE::LinkCapacityEstimator roundedUp;

		roundedDown.OnOveruseDetected(500499);
		roundedUp.OnOveruseDetected(500500);

		REQUIRE(roundedDown.GetEstimate() == 500000);
		REQUIRE(roundedUp.GetEstimate() == 501000);
	}

	SECTION("the bounds bracket the estimate by three standard deviations")
	{
		RTC::BWE::LinkCapacityEstimator linkCapacityEstimator;

		linkCapacityEstimator.OnOveruseDetected(500000);

		// A single sample matching the estimate leaves the tracked variance at its
		// minimum of 0.4, so the standard deviation is sqrt(0.4 * 500) = 14.14 kbps
		// and the interval is 42.43 kbps wide on each side.
		REQUIRE(linkCapacityEstimator.GetUpperBound() == 542426);
		REQUIRE(linkCapacityEstimator.GetLowerBound() == 457573);
	}

	SECTION("a sample far from the estimate widens the bounds")
	{
		RTC::BWE::LinkCapacityEstimator linkCapacityEstimator;

		linkCapacityEstimator.OnOveruseDetected(500000);
		linkCapacityEstimator.OnOveruseDetected(1000000);

		// The error saturates the tracked variance at its maximum of 2.5, so the
		// standard deviation becomes sqrt(2.5 * 525) = 36.23 kbps.
		REQUIRE(linkCapacityEstimator.GetEstimate() == 525000);
		REQUIRE(linkCapacityEstimator.GetUpperBound() == 633685);
		REQUIRE(linkCapacityEstimator.GetLowerBound() == 416314);
	}

	SECTION("resetting drops the estimate but keeps how noisy the samples were")
	{
		RTC::BWE::LinkCapacityEstimator linkCapacityEstimator;

		linkCapacityEstimator.OnOveruseDetected(500000);
		linkCapacityEstimator.OnOveruseDetected(1000000);
		linkCapacityEstimator.Reset();

		REQUIRE(!linkCapacityEstimator.GetEstimate().has_value());
		REQUIRE(linkCapacityEstimator.GetUpperBound() == RTC::BWE::Types::BitrateInfinite);
		REQUIRE(linkCapacityEstimator.GetLowerBound() == 0);

		// The next sample is taken as is, since there is nothing to filter it
		// against anymore.
		linkCapacityEstimator.OnOveruseDetected(500000);

		REQUIRE(linkCapacityEstimator.GetEstimate() == 500000);

		// The variance was not dropped along with the estimate, so the interval is
		// still far wider than the one of a network that had always been quiet.
		// It decays from 2.5 towards its minimum as quiet samples come in.
		REQUIRE(linkCapacityEstimator.GetUpperBound() == 603380);
		REQUIRE(linkCapacityEstimator.GetLowerBound() == 396619);
	}
}
