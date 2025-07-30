#include "common.hpp"
#include "RTC/RtpPacket.hpp"
#include "RTC/SharedRtpPacket.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memcpy()

using namespace RTC;

static RtpPacket* CreateRtpPacket(
  uint8_t* buffer, size_t len, uint32_t ssrc, uint16_t seq, uint32_t timestamp)
{
	auto* packet = RtpPacket::Parse(buffer, len);

	packet->SetSequenceNumber(seq);
	packet->SetTimestamp(timestamp);

	return packet;
}

static void CompareRtpPackets(const RTC::RtpPacket* packet1, const RTC::RtpPacket* packet2)
{
	REQUIRE(packet1->GetSsrc() == packet2->GetSsrc());
	REQUIRE(packet1->GetSequenceNumber() == packet2->GetSequenceNumber());
	REQUIRE(packet1->GetTimestamp() == packet2->GetTimestamp());
}

SCENARIO("SharedRtpPacket", "[rtp][sharedrtppacket]")
{
	// clang-format off
	uint8_t rtpBuffer1[] =
	{
		0b10000000, 0b01111011, 0b01010010, 0b00001110,
		0b01011011, 0b01101011, 0b11001010, 0b10110101,
		0, 0, 0, 2
	};
	// clang-format on

	uint8_t rtpBuffer2[1500];

	std::memcpy(rtpBuffer2, rtpBuffer1, sizeof(rtpBuffer1));

	SECTION("default constructor and assign later")
	{
		auto* packetA = CreateRtpPacket(rtpBuffer1, sizeof(rtpBuffer1), 11111111, 1111, 111111);
		auto* packetB = CreateRtpPacket(rtpBuffer2, sizeof(rtpBuffer2), 22222222, 2222, 222222);

		RTC::SharedRtpPacket sharedPacket;

		REQUIRE(!sharedPacket.HasPacket());
		REQUIRE(sharedPacket.GetPacket() == nullptr);

		sharedPacket.Assign(packetA);

		REQUIRE(sharedPacket.HasPacket());
		CompareRtpPackets(sharedPacket.GetPacket(), packetA);

		sharedPacket.Reset();

		REQUIRE(!sharedPacket.HasPacket());
		REQUIRE(sharedPacket.GetPacket() == nullptr);

		delete packetA;
		delete packetB;
	}

	SECTION("constructor with packet and copy constructor")
	{
		auto* packetA = CreateRtpPacket(rtpBuffer1, sizeof(rtpBuffer1), 11111111, 1111, 111111);
		auto* packetB = CreateRtpPacket(rtpBuffer2, sizeof(rtpBuffer2), 22222222, 2222, 222222);

		// Create sharedPacket1 using constructor with a RtpPacket.
		RTC::SharedRtpPacket sharedPacket1(packetA);

		REQUIRE(sharedPacket1.HasPacket());
		CompareRtpPackets(sharedPacket1.GetPacket(), packetA);

		// Create sharedPacket2 using copy constructor.
		RTC::SharedRtpPacket sharedPacket2(sharedPacket1);

		REQUIRE(sharedPacket2.HasPacket());
		CompareRtpPackets(sharedPacket2.GetPacket(), packetA);

		sharedPacket2.Assign(packetB);

		REQUIRE(sharedPacket1.HasPacket());
		CompareRtpPackets(sharedPacket1.GetPacket(), packetB);
		REQUIRE(sharedPacket2.HasPacket());
		CompareRtpPackets(sharedPacket2.GetPacket(), packetB);
		REQUIRE(sharedPacket1.GetPacket() == sharedPacket2.GetPacket());

		sharedPacket1.Reset();

		REQUIRE(!sharedPacket1.HasPacket());
		REQUIRE(sharedPacket1.GetPacket() == nullptr);
		REQUIRE(!sharedPacket2.HasPacket());
		REQUIRE(sharedPacket2.GetPacket() == nullptr);

		delete packetA;
		delete packetB;
	}

	SECTION("copy assignment operator")
	{
		auto* packetA = CreateRtpPacket(rtpBuffer1, sizeof(rtpBuffer1), 11111111, 1111, 111111);
		auto* packetB = CreateRtpPacket(rtpBuffer2, sizeof(rtpBuffer2), 22222222, 2222, 222222);

		RTC::SharedRtpPacket sharedPacket1(packetA);

		REQUIRE(sharedPacket1.HasPacket());
		CompareRtpPackets(sharedPacket1.GetPacket(), packetA);

		RTC::SharedRtpPacket sharedPacket2;

		// Fill sharedPacket2 using copy assignment operator.
		sharedPacket2 = sharedPacket1;

		REQUIRE(sharedPacket2.HasPacket());
		CompareRtpPackets(sharedPacket2.GetPacket(), packetA);

		sharedPacket2.Assign(packetB);

		REQUIRE(sharedPacket1.HasPacket());
		CompareRtpPackets(sharedPacket1.GetPacket(), packetB);
		REQUIRE(sharedPacket2.HasPacket());
		CompareRtpPackets(sharedPacket2.GetPacket(), packetB);
		REQUIRE(sharedPacket1.GetPacket() == sharedPacket2.GetPacket());

		sharedPacket1.Reset();

		REQUIRE(!sharedPacket1.HasPacket());
		REQUIRE(sharedPacket1.GetPacket() == nullptr);
		REQUIRE(!sharedPacket2.HasPacket());
		REQUIRE(sharedPacket2.GetPacket() == nullptr);

		delete packetA;
		delete packetB;
	}

	SECTION("assign nullptr")
	{
		auto* packetA = CreateRtpPacket(rtpBuffer1, sizeof(rtpBuffer1), 11111111, 1111, 111111);
		auto* packetB = CreateRtpPacket(rtpBuffer2, sizeof(rtpBuffer2), 22222222, 2222, 222222);

		RTC::SharedRtpPacket sharedPacket(packetA);

		REQUIRE(sharedPacket.HasPacket());
		CompareRtpPackets(sharedPacket.GetPacket(), packetA);

		sharedPacket.Assign(nullptr);

		REQUIRE(!sharedPacket.HasPacket());
		REQUIRE(sharedPacket.GetPacket() == nullptr);

		delete packetA;
		delete packetB;
	}
}
