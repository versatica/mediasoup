#include "common.hpp"
#include "RTC/RTP/ProbationGenerator.hpp"
#include <catch2/catch_test_macros.hpp>

using namespace RTC;

SCENARIO("RTP ProbationGenerator", "[rtp][probationgenerator]")
{
	SECTION("ProbationGenerator generates RTP Packets of the requested length")
	{
		RTP::ProbationGenerator probationGenerator;

		auto* packet = probationGenerator.GetNextPacket(1000);
		auto seq     = packet->GetSequenceNumber();

		REQUIRE(packet->GetSsrc() == RTP::ProbationGenerator::Ssrc);
		REQUIRE(packet->GetPayloadType() == RTP::ProbationGenerator::PayloadType);
		REQUIRE(packet->GetLength() == 1000);
		REQUIRE(packet->IsPaddedTo4Bytes());

		// If given length is higher than ProbationGenerator::ProbationPacketMaxLength
		// then that limit value is used instead.
		packet = probationGenerator.GetNextPacket(RTP::ProbationGenerator::ProbationPacketMaxLength + 10);

		REQUIRE(packet->GetSequenceNumber() == seq + 1);
		REQUIRE(packet->GetLength() == RTP::ProbationGenerator::ProbationPacketMaxLength);
		REQUIRE(packet->IsPaddedTo4Bytes());

		// If given length is less than probation packet minimum length, then that
		// limit value is used instead.
		packet = probationGenerator.GetNextPacket(probationGenerator.GetProbationPacketMinLength() - 10);

		REQUIRE(packet->GetSequenceNumber() == seq + 2);
		REQUIRE(packet->GetLength() == probationGenerator.GetProbationPacketMinLength());
		REQUIRE(packet->IsPaddedTo4Bytes());
	}
}
