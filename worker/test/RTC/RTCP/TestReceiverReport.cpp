#include "common.hpp"
#include "catch.hpp"
#include "RTC/RTCP/ReceiverReport.hpp"
#include "RTC/RTCP/SenderReport.hpp" // sizeof(SenderReport::Header)

using namespace RTC::RTCP;

namespace TestReceiverReport
{
	// RTCP Packet. Sender Report and Receiver Report.

	// clang-format off
	uint8_t buffer[] =
	{
		0x81, 0xc8, 0x00, 0x0c, // Type: 200 (Sender Report), Count: 1, Length: 12
		0x5d, 0x93, 0x15, 0x34, // SSRC: 0x5d931534
		0xdd, 0x3a, 0xc1, 0xb4, // NTP Sec: 3711615412
		0x76, 0x54, 0x71, 0x71, // NTP Frac: 1985245553
		0x00, 0x08, 0xcf, 0x00, // RTP timestamp: 577280
		0x00, 0x00, 0x0e, 0x18, // Packet count: 3608
		0x00, 0x08, 0xcf, 0x00, // Octed count: 577280
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
	uint8_t* rrBuffer = buffer + sizeof(Packet::CommonHeader) + sizeof(SenderReport::Header);

	uint32_t ssrc                       = 0x01932db4;
	uint8_t fractionLost                = 0;
	uint8_t totalLost                   = 1;
	uint32_t lastSeq                    = 0;
	uint32_t jitter                     = 0;
	uint32_t lastSenderReport           = 0;
	uint32_t delaySinceLastSenderReport = 5;

	void verifyReceiverReport(ReceiverReport* report)
	{
		REQUIRE(report->GetSsrc() == ssrc);
		REQUIRE(report->GetFractionLost() == fractionLost);
		REQUIRE(report->GetTotalLost() == totalLost);
		REQUIRE(report->GetLastSeq() == lastSeq);
		REQUIRE(report->GetJitter() == jitter);
		REQUIRE(report->GetLastSenderReport() == lastSenderReport);
		REQUIRE(report->GetDelaySinceLastSenderReport() == delaySinceLastSenderReport);
	}
}

using namespace TestReceiverReport;

SCENARIO("RTCP RR parsing", "[parser][rtcp][rr]")
{
	SECTION("parse ReceiverReport")
	{
		ReceiverReport* report = ReceiverReport::Parse(rrBuffer, sizeof(ReceiverReport::Header));

		REQUIRE(report);

		verifyReceiverReport(report);

		delete report;
	}

	SECTION("create ReceiverReport")
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

		verifyReceiverReport(&report1);

		SECTION("create a report out of the existing one")
		{
			ReceiverReport report2(&report1);

			verifyReceiverReport(&report2);
		}
	}
}
