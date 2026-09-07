#include "common.hpp"
#include "RTC/SCTP/public/SctpOptions.hpp"
#include "RTC/SCTP/tx/RetransmissionTimeout.hpp"
#include <catch2/catch_test_macros.hpp>

SCENARIO("SCTP RetransmissionTimeout", "[sctp][retransmissiontimeout]")
{
	// NOTE: The options are in milliseconds, while the class works in
	// microseconds, so the expectations below are in microseconds.
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

		REQUIRE(rto.GetRtoUs() == InitialRtoMs * 1000);
	}

	SECTION("too large values don't affect RTO")
	{
		RTC::SCTP::RetransmissionTimeout rto(makeSctpOptions());

		rto.ObserveRttUs((MaxRttMs + 100) * 1000);

		REQUIRE(rto.GetRtoUs() == InitialRtoMs * 1000);

		rto.ObserveRttUs(124 * 1000);

		REQUIRE(rto.GetRtoUs() == 372000);

		rto.ObserveRttUs((MaxRttMs + 100) * 1000);

		REQUIRE(rto.GetRtoUs() == 372000);
	}

	SECTION("sub-millisecond RTT is observed")
	{
		// A RTT below a millisecond is a normal measurement over a LAN or over
		// loopback, and it must take part in the RTO computation.
		RTC::SCTP::RetransmissionTimeout rto(makeSctpOptions());

		rto.ObserveRttUs(500);

		// First measurement, so srtt is the RTT itself and the RTT variance is half
		// of it, which the minimum RTT variance floor then dominates.
		REQUIRE(rto.GetRtoUs() == MinRtoMs * 1000);
		REQUIRE(rto.GetSrttUs() == 500);
	}

	SECTION("zero and negative values don't affect RTO")
	{
		RTC::SCTP::RetransmissionTimeout rto(makeSctpOptions());

		rto.ObserveRttUs(0);

		REQUIRE(rto.GetRtoUs() == InitialRtoMs * 1000);

		rto.ObserveRttUs(-1);

		REQUIRE(rto.GetRtoUs() == InitialRtoMs * 1000);
	}

	SECTION("will never go below minimum RTO")
	{
		RTC::SCTP::RetransmissionTimeout rto(makeSctpOptions());

		for (int i{ 0 }; i < 1000; ++i)
		{
			rto.ObserveRttUs(1 * 1000);
		}

		REQUIRE(rto.GetRtoUs() <= MinRtoMs * 1000);
	}

	SECTION("will never go above maximum RTO")
	{
		RTC::SCTP::RetransmissionTimeout rto(makeSctpOptions());

		for (int i{ 0 }; i < 1000; ++i)
		{
			rto.ObserveRttUs((MaxRttMs - 1) * 1000);
			// Adding jitter, which would make it RTO be well above RTT.
			rto.ObserveRttUs((MaxRttMs - 100) * 1000);
		}

		REQUIRE(rto.GetRtoUs() >= MaxRtoMs * 1000);
	}

	SECTION("calculates RTO for stable RTT")
	{
		RTC::SCTP::RetransmissionTimeout rto(makeSctpOptions());

		rto.ObserveRttUs(124 * 1000);

		REQUIRE(rto.GetRtoUs() == 372000);

		rto.ObserveRttUs(128 * 1000);

		REQUIRE(rto.GetRtoUs() == 314500);

		rto.ObserveRttUs(123 * 1000);

		REQUIRE(rto.GetRtoUs() == 268313);

		rto.ObserveRttUs(125 * 1000);

		REQUIRE(rto.GetRtoUs() == 234398);

		rto.ObserveRttUs(127 * 1000);

		REQUIRE(rto.GetRtoUs() == 234724);
	}

	SECTION("calculates RTO for unstable RTT")
	{
		RTC::SCTP::RetransmissionTimeout rto(makeSctpOptions());

		rto.ObserveRttUs(124 * 1000);

		REQUIRE(rto.GetRtoUs() == 372000);

		rto.ObserveRttUs(402 * 1000);

		REQUIRE(rto.GetRtoUs() == 622750);

		rto.ObserveRttUs(728 * 1000);

		REQUIRE(rto.GetRtoUs() == 800000);

		rto.ObserveRttUs(89 * 1000);

		REQUIRE(rto.GetRtoUs() == 800000);

		rto.ObserveRttUs(126 * 1000);

		REQUIRE(rto.GetRtoUs() == 800000);
	}

	SECTION("will stabilize RTO after a while")
	{
		RTC::SCTP::RetransmissionTimeout rto(makeSctpOptions());

		rto.ObserveRttUs(124 * 1000);
		rto.ObserveRttUs(402 * 1000);
		rto.ObserveRttUs(728 * 1000);
		rto.ObserveRttUs(89 * 1000);
		rto.ObserveRttUs(126 * 1000);

		REQUIRE(rto.GetRtoUs() == 800000);

		rto.ObserveRttUs(124 * 1000);

		REQUIRE(rto.GetRtoUs() == 800000);

		rto.ObserveRttUs(122 * 1000);

		REQUIRE(rto.GetRtoUs() == 709247);

		rto.ObserveRttUs(123 * 1000);

		REQUIRE(rto.GetRtoUs() == 630287);

		rto.ObserveRttUs(124 * 1000);

		REQUIRE(rto.GetRtoUs() == 561742);

		rto.ObserveRttUs(122 * 1000);

		REQUIRE(rto.GetRtoUs() == 504830);

		rto.ObserveRttUs(124 * 1000);

		REQUIRE(rto.GetRtoUs() == 453768);

		rto.ObserveRttUs(124 * 1000);

		REQUIRE(rto.GetRtoUs() == 409954);

		rto.ObserveRttUs(124 * 1000);

		REQUIRE(rto.GetRtoUs() == 372264);

		rto.ObserveRttUs(124 * 1000);

		REQUIRE(rto.GetRtoUs() == 339772);
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
			rto.ObserveRttUs(124 * 1000);
		}

		REQUIRE(rto.GetRtoUs() == 234000);
	}

	SECTION("can specify smaller minimum RTT variance")
	{
		auto sctpOptions = makeSctpOptions();

		sctpOptions.minRttVarianceMs = MinRttVarianceMs - 100;

		RTC::SCTP::RetransmissionTimeout rto(sctpOptions);

		for (int i{ 0 }; i < 1000; ++i)
		{
			rto.ObserveRttUs(124 * 1000);
		}

		REQUIRE(rto.GetRtoUs() == 184000);
	}

	SECTION("can specify larger minimum RTT variance")
	{
		auto sctpOptions = makeSctpOptions();

		sctpOptions.minRttVarianceMs = MinRttVarianceMs + 100;

		RTC::SCTP::RetransmissionTimeout rto(sctpOptions);

		for (int i{ 0 }; i < 1000; ++i)
		{
			rto.ObserveRttUs(124 * 1000);
		}

		REQUIRE(rto.GetRtoUs() == 284000);
	}
}
