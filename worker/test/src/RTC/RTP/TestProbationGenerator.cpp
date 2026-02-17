#include "common.hpp"
#include "RTC/RTP/ProbationGenerator.hpp"
#include <catch2/catch_test_macros.hpp>

SCENARIO("RTP ProbationGenerator", "[rtp][probation-generator]")
{
	SECTION("ProbationGenerator generates RTP Packets of the requested length")
	{
		RTC::RTP::ProbationGenerator probationGenerator;

		const auto* packet = probationGenerator.GetNextPacket(1000);
		const auto seq     = packet->GetSequenceNumber();

		REQUIRE(packet->GetSsrc() == RTC::RTP::ProbationGenerator::Ssrc);
		REQUIRE(packet->GetPayloadType() == RTC::RTP::ProbationGenerator::PayloadType);
		REQUIRE(packet->GetLength() == 1000);
		REQUIRE(packet->IsPaddedTo4Bytes());

		// If given length is higher than ProbationGenerator::ProbationPacketMaxLength
		// then that limit value is used instead.
		packet =
		  probationGenerator.GetNextPacket(RTC::RTP::ProbationGenerator::ProbationPacketMaxLength + 10);

		REQUIRE(packet->GetSequenceNumber() == seq + 1);
		REQUIRE(packet->GetLength() == RTC::RTP::ProbationGenerator::ProbationPacketMaxLength);
		REQUIRE(packet->IsPaddedTo4Bytes());

		// If given length is less than probation packet minimum length, then that
		// limit value is used instead.
		packet = probationGenerator.GetNextPacket(probationGenerator.GetProbationPacketMinLength() - 10);

		REQUIRE(packet->GetSequenceNumber() == seq + 2);
		REQUIRE(packet->GetLength() == probationGenerator.GetProbationPacketMinLength());
		REQUIRE(packet->IsPaddedTo4Bytes());
	}
}
