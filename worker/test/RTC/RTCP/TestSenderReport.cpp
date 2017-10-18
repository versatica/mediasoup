#include "common.hpp"
#include "catch.hpp"
#include "RTC/RTCP/SenderReport.hpp"
#include "RTC/RTCP/ReceiverReport.hpp"

using namespace RTC::RTCP;

SCENARIO("RTCP SR parsing", "[parser][rtcp][sr]")
{
	SECTION("parse SenderReport")
	{
		uint8_t buffer[] =
		{
			0x00, 0x00, 0x04, 0xD2, // ssrc
			0x00, 0x00, 0x04, 0xD2, // ntp sec
			0x00, 0x00, 0x04, 0xD2, // ntp frac
			0x00, 0x00, 0x04, 0xD2, // rtp ts
			0x00, 0x00, 0x04, 0xD2, // packet count
			0x00, 0x00, 0x04, 0xD2, // octet count
		};

		uint32_t ssrc        = 1234;
		uint32_t ntpSec      = 1234;
		uint32_t ntpFrac     = 1234;
		uint32_t rtpTs       = 1234;
		uint32_t packetCount = 1234;
		uint32_t octetCount  = 1234;
		SenderReport* report = SenderReport::Parse(buffer, sizeof(SenderReport::Header));

		REQUIRE(report);

		REQUIRE(report->GetSsrc() == ssrc);
		REQUIRE(report->GetNtpSec() == ntpSec);
		REQUIRE(report->GetNtpFrac() == ntpFrac);
		REQUIRE(report->GetRtpTs() == rtpTs);
		REQUIRE(report->GetPacketCount() == packetCount);
		REQUIRE(report->GetOctetCount() == octetCount);

		delete report;
	}

	SECTION("create SenderReport")
	{
		uint32_t ssrc        = 1234;
		uint32_t ntpSec      = 1234;
		uint32_t ntpFrac     = 1234;
		uint32_t rtpTs       = 1234;
		uint32_t packetCount = 1234;
		uint32_t octetCount  = 1234;
		// Create local report and check content.
		// SenderReport();
		SenderReport report1;

		report1.SetSsrc(ssrc);
		report1.SetNtpSec(ntpSec);
		report1.SetNtpFrac(ntpFrac);
		report1.SetRtpTs(rtpTs);
		report1.SetPacketCount(packetCount);
		report1.SetOctetCount(octetCount);

		REQUIRE(report1.GetSsrc() == ssrc);
		REQUIRE(report1.GetNtpSec() == ntpSec);
		REQUIRE(report1.GetNtpFrac() == ntpFrac);
		REQUIRE(report1.GetRtpTs() == rtpTs);
		REQUIRE(report1.GetPacketCount() == packetCount);
		REQUIRE(report1.GetOctetCount() == octetCount);

		// Create report out of the existing one and check content.
		// SenderReport(SenderReport* report);
		SenderReport report2(&report1);

		REQUIRE(report2.GetSsrc() == ssrc);
		REQUIRE(report2.GetNtpSec() == ntpSec);
		REQUIRE(report2.GetNtpFrac() == ntpFrac);
		REQUIRE(report2.GetRtpTs() == rtpTs);
		REQUIRE(report2.GetPacketCount() == packetCount);
		REQUIRE(report2.GetOctetCount() == octetCount);
	}

	SECTION("parse SenderReport")
	{
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

		uint32_t ssrc        = 0x5d931534;
		uint32_t ntpSec      = 3711615412;
		uint32_t ntpFrac     = 1985245553;
		uint32_t rtpTs       = 577280;
		uint32_t packetCount = 3608;
		uint32_t octetCount  = 577280;

		auto packet = Packet::Parse(buffer, sizeof(buffer));

		REQUIRE(packet);

		auto sr = dynamic_cast<SenderReportPacket*>(packet);
		auto sIt     = sr->Begin();
		auto sReport = *sIt;

		REQUIRE(sReport->GetSsrc() == ssrc);
		REQUIRE(sReport->GetNtpSec() == ntpSec);
		REQUIRE(sReport->GetNtpFrac() == ntpFrac);
		REQUIRE(sReport->GetRtpTs() == rtpTs);
		REQUIRE(sReport->GetPacketCount() == packetCount);
		REQUIRE(sReport->GetOctetCount() == octetCount);

		REQUIRE(packet->GetNext());

		auto rr      = dynamic_cast<ReceiverReportPacket*>(packet->GetNext());
		auto rIt     = rr->Begin();
		auto rReport = *rIt;

		ssrc                                = 0x01932db4;

		uint8_t fractionLost                = 0;
		uint8_t totalLost                   = 1;
		uint32_t lastSeq                    = 0;
		uint32_t jitter                     = 0;
		uint32_t lastSenderReport           = 0;
		uint32_t delaySinceLastSenderReport = 5;

		REQUIRE(rReport);
		REQUIRE(rReport->GetSsrc() == ssrc);
		REQUIRE(rReport->GetFractionLost() == fractionLost);
		REQUIRE(rReport->GetTotalLost() == totalLost);
		REQUIRE(rReport->GetLastSeq() == lastSeq);
		REQUIRE(rReport->GetJitter() == jitter);
		REQUIRE(rReport->GetLastSenderReport() == lastSenderReport);
		REQUIRE(rReport->GetDelaySinceLastSenderReport() == delaySinceLastSenderReport);
	}
}
