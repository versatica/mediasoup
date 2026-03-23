#include "common.hpp"
#include "RTC/RTCP/ReceiverReport.hpp"
#include <catch2/catch_test_macros.hpp>

SCENARIO("RTCP ReceiverReport", "[rtcp][receiver-report]")
{
	// RTCP Receiver Report Packet.

	// clang-format off
	alignas(4) uint8_t buffer[] =
	{
		0x81, 0xc9, 0x00, 0x07, // Type: 201 (Receiver Report), Count: 1, Length: 7
		0x5d, 0x93, 0x15, 0x34, // Sender SSRC: 0x5d931534
		// Receiver Report
		0x01, 0x93, 0x2d, 0xb4, // SSRC. 0x01932db4
		0x00, 0xFF, 0xFF, 0xFF, // Fraction lost: 0, Total lost: -1
		0x00, 0x00, 0x00, 0x00, // Extended highest sequence number: 0
		0x00, 0x00, 0x00, 0x00, // Jitter: 0
		0x00, 0x00, 0x00, 0x00, // Last SR: 0
		0x00, 0x00, 0x00, 0x05  // DLSR: 0
	};
	// clang-format on

	// Receiver Report buffer start point.
	const uint8_t* rrBuffer =
	  buffer + RTC::RTCP::Packet::CommonHeaderSize + sizeof(uint32_t); // Sender SSRC.

	const uint32_t ssrc{ 0x01932db4 };
	const uint8_t fractionLost{ 0 };
	const int32_t totalLost{ -1 };
	const uint32_t lastSeq{ 0 };
	const uint32_t jitter{ 0 };
	const uint32_t lastSenderReport{ 0 };
	const uint32_t delaySinceLastSenderReport{ 5 };

	// NOTE: No need to pass const integers to the lambda.
	auto verify = [](RTC::RTCP::ReceiverReport* report)
	{
		REQUIRE(report->GetSsrc() == ssrc);
		REQUIRE(report->GetFractionLost() == fractionLost);
		REQUIRE(report->GetTotalLost() == totalLost);
		REQUIRE(report->GetLastSeq() == lastSeq);
		REQUIRE(report->GetJitter() == jitter);
		REQUIRE(report->GetLastSenderReport() == lastSenderReport);
		REQUIRE(report->GetDelaySinceLastSenderReport() == delaySinceLastSenderReport);
	};

	SECTION("alignof() RTCP structs")
	{
		REQUIRE(alignof(RTC::RTCP::ReceiverReport::Header) == 4);
	}

	SECTION("parse RR packet with a single report")
	{
		std::unique_ptr<RTC::RTCP::ReceiverReportPacket> packet{ RTC::RTCP::ReceiverReportPacket::Parse(
			buffer, sizeof(buffer)) };

		REQUIRE(packet->GetCount() == 1);

		auto* report = *(packet->Begin());

		verify(report);

		SECTION("serialize packet instance")
		{
			alignas(4) uint8_t serialized[sizeof(buffer)] = { 0 };

			packet->Serialize(serialized);

			std::unique_ptr<RTC::RTCP::ReceiverReportPacket> packet2{
				RTC::RTCP::ReceiverReportPacket::Parse(serialized, sizeof(buffer))
			};

			REQUIRE(packet2->GetType() == RTC::RTCP::Type::RR);
			REQUIRE(packet2->GetCount() == 1);
			REQUIRE(packet2->GetSize() == 32);

			auto* buf = reinterpret_cast<RTC::RTCP::Packet::CommonHeader*>(buffer);

			REQUIRE(ntohs(buf->length) == 7);

			report = *(packet2->Begin());

			verify(report);

			SECTION("compare serialized packet with original buffer")
			{
				REQUIRE(std::memcmp(buffer, serialized, sizeof(buffer)) == 0);
			}
		}
	}

	SECTION("parse RR")
	{
		std::unique_ptr<RTC::RTCP::ReceiverReport> report{ RTC::RTCP::ReceiverReport::Parse(
			rrBuffer, RTC::RTCP::ReceiverReport::HeaderSize) };

		REQUIRE(report);

		verify(report.get());
	}

	SECTION("create RR packet with more than 31 reports")
	{
		const size_t count = 33;

		RTC::RTCP::ReceiverReportPacket packet;

		for (size_t i = 1; i <= count; i++)
		{
			// Create report and add to packet.
			auto* report = new RTC::RTCP::ReceiverReport();

			report->SetSsrc(i);
			report->SetFractionLost(i);
			report->SetTotalLost(i);
			report->SetLastSeq(i);
			report->SetJitter(i);
			report->SetLastSenderReport(i);
			report->SetDelaySinceLastSenderReport(i);

			packet.AddReport(report);
		}

		REQUIRE(packet.GetCount() == count);

		alignas(4) uint8_t buffer[1500] = { 0 };

		// Serialization must contain 2 RR packets since report count exceeds 31.
		packet.Serialize(buffer);

		std::unique_ptr<RTC::RTCP::ReceiverReportPacket> packet2{
			static_cast<RTC::RTCP::ReceiverReportPacket*>(RTC::RTCP::Packet::Parse(buffer, sizeof(buffer)))
		};

		REQUIRE(packet2 != nullptr);
		REQUIRE(packet2->GetCount() == 31);

		auto reportIt = packet2->Begin();

		for (size_t i = 1; i <= 31; ++i, ++reportIt)
		{
			auto* report = *reportIt;

			REQUIRE(report->GetSsrc() == i);
			REQUIRE(report->GetFractionLost() == i);
			REQUIRE(report->GetTotalLost() == static_cast<int32_t>(i));
			REQUIRE(report->GetLastSeq() == i);
			REQUIRE(report->GetJitter() == i);
			REQUIRE(report->GetLastSenderReport() == i);
			REQUIRE(report->GetDelaySinceLastSenderReport() == i);
		}

		auto* packet3 = static_cast<RTC::RTCP::ReceiverReportPacket*>(packet2->GetNext());

		REQUIRE(packet3 != nullptr);
		REQUIRE(packet3->GetCount() == 2);

		reportIt = packet3->Begin();

		for (size_t i = 1; i <= 2; ++i, ++reportIt)
		{
			auto* report = *reportIt;

			REQUIRE(report->GetSsrc() == 31 + i);
			REQUIRE(report->GetFractionLost() == 31 + i);
			REQUIRE(report->GetTotalLost() == static_cast<int32_t>(31 + i));
			REQUIRE(report->GetLastSeq() == 31 + i);
			REQUIRE(report->GetJitter() == 31 + i);
			REQUIRE(report->GetLastSenderReport() == 31 + i);
			REQUIRE(report->GetDelaySinceLastSenderReport() == 31 + i);
		}

		delete packet3;
	}

	SECTION("create RR report")
	{
		// Create local report and check content.
		RTC::RTCP::ReceiverReport report1;

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
			RTC::RTCP::ReceiverReport report2(&report1);

			verify(&report2);
		}
	}
}
