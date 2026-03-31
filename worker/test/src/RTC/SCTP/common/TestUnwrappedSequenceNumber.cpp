#include "common.hpp"
#include "RTC/SCTP/common/UnwrappedSequenceNumber.hpp"
#include <catch2/catch_test_macros.hpp>

SCENARIO("SCTP UnwrappedSequenceNumber", "[sctp]")
{
	using TestSequence = RTC::SCTP::UnwrappedSequenceNumber<uint16_t>;

	SECTION("simple unwrapping")
	{
		TestSequence::Unwrapper unwrapper;

		TestSequence s0 = unwrapper.Unwrap(0);
		TestSequence s1 = unwrapper.Unwrap(1);
		TestSequence s2 = unwrapper.Unwrap(2);
		TestSequence s3 = unwrapper.Unwrap(3);

		REQUIRE(s0 < s1);
		REQUIRE(s0 < s2);
		REQUIRE(s0 < s3);
		REQUIRE(s1 < s2);
		REQUIRE(s1 < s3);
		REQUIRE(s2 < s3);

		REQUIRE(TestSequence::Difference(s1, s0) == 1);
		REQUIRE(TestSequence::Difference(s2, s0) == 2);
		REQUIRE(TestSequence::Difference(s3, s0) == 3);

		REQUIRE(s1 > s0);
		REQUIRE(s2 > s0);
		REQUIRE(s3 > s0);
		REQUIRE(s2 > s1);
		REQUIRE(s3 > s1);
		REQUIRE(s3 > s2);

		s0.Increment();
		REQUIRE(s0 == s1);

		s1.Increment();
		REQUIRE(s1 == s2);

		s2.Increment();
		REQUIRE(s2 == s3);

		REQUIRE(TestSequence::AddTo(s0, 2) == s3);
	}

	SECTION("mid value unwrapping")
	{
		TestSequence::Unwrapper unwrapper;

		TestSequence s0 = unwrapper.Unwrap(0x7FFE);
		TestSequence s1 = unwrapper.Unwrap(0x7FFF);
		TestSequence s2 = unwrapper.Unwrap(0x8000);
		TestSequence s3 = unwrapper.Unwrap(0x8001);

		REQUIRE(s0 < s1);
		REQUIRE(s0 < s2);
		REQUIRE(s0 < s3);
		REQUIRE(s1 < s2);
		REQUIRE(s1 < s3);
		REQUIRE(s2 < s3);

		REQUIRE(TestSequence::Difference(s1, s0) == 1);
		REQUIRE(TestSequence::Difference(s2, s0) == 2);
		REQUIRE(TestSequence::Difference(s3, s0) == 3);

		REQUIRE(s1 > s0);
		REQUIRE(s2 > s0);
		REQUIRE(s3 > s0);
		REQUIRE(s2 > s1);
		REQUIRE(s3 > s1);
		REQUIRE(s3 > s2);

		s0.Increment();
		REQUIRE(s0 == s1);

		s1.Increment();
		REQUIRE(s1 == s2);

		s2.Increment();
		REQUIRE(s2 == s3);

		REQUIRE(TestSequence::AddTo(s0, 2) == s3);
	}

	SECTION("wrapped unwrapping")
	{
		TestSequence::Unwrapper unwrapper;

		TestSequence s0 = unwrapper.Unwrap(0xFFFE);
		TestSequence s1 = unwrapper.Unwrap(0xFFFF);
		TestSequence s2 = unwrapper.Unwrap(0x0000);
		TestSequence s3 = unwrapper.Unwrap(0x0001);

		REQUIRE(s0 < s1);
		REQUIRE(s0 < s2);
		REQUIRE(s0 < s3);
		REQUIRE(s1 < s2);
		REQUIRE(s1 < s3);
		REQUIRE(s2 < s3);

		REQUIRE(TestSequence::Difference(s1, s0) == 1);
		REQUIRE(TestSequence::Difference(s2, s0) == 2);
		REQUIRE(TestSequence::Difference(s3, s0) == 3);

		REQUIRE(s1 > s0);
		REQUIRE(s2 > s0);
		REQUIRE(s3 > s0);
		REQUIRE(s2 > s1);
		REQUIRE(s3 > s1);
		REQUIRE(s3 > s2);

		s0.Increment();
		REQUIRE(s0 == s1);

		s1.Increment();
		REQUIRE(s1 == s2);

		s2.Increment();
		REQUIRE(s2 == s3);

		REQUIRE(TestSequence::AddTo(s0, 2) == s3);
	}

	SECTION("wrap around a few times")
	{
		TestSequence::Unwrapper unwrapper;

		const TestSequence s0 = unwrapper.Unwrap(0);
		TestSequence prev     = s0;

		for (uint32_t i{ 1 }; i < 65536 * 3; ++i)
		{
			const auto wrapped    = static_cast<uint16_t>(i);
			const TestSequence si = unwrapper.Unwrap(wrapped);

			REQUIRE(s0 < si);
			REQUIRE(prev < si);

			prev = si;
		}
	}

	SECTION("increment is same as wrapped")
	{
		TestSequence::Unwrapper unwrapper;

		TestSequence s0   = unwrapper.Unwrap(0);
		TestSequence prev = s0;

		for (uint32_t i{ 1 }; i < 65536 * 2; ++i)
		{
			const auto wrapped    = static_cast<uint16_t>(i);
			const TestSequence si = unwrapper.Unwrap(wrapped);

			s0.Increment();
			REQUIRE(s0 == si);

			prev = si;
		}
	}

	SECTION("unwrapping larger number is always larger")
	{
		TestSequence::Unwrapper unwrapper;

		for (uint32_t i{ 1 }; i < 65536 * 2; ++i)
		{
			const auto wrapped    = static_cast<uint16_t>(i);
			const TestSequence si = unwrapper.Unwrap(wrapped);

			REQUIRE(unwrapper.Unwrap(wrapped + 1) > si);
			REQUIRE(unwrapper.Unwrap(wrapped + 5) > si);
			REQUIRE(unwrapper.Unwrap(wrapped + 10) > si);
			REQUIRE(unwrapper.Unwrap(wrapped + 100) > si);
		}
	}

	SECTION("unwrapping smaller number is always smaller")
	{
		TestSequence::Unwrapper unwrapper;

		for (uint32_t i{ 1 }; i < 65536 * 2; ++i)
		{
			const auto wrapped    = static_cast<uint16_t>(i);
			const TestSequence si = unwrapper.Unwrap(wrapped);

			REQUIRE(unwrapper.Unwrap(wrapped - 1) < si);
			REQUIRE(unwrapper.Unwrap(wrapped - 5) < si);
			REQUIRE(unwrapper.Unwrap(wrapped - 10) < si);
			REQUIRE(unwrapper.Unwrap(wrapped - 100) < si);
		}
	}

	SECTION("difference is absolute")
	{
		TestSequence::Unwrapper unwrapper;
		const TestSequence thisValue  = unwrapper.Unwrap(10);
		const TestSequence otherValue = TestSequence::AddTo(thisValue, 100);

		REQUIRE(TestSequence::Difference(thisValue, otherValue) == 100);
		REQUIRE(TestSequence::Difference(otherValue, thisValue) == 100);

		const TestSequence minusValue = TestSequence::AddTo(thisValue, -100);

		REQUIRE(TestSequence::Difference(thisValue, minusValue) == 100);
		REQUIRE(TestSequence::Difference(minusValue, thisValue) == 100);
	}
}
