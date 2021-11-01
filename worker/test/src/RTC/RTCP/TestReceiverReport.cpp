#include "common.hpp"
#include "RTC/RTCP/ReceiverReport.hpp"
#include "RTC/RTCP/SenderReport.hpp" // sizeof(SenderReport::Header)
#include <catch2/catch.hpp>

using namespace RTC::RTCP;

namespace TestReceiverReport
{
	// RTCP Receiver Report Packet.

	// clang-format off
	uint8_t buffer[] =
	{
		0x81, 0xc9, 0x00, 0x07, // Type: 201 (Receiver Report), Count: 1, Length: 7
		0x5d, 0x93, 0x15, 0x34, // Sender SSRC: 0x5d931534
		// Receiver Report
		0x01, 0x93, 0x2d, 0xb4, // SSRC. 0x01932db4
		0x00, 0x00, 0x00, 0x01, // Fraction lost: 0, Total lost: 1
		0x00, 0x00, 0x00, 0x00, // Extended highest sequence number: 0
		0x00, 0x00, 0x00, 0x00, // Jitter: 0
		0x00, 0x00, 0x00, 0x00, // Last SR: 0
		0x00, 0x00, 0x00, 0x05  // DLSR: 0
	};
	// clang-format on

	// Receiver Report buffer start point.
	uint8_t* rrBuffer = buffer + Packet::CommonHeaderSize + sizeof(uint32_t) /*Sender SSRC*/;

	uint32_t ssrc{ 0x01932db4 };
	uint8_t fractionLost{ 0 };
	uint8_t totalLost{ 1 };
	uint32_t lastSeq{ 0 };
	uint32_t jitter{ 0 };
	uint32_t lastSenderReport{ 0 };
	uint32_t delaySinceLastSenderReport{ 5 };

	void verify(ReceiverReport* report)
	{
		REQUIRE(report->GetSsrc() == ssrc);
		REQUIRE(report->GetFractionLost() == fractionLost);
		REQUIRE(report->GetTotalLost() == totalLost);
		REQUIRE(report->GetLastSeq() == lastSeq);
		REQUIRE(report->GetJitter() == jitter);
		REQUIRE(report->GetLastSenderReport() == lastSenderReport);
		REQUIRE(report->GetDelaySinceLastSenderReport() == delaySinceLastSenderReport);
	}
} // namespace TestReceiverReport

using namespace TestReceiverReport;

SCENARIO("RTCP RR parsing", "[parser][rtcp][rr]")
{
	SECTION("parse RR packet")
	{
		ReceiverReportPacket* packet = ReceiverReportPacket::Parse(buffer, sizeof(buffer));

		auto* report = *(packet->Begin());

		verify(report);

		SECTION("serialize packet instance")
		{
			uint8_t serialized[sizeof(buffer)] = { 0 };

			packet->Serialize(serialized);

			SECTION("compare serialized packet with original buffer")
			{
				REQUIRE(std::memcmp(buffer, serialized, sizeof(buffer)) == 0);
			}
		}

		delete packet;
	}

	SECTION("parse RR")
	{
		ReceiverReport* report = ReceiverReport::Parse(rrBuffer, ReceiverReport::HeaderSize);

		REQUIRE(report);

		verify(report);

		delete report;
	}

	SECTION("create RR")
	{
		// Create local report and check content.
		ReceiverReport report1;

		report1.SetSsrc(ssrc);
		report1.SetFractionLost(fractionLost);
		report1.SetTotalLost(totalLost);
		report1.SetLastSeq(lastSeq);
		report1.SetJitter(jitter);
		report1.SetLastSenderReport(lastSenderReport);
		report1.SetDelaySinceLastSenderReport(delaySinceLastSenderReport);

		verify(&report1);

		SECTION("create a report out of the existing one")
		{
			ReceiverReport report2(&report1);

			verify(&report2);
		}
	}
}
