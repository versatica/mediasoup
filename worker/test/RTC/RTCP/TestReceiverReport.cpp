#include "common.hpp"
#include "catch.hpp"
#include "RTC/RTCP/ReceiverReport.hpp"

using namespace RTC::RTCP;

SCENARIO("RTCP RR parsing", "[parser][rtcp][rr]")
{
	SECTION("parse ReceiverReport")
	{
		uint8_t buffer[] =
		{
			0x00, 0x00, 0x04, 0xD2,	// ssrc
			0x01,                   // fractionLost
			0x00, 0x00, 0x04,       // totalLost
			0x00, 0x00, 0x04, 0xD2,	// lastSeq
			0x00, 0x00, 0x04, 0xD2,	// jitter
			0x00, 0x00, 0x04, 0xD2,	// lsr
			0x00, 0x00, 0x04, 0xD2  // dlsr
		};

		uint32_t ssrc                       = 1234;
		uint8_t fractionLost                = 1;
		int32_t totalLost                   = 4;
		uint32_t lastSeq                    = 1234;
		uint32_t jitter                     = 1234;
		uint32_t lastSenderReport           = 1234;
		uint32_t delaySinceLastSenderReport = 1234;
		ReceiverReport* report = ReceiverReport::Parse(buffer, sizeof(ReceiverReport::Header));

		REQUIRE(report);
		REQUIRE(report->GetSsrc() == ssrc);
		REQUIRE(report->GetFractionLost() == fractionLost);
		REQUIRE(report->GetTotalLost() == totalLost);
		REQUIRE(report->GetLastSeq() == lastSeq);
		REQUIRE(report->GetJitter() == jitter);
		REQUIRE(report->GetLastSenderReport() == lastSenderReport);
		REQUIRE(report->GetDelaySinceLastSenderReport() == delaySinceLastSenderReport);
	}

	SECTION("create ReceiverReport")
	{
		uint32_t ssrc                       = 1234;
		uint8_t fractionLost                = 1;
		int32_t totalLost                   = 4;
		uint32_t lastSeq                    = 1234;
		uint32_t jitter                     = 1234;
		uint32_t lastSenderReport           = 1234;
		uint32_t delaySinceLastSenderReport = 1234;
		// Create local report and check content.
		// ReceiverReport();
		ReceiverReport report1;

		report1.SetSsrc(ssrc);
		report1.SetFractionLost(fractionLost);
		report1.SetTotalLost(totalLost);
		report1.SetLastSeq(lastSeq);
		report1.SetJitter(jitter);
		report1.SetLastSenderReport(lastSenderReport);
		report1.SetDelaySinceLastSenderReport(delaySinceLastSenderReport);

		REQUIRE(report1.GetSsrc() == ssrc);
		REQUIRE(report1.GetFractionLost() == fractionLost);
		REQUIRE(report1.GetTotalLost() == totalLost);
		REQUIRE(report1.GetLastSeq() == lastSeq);
		REQUIRE(report1.GetJitter() == jitter);
		REQUIRE(report1.GetLastSenderReport() == lastSenderReport);
		REQUIRE(report1.GetDelaySinceLastSenderReport() == delaySinceLastSenderReport);

		// Create report out of the existing one and check content.
		// ReceiverReport(ReceiverReport* report);
		ReceiverReport report2(&report1);

		REQUIRE(report2.GetSsrc() == ssrc);
		REQUIRE(report2.GetFractionLost() == fractionLost);
		REQUIRE(report2.GetTotalLost() == totalLost);
		REQUIRE(report2.GetLastSeq() == lastSeq);
		REQUIRE(report2.GetJitter() == jitter);
		REQUIRE(report2.GetLastSenderReport() == lastSenderReport);
		REQUIRE(report2.GetDelaySinceLastSenderReport() == delaySinceLastSenderReport);
	}
}
