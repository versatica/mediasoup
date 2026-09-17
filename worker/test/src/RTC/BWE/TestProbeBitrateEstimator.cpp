#include "common.hpp"
#include "RTC/BWE/BweTypes.hpp"
#include "RTC/BWE/ProbeBitrateEstimator.hpp"
#include <catch2/catch_test_macros.hpp>

SCENARIO("BWE ProbeBitrateEstimator", "[bwe][probebitrateestimator]")
{
	constexpr int64_t ReferenceTimeUs{ 1000 * 1000 * 1000 };
	constexpr int64_t DefaultMinProbes{ 5 };
	constexpr int64_t DefaultMinBytes{ 5000 };
	// Fraction of the capacity found that is aimed for once a burst arrives
	// clearly slower than it was sent.
	constexpr double TargetUtilizationFraction{ 0.95 };

	RTC::BWE::ProbeBitrateEstimator probeBitrateEstimator;
	std::optional<int64_t> measuredBitrate;

	// Feed the feedback of one packet of a burst, with times given in
	// milliseconds from an arbitrary instant.
	const auto addPacketFeedback = [&probeBitrateEstimator, &measuredBitrate](
	                                 int64_t clusterId,
	                                 int64_t sizeBytes,
	                                 int64_t sendTimeMs,
	                                 int64_t receiveTimeMs,
	                                 int64_t minProbes = DefaultMinProbes,
	                                 int64_t minBytes  = DefaultMinBytes) -> void
	{
		RTC::BWE::Types::PacketResult packetResult;

		packetResult.sentPacket.sendTimeUs = ReferenceTimeUs + (sendTimeMs * 1000);
		packetResult.sentPacket.size       = static_cast<size_t>(sizeBytes);
		packetResult.sentPacket.probeCluster =
		  RTC::BWE::Types::ProbeCluster{ .id = clusterId, .minProbes = minProbes, .minBytes = minBytes };
		packetResult.receiveTimeUs = ReferenceTimeUs + (receiveTimeMs * 1000);

		measuredBitrate = probeBitrateEstimator.HandleProbeAndEstimateBitrate(packetResult);
	};

	SECTION("a burst delivered at the pace it was sent measures that pace")
	{
		addPacketFeedback(0, 1000, 0, 10);
		addPacketFeedback(0, 1000, 10, 20);
		addPacketFeedback(0, 1000, 20, 30);
		addPacketFeedback(0, 1000, 30, 40);

		REQUIRE(measuredBitrate.has_value());
		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		REQUIRE(measuredBitrate.value() == 800000);
	}

	SECTION("a burst of too few packets measures nothing")
	{
		addPacketFeedback(0, 2000, 0, 10);
		addPacketFeedback(0, 2000, 10, 20);
		addPacketFeedback(0, 2000, 20, 30);

		REQUIRE(!measuredBitrate.has_value());
	}

	SECTION("a burst of too few bytes measures nothing")
	{
		constexpr int64_t MinBytes{ 6000 };

		addPacketFeedback(0, 800, 0, 10, DefaultMinProbes, MinBytes);
		addPacketFeedback(0, 800, 10, 20, DefaultMinProbes, MinBytes);
		addPacketFeedback(0, 800, 20, 30, DefaultMinProbes, MinBytes);
		addPacketFeedback(0, 800, 30, 40, DefaultMinProbes, MinBytes);
		addPacketFeedback(0, 800, 40, 50, DefaultMinProbes, MinBytes);

		REQUIRE(!measuredBitrate.has_value());
	}

	SECTION("a small burst is measured all the same")
	{
		constexpr int64_t MinBytes{ 1000 };

		addPacketFeedback(0, 150, 0, 10, DefaultMinProbes, MinBytes);
		addPacketFeedback(0, 150, 10, 20, DefaultMinProbes, MinBytes);
		addPacketFeedback(0, 150, 20, 30, DefaultMinProbes, MinBytes);
		addPacketFeedback(0, 150, 30, 40, DefaultMinProbes, MinBytes);
		addPacketFeedback(0, 150, 40, 50, DefaultMinProbes, MinBytes);
		addPacketFeedback(0, 150, 50, 60, DefaultMinProbes, MinBytes);

		REQUIRE(measuredBitrate.has_value());
		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		REQUIRE(measuredBitrate.value() == 120000);
	}

	SECTION("a large burst is measured all the same")
	{
		constexpr int64_t MinProbes{ 30 };
		constexpr int64_t MinBytes{ 312500 };

		int64_t sendTimeMs{ 0 };
		int64_t receiveTimeMs{ 5 };

		for (int64_t i{ 0 }; i < 25; ++i)
		{
			addPacketFeedback(0, 12500, sendTimeMs, receiveTimeMs, MinProbes, MinBytes);

			++sendTimeMs;
			++receiveTimeMs;
		}

		REQUIRE(measuredBitrate.has_value());
		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		REQUIRE(measuredBitrate.value() == 100000000);
	}

	SECTION("a burst arriving somewhat faster than it was sent measures the pace it was sent at")
	{
		addPacketFeedback(0, 1000, 0, 15);
		addPacketFeedback(0, 1000, 10, 30);
		addPacketFeedback(0, 1000, 20, 35);
		addPacketFeedback(0, 1000, 30, 40);

		REQUIRE(measuredBitrate.has_value());
		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		REQUIRE(measuredBitrate.value() == 800000);
	}

	SECTION("a burst arriving far faster than it was sent measures nothing")
	{
		addPacketFeedback(0, 1000, 0, 19);
		addPacketFeedback(0, 1000, 10, 22);
		addPacketFeedback(0, 1000, 20, 25);
		addPacketFeedback(0, 1000, 40, 27);

		REQUIRE(!measuredBitrate.has_value());
	}

	SECTION("a burst arriving clearly slower than it was sent aims below what arrived")
	{
		addPacketFeedback(0, 1000, 0, 10);
		addPacketFeedback(0, 1000, 10, 40);
		addPacketFeedback(0, 1000, 20, 70);
		addPacketFeedback(0, 1000, 30, 85);

		// It was sent at 800 kbps and arrived at 320 kbps.
		REQUIRE(measuredBitrate.has_value());
		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		REQUIRE(measuredBitrate.value() == static_cast<int64_t>(TargetUtilizationFraction * 320000));
	}

	SECTION("a burst that arrives all at once measures nothing")
	{
		addPacketFeedback(0, 1000, 0, 50);
		addPacketFeedback(0, 1000, 10, 50);
		addPacketFeedback(0, 1000, 20, 50);
		addPacketFeedback(0, 1000, 40, 50);

		REQUIRE(!measuredBitrate.has_value());
	}

	SECTION("bursts are measured apart from each other")
	{
		addPacketFeedback(0, 1000, 0, 10);
		addPacketFeedback(0, 1000, 10, 20);
		addPacketFeedback(0, 1000, 20, 30);
		addPacketFeedback(0, 1000, 40, 60);

		// Sent at 600 kbps, arrived at 480 kbps.
		REQUIRE(measuredBitrate.has_value());
		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		REQUIRE(measuredBitrate.value() == static_cast<int64_t>(TargetUtilizationFraction * 480000));

		addPacketFeedback(0, 1000, 50, 60);

		// Sent at 640 kbps, arrived at 640 kbps.
		REQUIRE(measuredBitrate.has_value());
		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		REQUIRE(measuredBitrate.value() == 640000);

		addPacketFeedback(1, 1000, 60, 70);
		addPacketFeedback(1, 1000, 65, 77);
		addPacketFeedback(1, 1000, 70, 84);
		addPacketFeedback(1, 1000, 75, 90);

		// Sent at 1600 kbps, arrived at 1200 kbps.
		REQUIRE(measuredBitrate.has_value());
		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		REQUIRE(measuredBitrate.value() == static_cast<int64_t>(TargetUtilizationFraction * 1200000));
	}

	SECTION("a burst is forgotten once it can receive nothing else")
	{
		addPacketFeedback(0, 1000, 0, 10);
		addPacketFeedback(0, 1000, 10, 20);
		addPacketFeedback(0, 1000, 20, 30);

		addPacketFeedback(1, 1000, 60, 70);
		addPacketFeedback(1, 1000, 65, 77);
		addPacketFeedback(1, 1000, 70, 84);
		addPacketFeedback(1, 1000, 75, 90);

		REQUIRE(measuredBitrate.has_value());
		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		REQUIRE(measuredBitrate.value() == static_cast<int64_t>(TargetUtilizationFraction * 1200000));

		// The packet that the first burst was missing, arriving six seconds later.
		addPacketFeedback(0, 1000, 40 + 6000, 60 + 6000);

		REQUIRE(!measuredBitrate.has_value());
	}

	SECTION("the packet sent last doesn't count towards the pace it was sent at")
	{
		addPacketFeedback(0, 1000, 0, 10);
		addPacketFeedback(0, 1000, 10, 20);
		addPacketFeedback(0, 1000, 20, 30);
		addPacketFeedback(0, 1000, 30, 40);
		addPacketFeedback(0, 1500, 40, 50);

		// Sent at 800 kbps and arrived at 900 kbps, so the pace it was sent at is
		// what the bigger last packet doesn't inflate.
		REQUIRE(measuredBitrate.has_value());
		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		REQUIRE(measuredBitrate.value() == 800000);
	}

	SECTION("the packet received first doesn't count towards the pace it arrived at")
	{
		addPacketFeedback(0, 1500, 0, 10);
		addPacketFeedback(0, 1000, 10, 20);
		addPacketFeedback(0, 1000, 20, 30);
		addPacketFeedback(0, 1000, 30, 40);

		// Sent at 933 kbps and arrived at 800 kbps.
		REQUIRE(measuredBitrate.has_value());
		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		REQUIRE(measuredBitrate.value() == static_cast<int64_t>(TargetUtilizationFraction * 800000));
	}

	SECTION("there is nothing to take before any burst has been measured")
	{
		REQUIRE(!probeBitrateEstimator.FetchAndResetLastEstimatedBitrate().has_value());
	}

	SECTION("what has been measured is only taken once")
	{
		addPacketFeedback(0, 1000, 0, 10);
		addPacketFeedback(0, 1000, 10, 20);
		addPacketFeedback(0, 1000, 20, 30);
		addPacketFeedback(0, 1000, 30, 40);

		const auto estimatedBitrate = probeBitrateEstimator.FetchAndResetLastEstimatedBitrate();

		REQUIRE(estimatedBitrate.has_value());
		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		REQUIRE(estimatedBitrate.value() == 800000);
		REQUIRE(!probeBitrateEstimator.FetchAndResetLastEstimatedBitrate().has_value());
	}
}
