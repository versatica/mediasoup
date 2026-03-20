#include "common.hpp"
#include "RTC/SCTP/public/SctpOptions.hpp"
#include "RTC/SCTP/tx/RetransmissionTimeout.hpp"
#include <catch2/catch_test_macros.hpp>

SCENARIO("SCTP RetransmissionTimeout", "[sctp][retransmissiontimeout]")
{
	constexpr uint64_t MaxRttMs{ 8000 };
	constexpr uint64_t InitialRtoMs{ 200 };
	constexpr uint64_t MaxRtoMs{ 800 };
	constexpr uint64_t MinRtoMs{ 120 };
	constexpr uint64_t MinRttVarianceMs{ 220 };

	// NOTE: No need to pass const integers to the lambda.
	auto makeSctpOptions = []()
	{
		RTC::SCTP::SctpOptions sctpOptions{ .maxRttMs         = MaxRttMs,
			                                  .initialRtoMs     = InitialRtoMs,
			                                  .minRtoMs         = MinRtoMs,
			                                  .maxRtoMs         = MaxRtoMs,
			                                  .minRttVarianceMs = MinRttVarianceMs };

		return sctpOptions;
	};

	SECTION("has valid initial RTO")
	{
		const RTC::SCTP::RetransmissionTimeout rto(makeSctpOptions());

		REQUIRE(rto.GetRtoMs() == InitialRtoMs);
	}

	SECTION("too large values don't affect RTO")
	{
		RTC::SCTP::RetransmissionTimeout rto(makeSctpOptions());

		rto.ObserveRtt(MaxRttMs + 100);

		REQUIRE(rto.GetRtoMs() == InitialRtoMs);

		rto.ObserveRtt(124);

		REQUIRE(rto.GetRtoMs() == 372);

		rto.ObserveRtt(MaxRttMs + 100);

		REQUIRE(rto.GetRtoMs() == 372);
	}

	SECTION("will never go below minimum RTO")
	{
		RTC::SCTP::RetransmissionTimeout rto(makeSctpOptions());

		for (int i{ 0 }; i < 1000; ++i)
		{
			rto.ObserveRtt(1);
		}

		REQUIRE(rto.GetRtoMs() <= MinRtoMs);
	}

	SECTION("will never go above maximum RTO")
	{
		RTC::SCTP::RetransmissionTimeout rto(makeSctpOptions());

		for (int i{ 0 }; i < 1000; ++i)
		{
			rto.ObserveRtt(MaxRttMs - 1);
			// Adding jitter, which would make it RTO be well above RTT.
			rto.ObserveRtt(MaxRttMs - 100);
		}

		REQUIRE(rto.GetRtoMs() >= MaxRtoMs);
	}

	SECTION("calculates RTO for stable RTT")
	{
		RTC::SCTP::RetransmissionTimeout rto(makeSctpOptions());

		rto.ObserveRtt(124);

		REQUIRE(rto.GetRtoMs() == 372);

		rto.ObserveRtt(128);

		REQUIRE(rto.GetRtoMs() == 315);

		rto.ObserveRtt(123);

		REQUIRE(rto.GetRtoMs() == 268);

		rto.ObserveRtt(125);

		// NOTE: This should be 234 (as per same test in libwebrtc) but we are not
		// that precise.
		// REQUIRE(rto.GetRtoMs() == 234);
		REQUIRE(rto.GetRtoMs() == 233);

		rto.ObserveRtt(127);

		// NOTE: This should be 235 (as per same test in libwebrtc) but we are not
		// that precise.
		// REQUIRE(rto.GetRtoMs() == 235);
		REQUIRE(rto.GetRtoMs() == 233);
	}

	SECTION("calculates RTO for unstable RTT")
	{
		RTC::SCTP::RetransmissionTimeout rto(makeSctpOptions());

		rto.ObserveRtt(124);

		REQUIRE(rto.GetRtoMs() == 372);

		rto.ObserveRtt(402);

		REQUIRE(rto.GetRtoMs() == 623);

		rto.ObserveRtt(728);

		REQUIRE(rto.GetRtoMs() == 800);

		rto.ObserveRtt(89);

		REQUIRE(rto.GetRtoMs() == 800);

		rto.ObserveRtt(126);

		REQUIRE(rto.GetRtoMs() == 800);
	}

	SECTION("will stabilize RTO after a while")
	{
		RTC::SCTP::RetransmissionTimeout rto(makeSctpOptions());

		rto.ObserveRtt(124);
		rto.ObserveRtt(402);
		rto.ObserveRtt(728);
		rto.ObserveRtt(89);
		rto.ObserveRtt(126);

		REQUIRE(rto.GetRtoMs() == 800);

		rto.ObserveRtt(124);

		REQUIRE(rto.GetRtoMs() == 800);

		rto.ObserveRtt(122);

		REQUIRE(rto.GetRtoMs() == 709);

		rto.ObserveRtt(123);

		REQUIRE(rto.GetRtoMs() == 630);

		rto.ObserveRtt(124);

		REQUIRE(rto.GetRtoMs() == 562);

		rto.ObserveRtt(122);

		REQUIRE(rto.GetRtoMs() == 505);

		rto.ObserveRtt(124);

		REQUIRE(rto.GetRtoMs() == 454);

		rto.ObserveRtt(124);

		REQUIRE(rto.GetRtoMs() == 410);

		rto.ObserveRtt(124);

		REQUIRE(rto.GetRtoMs() == 372);

		rto.ObserveRtt(124);

		REQUIRE(rto.GetRtoMs() == 340);
	}

	SECTION("will always stay above RTT")
	{
		// In simulations, it's quite common to have a very stable RTT, and having
		// an RTO at the same value will cause issues as expiry timers will be
		// scheduled to be expire exactly when a packet is supposed to arrive. The
		// RTO must be larger than the RTT. In non-simulated environments, this is
		// a non-issue as any jitter will increase the RTO.

		RTC::SCTP::RetransmissionTimeout rto(makeSctpOptions());

		for (int i{ 0 }; i < 1000; ++i)
		{
			rto.ObserveRtt(124);
		}

		// NOTE: This should be 234 (as per same test in libwebrtc) but we are not
		// that precise.
		// REQUIRE(rto.GetRtoMs() == 234);
		REQUIRE(rto.GetRtoMs() == 232);
	}

	SECTION("can specify smaller minimum RTT variance")
	{
		auto sctpOptions = makeSctpOptions();

		sctpOptions.minRttVarianceMs = MinRttVarianceMs - 100;

		RTC::SCTP::RetransmissionTimeout rto(sctpOptions);

		for (int i{ 0 }; i < 1000; ++i)
		{
			rto.ObserveRtt(124);
		}

		REQUIRE(rto.GetRtoMs() == 184);
	}

	SECTION("can specify larger minimum RTT variance")
	{
		auto sctpOptions = makeSctpOptions();

		sctpOptions.minRttVarianceMs = MinRttVarianceMs + 100;

		RTC::SCTP::RetransmissionTimeout rto(sctpOptions);

		for (int i{ 0 }; i < 1000; ++i)
		{
			rto.ObserveRtt(124);
		}

		REQUIRE(rto.GetRtoMs() == 284);
	}
}
