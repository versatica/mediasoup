#include "common.hpp"
#include "RTC/BWE/BitrateProber.hpp"
#include "RTC/BWE/BweTypes.hpp"
#include <catch2/catch_test_macros.hpp>

SCENARIO("BWE BitrateProber", "[bwe][bitrateprober]")
{
	// The clock starts well away from zero so that a mistake taking a time for a
	// duration doesn't go unnoticed.
	constexpr int64_t InitialTimeUs{ 100000000 };
	constexpr size_t ProbeSize{ 1000 };

	int64_t nowUs{ InitialTimeUs };

	// What a bitrate held for the given time carries, rounded rather than
	// truncated, which is what decides the bytes a burst is meant to be made of.
	const auto bytesFor = [](int64_t bitrate, int64_t durationUs) -> size_t
	{
		return static_cast<size_t>(((bitrate * durationUs) + 4000000) / (8 * 1000000));
	};

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

	SECTION("a burst goes out at the bitrate it was asked for, and then the next one")
	{
		constexpr int64_t TestBitrate1{ 900000 };
		constexpr int64_t TestBitrate2{ 1800000 };
		constexpr int64_t ClusterSize{ 5 };
		constexpr int64_t MinProbeDurationUs{ 15 * 1000 };

		RTC::BWE::BitrateProber prober;

		REQUIRE(!prober.IsProbing());

		const int64_t startTimeUs = nowUs;

		REQUIRE(!prober.GetNextProbeTimeUs(nowUs).has_value());

		prober.CreateProbeCluster(makeClusterConfig(0, nowUs, TestBitrate1, 2 * 1000));
		prober.CreateProbeCluster(makeClusterConfig(1, nowUs, TestBitrate2, 2 * 1000));

		REQUIRE(!prober.IsProbing());

		prober.OnIncomingPacket(ProbeSize);

		REQUIRE(prober.IsProbing());
		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		REQUIRE(prober.GetCurrentCluster(nowUs).value().id == 0);

		// The first packet goes out at the first chance there is.
		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		REQUIRE(prober.GetNextProbeTimeUs(nowUs).value() == nowUs);

		for (int64_t i{ 0 }; i < ClusterSize; ++i)
		{
			// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
			nowUs = std::max(nowUs, prober.GetNextProbeTimeUs(nowUs).value());

			// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
			REQUIRE(nowUs == std::max(nowUs, prober.GetNextProbeTimeUs(nowUs).value()));
			// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
			REQUIRE(prober.GetCurrentCluster(nowUs).value().id == 0);

			prober.ProbeSent(nowUs, ProbeSize);
		}

		REQUIRE(nowUs - startTimeUs >= MinProbeDurationUs);

		// The bitrate it went out at is within 10% of the one asked for.
		int64_t bitrate =
		  (static_cast<int64_t>(ProbeSize) * (ClusterSize - 1) * 8 * 1000000) / (nowUs - startTimeUs);

		REQUIRE(bitrate > (TestBitrate1 * 9) / 10);
		REQUIRE(bitrate < (TestBitrate1 * 11) / 10);

		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		nowUs = std::max(nowUs, prober.GetNextProbeTimeUs(nowUs).value());

		const int64_t probe2StartedUs = nowUs;

		for (int64_t i{ 0 }; i < ClusterSize; ++i)
		{
			// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
			nowUs = std::max(nowUs, prober.GetNextProbeTimeUs(nowUs).value());

			// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
			REQUIRE(nowUs == std::max(nowUs, prober.GetNextProbeTimeUs(nowUs).value()));
			// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
			REQUIRE(prober.GetCurrentCluster(nowUs).value().id == 1);

			prober.ProbeSent(nowUs, ProbeSize);
		}

		const int64_t durationUs = nowUs - probe2StartedUs;

		REQUIRE(durationUs >= MinProbeDurationUs);

		bitrate = (static_cast<int64_t>(ProbeSize) * (ClusterSize - 1) * 8 * 1000000) / durationUs;

		REQUIRE(bitrate > (TestBitrate2 * 9) / 10);
		REQUIRE(bitrate < (TestBitrate2 * 11) / 10);

		REQUIRE(!prober.GetNextProbeTimeUs(nowUs).has_value());
		REQUIRE(!prober.IsProbing());
	}

	SECTION("nothing goes out until a packet of somebody else's does")
	{
		RTC::BWE::BitrateProber prober;

		REQUIRE(!prober.GetNextProbeTimeUs(nowUs).has_value());

		prober.CreateProbeCluster(makeClusterConfig(0, nowUs, 900000, 2 * 1000));

		REQUIRE(!prober.IsProbing());

		prober.OnIncomingPacket(ProbeSize);

		REQUIRE(prober.IsProbing());
		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		REQUIRE(nowUs == std::max(nowUs, prober.GetNextProbeTimeUs(nowUs).value()));

		prober.ProbeSent(nowUs, ProbeSize);
	}

	SECTION("a packet that goes out long after it was due throws its burst away")
	{
		constexpr int64_t MaxProbeDelayUs{ 3 * 1000 };

		const RTC::BWE::BitrateProber::BitrateProberOptions options{ .maxProbeDelayUs = MaxProbeDelayUs };

		RTC::BWE::BitrateProber prober(options);

		prober.CreateProbeCluster(makeClusterConfig(0, nowUs, 900000, 2 * 1000));

		prober.OnIncomingPacket(ProbeSize);

		REQUIRE(prober.IsProbing());
		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		REQUIRE(prober.GetCurrentCluster(nowUs).value().id == 0);

		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		nowUs = std::max(nowUs, prober.GetNextProbeTimeUs(nowUs).value());

		prober.ProbeSent(nowUs, ProbeSize);

		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		const int64_t nextProbeTimeUs = prober.GetNextProbeTimeUs(nowUs).value();

		REQUIRE(nextProbeTimeUs > nowUs);

		nowUs += (nextProbeTimeUs - nowUs) + MaxProbeDelayUs + 1000;

		// It still says the instant it wanted to go out at.
		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		REQUIRE(prober.GetNextProbeTimeUs(nowUs).value() == nextProbeTimeUs);
		// And the only burst there was is gone.
		REQUIRE(!prober.GetCurrentCluster(nowUs).has_value());
	}

	SECTION("only so many bursts may be waiting at once")
	{
		RTC::BWE::BitrateProber prober;

		prober.CreateProbeCluster(makeClusterConfig(0, nowUs, 900000, 2 * 1000));
		prober.OnIncomingPacket(ProbeSize);

		REQUIRE(prober.IsProbing());
		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		REQUIRE(prober.GetCurrentCluster(nowUs).value().id == 0);

		for (int64_t i{ 1 }; i < 11; ++i)
		{
			prober.CreateProbeCluster(makeClusterConfig(i, nowUs, 900000, 2 * 1000));
			prober.OnIncomingPacket(ProbeSize);
		}

		// Some of them have been dropped.
		REQUIRE(prober.IsProbing());
		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		REQUIRE(prober.GetCurrentCluster(nowUs).value().id >= 5);

		const int64_t maxExpectedProbeTimeUs = nowUs + (1000 * 1000);

		while (prober.IsProbing() && nowUs < maxExpectedProbeTimeUs)
		{
			// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
			nowUs = std::max(nowUs, prober.GetNextProbeTimeUs(nowUs).value());

			prober.ProbeSent(nowUs, ProbeSize);
		}

		REQUIRE(!prober.IsProbing());
	}

	SECTION("a packet too small to measure from doesn't start a burst")
	{
		RTC::BWE::BitrateProber prober;

		prober.SetEnabled(true);

		REQUIRE(!prober.IsProbing());

		prober.CreateProbeCluster(makeClusterConfig(0, nowUs, 1000000, 2 * 1000));
		prober.OnIncomingPacket(100);

		REQUIRE(!prober.IsProbing());
	}

	SECTION("unless any packet is said to do")
	{
		const RTC::BWE::BitrateProber::BitrateProberOptions options{ .minPacketSize = 0 };

		RTC::BWE::BitrateProber prober(options);

		prober.SetEnabled(true);

		REQUIRE(!prober.IsProbing());

		prober.CreateProbeCluster(makeClusterConfig(0, nowUs, 1000000, 2 * 1000));
		prober.OnIncomingPacket(10);

		REQUIRE(prober.IsProbing());
	}

	SECTION("the shot of a fast burst carries more than a millisecond of it")
	{
		constexpr int64_t HighBitrate{ 10000000 };

		RTC::BWE::BitrateProber prober;

		prober.CreateProbeCluster(makeClusterConfig(0, nowUs, HighBitrate, 2 * 1000));

		REQUIRE(prober.GetRecommendedMinProbeSize() > bytesFor(HighBitrate, 1000));
	}

	SECTION("and how much it carries is the time between shots that was asked for")
	{
		constexpr int64_t HighBitrate{ 10000000 };

		RTC::BWE::BitrateProber prober;

		prober.SetEnabled(true);

		prober.CreateProbeCluster(makeClusterConfig(0, nowUs, HighBitrate, 20 * 1000));

		REQUIRE(prober.GetRecommendedMinProbeSize() == bytesFor(HighBitrate, 20 * 1000));

		prober.OnIncomingPacket(ProbeSize);

		// Having sent what a shot should carry, the next one is due a whole time
		// between shots later.
		prober.ProbeSent(nowUs, prober.GetRecommendedMinProbeSize());

		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		REQUIRE(prober.GetNextProbeTimeUs(nowUs).value() == nowUs + (20 * 1000));
	}

	SECTION("a slow burst still takes the count of packets it asked for")
	{
		constexpr int64_t Bitrate{ 100000 };
		constexpr size_t PacketSize{ 1000 };

		RTC::BWE::BitrateProber prober;

		prober.CreateProbeCluster(makeClusterConfig(0, nowUs, Bitrate, 2 * 1000));
		prober.OnIncomingPacket(PacketSize);

		for (int64_t i{ 0 }; i < 5; ++i)
		{
			REQUIRE(prober.IsProbing());

			prober.ProbeSent(nowUs, PacketSize);
		}

		REQUIRE(!prober.IsProbing());
	}

	SECTION("and a fast one takes the bytes it asked for")
	{
		constexpr int64_t Bitrate{ 10000000 };
		constexpr size_t PacketSize{ 1000 };

		const size_t expectedDataSent = bytesFor(Bitrate, 15 * 1000);

		RTC::BWE::BitrateProber prober;

		prober.CreateProbeCluster(makeClusterConfig(0, nowUs, Bitrate, 2 * 1000));
		prober.OnIncomingPacket(PacketSize);

		size_t dataSent{ 0 };

		while (dataSent < expectedDataSent)
		{
			REQUIRE(prober.IsProbing());

			prober.ProbeSent(nowUs, PacketSize);

			dataSent += PacketSize;
		}

		REQUIRE(!prober.IsProbing());
	}

	SECTION("however fast it is")
	{
		constexpr int64_t Bitrate{ 1000000000 };
		constexpr size_t PacketSize{ 1000 };

		const size_t expectedDataSent = bytesFor(Bitrate, 15 * 1000);

		RTC::BWE::BitrateProber prober;

		prober.CreateProbeCluster(makeClusterConfig(0, nowUs, Bitrate, 2 * 1000));
		prober.OnIncomingPacket(PacketSize);

		size_t dataSent{ 0 };

		while (dataSent < expectedDataSent)
		{
			REQUIRE(prober.IsProbing());

			prober.ProbeSent(nowUs, PacketSize);

			dataSent += PacketSize;
		}

		REQUIRE(!prober.IsProbing());
	}

	SECTION("a burst that never started goes stale")
	{
		constexpr int64_t Bitrate{ 300000 };
		constexpr size_t SmallPacketSize{ 20 };
		// Two bursts of five packets each.
		constexpr size_t ExpectedDataSent{ SmallPacketSize * 2 * 5 };
		constexpr int64_t TimeoutUs{ 5000 * 1000 };

		RTC::BWE::BitrateProber prober;

		prober.CreateProbeCluster(makeClusterConfig(0, nowUs, Bitrate, 2 * 1000));
		prober.OnIncomingPacket(SmallPacketSize);

		REQUIRE(!prober.IsProbing());

		nowUs += TimeoutUs;

		prober.CreateProbeCluster(makeClusterConfig(1, nowUs, Bitrate / 10, 2 * 1000));
		prober.OnIncomingPacket(SmallPacketSize);

		REQUIRE(!prober.IsProbing());

		nowUs += 1000;

		prober.CreateProbeCluster(makeClusterConfig(2, nowUs, Bitrate / 10, 2 * 1000));
		prober.OnIncomingPacket(SmallPacketSize);

		REQUIRE(prober.IsProbing());

		size_t dataSent{ 0 };

		while (dataSent < ExpectedDataSent)
		{
			REQUIRE(prober.IsProbing());

			prober.ProbeSent(nowUs, SmallPacketSize);

			dataSent += SmallPacketSize;
		}

		REQUIRE(!prober.IsProbing());
	}

	SECTION("a burst may start with no packet of somebody else's at all")
	{
		const RTC::BWE::BitrateProber::BitrateProberOptions options{ .minPacketSize = 0 };

		RTC::BWE::BitrateProber prober(options);

		prober.CreateProbeCluster(makeClusterConfig(0, nowUs, 300000, 2 * 1000));

		REQUIRE(prober.IsProbing());
	}

	SECTION("and so may the one after it")
	{
		constexpr int64_t Bitrate{ 300000 };

		const RTC::BWE::BitrateProber::BitrateProberOptions options{ .minPacketSize = 0 };

		RTC::BWE::BitrateProber prober(options);

		auto clusterConfig = makeClusterConfig(0, nowUs, Bitrate, 2 * 1000);

		clusterConfig.targetProbeCount = 1;

		prober.CreateProbeCluster(clusterConfig);

		REQUIRE(prober.IsProbing());

		prober.ProbeSent(nowUs + 1000, bytesFor(Bitrate, 15 * 1000));

		REQUIRE(!prober.IsProbing());

		clusterConfig.id   = 2;
		clusterConfig.atUs = nowUs + (100 * 1000);

		prober.CreateProbeCluster(clusterConfig);

		REQUIRE(prober.IsProbing());
	}
}
