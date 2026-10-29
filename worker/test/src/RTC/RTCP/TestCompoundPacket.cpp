#include "common.hpp"
#include "RTC/Consts.hpp"
#include "RTC/RTCP/CompoundPacket.hpp"
#include "RTC/RTCP/ReceiverReport.hpp"
#include "RTC/RTCP/Sdes.hpp"
#include "RTC/RTCP/SenderReport.hpp"
#include "RTC/RTCP/XrDelaySinceLastRr.hpp"
#include "Utils.hpp"
#include <catch2/catch_test_macros.hpp>
#include <string>

SCENARIO("RTCP CompoundPacket", "[rtcp][compoundpacket]")
{
	// Builds what a stream being sent reports, with a CNAME of given length.
	const auto buildSendStreamItems =
	  [](uint32_t ssrc, size_t cnameLength) -> RTC::RTCP::CompoundPacket::SendStreamItems
	{
		const std::string cname(cnameLength, 'a');

		auto senderReport             = std::make_unique<RTC::RTCP::SenderReport>();
		auto sdesChunk                = std::make_unique<RTC::RTCP::SdesChunk>(ssrc);
		auto delaySinceLastRrSsrcInfo = std::make_unique<RTC::RTCP::DelaySinceLastRr::SsrcInfo>();

		senderReport->SetSsrc(ssrc);
		sdesChunk->AddItem(
		  new RTC::RTCP::SdesItem(RTC::RTCP::SdesItem::Type::CNAME, cname.size(), cname.c_str()));
		delaySinceLastRrSsrcInfo->SetSsrc(ssrc);

		return {
			.senderReport             = std::move(senderReport),
			.sdesChunk                = std::move(sdesChunk),
			.delaySinceLastRrSsrcInfo = std::move(delaySinceLastRrSsrcInfo),
		};
	};

	SECTION("send stream items that don't fit are given back")
	{
		RTC::RTCP::CompoundPacket packet;

		// Each stream takes 28 bytes of SR, 108 of SDES chunk (with a 100 bytes
		// CNAME) and 12 of DLRR, plus 16 bytes of headers shared by all of them
		// (SDES packet, XR packet and DLRR block). So 8 streams take exactly 1200
		// bytes.
		for (uint32_t ssrc{ 1 }; ssrc <= 8; ++ssrc)
		{
			auto items = buildSendStreamItems(ssrc, 100);

			REQUIRE(packet.AddSendStreamItems(items) == true);

			REQUIRE(!items.senderReport);
			REQUIRE(!items.sdesChunk);
			REQUIRE(!items.delaySinceLastRrSsrcInfo);
		}

		REQUIRE(packet.GetSenderReportCount() == 8);
		REQUIRE(packet.GetSize() == RTC::Consts::OutgoingRtcpPacketMaxSize);

		// A 9th stream does not fit, so its items are given back and the packet
		// is left as it was.
		auto items = buildSendStreamItems(9, 100);

		REQUIRE(packet.AddSendStreamItems(items) == false);

		REQUIRE(items.senderReport);
		REQUIRE(items.sdesChunk);
		REQUIRE(items.delaySinceLastRrSsrcInfo);
		REQUIRE(packet.GetSenderReportCount() == 8);
		REQUIRE(packet.GetSize() == RTC::Consts::OutgoingRtcpPacketMaxSize);

		// And they can be added to a new packet.
		RTC::RTCP::CompoundPacket packet2;

		REQUIRE(packet2.AddSendStreamItems(items) == true);

		REQUIRE(packet2.GetSenderReportCount() == 1);
		REQUIRE(packet2.HasDelaySinceLastRr() == true);
	}

	SECTION("a DLRR block created for items that don't fit is removed")
	{
		RTC::RTCP::CompoundPacket packet;

		// A SR without DLRR and with a 255 bytes CNAME takes 28 + 264 bytes, plus
		// 4 of SDES packet header. So 4 of them take 1172 bytes.
		for (uint32_t ssrc{ 1 }; ssrc <= 4; ++ssrc)
		{
			auto items = buildSendStreamItems(ssrc, 255);

			items.delaySinceLastRrSsrcInfo.reset();

			REQUIRE(packet.AddSendStreamItems(items) == true);
		}

		REQUIRE(packet.GetSize() == 1172);
		REQUIRE(packet.HasDelaySinceLastRr() == false);

		// A 5th one with DLRR does not fit, and the DLRR block created for it
		// must not be left behind.
		auto items = buildSendStreamItems(5, 255);

		REQUIRE(packet.AddSendStreamItems(items) == false);

		REQUIRE(packet.GetSize() == 1172);
		REQUIRE(packet.HasDelaySinceLastRr() == false);
	}

	SECTION("receiver reports that don't fit are given back")
	{
		const Utils::Time::Ntp ntp{ .seconds = 1, .fractions = 2 };

		RTC::RTCP::CompoundPacket packet;

		// Each report takes 24 bytes, each RR packet of up to 31 reports takes 8
		// bytes of header, and the Receiver Reference Time takes 20 bytes. So 48
		// reports take 1188 bytes.
		for (uint32_t ssrc{ 1 }; ssrc <= 48; ++ssrc)
		{
			auto report = std::make_unique<RTC::RTCP::ReceiverReport>();

			report->SetSsrc(ssrc);

			REQUIRE(packet.AddReceiverReport(report, ntp) == true);

			REQUIRE(!report);
		}

		REQUIRE(packet.GetReceiverReportCount() == 48);
		REQUIRE(packet.HasReceiverReferenceTime() == true);
		REQUIRE(packet.GetSize() == 1188);

		// A 49th report does not fit, so it is given back and the packet is left
		// as it was.
		auto report = std::make_unique<RTC::RTCP::ReceiverReport>();

		report->SetSsrc(49);

		REQUIRE(packet.AddReceiverReport(report, ntp) == false);

		REQUIRE(report);
		REQUIRE(packet.GetReceiverReportCount() == 48);
		REQUIRE(packet.GetSize() == 1188);

		// And it can be added to a new packet, which gets its own Receiver
		// Reference Time.
		RTC::RTCP::CompoundPacket packet2;

		REQUIRE(packet2.AddReceiverReport(report, ntp) == true);

		REQUIRE(packet2.GetReceiverReportCount() == 1);
		REQUIRE(packet2.HasReceiverReferenceTime() == true);
	}
}
