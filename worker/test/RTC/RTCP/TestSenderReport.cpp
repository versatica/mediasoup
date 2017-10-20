#include "common.hpp"
#include "catch.hpp"
#include "RTC/RTCP/SenderReport.hpp"
#include <cstring> // std::memcmp()

using namespace RTC::RTCP;

namespace TestSenderReport
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

	// Sender Report buffer start point.
	uint8_t* srBuffer = buffer + sizeof(Packet::CommonHeader);

	// SR values.
	uint32_t ssrc        = 0x5d931534;
	uint32_t ntpSec      = 3711615412;
	uint32_t ntpFrac     = 1985245553;
	uint32_t rtpTs       = 577280;
	uint32_t packetCount = 3608;
	uint32_t octetCount  = 577280;

	void verify(SenderReport* report)
	{
		REQUIRE(report->GetSsrc() == ssrc);
		REQUIRE(report->GetNtpSec() == ntpSec);
		REQUIRE(report->GetNtpFrac() == ntpFrac);
		REQUIRE(report->GetRtpTs() == rtpTs);
		REQUIRE(report->GetPacketCount() == packetCount);
		REQUIRE(report->GetOctetCount() == octetCount);
	}
}

using namespace TestSenderReport;

SCENARIO("RTCP SR parsing", "[parser][rtcp][sr]")
{
	SECTION("parse sender report")
	{
		SenderReport* report = SenderReport::Parse(srBuffer, sizeof(SenderReport::Header));

		REQUIRE(report);

		verify(report);

		SECTION("serialize SenderReport instance")
		{
			uint8_t serialized[sizeof(SenderReport::Header)] = { 0 };

			report->Serialize(serialized);

			SECTION("compare serialized SenderReport with original buffer")
			{
				REQUIRE(std::memcmp(srBuffer, serialized, sizeof(SenderReport::Header)) == 0);
			}
		}

		delete report;
	}

	SECTION("create SenderReport")
	{
		// Create local report and check content.
		SenderReport report1;

		report1.SetSsrc(ssrc);
		report1.SetNtpSec(ntpSec);
		report1.SetNtpFrac(ntpFrac);
		report1.SetRtpTs(rtpTs);
		report1.SetPacketCount(packetCount);
		report1.SetOctetCount(octetCount);

		verify(&report1);

		SECTION("create a report out of the existing one")
		{
			SenderReport report2(&report1);

			verify(&report2);
		}
	}

	SECTION("parse Packet with SenderReport and ReceiverReport")
	{
		auto packet = Packet::Parse(buffer, sizeof(buffer));

		REQUIRE(packet);
		REQUIRE(packet->GetType() == Type::SR);

		auto sr     = dynamic_cast<SenderReportPacket*>(packet);
		auto it     = sr->Begin();
		auto report = *it;

		verify(report);

		REQUIRE(packet->GetNext());
		REQUIRE(packet->GetNext()->GetType() == Type::RR);
	}
}
