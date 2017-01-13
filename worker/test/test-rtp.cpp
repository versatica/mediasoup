#include "include/catch.hpp"
#include "include/helpers.h"
#include "common.h"
#include "RTC/RtpPacket.h"

using namespace RTC;

uint8_t buffer[65536];

SCENARIO("parse RTP packets", "[parser][rtp]")
{
	SECTION("audio/opus packet")
	{
		size_t len;

		if (!Helpers::ReadBinaryFile("data/packet1.raw", buffer, &len))
			FAIL("cannot open file");

		RtpPacket* packet = RtpPacket::Parse(buffer, len);

		if (!packet)
			FAIL("not a RTP packet");

		// packet->Dump();

		REQUIRE(packet->HasMarker() == false);
		REQUIRE(packet->HasExtensionHeader() == true);
		REQUIRE(packet->GetExtensionHeaderId() == 48862);
		REQUIRE(packet->GetExtensionHeaderLength() == 4);
		REQUIRE(packet->GetPayloadType() == 111);
		REQUIRE(packet->GetSequenceNumber() == 23617);
		REQUIRE(packet->GetTimestamp() == 1660241882);
		REQUIRE(packet->GetSsrc() == 2674985186);

		delete packet;
	}
}
