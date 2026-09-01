#include "common.hpp"
#include "RTC/BWE/InterArrivalDelta.hpp"
#include <catch2/catch_test_macros.hpp>

SCENARIO("BWE InterArrivalDelta", "[bwe][interarrivaldelta]")
{
	constexpr size_t PacketSize{ 1000 };
	// Base network delay between send and arrival times.
	constexpr uint64_t BaseDelayMs{ 100 };
	constexpr uint64_t InitialSendTimeMs{ 1000000 };

	// Feed a packet with explicit arrival and feedback times.
	auto feedAt = [](
	                RTC::BWE::InterArrivalDelta& interArrivalDelta,
	                uint64_t sendTimeMs,
	                uint64_t arrivalTimeMs,
	                uint64_t feedbackAtMs) -> std::optional<RTC::BWE::InterArrivalDelta::Deltas>
	{
		return interArrivalDelta.ComputeDeltas(sendTimeMs, arrivalTimeMs, feedbackAtMs, PacketSize);
	};

	// Feed a packet whose arrival time is its send time plus the base network
	// delay, which is the well behaved case.
	auto feed = [&feedAt](
	              RTC::BWE::InterArrivalDelta& interArrivalDelta,
	              uint64_t sendTimeMs) -> std::optional<RTC::BWE::InterArrivalDelta::Deltas>
	{
		const uint64_t arrivalTimeMs = sendTimeMs + BaseDelayMs;

		return feedAt(interArrivalDelta, sendTimeMs, arrivalTimeMs, /*feedbackAtMs*/ arrivalTimeMs);
	};

	SECTION("first packet doesn't produce deltas")
	{
		RTC::BWE::InterArrivalDelta interArrivalDelta;

		REQUIRE(!feed(interArrivalDelta, InitialSendTimeMs).has_value());
	}

	SECTION("packets within the same send time group don't produce deltas")
	{
		RTC::BWE::InterArrivalDelta interArrivalDelta;

		// All of them are sent within 5 ms of the first one.
		REQUIRE(!feed(interArrivalDelta, InitialSendTimeMs).has_value());
		REQUIRE(!feed(interArrivalDelta, InitialSendTimeMs + 1).has_value());
		REQUIRE(!feed(interArrivalDelta, InitialSendTimeMs + 3).has_value());
		REQUIRE(!feed(interArrivalDelta, InitialSendTimeMs + 5).has_value());
	}

	SECTION("first group transition doesn't produce deltas yet")
	{
		RTC::BWE::InterArrivalDelta interArrivalDelta;

		REQUIRE(!feed(interArrivalDelta, InitialSendTimeMs).has_value());

		// Sent more than 5 ms later, so this starts a second group. There is still
		// no previous group to compare with.
		REQUIRE(!feed(interArrivalDelta, InitialSendTimeMs + 10).has_value());
	}

	SECTION("third group produces deltas between the two previous ones")
	{
		RTC::BWE::InterArrivalDelta interArrivalDelta;

		// Group A: three packets, the last one sent at +4.
		REQUIRE(!feed(interArrivalDelta, InitialSendTimeMs).has_value());
		REQUIRE(!feed(interArrivalDelta, InitialSendTimeMs + 2).has_value());
		REQUIRE(!feed(interArrivalDelta, InitialSendTimeMs + 4).has_value());

		// Group B: a single packet sent at +10.
		REQUIRE(!feed(interArrivalDelta, InitialSendTimeMs + 10).has_value());

		// Group C starts, so the deltas between B and A are ready.
		const auto deltas = feed(interArrivalDelta, InitialSendTimeMs + 20);

		REQUIRE(deltas.has_value());
		// Send time of B (+10) minus send time of A (+4).
		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		REQUIRE(deltas.value().sendDeltaMs == 6);
		// Same value since every packet had the very same network delay.
		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		REQUIRE(deltas.value().arrivalDeltaMs == 6);
		// B holds a single packet while A holds three of them.
		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		REQUIRE(deltas.value().sizeDelta == -2 * static_cast<int64_t>(PacketSize));
	}

	SECTION("increasing network delay produces a bigger arrival delta")
	{
		RTC::BWE::InterArrivalDelta interArrivalDelta;

		// Group A.
		REQUIRE(!feedAt(
		           interArrivalDelta,
		           /*sendTimeMs*/ InitialSendTimeMs,
		           /*arrivalTimeMs*/ InitialSendTimeMs + BaseDelayMs,
		           /*feedbackAtMs*/ InitialSendTimeMs + BaseDelayMs)
		           .has_value());

		// Group B, arriving 20 ms later than the base delay would suggest.
		REQUIRE(!feedAt(
		           interArrivalDelta,
		           /*sendTimeMs*/ InitialSendTimeMs + 10,
		           /*arrivalTimeMs*/ InitialSendTimeMs + 10 + BaseDelayMs + 20,
		           /*feedbackAtMs*/ InitialSendTimeMs + 10 + BaseDelayMs + 20)
		           .has_value());

		// Group C.
		const auto deltas = feedAt(
		  interArrivalDelta,
		  /*sendTimeMs*/ InitialSendTimeMs + 20,
		  /*arrivalTimeMs*/ InitialSendTimeMs + 20 + BaseDelayMs + 20,
		  /*feedbackAtMs*/ InitialSendTimeMs + 20 + BaseDelayMs + 20);

		REQUIRE(deltas.has_value());
		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		REQUIRE(deltas.value().sendDeltaMs == 10);
		// 20 ms of extra queuing delay on top of the send delta.
		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		REQUIRE(deltas.value().arrivalDeltaMs == 30);
	}

	SECTION("packet sent before the current group is discarded")
	{
		RTC::BWE::InterArrivalDelta interArrivalDelta;

		REQUIRE(!feed(interArrivalDelta, InitialSendTimeMs + 100).has_value());

		// Sent before the first packet of the current group.
		REQUIRE(!feed(interArrivalDelta, InitialSendTimeMs).has_value());
	}

	SECTION("a packet serialized by the network is absorbed into the ongoing burst")
	{
		RTC::BWE::InterArrivalDelta interArrivalDelta;

		// Group A.
		REQUIRE(!feedAt(
		           interArrivalDelta,
		           /*sendTimeMs*/ InitialSendTimeMs,
		           /*arrivalTimeMs*/ InitialSendTimeMs + BaseDelayMs,
		           /*feedbackAtMs*/ InitialSendTimeMs + BaseDelayMs)
		           .has_value());

		// Sent 10 ms later, so it would start a new group, but it arrived just 2 ms
		// later, meaning that the network serialized it together with the previous
		// packet. It must be absorbed into the very same burst.
		REQUIRE(!feedAt(
		           interArrivalDelta,
		           /*sendTimeMs*/ InitialSendTimeMs + 10,
		           /*arrivalTimeMs*/ InitialSendTimeMs + BaseDelayMs + 2,
		           /*feedbackAtMs*/ InitialSendTimeMs + BaseDelayMs + 2)
		           .has_value());

		// Given that both packets are still within the same group, the next group
		// transition cannot produce deltas yet.
		REQUIRE(!feedAt(
		           interArrivalDelta,
		           /*sendTimeMs*/ InitialSendTimeMs + 200,
		           /*arrivalTimeMs*/ InitialSendTimeMs + 200 + BaseDelayMs,
		           /*feedbackAtMs*/ InitialSendTimeMs + 200 + BaseDelayMs)
		           .has_value());
	}

	SECTION("a burst longer than the maximum duration ends")
	{
		RTC::BWE::InterArrivalDelta interArrivalDelta;

		uint64_t sendTimeMs    = InitialSendTimeMs;
		uint64_t arrivalTimeMs = InitialSendTimeMs + BaseDelayMs;
		bool gotDeltas{ false };

		// Each packet is sent 10 ms after the previous one but arrives just 4 ms
		// later, so every single one of them qualifies for the ongoing burst. The
		// burst must end anyway once it lasts 100 ms, which takes 26 packets. Two
		// bursts must be completed before deltas can be computed between them.
		for (int i{ 0 }; i < 60; ++i)
		{
			sendTimeMs += 10;
			arrivalTimeMs += 4;

			if (feedAt(interArrivalDelta, sendTimeMs, arrivalTimeMs, /*feedbackAtMs*/ arrivalTimeMs).has_value())
			{
				gotDeltas = true;
			}
		}

		// The burst was broken, so groups were created and deltas were computed.
		REQUIRE(gotDeltas);
	}

	SECTION("a remote clock jump resets the state")
	{
		RTC::BWE::InterArrivalDelta interArrivalDelta;

		// Group A.
		REQUIRE(!feedAt(
		           interArrivalDelta,
		           /*sendTimeMs*/ InitialSendTimeMs,
		           /*arrivalTimeMs*/ InitialSendTimeMs + BaseDelayMs,
		           /*feedbackAtMs*/ InitialSendTimeMs + BaseDelayMs)
		           .has_value());

		// Group B, whose arrival time jumps 4 seconds ahead while our own clock just
		// advances 10 ms.
		REQUIRE(!feedAt(
		           interArrivalDelta,
		           /*sendTimeMs*/ InitialSendTimeMs + 10,
		           /*arrivalTimeMs*/ InitialSendTimeMs + BaseDelayMs + 4000,
		           /*feedbackAtMs*/ InitialSendTimeMs + BaseDelayMs + 10)
		           .has_value());

		// Group C would produce the deltas between B and A, but the clock offset
		// change is detected instead and the state is reset.
		REQUIRE(!feedAt(
		           interArrivalDelta,
		           /*sendTimeMs*/ InitialSendTimeMs + 20,
		           /*arrivalTimeMs*/ InitialSendTimeMs + BaseDelayMs + 4010,
		           /*feedbackAtMs*/ InitialSendTimeMs + BaseDelayMs + 20)
		           .has_value());

		// Given that the state was reset, three fresh groups are needed again before
		// any deltas are produced.
		REQUIRE(!feed(interArrivalDelta, InitialSendTimeMs + 30).has_value());
		REQUIRE(!feed(interArrivalDelta, InitialSendTimeMs + 40).has_value());
		REQUIRE(feed(interArrivalDelta, InitialSendTimeMs + 50).has_value());
	}

	SECTION("groups arriving out of order don't produce deltas")
	{
		RTC::BWE::InterArrivalDelta interArrivalDelta;

		// Group A, completed at +400.
		REQUIRE(!feedAt(
		           interArrivalDelta,
		           /*sendTimeMs*/ InitialSendTimeMs,
		           /*arrivalTimeMs*/ InitialSendTimeMs + 400,
		           /*feedbackAtMs*/ InitialSendTimeMs + 400)
		           .has_value());

		// Group B starts.
		REQUIRE(!feedAt(
		           interArrivalDelta,
		           /*sendTimeMs*/ InitialSendTimeMs + 20,
		           /*arrivalTimeMs*/ InitialSendTimeMs + 500,
		           /*feedbackAtMs*/ InitialSendTimeMs + 500)
		           .has_value());

		// A packet of group B that arrives way earlier than the rest, dragging the
		// arrival time of the whole group back to +100, hence before group A.
		REQUIRE(!feedAt(
		           interArrivalDelta,
		           /*sendTimeMs*/ InitialSendTimeMs + 22,
		           /*arrivalTimeMs*/ InitialSendTimeMs + 100,
		           /*feedbackAtMs*/ InitialSendTimeMs + 100)
		           .has_value());

		// Group C. The deltas between B and A would be negative, so they are
		// discarded.
		REQUIRE(!feedAt(
		           interArrivalDelta,
		           /*sendTimeMs*/ InitialSendTimeMs + 40,
		           /*arrivalTimeMs*/ InitialSendTimeMs + 600,
		           /*feedbackAtMs*/ InitialSendTimeMs + 600)
		           .has_value());
	}
}
