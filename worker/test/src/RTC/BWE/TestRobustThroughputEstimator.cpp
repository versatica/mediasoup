#include "common.hpp"
#include "RTC/BWE/BweTypes.hpp"
#include "RTC/BWE/RobustThroughputEstimator.hpp"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

SCENARIO("BWE RobustThroughputEstimator", "[bwe][robustthroughputestimator]")
{
	constexpr size_t PacketSize{ 1000 };
	constexpr int64_t Rate800Kbps{ 800000 };
	constexpr int64_t Rate1600Kbps{ 1600000 };
	constexpr int64_t Rate400Kbps{ 400000 };
	constexpr int64_t Rate200Kbps{ 200000 };
	constexpr int64_t Rate4800Kbps{ 4800000 };

	// Builds the packets a feedback would report, advancing the send and the
	// arrival clocks at the given rates.
	struct FeedbackGenerator
	{
		std::vector<RTC::BWE::Types::PacketResult> CreateFeedbackVector(
		  size_t numberOfPackets, size_t packetSize, int64_t sendRate, int64_t recvRate)
		{
			std::vector<RTC::BWE::Types::PacketResult> packetResults(numberOfPackets);

			for (size_t idx{ 0 }; idx < numberOfPackets; ++idx)
			{
				packetResults[idx].sentPacket.sendTimeUs     = this->sendClockUs;
				packetResults[idx].sentPacket.sequenceNumber = this->sequenceNumber;
				packetResults[idx].sentPacket.size           = packetSize;

				this->sendClockUs += static_cast<int64_t>(packetSize) * 8 * 1000000 / sendRate;
				this->recvClockUs += static_cast<int64_t>(packetSize) * 8 * 1000000 / recvRate;
				this->sequenceNumber += 1;

				packetResults[idx].receiveTimeUs = this->recvClockUs;
			}

			return packetResults;
		}

		int64_t sendClockUs{ 100000 * 1000 };
		int64_t recvClockUs{ 10000 * 1000 };
		int64_t sequenceNumber{ 100 };
	};

	SECTION("there is no estimate until there are enough packets")
	{
		FeedbackGenerator feedbackGenerator;
		RTC::BWE::RobustThroughputEstimator throughputEstimator;

		auto packetResults =
		  feedbackGenerator.CreateFeedbackVector(9, PacketSize, Rate800Kbps, Rate800Kbps);

		throughputEstimator.IncomingPacketFeedbackVector(packetResults);

		REQUIRE(!throughputEstimator.GetBitrate().has_value());

		// The estimate appears once `requiredPackets` packets have been received.
		packetResults = feedbackGenerator.CreateFeedbackVector(1, PacketSize, Rate800Kbps, Rate800Kbps);

		throughputEstimator.IncomingPacketFeedbackVector(packetResults);

		REQUIRE(throughputEstimator.GetBitrate() == Rate800Kbps);

		// And it stays put while the send and arrival rates do.
		packetResults = feedbackGenerator.CreateFeedbackVector(15, PacketSize, Rate800Kbps, Rate800Kbps);

		throughputEstimator.IncomingPacketFeedbackVector(packetResults);

		REQUIRE(throughputEstimator.GetBitrate() == Rate800Kbps);
	}

	SECTION("the estimate follows the rate up and down")
	{
		FeedbackGenerator feedbackGenerator;
		RTC::BWE::RobustThroughputEstimator throughputEstimator;

		// One second at 800 kbps, stable.
		for (int i{ 0 }; i < 10; ++i)
		{
			const auto packetResults =
			  feedbackGenerator.CreateFeedbackVector(10, PacketSize, Rate800Kbps, Rate800Kbps);

			throughputEstimator.IncomingPacketFeedbackVector(packetResults);

			REQUIRE(throughputEstimator.GetBitrate() == Rate800Kbps);
		}

		// One second at 1600 kbps, the estimate climbs without overshooting.
		for (int i{ 0 }; i < 20; ++i)
		{
			const auto packetResults =
			  feedbackGenerator.CreateFeedbackVector(10, PacketSize, Rate1600Kbps, Rate1600Kbps);

			throughputEstimator.IncomingPacketFeedbackVector(packetResults);

			const auto throughput = throughputEstimator.GetBitrate();

			REQUIRE(throughput.has_value());
			// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
			REQUIRE(throughput.value() >= Rate800Kbps);
			// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
			REQUIRE(throughput.value() <= Rate1600Kbps);
		}

		// Another second at 1600 kbps, now stable there.
		for (int i{ 0 }; i < 20; ++i)
		{
			const auto packetResults =
			  feedbackGenerator.CreateFeedbackVector(10, PacketSize, Rate1600Kbps, Rate1600Kbps);

			throughputEstimator.IncomingPacketFeedbackVector(packetResults);

			REQUIRE(throughputEstimator.GetBitrate() == Rate1600Kbps);
		}

		// Down to 400 kbps, the estimate falls without undershooting.
		for (int i{ 0 }; i < 5; ++i)
		{
			const auto packetResults =
			  feedbackGenerator.CreateFeedbackVector(10, PacketSize, Rate400Kbps, Rate400Kbps);

			throughputEstimator.IncomingPacketFeedbackVector(packetResults);

			const auto throughput = throughputEstimator.GetBitrate();

			REQUIRE(throughput.has_value());
			// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
			REQUIRE(throughput.value() <= Rate1600Kbps);
			// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
			REQUIRE(throughput.value() >= Rate400Kbps);
		}

		// And stable at 400 kbps.
		for (int i{ 0 }; i < 5; ++i)
		{
			const auto packetResults =
			  feedbackGenerator.CreateFeedbackVector(10, PacketSize, Rate400Kbps, Rate400Kbps);

			throughputEstimator.IncomingPacketFeedbackVector(packetResults);

			REQUIRE(throughputEstimator.GetBitrate() == Rate400Kbps);
		}
	}

	SECTION("the estimate is capped by the arrival rate")
	{
		FeedbackGenerator feedbackGenerator;
		RTC::BWE::RobustThroughputEstimator throughputEstimator;

		const auto packetResults =
		  feedbackGenerator.CreateFeedbackVector(20, PacketSize, Rate800Kbps, Rate200Kbps);

		throughputEstimator.IncomingPacketFeedbackVector(packetResults);

		const auto throughput = throughputEstimator.GetBitrate();

		REQUIRE(throughput.has_value());
		REQUIRE_THAT(
		  // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		  static_cast<double>(throughput.value()),
		  Catch::Matchers::WithinAbs(Rate200Kbps, 0.05 * Rate200Kbps));
	}

	SECTION("the estimate is capped by the send rate")
	{
		FeedbackGenerator feedbackGenerator;
		RTC::BWE::RobustThroughputEstimator throughputEstimator;

		const auto packetResults =
		  feedbackGenerator.CreateFeedbackVector(20, PacketSize, Rate400Kbps, Rate800Kbps);

		throughputEstimator.IncomingPacketFeedbackVector(packetResults);

		const auto throughput = throughputEstimator.GetBitrate();

		REQUIRE(throughput.has_value());
		REQUIRE_THAT(
		  // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		  static_cast<double>(throughput.value()),
		  Catch::Matchers::WithinAbs(Rate400Kbps, 0.05 * Rate400Kbps));
	}

	SECTION("a delay spike doesn't drop the estimate")
	{
		// A 500 ms window amplifies the effect of the spike.
		constexpr RTC::BWE::RobustThroughputEstimator::RobustThroughputEstimatorOptions ShortWindowOptions{
			.minWindowDurationUs = 500 * 1000
		};

		FeedbackGenerator feedbackGenerator;
		RTC::BWE::RobustThroughputEstimator throughputEstimator(ShortWindowOptions);

		auto packetResults =
		  feedbackGenerator.CreateFeedbackVector(20, PacketSize, Rate800Kbps, Rate800Kbps);

		throughputEstimator.IncomingPacketFeedbackVector(packetResults);

		REQUIRE(throughputEstimator.GetBitrate() == Rate800Kbps);

		// The spike: 25 packets are sent and none arrives.
		feedbackGenerator.recvClockUs += 250 * 1000;

		// They are all delivered over the next 50 ms. Five more were sent
		// meanwhile, so 30 packets of 1000 bytes in 50 ms, which is 4800 kbps.
		for (int i{ 0 }; i < 30; ++i)
		{
			packetResults =
			  feedbackGenerator.CreateFeedbackVector(1, PacketSize, Rate800Kbps, Rate4800Kbps);

			throughputEstimator.IncomingPacketFeedbackVector(packetResults);

			const auto throughput = throughputEstimator.GetBitrate();

			REQUIRE(throughput.has_value());
			REQUIRE_THAT(
			  // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
			  static_cast<double>(throughput.value()),
			  Catch::Matchers::WithinAbs(Rate800Kbps, 0.05 * Rate800Kbps));
		}

		// Back to the normal rate. Once the packets from before the gap leave the
		// window the arrival rate looks high, but the send rate caps the estimate.
		for (int i{ 0 }; i < 20; ++i)
		{
			packetResults = feedbackGenerator.CreateFeedbackVector(5, PacketSize, Rate800Kbps, Rate800Kbps);

			throughputEstimator.IncomingPacketFeedbackVector(packetResults);

			const auto throughput = throughputEstimator.GetBitrate();

			REQUIRE(throughput.has_value());
			REQUIRE_THAT(
			  // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
			  static_cast<double>(throughput.value()),
			  Catch::Matchers::WithinAbs(Rate800Kbps, 0.05 * Rate800Kbps));
		}
	}

	SECTION("losing half of the packets halves the estimate")
	{
		FeedbackGenerator feedbackGenerator;
		RTC::BWE::RobustThroughputEstimator throughputEstimator;

		auto packetResults =
		  feedbackGenerator.CreateFeedbackVector(20, PacketSize, Rate800Kbps, Rate800Kbps);

		for (size_t idx{ 0 }; idx < packetResults.size(); ++idx)
		{
			if (idx % 2 == 1)
			{
				packetResults[idx].receiveTimeUs.reset();
			}
		}

		std::ranges::sort(packetResults, RTC::BWE::Types::PacketResult::ReceiveTimeOrder());

		throughputEstimator.IncomingPacketFeedbackVector(packetResults);

		const auto throughput = throughputEstimator.GetBitrate();

		REQUIRE(throughput.has_value());
		REQUIRE_THAT(
		  // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		  static_cast<double>(throughput.value()),
		  Catch::Matchers::WithinAbs(Rate800Kbps / 2.0, 0.05 * Rate800Kbps / 2.0));
	}

	SECTION("a reordered feedback drops the estimate and it recovers when it arrives")
	{
		FeedbackGenerator feedbackGenerator;
		RTC::BWE::RobustThroughputEstimator throughputEstimator;

		auto packetResults =
		  feedbackGenerator.CreateFeedbackVector(20, PacketSize, Rate800Kbps, Rate800Kbps);

		throughputEstimator.IncomingPacketFeedbackVector(packetResults);

		REQUIRE(throughputEstimator.GetBitrate() == Rate800Kbps);

		const auto delayedPacketResults =
		  feedbackGenerator.CreateFeedbackVector(10, PacketSize, Rate800Kbps, Rate800Kbps);

		packetResults = feedbackGenerator.CreateFeedbackVector(10, PacketSize, Rate800Kbps, Rate800Kbps);

		// Some feedback is missing, so the estimate is expected to drop.
		throughputEstimator.IncomingPacketFeedbackVector(packetResults);

		const auto throughput = throughputEstimator.GetBitrate();

		REQUIRE(throughput.has_value());
		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		REQUIRE(throughput.value() < Rate800Kbps);

		// And it recovers completely as soon as the missing feedback arrives.
		throughputEstimator.IncomingPacketFeedbackVector(delayedPacketResults);

		REQUIRE(throughputEstimator.GetBitrate() == Rate800Kbps);

		// From there on it's as stable as if nothing had been reordered.
		for (int i{ 0 }; i < 10; ++i)
		{
			packetResults =
			  feedbackGenerator.CreateFeedbackVector(15, PacketSize, Rate800Kbps, Rate800Kbps);

			throughputEstimator.IncomingPacketFeedbackVector(packetResults);

			REQUIRE(throughputEstimator.GetBitrate() == Rate800Kbps);
		}
	}

	SECTION("a packet arriving a second late doesn't drop the estimate")
	{
		// A 500 ms window amplifies the effect of the reordering.
		constexpr RTC::BWE::RobustThroughputEstimator::RobustThroughputEstimatorOptions ShortWindowOptions{
			.minWindowDurationUs = 500 * 1000
		};

		FeedbackGenerator feedbackGenerator;
		RTC::BWE::RobustThroughputEstimator throughputEstimator(ShortWindowOptions);

		auto delayedPacketResults =
		  feedbackGenerator.CreateFeedbackVector(1, PacketSize, Rate800Kbps, Rate800Kbps);

		for (int i{ 0 }; i < 10; ++i)
		{
			const auto packetResults =
			  feedbackGenerator.CreateFeedbackVector(10, PacketSize, Rate800Kbps, Rate800Kbps);

			throughputEstimator.IncomingPacketFeedbackVector(packetResults);

			REQUIRE(throughputEstimator.GetBitrate() == Rate800Kbps);
		}

		// The delayed packet arrives about a second after it should have. With a
		// 500 ms window it was sent well before the second oldest packet, and even
		// so the send rate must not drop.
		delayedPacketResults.front().receiveTimeUs = feedbackGenerator.recvClockUs;

		throughputEstimator.IncomingPacketFeedbackVector(delayedPacketResults);

		{
			const auto throughput = throughputEstimator.GetBitrate();

			REQUIRE(throughput.has_value());
			REQUIRE_THAT(
			  // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
			  static_cast<double>(throughput.value()),
			  Catch::Matchers::WithinAbs(Rate800Kbps, 0.05 * Rate800Kbps));
		}

		for (int i{ 0 }; i < 10; ++i)
		{
			const auto packetResults =
			  feedbackGenerator.CreateFeedbackVector(10, PacketSize, Rate800Kbps, Rate800Kbps);

			throughputEstimator.IncomingPacketFeedbackVector(packetResults);

			const auto throughput = throughputEstimator.GetBitrate();

			REQUIRE(throughput.has_value());
			REQUIRE_THAT(
			  // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
			  static_cast<double>(throughput.value()),
			  Catch::Matchers::WithinAbs(Rate800Kbps, 0.05 * Rate800Kbps));
		}
	}

	SECTION("the window is dropped when the arrival clock moves backwards")
	{
		FeedbackGenerator feedbackGenerator;
		RTC::BWE::RobustThroughputEstimator throughputEstimator;

		auto packetResults =
		  feedbackGenerator.CreateFeedbackVector(20, PacketSize, Rate800Kbps, Rate800Kbps);

		throughputEstimator.IncomingPacketFeedbackVector(packetResults);

		REQUIRE(throughputEstimator.GetBitrate() == Rate800Kbps);

		feedbackGenerator.recvClockUs -= 2 * 1000 * 1000;

		packetResults =
		  feedbackGenerator.CreateFeedbackVector(20, PacketSize, Rate1600Kbps, Rate1600Kbps);

		throughputEstimator.IncomingPacketFeedbackVector(packetResults);

		REQUIRE(throughputEstimator.GetBitrate() == Rate1600Kbps);
	}

	SECTION("pausing and resuming the stream invalidates the estimate until there is data again")
	{
		FeedbackGenerator feedbackGenerator;
		RTC::BWE::RobustThroughputEstimator throughputEstimator;

		auto packetResults =
		  feedbackGenerator.CreateFeedbackVector(20, PacketSize, Rate800Kbps, Rate800Kbps);

		throughputEstimator.IncomingPacketFeedbackVector(packetResults);

		{
			const auto throughput = throughputEstimator.GetBitrate();

			REQUIRE(throughput.has_value());
			REQUIRE_THAT(
			  // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
			  static_cast<double>(throughput.value()),
			  Catch::Matchers::WithinAbs(Rate800Kbps, 0.05 * Rate800Kbps));
		}

		// Nothing is sent and no feedback arrives for 60 seconds.
		feedbackGenerator.sendClockUs += 60 * 1000 * 1000;
		feedbackGenerator.recvClockUs += 60 * 1000 * 1000;

		// Sending resumes at the same rate. The estimate is invalid at first, since
		// there is no recent data to compute it from.
		packetResults = feedbackGenerator.CreateFeedbackVector(5, PacketSize, Rate800Kbps, Rate800Kbps);

		throughputEstimator.IncomingPacketFeedbackVector(packetResults);

		REQUIRE(!throughputEstimator.GetBitrate().has_value());

		// And it's back to the usual level once there is enough data again.
		for (int i{ 0 }; i < 4; ++i)
		{
			packetResults = feedbackGenerator.CreateFeedbackVector(5, PacketSize, Rate800Kbps, Rate800Kbps);

			throughputEstimator.IncomingPacketFeedbackVector(packetResults);

			REQUIRE(throughputEstimator.GetBitrate() == Rate800Kbps);
		}
	}
}
