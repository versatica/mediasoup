#include "common.hpp"
#include "RTC/RTCP/FeedbackRtpTmmb.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memcmp()

SCENARIO("RTCP Feedback RTP TMMBR", "[rtcp][feedback-rtp][tmmb]")
{
	// RTCP TMMBR packet.

	// clang-format off
	alignas(4) uint8_t buffer[] =
	{
		0x83, 0xcd, 0x00, 0x04, // Type: 205 (Generic RTP Feedback), Count: 8 (TMMBR) Length: 7
		0x00, 0x00, 0x00, 0x01, // Sender SSRC: 0x00000001
		0x03, 0x30, 0xbd, 0xee, // Media source SSRC: 0x0330bdee
		0x02, 0xd0, 0x37, 0x02, // SSRC: 0x02d03702
		0x18, 0x2c, 0x9e, 0x00
	};
	// clang-format on

	// TMMBR values.
	const uint32_t senderSsrc{ 0x00000001 };
	const uint32_t mediaSsrc{ 0x0330bdee };
	const uint32_t ssrc{ 0x02d03702 };
	const uint64_t bitrate{ 365504 };
	const uint16_t overhead{ 0 };

	// NOTE: No need to pass const integers to the lambda.
	auto verify = [](RTC::RTCP::FeedbackRtpTmmbrPacket* packet)
	{
		REQUIRE(packet->GetSenderSsrc() == senderSsrc);
		REQUIRE(packet->GetMediaSsrc() == mediaSsrc);

		auto it          = packet->Begin();
		const auto* item = *it;

		REQUIRE(item);
		REQUIRE(item->GetSsrc() == ssrc);
		REQUIRE(item->GetBitrate() == bitrate);
		REQUIRE(item->GetOverhead() == overhead);
	};

	SECTION("alignof() RTCP structs")
	{
		REQUIRE(alignof(RTC::RTCP::FeedbackRtpTmmbrItem::Header) == 4);
		REQUIRE(alignof(RTC::RTCP::FeedbackRtpTmmbnItem::Header) == 4);
	}

	SECTION("parse FeedbackRtpTmmbrPacket")
	{
		std::unique_ptr<RTC::RTCP::FeedbackRtpTmmbrPacket> packet{
			RTC::RTCP::FeedbackRtpTmmbrPacket::Parse(buffer, sizeof(buffer))
		};

		REQUIRE(packet);

		verify(packet.get());

		SECTION("serialize packet instance")
		{
			alignas(4) uint8_t serialized[sizeof(buffer)] = { 0 };

			packet->Serialize(serialized);

			// NOTE: Do not compare byte by byte since different binary values can
			// represent the same content.
			SECTION("create a packet out of the serialized buffer")
			{
				const std::unique_ptr<RTC::RTCP::FeedbackRtpTmmbrPacket> packet{
					RTC::RTCP::FeedbackRtpTmmbrPacket::Parse(buffer, sizeof(buffer))
				};

				verify(packet.get());
			}
		}
	}

	SECTION("create FeedbackRtpTmmbrPacket")
	{
		RTC::RTCP::FeedbackRtpTmmbrPacket packet(senderSsrc, mediaSsrc);
		auto* item = new RTC::RTCP::FeedbackRtpTmmbrItem();

		item->SetSsrc(ssrc);
		item->SetBitrate(bitrate);
		item->SetOverhead(overhead);

		packet.AddItem(item);

		verify(&packet);
	}
}
