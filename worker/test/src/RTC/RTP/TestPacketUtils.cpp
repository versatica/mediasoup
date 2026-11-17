#include "common.hpp"
#include "RTC/RTP/Packet.hpp"
#include "RTC/RTP/PacketUtils.hpp"
#include "test/include/RTC/RTP/rtpCommon.hpp" // in worker/test/include/
#include <catch2/catch_test_macros.hpp>

SCENARIO("RTP PacketUtils", "[rtp][packetutils]")
{
	const auto compareRtpPackets =
	  [](const RTC::RTP::Packet* packet1, const RTC::RTP::Packet* packet2) -> void
	{
		REQUIRE(packet1->GetSsrc() == packet2->GetSsrc());
		REQUIRE(packet1->GetSequenceNumber() == packet2->GetSequenceNumber());
		REQUIRE(packet1->GetTimestamp() == packet2->GetTimestamp());
		REQUIRE(packet1->GetLength() == packet2->GetLength());
	};

	const std::unique_ptr<RTC::RTP::Packet> packetA{ RTC::RTP::Packet::Factory(
		rtpCommon::FactoryBuffer, 2000) };

	packetA->SetSequenceNumber(1111);
	packetA->SetTimestamp(111111);
	packetA->SetSsrc(11111111);

	const std::unique_ptr<RTC::RTP::Packet> packetB{ RTC::RTP::Packet::Factory(
		rtpCommon::FactoryBuffer + 2000, 2000) };

	packetB->SetSequenceNumber(2222);
	packetB->SetTimestamp(222222);
	packetB->SetSsrc(22222222);

	SECTION("default constructor and assign later")
	{
		std::shared_ptr<RTC::RTP::Packet> sharedPacket;

		REQUIRE(!sharedPacket);
		REQUIRE(sharedPacket.get() == nullptr);

		sharedPacket = RTC::RTP::PacketUtils::ClonePacket(packetA.get());

		REQUIRE(sharedPacket);
		compareRtpPackets(sharedPacket.get(), packetA.get());

		sharedPacket.reset();

		REQUIRE(!sharedPacket);
		REQUIRE(sharedPacket.get() == nullptr);
	}

	SECTION("clone packet and copy constructor")
	{
		// Create sharedPacket1 holding a clone of packetA.
		std::shared_ptr<RTC::RTP::Packet> sharedPacket1 =
		  RTC::RTP::PacketUtils::ClonePacket(packetA.get());

		REQUIRE(sharedPacket1);
		compareRtpPackets(sharedPacket1.get(), packetA.get());

		// Create sharedPacket2 using copy constructor.
		const std::shared_ptr<RTC::RTP::Packet> sharedPacket2(sharedPacket1);

		REQUIRE(sharedPacket2);
		compareRtpPackets(sharedPacket2.get(), packetA.get());
		REQUIRE(sharedPacket1.get() == sharedPacket2.get());

		RTC::RTP::PacketUtils::AssertSamePacket(sharedPacket1, sharedPacket1.get());
		RTC::RTP::PacketUtils::AssertSamePacket(sharedPacket1, sharedPacket2.get());
		RTC::RTP::PacketUtils::AssertSamePacket(sharedPacket2, sharedPacket2.get());
		RTC::RTP::PacketUtils::AssertSamePacket(sharedPacket2, sharedPacket1.get());

		sharedPacket1.reset();

		REQUIRE(!sharedPacket1);
		REQUIRE(sharedPacket1.get() == nullptr);
	}

	SECTION("copy assignment operator")
	{
		std::shared_ptr<RTC::RTP::Packet> sharedPacket1 =
		  RTC::RTP::PacketUtils::ClonePacket(packetA.get());

		REQUIRE(sharedPacket1);
		compareRtpPackets(sharedPacket1.get(), packetA.get());

		std::shared_ptr<RTC::RTP::Packet> sharedPacket2;

		// Fill sharedPacket2 using copy assignment operator.
		sharedPacket2 = sharedPacket1;

		REQUIRE(sharedPacket2);
		compareRtpPackets(sharedPacket2.get(), packetA.get());
		REQUIRE(sharedPacket1.get() == sharedPacket2.get());

		RTC::RTP::PacketUtils::AssertSamePacket(sharedPacket1, sharedPacket1.get());
		RTC::RTP::PacketUtils::AssertSamePacket(sharedPacket1, sharedPacket2.get());
		RTC::RTP::PacketUtils::AssertSamePacket(sharedPacket2, sharedPacket2.get());
		RTC::RTP::PacketUtils::AssertSamePacket(sharedPacket2, sharedPacket1.get());

		sharedPacket1.reset();

		REQUIRE(!sharedPacket1);
		REQUIRE(sharedPacket1.get() == nullptr);
	}

	SECTION("assign nullptr")
	{
		std::shared_ptr<RTC::RTP::Packet> sharedPacket =
		  RTC::RTP::PacketUtils::ClonePacket(packetA.get());

		REQUIRE(sharedPacket);
		compareRtpPackets(sharedPacket.get(), packetA.get());

		sharedPacket = nullptr;

		REQUIRE(!sharedPacket);
		REQUIRE(sharedPacket.get() == nullptr);
	}
}
