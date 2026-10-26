#include "common.hpp"
#include "RTC/BWE/InterArrival.hpp"
#include <catch2/catch_test_macros.hpp>

SCENARIO("BWE InterArrival", "[bwe][interarrival]")
{
	constexpr int64_t TimestampGroupLengthUs{ 5000 };
	constexpr int64_t MinStepUs{ 20 };
	constexpr int64_t TriggerNewGroupUs{ TimestampGroupLengthUs + MinStepUs };
	constexpr int64_t BurstThresholdMs{ 5 };
	constexpr int AbsSendTimeFraction{ 18 };
	constexpr int AbsSendTimeInterArrivalUpshift{ 8 };
	constexpr int InterArrivalShift{ AbsSendTimeFraction + AbsSendTimeInterArrivalUpshift };
	constexpr double RtpTimestampToUs{ 1000.0 / 90.0 };
	constexpr double AbsSendTimeToUs{ 1000000.0 / static_cast<double>(1 << InterArrivalShift) };
	// NOTE: Both are private to the class, so they are repeated here.
	constexpr int64_t ArrivalTimeOffsetThresholdMs{ 3000 };
	constexpr size_t ReorderedResetThreshold{ 3 };

	// The given instant, expressed as an RTP timestamp of a 90 kHz clock.
	const auto makeRtpTimestamp = [](int64_t us) -> uint32_t
	{
		return static_cast<uint32_t>(static_cast<uint64_t>((us * 90) + 500) / 1000);
	};

	// The same, as the abs-send-time the estimator works with: 24 bits of a 6.18
	// fixed point value, shifted up to use the whole 32 bits.
	const auto makeAbsSendTime = [](int64_t us) -> uint32_t
	{
		const uint32_t absSendTime =
		  static_cast<uint32_t>(((static_cast<uint64_t>(us) << 18) + 500000) / 1000000) & 0x00FFFFFF;

		return absSendTime << 8;
	};

	// The two instances that every case drives at once, one on each kind of
	// timestamp, so that what is asserted holds however the sender counts time.
	RTC::BWE::InterArrival interArrivalRtp(makeRtpTimestamp(TimestampGroupLengthUs), RtpTimestampToUs);
	RTC::BWE::InterArrival interArrivalAst(makeAbsSendTime(TimestampGroupLengthUs), AbsSendTimeToUs);
	// And one whose timestamps are plain milliseconds, for the cases that don't
	// care about how they are encoded.
	RTC::BWE::InterArrival interArrival(TimestampGroupLengthUs / 1000, 1000.0);

	// Neither instance closes a group with the given packet.
	const auto expectFalse = [&interArrivalRtp, &interArrivalAst, &makeRtpTimestamp, &makeAbsSendTime](
	                           int64_t timestampUs, int64_t arrivalTimeMs, size_t packetSize) -> void
	{
		REQUIRE(!interArrivalRtp
		           .ComputeDeltas(
		             makeRtpTimestamp(timestampUs), arrivalTimeMs * 1000, arrivalTimeMs * 1000, packetSize)
		           .has_value());
		REQUIRE(!interArrivalAst
		           .ComputeDeltas(
		             makeAbsSendTime(timestampUs), arrivalTimeMs * 1000, arrivalTimeMs * 1000, packetSize)
		           .has_value());
	};

	// Both instances close a group with the given packet, and its deltas are the
	// expected ones.
	//
	// NOTE: The timestamp delta is compared within a tolerance because rounding
	// the very same instant into each kind of timestamp lands on different ticks.
	const auto expectTrue = [&interArrivalRtp, &interArrivalAst, &makeRtpTimestamp, &makeAbsSendTime](
	                          int64_t timestampUs,
	                          int64_t arrivalTimeMs,
	                          size_t packetSize,
	                          int64_t expectedTimestampDeltaUs,
	                          int64_t expectedArrivalDeltaMs,
	                          int64_t expectedSizeDelta,
	                          uint32_t timestampNear) -> void
	{
		const auto expectDeltas = [](
		                            const std::optional<RTC::BWE::InterArrival::Deltas>& deltas,
		                            uint32_t expectedTimestampDelta,
		                            int64_t expectedArrivalDeltaUs,
		                            int64_t expectedSizeDelta,
		                            uint32_t timestampNear) -> void
		{
			REQUIRE(deltas.has_value());

			// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
			const auto& value            = deltas.value();
			const uint32_t timestampDiff = value.timestampDelta > expectedTimestampDelta
			                                 ? value.timestampDelta - expectedTimestampDelta
			                                 : expectedTimestampDelta - value.timestampDelta;

			REQUIRE(timestampDiff <= timestampNear);
			REQUIRE(value.arrivalDeltaUs == expectedArrivalDeltaUs);
			REQUIRE(value.sizeDelta == expectedSizeDelta);
		};

		expectDeltas(
		  interArrivalRtp.ComputeDeltas(
		    makeRtpTimestamp(timestampUs), arrivalTimeMs * 1000, arrivalTimeMs * 1000, packetSize),
		  makeRtpTimestamp(expectedTimestampDeltaUs),
		  expectedArrivalDeltaMs * 1000,
		  expectedSizeDelta,
		  timestampNear);
		expectDeltas(
		  interArrivalAst.ComputeDeltas(
		    makeAbsSendTime(timestampUs), arrivalTimeMs * 1000, arrivalTimeMs * 1000, packetSize),
		  makeAbsSendTime(expectedTimestampDeltaUs),
		  expectedArrivalDeltaMs * 1000,
		  expectedSizeDelta,
		  timestampNear << 8);
	};

	// Walk the timestamps across the point where they wrap around, a quarter of
	// their range at a time so that no packet is taken as reordered just for
	// being near it.
	const auto wrapTestHelper =
	  [&expectFalse,
		 &expectTrue](int64_t wrapStartUs, uint32_t timestampNear, bool unorderlyWithinGroup) -> void
	{
		// G1.
		int64_t arrivalTime = 17;

		expectFalse(0, arrivalTime, 1);

		// G2.
		arrivalTime += BurstThresholdMs + 1;

		expectFalse(wrapStartUs / 4, arrivalTime, 1);

		// G3.
		arrivalTime += BurstThresholdMs + 1;

		// Delta G2-G1.
		expectTrue(wrapStartUs / 2, arrivalTime, 1, wrapStartUs / 4, 6, 0, 0);

		// G4.
		arrivalTime += BurstThresholdMs + 1;

		const int64_t g4ArrivalTime = arrivalTime;

		// Delta G3-G2.
		expectTrue(
		  (wrapStartUs / 2) + (wrapStartUs / 4), arrivalTime, 1, wrapStartUs / 4, 6, 0, timestampNear);

		// G5.
		arrivalTime += BurstThresholdMs + 1;

		// Delta G4-G3.
		expectTrue(wrapStartUs, arrivalTime, 2, wrapStartUs / 4, 6, 0, timestampNear);

		for (int64_t i{ 0 }; i < 10; ++i)
		{
			// Slowly step across the wrap point.
			arrivalTime += BurstThresholdMs + 1;

			if (unorderlyWithinGroup)
			{
				// These packets arrive with timestamps in decreasing order but are
				// accumulated into the group anyway, because they are still higher than
				// the one the group began with.
				expectFalse(wrapStartUs + (MinStepUs * (9 - i)), arrivalTime, 1);
			}
			else
			{
				expectFalse(wrapStartUs + (MinStepUs * i), arrivalTime, 1);
			}
		}

		const int64_t g5ArrivalTime = arrivalTime;

		// This packet is out of order and is dropped.
		arrivalTime += BurstThresholdMs + 1;

		expectFalse(wrapStartUs - 100, arrivalTime, 100);

		// G6.
		arrivalTime += BurstThresholdMs + 1;

		const int64_t g6ArrivalTime = arrivalTime;

		// Delta G5-G4.
		expectTrue(
		  wrapStartUs + TriggerNewGroupUs,
		  arrivalTime,
		  10,
		  (wrapStartUs / 4) + (9 * MinStepUs),
		  g5ArrivalTime - g4ArrivalTime,
		  (2 + 10) - 1,
		  timestampNear);

		// This packet is out of order and is dropped.
		arrivalTime += BurstThresholdMs + 1;

		expectFalse(wrapStartUs + TimestampGroupLengthUs, arrivalTime, 100);

		// G7.
		arrivalTime += BurstThresholdMs + 1;

		// Delta G6-G5.
		expectTrue(
		  wrapStartUs + (2 * TriggerNewGroupUs),
		  arrivalTime,
		  100,
		  TriggerNewGroupUs - (9 * MinStepUs),
		  g6ArrivalTime - g5ArrivalTime,
		  10 - (2 + 10),
		  timestampNear);
	};

	SECTION("the very first packet closes no group")
	{
		expectFalse(0, 17, 1);
	}

	SECTION("the first group is only closed by the packet that starts the third one")
	{
		// G1.
		int64_t arrivalTime = 17;

		const int64_t g1ArrivalTime = arrivalTime;

		expectFalse(0, arrivalTime, 1);

		// G2.
		arrivalTime += BurstThresholdMs + 1;

		const int64_t g2ArrivalTime = arrivalTime;

		expectFalse(TriggerNewGroupUs, arrivalTime, 2);

		// G3. Only once the first packet of the third group arrives do the deltas
		// between the first two show up.
		arrivalTime += BurstThresholdMs + 1;

		expectTrue(
		  2 * TriggerNewGroupUs, arrivalTime, 1, TriggerNewGroupUs, g2ArrivalTime - g1ArrivalTime, 1, 0);
	}

	SECTION("each group is closed by the one that follows it")
	{
		// G1.
		int64_t arrivalTime = 17;

		const int64_t g1ArrivalTime = arrivalTime;

		expectFalse(0, arrivalTime, 1);

		// G2.
		arrivalTime += BurstThresholdMs + 1;

		const int64_t g2ArrivalTime = arrivalTime;

		expectFalse(TriggerNewGroupUs, arrivalTime, 2);

		// G3.
		arrivalTime += BurstThresholdMs + 1;

		const int64_t g3ArrivalTime = arrivalTime;

		expectTrue(
		  2 * TriggerNewGroupUs, arrivalTime, 1, TriggerNewGroupUs, g2ArrivalTime - g1ArrivalTime, 1, 0);

		// G4. The first packet of the fourth group yields the deltas between the
		// second and the third.
		arrivalTime += BurstThresholdMs + 1;

		expectTrue(
		  3 * TriggerNewGroupUs, arrivalTime, 2, TriggerNewGroupUs, g3ArrivalTime - g2ArrivalTime, -1, 0);
	}

	SECTION("a group accumulates every packet whose timestamp falls within it")
	{
		// G1.
		int64_t arrivalTime = 17;

		const int64_t g1ArrivalTime = arrivalTime;

		expectFalse(0, arrivalTime, 1);

		// G2.
		arrivalTime += BurstThresholdMs + 1;

		expectFalse(TriggerNewGroupUs, 28, 2);

		int64_t timestamp = TriggerNewGroupUs;

		for (int64_t i{ 0 }; i < 10; ++i)
		{
			// A bunch of packets arriving within the same group.
			arrivalTime += BurstThresholdMs + 1;
			timestamp += MinStepUs;

			expectFalse(timestamp, arrivalTime, 1);
		}

		const int64_t g2ArrivalTime = arrivalTime;
		const int64_t g2Timestamp   = timestamp;

		// G3.
		arrivalTime = 500;

		// Delta G2-G1.
		expectTrue(
		  2 * TriggerNewGroupUs,
		  arrivalTime,
		  100,
		  g2Timestamp,
		  g2ArrivalTime - g1ArrivalTime,
		  (2 + 10) - 1,
		  0);
	}

	SECTION("a packet older than the group it arrives into is dropped")
	{
		// G1.
		int64_t arrivalTime = 17;
		int64_t timestamp   = 0;

		expectFalse(timestamp, arrivalTime, 1);

		const int64_t g1Timestamp   = timestamp;
		const int64_t g1ArrivalTime = arrivalTime;

		// G2.
		arrivalTime += 11;
		timestamp += TriggerNewGroupUs;

		expectFalse(timestamp, 28, 2);

		for (int64_t i{ 0 }; i < 10; ++i)
		{
			arrivalTime += BurstThresholdMs + 1;
			timestamp += MinStepUs;

			expectFalse(timestamp, arrivalTime, 1);
		}

		const int64_t g2Timestamp   = timestamp;
		const int64_t g2ArrivalTime = arrivalTime;

		// This packet is out of order and is dropped.
		arrivalTime = 281;

		expectFalse(g1Timestamp, arrivalTime, 100);

		// G3.
		arrivalTime = 500;
		timestamp   = 2 * TriggerNewGroupUs;

		// Delta G2-G1.
		expectTrue(
		  timestamp,
		  arrivalTime,
		  100,
		  g2Timestamp - g1Timestamp,
		  g2ArrivalTime - g1ArrivalTime,
		  (2 + 10) - 1,
		  0);
	}

	SECTION("packets whose timestamps go backwards still join the group they belong to")
	{
		// G1.
		int64_t arrivalTime = 17;
		int64_t timestamp   = 0;

		expectFalse(timestamp, arrivalTime, 1);

		const int64_t g1Timestamp   = timestamp;
		const int64_t g1ArrivalTime = arrivalTime;

		// G2.
		timestamp += TriggerNewGroupUs;
		arrivalTime += 11;

		expectFalse(TriggerNewGroupUs, 28, 2);

		timestamp += 10 * MinStepUs;

		const int64_t g2Timestamp = timestamp;

		for (int64_t i{ 0 }; i < 10; ++i)
		{
			// These arrive with timestamps in decreasing order but are accumulated
			// into the group anyway, because they are still higher than the one it
			// began with.
			arrivalTime += BurstThresholdMs + 1;

			expectFalse(timestamp, arrivalTime, 1);

			timestamp -= MinStepUs;
		}

		const int64_t g2ArrivalTime = arrivalTime;

		// This one, however, is older than the group and is dropped.
		arrivalTime = 281;
		timestamp   = g1Timestamp;

		expectFalse(timestamp, arrivalTime, 100);

		// G3.
		timestamp   = 2 * TriggerNewGroupUs;
		arrivalTime = 500;

		expectTrue(
		  timestamp,
		  arrivalTime,
		  100,
		  g2Timestamp - g1Timestamp,
		  g2ArrivalTime - g1ArrivalTime,
		  (2 + 10) - 1,
		  0);
	}

	SECTION("packets arriving in a burst make up a single group")
	{
		// G1.
		const int64_t g1ArrivalTime = 17;

		expectFalse(0, g1ArrivalTime, 1);

		// G2.
		int64_t timestamp = TriggerNewGroupUs;
		// Simulate no packets arriving for 100 ms.
		int64_t arrivalTime = 100;

		for (int64_t i{ 0 }; i < 10; ++i)
		{
			// A bunch of packets arriving in one burst, within 5 ms of each other.
			timestamp += 30000;
			arrivalTime += BurstThresholdMs;

			expectFalse(timestamp, arrivalTime, 1);
		}

		const int64_t g2ArrivalTime = arrivalTime;
		const int64_t g2Timestamp   = timestamp;

		// G3.
		timestamp += 30000;
		arrivalTime += BurstThresholdMs + 1;

		// Delta G2-G1.
		expectTrue(timestamp, arrivalTime, 100, g2Timestamp, g2ArrivalTime - g1ArrivalTime, 10 - 1, 0);
	}

	SECTION("packets spaced further apart than the burst threshold make up their own groups")
	{
		// G1.
		expectFalse(0, 17, 1);

		// G2.
		const int64_t timestamp   = TriggerNewGroupUs;
		const int64_t arrivalTime = 28;

		expectFalse(timestamp, arrivalTime, 2);

		// G3. Delta G2-G1.
		expectTrue(
		  TriggerNewGroupUs + 30000,
		  arrivalTime + BurstThresholdMs + 1,
		  100,
		  timestamp - 0,
		  arrivalTime - 17,
		  2 - 1,
		  0);
	}

	// NOTE: Each of these instants lands on 0xfffffffe once turned into its kind
	// of timestamp, so the walk below crosses the wrap around.
	constexpr int64_t StartRtpTimestampWrapUs{ 47721858827 };
	constexpr int64_t StartAbsSendTimeWrapUs{ 63999995 };

	SECTION("RTP timestamps wrapping around")
	{
		wrapTestHelper(StartRtpTimestampWrapUs, 1, false);
	}

	SECTION("abs-send-time wrapping around")
	{
		wrapTestHelper(StartAbsSendTimeWrapUs, 1, false);
	}

	SECTION("RTP timestamps wrapping around with a group whose timestamps go backwards")
	{
		wrapTestHelper(StartRtpTimestampWrapUs, 1, true);
	}

	SECTION("abs-send-time wrapping around with a group whose timestamps go backwards")
	{
		wrapTestHelper(StartAbsSendTimeWrapUs, 1, true);
	}

	SECTION("the arrival times jumping forward resets everything")
	{
		constexpr size_t PacketSize{ 1000 };
		constexpr int64_t TimeDeltaMs{ 30 };

		int64_t sendTimeMs    = 10000;
		int64_t arrivalTimeMs = 20000;
		int64_t systemTimeMs  = 30000;

		REQUIRE(
		  !interArrival
		     .ComputeDeltas(
		       static_cast<uint32_t>(sendTimeMs), arrivalTimeMs * 1000, systemTimeMs * 1000, PacketSize)
		     .has_value());

		sendTimeMs += TimeDeltaMs;
		arrivalTimeMs += TimeDeltaMs;
		systemTimeMs += TimeDeltaMs;

		REQUIRE(
		  !interArrival
		     .ComputeDeltas(
		       static_cast<uint32_t>(sendTimeMs), arrivalTimeMs * 1000, systemTimeMs * 1000, PacketSize)
		     .has_value());

		sendTimeMs += TimeDeltaMs;
		arrivalTimeMs += TimeDeltaMs + ArrivalTimeOffsetThresholdMs;
		systemTimeMs += TimeDeltaMs;

		auto deltas = interArrival.ComputeDeltas(
		  static_cast<uint32_t>(sendTimeMs), arrivalTimeMs * 1000, systemTimeMs * 1000, PacketSize);

		REQUIRE(deltas.has_value());
		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		REQUIRE(deltas.value().timestampDelta == TimeDeltaMs);
		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		REQUIRE(deltas.value().arrivalDeltaUs == TimeDeltaMs * 1000);
		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		REQUIRE(deltas.value().sizeDelta == 0);

		sendTimeMs += TimeDeltaMs;
		arrivalTimeMs += TimeDeltaMs;
		systemTimeMs += TimeDeltaMs;

		// The jump of the previous arrival time is detected now and causes a reset.
		REQUIRE(
		  !interArrival
		     .ComputeDeltas(
		       static_cast<uint32_t>(sendTimeMs), arrivalTimeMs * 1000, systemTimeMs * 1000, PacketSize)
		     .has_value());

		// The two packets that follow give no deltas, since everything starts over.
		for (int64_t i{ 0 }; i < 2; ++i)
		{
			sendTimeMs += TimeDeltaMs;
			arrivalTimeMs += TimeDeltaMs;
			systemTimeMs += TimeDeltaMs;

			REQUIRE(
			  !interArrival
			     .ComputeDeltas(
			       static_cast<uint32_t>(sendTimeMs), arrivalTimeMs * 1000, systemTimeMs * 1000, PacketSize)
			     .has_value());
		}

		sendTimeMs += TimeDeltaMs;
		arrivalTimeMs += TimeDeltaMs;
		systemTimeMs += TimeDeltaMs;

		deltas = interArrival.ComputeDeltas(
		  static_cast<uint32_t>(sendTimeMs), arrivalTimeMs * 1000, systemTimeMs * 1000, PacketSize);

		REQUIRE(deltas.has_value());
		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		REQUIRE(deltas.value().timestampDelta == TimeDeltaMs);
		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		REQUIRE(deltas.value().arrivalDeltaUs == TimeDeltaMs * 1000);
		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		REQUIRE(deltas.value().sizeDelta == 0);
	}

	SECTION("the arrival times jumping backwards resets everything")
	{
		constexpr size_t PacketSize{ 1000 };
		constexpr int64_t TimeDeltaMs{ 30 };

		int64_t sendTimeMs    = 10000;
		int64_t arrivalTimeMs = 20000;
		int64_t systemTimeMs  = 30000;

		REQUIRE(
		  !interArrival
		     .ComputeDeltas(
		       static_cast<uint32_t>(sendTimeMs), arrivalTimeMs * 1000, systemTimeMs * 1000, PacketSize)
		     .has_value());

		sendTimeMs += TimeDeltaMs;
		arrivalTimeMs += TimeDeltaMs;
		systemTimeMs += TimeDeltaMs;

		REQUIRE(
		  !interArrival
		     .ComputeDeltas(
		       static_cast<uint32_t>(sendTimeMs), arrivalTimeMs * 1000, systemTimeMs * 1000, PacketSize)
		     .has_value());

		sendTimeMs += TimeDeltaMs;
		arrivalTimeMs += TimeDeltaMs;
		systemTimeMs += TimeDeltaMs;

		auto deltas = interArrival.ComputeDeltas(
		  static_cast<uint32_t>(sendTimeMs), arrivalTimeMs * 1000, systemTimeMs * 1000, PacketSize);

		REQUIRE(deltas.has_value());
		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		REQUIRE(deltas.value().timestampDelta == TimeDeltaMs);
		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		REQUIRE(deltas.value().arrivalDeltaUs == TimeDeltaMs * 1000);
		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		REQUIRE(deltas.value().sizeDelta == 0);

		// Three groups arriving out of order give nothing, and after the reset two
		// more are needed before the first valid deltas show up again.
		arrivalTimeMs -= 1000;

		for (size_t i{ 0 }; i < ReorderedResetThreshold + 3; ++i)
		{
			sendTimeMs += TimeDeltaMs;
			arrivalTimeMs += TimeDeltaMs;
			systemTimeMs += TimeDeltaMs;

			REQUIRE(
			  !interArrival
			     .ComputeDeltas(
			       static_cast<uint32_t>(sendTimeMs), arrivalTimeMs * 1000, systemTimeMs * 1000, PacketSize)
			     .has_value());
		}

		sendTimeMs += TimeDeltaMs;
		arrivalTimeMs += TimeDeltaMs;
		systemTimeMs += TimeDeltaMs;

		deltas = interArrival.ComputeDeltas(
		  static_cast<uint32_t>(sendTimeMs), arrivalTimeMs * 1000, systemTimeMs * 1000, PacketSize);

		REQUIRE(deltas.has_value());
		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		REQUIRE(deltas.value().timestampDelta == TimeDeltaMs);
		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		REQUIRE(deltas.value().arrivalDeltaUs == TimeDeltaMs * 1000);
		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		REQUIRE(deltas.value().sizeDelta == 0);
	}
}
