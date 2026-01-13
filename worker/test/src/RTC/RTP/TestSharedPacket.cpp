#include "common.hpp"
#include "RTC/RTP/Packet.hpp"
#include "RTC/RTP/SharedPacket.hpp"
#include "RTC/RTP/rtpCommon.hpp" // in worker/test/include/
#include <catch2/catch_test_macros.hpp>

using namespace RTC;

static void CompareRtpPackets(const RTP::Packet* packet1, const RTP::Packet* packet2)
{
	REQUIRE(packet1->GetSsrc() == packet2->GetSsrc());
	REQUIRE(packet1->GetSequenceNumber() == packet2->GetSequenceNumber());
	REQUIRE(packet1->GetTimestamp() == packet2->GetTimestamp());
	REQUIRE(packet1->GetLength() == packet2->GetLength());
}

SCENARIO("RTP SharedPacket", "[rtp][sharedpacket]")
{
	auto* packetA = RTP::Packet::Factory(FactoryBuffer, 2000);

	packetA->SetSequenceNumber(1111);
	packetA->SetTimestamp(111111);
	packetA->SetSsrc(11111111);

	auto* packetB = RTP::Packet::Factory(FactoryBuffer + 2000, 2000);

	packetB->SetSequenceNumber(2222);
	packetB->SetTimestamp(222222);
	packetB->SetSsrc(22222222);

	SECTION("default constructor and assign later")
	{
		RTP::SharedPacket sharedPacket;

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
		// Create sharedPacket1 using constructor with a Packet.
		RTP::SharedPacket sharedPacket1(packetA);

		REQUIRE(sharedPacket1.HasPacket());
		CompareRtpPackets(sharedPacket1.GetPacket(), packetA);

		// Create sharedPacket2 using copy constructor.
		RTP::SharedPacket sharedPacket2(sharedPacket1);

		REQUIRE(sharedPacket2.HasPacket());
		CompareRtpPackets(sharedPacket2.GetPacket(), packetA);

		sharedPacket2.Assign(packetB);

		REQUIRE(sharedPacket1.HasPacket());
		CompareRtpPackets(sharedPacket1.GetPacket(), packetB);
		REQUIRE(sharedPacket2.HasPacket());
		CompareRtpPackets(sharedPacket2.GetPacket(), packetB);
		REQUIRE(sharedPacket1.GetPacket() == sharedPacket2.GetPacket());

		sharedPacket1.AssertSamePacket(sharedPacket1.GetPacket());
		sharedPacket1.AssertSamePacket(sharedPacket2.GetPacket());
		sharedPacket2.AssertSamePacket(sharedPacket2.GetPacket());
		sharedPacket2.AssertSamePacket(sharedPacket1.GetPacket());

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
		RTP::SharedPacket sharedPacket1(packetA);

		REQUIRE(sharedPacket1.HasPacket());
		CompareRtpPackets(sharedPacket1.GetPacket(), packetA);

		RTP::SharedPacket sharedPacket2;

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

		sharedPacket1.AssertSamePacket(sharedPacket1.GetPacket());
		sharedPacket1.AssertSamePacket(sharedPacket2.GetPacket());
		sharedPacket2.AssertSamePacket(sharedPacket2.GetPacket());
		sharedPacket2.AssertSamePacket(sharedPacket1.GetPacket());

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
		RTP::SharedPacket sharedPacket(packetA);

		REQUIRE(sharedPacket.HasPacket());
		CompareRtpPackets(sharedPacket.GetPacket(), packetA);

		sharedPacket.Assign(nullptr);

		REQUIRE(!sharedPacket.HasPacket());
		REQUIRE(sharedPacket.GetPacket() == nullptr);

		delete packetA;
		delete packetB;
	}
}
