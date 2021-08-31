#include "common.hpp"
#include "RTC/RTCP/XrDelaySinceLastRr.hpp"
#include "RTC/RTCP/XrReceiverReferenceTime.hpp"
#include <catch2/catch.hpp>
#include <cstring> // std::memcmp

using namespace RTC::RTCP;

SCENARIO("RTCP XrDelaySinceLastRt parsing", "[parser][rtcp][xr-dlrr]")
{
	SECTION("create RRT")
	{
		// Create local report and check content.
		auto* report1 = new ReceiverReferenceTime();

		report1->SetNtpSec(11111111);
		report1->SetNtpFrac(22222222);

		REQUIRE(report1->GetType() == ExtendedReportBlock::Type::RRT);
		REQUIRE(report1->GetNtpSec() == 11111111);
		REQUIRE(report1->GetNtpFrac() == 22222222);

		// Serialize the report into an external buffer.
		uint8_t bufferReport1[256]{ 0 };

		report1->Serialize(bufferReport1);

		// Create a new report out of the external buffer.
		auto report2 = ReceiverReferenceTime::Parse(bufferReport1, report1->GetSize());

		REQUIRE(report1->GetType() == report2->GetType());
		REQUIRE(report1->GetNtpSec() == report2->GetNtpSec());
		REQUIRE(report1->GetNtpFrac() == report2->GetNtpFrac());

		// Create a local packet.
		std::unique_ptr<ExtendedReportPacket> packet1(new ExtendedReportPacket());

		packet1->SetSsrc(2222);
		packet1->AddReport(report1);
		packet1->AddReport(report2);

		REQUIRE(packet1->GetType() == Type::XR);
		REQUIRE(packet1->GetCount() == 0);
		REQUIRE(packet1->GetSsrc() == 2222);

		// Total size:
		// -  RTCP common header
		// -  SSRC
		// -  block 1
		// -  block 2
		REQUIRE(packet1->GetSize() == 4 + 4 + 12 + 12);

		// Serialize the packet into an external buffer.
		uint8_t bufferPacket1[256]{ 0 };
		uint8_t bufferPacket2[256]{ 0 };

		packet1->Serialize(bufferPacket1);

		// Create a new packet out of the external buffer.
		auto packet2 = ExtendedReportPacket::Parse(bufferPacket1, packet1->GetSize());

		REQUIRE(packet2->GetType() == packet1->GetType());
		REQUIRE(packet2->GetCount() == packet1->GetCount());
		REQUIRE(packet2->GetSsrc() == packet1->GetSsrc());
		REQUIRE(packet2->GetSize() == packet1->GetSize());

		packet2->Serialize(bufferPacket2);

		REQUIRE(std::memcmp(bufferPacket1, bufferPacket2, packet1->GetSize()) == 0);
	}

	SECTION("create DLRR")
	{
		// Create local report and check content.
		auto* report1   = new DelaySinceLastRr();
		auto* ssrcInfo1 = new DelaySinceLastRr::SsrcInfo();

		ssrcInfo1->SetSsrc(1234);
		ssrcInfo1->SetLastReceiverReport(11111111);
		ssrcInfo1->SetDelaySinceLastReceiverReport(22222222);

		REQUIRE(ssrcInfo1->GetSsrc() == 1234);
		REQUIRE(ssrcInfo1->GetLastReceiverReport() == 11111111);
		REQUIRE(ssrcInfo1->GetDelaySinceLastReceiverReport() == 22222222);
		REQUIRE(ssrcInfo1->GetSize() == sizeof(DelaySinceLastRr::SsrcInfo::Body));

		report1->AddSsrcInfo(ssrcInfo1);

		// Serialize the report into an external buffer.
		uint8_t bufferReport1[256]{ 0 };

		report1->Serialize(bufferReport1);

		// Create a new report out of the external buffer.
		auto report2 = DelaySinceLastRr::Parse(bufferReport1, report1->GetSize());

		REQUIRE(report1->GetType() == report2->GetType());

		auto ssrcInfoIt = report2->Begin();
		auto* ssrcInfo2 = *ssrcInfoIt;

		REQUIRE(ssrcInfo1->GetSsrc() == ssrcInfo2->GetSsrc());
		REQUIRE(ssrcInfo1->GetLastReceiverReport() == ssrcInfo2->GetLastReceiverReport());
		REQUIRE(
		  ssrcInfo1->GetDelaySinceLastReceiverReport() == ssrcInfo2->GetDelaySinceLastReceiverReport());
		REQUIRE(ssrcInfo1->GetSize() == ssrcInfo2->GetSize());

		// Create a local packet.
		std::unique_ptr<ExtendedReportPacket> packet1(new ExtendedReportPacket());

		packet1->SetSsrc(2222);
		packet1->AddReport(report1);
		packet1->AddReport(report2);

		REQUIRE(packet1->GetType() == Type::XR);
		REQUIRE(packet1->GetCount() == 0);
		REQUIRE(packet1->GetSsrc() == 2222);

		// Total size:
		// -  RTCP common header
		// -  SSRC
		// -  block 1
		// -  block 2
		REQUIRE(packet1->GetSize() == 4 + 4 + 16 + 16);

		// Serialize the packet into an external buffer.
		uint8_t bufferPacket1[256]{ 0 };
		uint8_t bufferPacket2[256]{ 0 };

		packet1->Serialize(bufferPacket1);

		// Create a new packet out of the external buffer.
		auto packet2 = ExtendedReportPacket::Parse(bufferPacket1, packet1->GetSize());

		REQUIRE(packet2->GetType() == packet1->GetType());
		REQUIRE(packet2->GetCount() == packet1->GetCount());
		REQUIRE(packet2->GetSsrc() == packet1->GetSsrc());
		REQUIRE(packet2->GetSize() == packet1->GetSize());

		packet2->Serialize(bufferPacket2);

		REQUIRE(std::memcmp(bufferPacket1, bufferPacket2, packet1->GetSize()) == 0);
	}
}
