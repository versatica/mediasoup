#include "common.hpp"
#include "RTC/SCTP/public/SctpOptions.hpp"
#include "RTC/SCTP/tx/RetransmissionErrorCounter.hpp"
#include <catch2/catch_test_macros.hpp>
#include <optional>

SCENARIO("SCTP RetransmissionErrorCounter", "[sctp][retransmissionerrorcounter]")
{
	SECTION("can handle zero retransmission")
	{
		const RTC::SCTP::SctpOptions sctpOptions{ .maxRetransmissions = 0 };
		RTC::SCTP::RetransmissionErrorCounter counter(sctpOptions);

		REQUIRE(counter.GetCounter() == 0);
		REQUIRE(counter.Increment("test") == false); // One is too many.
		REQUIRE(counter.GetCounter() == 1);
	}

	SECTION("is exhausted at maximum")
	{
		const RTC::SCTP::SctpOptions sctpOptions{ .maxRetransmissions = 3 };
		RTC::SCTP::RetransmissionErrorCounter counter(sctpOptions);

		REQUIRE(counter.Increment("test") == true); // 1
		REQUIRE(counter.GetCounter() == 1);
		REQUIRE(counter.IsExhausted() == false);
		REQUIRE(counter.Increment("test") == true); // 2
		REQUIRE(counter.GetCounter() == 2);
		REQUIRE(counter.IsExhausted() == false);
		REQUIRE(counter.Increment("test") == true); // 3
		REQUIRE(counter.GetCounter() == 3);
		REQUIRE(counter.IsExhausted() == false);
		REQUIRE(counter.Increment("test") == false); // Too many retransmissions.
		REQUIRE(counter.GetCounter() == 4);
		REQUIRE(counter.IsExhausted() == true);
		REQUIRE(counter.Increment("test") == false); // One after too many.
		REQUIRE(counter.GetCounter() == 5);
		REQUIRE(counter.IsExhausted() == true);
	}

	SECTION("clearing counter")
	{
		const RTC::SCTP::SctpOptions sctpOptions{ .maxRetransmissions = 3 };
		RTC::SCTP::RetransmissionErrorCounter counter(sctpOptions);

		REQUIRE(counter.Increment("test") == true); // 1
		REQUIRE(counter.Increment("test") == true); // 2
		REQUIRE(counter.GetCounter() == 2);
		REQUIRE(counter.IsExhausted() == false);

		counter.Clear();

		REQUIRE(counter.Increment("test") == true); // 1
		REQUIRE(counter.Increment("test") == true); // 2
		REQUIRE(counter.Increment("test") == true); // 3
		REQUIRE(counter.IsExhausted() == false);
		REQUIRE(counter.Increment("test") == false); // Too many retransmissions.
		REQUIRE(counter.GetCounter() == 4);
		REQUIRE(counter.IsExhausted() == true);
	}

	SECTION("can be limitless")
	{
		const RTC::SCTP::SctpOptions sctpOptions{ .maxRetransmissions = std::nullopt };
		RTC::SCTP::RetransmissionErrorCounter counter(sctpOptions);

		for (size_t i{ 1 }; i < 1000; ++i)
		{
			REQUIRE(counter.Increment("test") == true);
			REQUIRE(counter.GetCounter() == i);
			REQUIRE(counter.IsExhausted() == false);
		}
	}
}
