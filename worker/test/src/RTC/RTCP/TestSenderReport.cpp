#include "common.hpp"
#include "RTC/RTCP/SenderReport.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memcmp()

SCENARIO("RTCP SenderReport", "[rtcp][sender-report]")
{
	// RTCP Packet. Sender Report and Receiver Report.

	// clang-format off
	alignas(4) uint8_t buffer[] =
	{
		0x80, 0xc8, 0x00, 0x06, // Type: 200 (Sender Report), Count: 0, Length: 6
		0x5d, 0x93, 0x15, 0x34, // SSRC: 0x5d931534
		0xdd, 0x3a, 0xc1, 0xb4, // NTP Sec: 3711615412
		0x76, 0x54, 0x71, 0x71, // NTP Frac: 1985245553
		0x00, 0x08, 0xcf, 0x00, // RTP timestamp: 577280
		0x00, 0x00, 0x0e, 0x18, // Packet count: 3608
		0x00, 0x08, 0xcf, 0x00  // Octet count: 577280
	};
	// clang-format on

	// Sender Report buffer start point.
	const uint8_t* srBuffer = buffer + RTC::RTCP::Packet::CommonHeaderSize;

	// SR values.
	const uint32_t ssrc{ 0x5d931534 };
	const uint32_t ntpSec{ 3711615412 };
	const uint32_t ntpFrac{ 1985245553 };
	const uint32_t rtpTs{ 577280 };
	const uint32_t packetCount{ 3608 };
	const uint32_t octetCount{ 577280 };

	// NOTE: No need to pass const integers to the lambda.
	auto verify = [](RTC::RTCP::SenderReport* report)
	{
		REQUIRE(report->GetSsrc() == ssrc);
		REQUIRE(report->GetNtpSec() == ntpSec);
		REQUIRE(report->GetNtpFrac() == ntpFrac);
		REQUIRE(report->GetRtpTs() == rtpTs);
		REQUIRE(report->GetPacketCount() == packetCount);
		REQUIRE(report->GetOctetCount() == octetCount);
	};

	SECTION("alignof() RTCP structs")
	{
		REQUIRE(alignof(RTC::RTCP::SenderReport::Header) == 4);
	}

	SECTION("parse SR packet")
	{
		std::unique_ptr<RTC::RTCP::SenderReportPacket> packet{ RTC::RTCP::SenderReportPacket::Parse(
			buffer, sizeof(buffer)) };

		auto* report = *(packet->Begin());

		verify(report);

		SECTION("serialize packet instance")
		{
			alignas(4) uint8_t serialized[sizeof(buffer)] = { 0 };

			packet->Serialize(serialized);

			SECTION("compare serialized packet with original buffer")
			{
				REQUIRE(std::memcmp(buffer, serialized, sizeof(buffer)) == 0);
			}
		}
	}

	SECTION("parse SR")
	{
		std::unique_ptr<RTC::RTCP::SenderReport> report{ RTC::RTCP::SenderReport::Parse(
			srBuffer, RTC::RTCP::SenderReport::HeaderSize) };

		REQUIRE(report);

		verify(report.get());

		SECTION("serialize SenderReport instance")
		{
			alignas(4) uint8_t serialized[RTC::RTCP::SenderReport::HeaderSize] = { 0 };

			report->Serialize(serialized);

			SECTION("compare serialized SenderReport with original buffer")
			{
				REQUIRE(std::memcmp(srBuffer, serialized, RTC::RTCP::SenderReport::HeaderSize) == 0);
			}
		}
	}

	SECTION("create SR packet multiple reports")
	{
		const size_t count = 3;

		RTC::RTCP::SenderReportPacket packet;

		for (size_t i = 1; i <= count; ++i)
		{
			// Create report and add to packet.
			auto* report = new RTC::RTCP::SenderReport();

			report->SetSsrc(i);
			report->SetNtpSec(i);
			report->SetNtpFrac(i);
			report->SetRtpTs(i);
			report->SetPacketCount(i);
			report->SetOctetCount(i);

			packet.AddReport(report);
		}

		alignas(4) uint8_t buffer[1500] = { 0 };

		// Serialization must contain 3 SR packets.
		packet.Serialize(buffer);

		// NOTE: clang-tidy says that this could be `const SenderReport* const reports`
		// but that's absolutely wrong!
		// NOLINTNEXTLINE(misc-const-correctness)
		const RTC::RTCP::SenderReport* reports[count]{ nullptr };

		std::unique_ptr<RTC::RTCP::SenderReportPacket> packet2{
			static_cast<RTC::RTCP::SenderReportPacket*>(RTC::RTCP::Packet::Parse(buffer, sizeof(buffer)))
		};

		REQUIRE(packet2 != nullptr);

		reports[0] = *(packet2->Begin());

		auto* packet3 = static_cast<RTC::RTCP::SenderReportPacket*>(packet2->GetNext());

		REQUIRE(packet3 != nullptr);

		reports[1] = *(packet3->Begin());

		auto* packet4 = static_cast<RTC::RTCP::SenderReportPacket*>(packet3->GetNext());

		REQUIRE(packet4 != nullptr);

		reports[2] = *(packet4->Begin());

		for (size_t i = 1; i <= count; ++i)
		{
			const auto* report = reports[i - 1];

			REQUIRE(report != nullptr);
			REQUIRE(report->GetSsrc() == i);
			REQUIRE(report->GetNtpSec() == i);
			REQUIRE(report->GetNtpFrac() == i);
			REQUIRE(report->GetRtpTs() == i);
			REQUIRE(report->GetPacketCount() == i);
			REQUIRE(report->GetOctetCount() == i);
		}

		delete packet3;
		delete packet4;
	}

	SECTION("create SR")
	{
		// Create local report and check content.
		RTC::RTCP::SenderReport report1;

		report1.SetSsrc(ssrc);
		report1.SetNtpSec(ntpSec);
		report1.SetNtpFrac(ntpFrac);
		report1.SetRtpTs(rtpTs);
		report1.SetPacketCount(packetCount);
		report1.SetOctetCount(octetCount);

		verify(&report1);

		SECTION("create a report out of the existing one")
		{
			RTC::RTCP::SenderReport report2(&report1);

			verify(&report2);
		}
	}
}
