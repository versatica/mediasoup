#include "common.hpp"
#include "Logger.hpp"
#include "RTC/RTCP/FeedbackRtpTransport.hpp"
#include <catch2/catch.hpp>
#include <cstring> // std::memcmp()

using namespace RTC::RTCP;

struct TestFeedbackRtpTransportInput
{
	TestFeedbackRtpTransportInput(uint16_t sequenceNumber, uint64_t timestamp, size_t maxPacketSize)
	  : sequenceNumber(sequenceNumber), timestamp(timestamp), maxPacketSize(maxPacketSize)
	{
	}

	uint16_t sequenceNumber{ 0u };
	uint64_t timestamp{ 0u };
	size_t maxPacketSize{ 0u };
};

void validate(
  const std::vector<struct TestFeedbackRtpTransportInput>& inputs,
  std::vector<struct FeedbackRtpTransportPacket::PacketResult> packetResults)
{
	auto inputsIterator        = inputs.begin();
	auto packetResultsIterator = packetResults.begin();
	auto lastInput             = *inputsIterator;

	for (++inputsIterator; inputsIterator != inputs.end(); ++inputsIterator, ++packetResultsIterator)
	{
		auto& input             = *inputsIterator;
		auto& packetResult      = *packetResultsIterator;
		uint16_t missingPackets = input.sequenceNumber - lastInput.sequenceNumber - 1;

		if (missingPackets > 0)
		{
			// All missing packets must be represented in packetResults.
			for (uint16_t i{ 0u }; i < missingPackets; ++i)
			{
				packetResult = *packetResultsIterator;

				REQUIRE(packetResult.sequenceNumber == lastInput.sequenceNumber + i + 1);
				REQUIRE(packetResult.received == false);

				packetResultsIterator++;
			}
		}
		else
		{
			REQUIRE(packetResult.sequenceNumber == lastInput.sequenceNumber + 1);
			REQUIRE(packetResult.sequenceNumber == input.sequenceNumber);
			REQUIRE(packetResult.received == true);
			REQUIRE(
			  static_cast<int32_t>(packetResult.receivedAtMs & 0x1FFFFFC0) / 64 ==
			  static_cast<int32_t>(input.timestamp & 0x1FFFFFC0) / 64);
		}

		lastInput = input;
	}
}

SCENARIO("RTCP Feeback RTP transport", "[parser][rtcp][feedback-rtp][transport]")
{
	static constexpr size_t RtcpMtu{ 1200u };

	uint32_t senderSsrc{ 1111u };
	uint32_t mediaSsrc{ 2222u };

	SECTION(
	  "create FeedbackRtpTransportPacket, small delta run length chunk and single large delta status packet")
	{
		auto* packet = new FeedbackRtpTransportPacket(senderSsrc, mediaSsrc);

		REQUIRE(packet);

		/* clang-format off */
		std::vector<struct TestFeedbackRtpTransportInput> inputs =
		{
			{ 999, 1000000000, RtcpMtu },  // Pre base.
			{ 1000, 1000000000, RtcpMtu }, // Base.
			{ 1001, 1000000001, RtcpMtu },
			{ 1002, 1000000012, RtcpMtu },
			{ 1003, 1000000015, RtcpMtu },
			{ 1004, 1000000017, RtcpMtu },
			{ 1005, 1000000018, RtcpMtu },
			{ 1006, 1000000018, RtcpMtu },
			{ 1007, 1000000018, RtcpMtu },
			{ 1008, 1000000018, RtcpMtu },
			{ 1009, 1000000019, RtcpMtu },
			{ 1010, 1000000010, RtcpMtu },
			{ 1011, 1000000011, RtcpMtu },
			{ 1012, 1000000011, RtcpMtu },
			{ 1013, 1000000013, RtcpMtu }
		};
		/* clang-format on */

		packet->SetFeedbackPacketCount(1);

		for (auto& input : inputs)
			packet->AddPacket(input.sequenceNumber, input.timestamp, input.maxPacketSize);

		REQUIRE(packet->GetLatestSequenceNumber() == 1013);
		REQUIRE(packet->GetLatestTimestamp() == 1000000013);

		// Add a packet with greater seq number but older timestamp.
		packet->AddPacket(1014, 1000000013 - 128, RtcpMtu);
		inputs.emplace_back(1014, 1000000013 - 128, RtcpMtu);

		REQUIRE(packet->GetLatestSequenceNumber() == 1014);
		REQUIRE(packet->GetLatestTimestamp() == 1000000013 - 128);

		packet->AddPacket(1015, 1000000015, RtcpMtu);
		inputs.emplace_back(1015, 1000000015, RtcpMtu);

		REQUIRE(packet->GetLatestSequenceNumber() == 1015);
		REQUIRE(packet->GetLatestTimestamp() == 1000000015);

		packet->Finish();
		validate(inputs, packet->GetPacketResults());

		REQUIRE(packet->GetBaseSequenceNumber() == 1000);
		REQUIRE(packet->GetPacketStatusCount() == 16);
		REQUIRE(packet->GetFeedbackPacketCount() == 1);
		REQUIRE(packet->GetPacketFractionLost() == 0);

		SECTION("serialize packet instance")
		{
			uint8_t buffer[1024];
			auto len = packet->Serialize(buffer);

			REQUIRE(packet->GetSize() == len);

			SECTION("parse serialized buffer")
			{
				auto* packet2 = FeedbackRtpTransportPacket::Parse(buffer, len);

				REQUIRE(packet2);
				REQUIRE(packet2->GetBaseSequenceNumber() == 1000);
				REQUIRE(packet2->GetPacketStatusCount() == 16);
				REQUIRE(packet2->GetFeedbackPacketCount() == 1);
				REQUIRE(packet2->GetPacketFractionLost() == 0);

				uint8_t buffer2[1024];
				auto len2 = packet2->Serialize(buffer2);

				REQUIRE(len == len2);
				REQUIRE(std::memcmp(buffer, buffer2, len) == 0);
				REQUIRE(packet2->GetSize() == len2);

				delete packet2;
			}
		}

		delete packet;
	}

	SECTION("create FeedbackRtpTransportPacket, run length chunk (2)")
	{
		auto* packet = new FeedbackRtpTransportPacket(senderSsrc, mediaSsrc);

		/* clang-format off */
		std::vector<TestFeedbackRtpTransportInput> inputs =
		{
			{ 999, 1000000000, RtcpMtu }, // Pre base.
			{ 1000, 1000000000, RtcpMtu }, // Base.
			{ 1050, 1000000216, RtcpMtu }
		};
		/* clang-format on */

		packet->SetFeedbackPacketCount(10);

		for (auto& input : inputs)
			packet->AddPacket(input.sequenceNumber, input.timestamp, input.maxPacketSize);

		packet->Finish();
		validate(inputs, packet->GetPacketResults());

		REQUIRE(packet->GetBaseSequenceNumber() == 1000);
		REQUIRE(packet->GetPacketStatusCount() == 51);
		REQUIRE(packet->GetFeedbackPacketCount() == 10);
		REQUIRE(packet->GetPacketFractionLost() > 0);
		REQUIRE(packet->GetLatestSequenceNumber() == 1050);
		REQUIRE(packet->GetLatestTimestamp() == 1000000216);

		SECTION("serialize packet instance")
		{
			uint8_t buffer[1024];
			auto len = packet->Serialize(buffer);

			REQUIRE(packet->GetSize() == len);

			SECTION("parse serialized buffer")
			{
				auto* packet2 = FeedbackRtpTransportPacket::Parse(buffer, len);

				REQUIRE(packet2);
				REQUIRE(packet2->GetBaseSequenceNumber() == 1000);
				REQUIRE(packet2->GetPacketStatusCount() == 51);
				REQUIRE(packet2->GetFeedbackPacketCount() == 10);
				REQUIRE(packet2->GetPacketFractionLost() > 0);

				uint8_t buffer2[1024];
				auto len2 = packet2->Serialize(buffer2);

				REQUIRE(len == len2);
				REQUIRE(std::memcmp(buffer, buffer2, len) == 0);
				REQUIRE(packet2->GetSize() == len2);

				delete packet2;
			}
		}

		delete packet;
	}

	SECTION("create FeedbackRtpTransportPacket, mixed chunks")
	{
		/* clang-format off */
		std::vector<TestFeedbackRtpTransportInput> inputs =
		{
			{ 999, 1000000000, RtcpMtu },  // Pre base.
			{ 1000, 1000000000, RtcpMtu }, // Base.
			{ 1001, 1000000100, RtcpMtu },
			{ 1002, 1000000200, RtcpMtu },
			{ 1015, 1000000300, RtcpMtu },
			{ 1016, 1000000400, RtcpMtu },
			{ 1017, 1000000500, RtcpMtu }
		};
		/* clang-format on */

		auto* packet = new FeedbackRtpTransportPacket(senderSsrc, mediaSsrc);

		packet->SetFeedbackPacketCount(1);

		for (auto& input : inputs)
			packet->AddPacket(input.sequenceNumber, input.timestamp, input.maxPacketSize);

		packet->Finish();
		validate(inputs, packet->GetPacketResults());

		REQUIRE(packet->GetBaseSequenceNumber() == 1000);
		REQUIRE(packet->GetPacketStatusCount() == 18);
		REQUIRE(packet->GetFeedbackPacketCount() == 1);
		REQUIRE(packet->GetPacketFractionLost() > 0);
		REQUIRE(packet->GetLatestSequenceNumber() == 1017);
		REQUIRE(packet->GetLatestTimestamp() == 1000000500);

		SECTION("serialize packet instance")
		{
			uint8_t buffer[1024];
			auto len = packet->Serialize(buffer);

			REQUIRE(packet->GetSize() == len);

			SECTION("parse serialized buffer")
			{
				auto* packet2 = FeedbackRtpTransportPacket::Parse(buffer, len);

				REQUIRE(packet2);
				REQUIRE(packet2->GetBaseSequenceNumber() == 1000);
				REQUIRE(packet2->GetPacketStatusCount() == 18);
				REQUIRE(packet2->GetFeedbackPacketCount() == 1);
				REQUIRE(packet2->GetPacketFractionLost() > 0);

				uint8_t buffer2[1024];
				auto len2 = packet2->Serialize(buffer2);

				REQUIRE(len == len2);
				REQUIRE(std::memcmp(buffer, buffer2, len) == 0);
				REQUIRE(packet2->GetSize() == len2);

				delete packet2;
			}
		}

		delete packet;
	}

	SECTION("create FeedbackRtpTransportPacket, incomplete two bit vector chunk")
	{
		std::vector<TestFeedbackRtpTransportInput> inputs = {
			{ 999, 1000000000, RtcpMtu },  // Pre base.
			{ 1000, 1000000100, RtcpMtu }, // Base.
			{ 1001, 1000000700, RtcpMtu },
		};

		auto* packet = new FeedbackRtpTransportPacket(senderSsrc, mediaSsrc);

		packet->SetFeedbackPacketCount(1);

		for (auto& input : inputs)
			packet->AddPacket(input.sequenceNumber, input.timestamp, input.maxPacketSize);

		packet->Finish();
		validate(inputs, packet->GetPacketResults());

		REQUIRE(packet->GetBaseSequenceNumber() == 1000);
		REQUIRE(packet->GetPacketStatusCount() == 2);
		REQUIRE(packet->GetFeedbackPacketCount() == 1);
		REQUIRE(packet->GetPacketFractionLost() == 0);
		REQUIRE(packet->GetLatestSequenceNumber() == 1001);
		REQUIRE(packet->GetLatestTimestamp() == 1000000700);

		SECTION("serialize packet instance")
		{
			uint8_t buffer[1024];
			auto len = packet->Serialize(buffer);

			REQUIRE(packet->GetSize() == len);

			SECTION("parse serialized buffer")
			{
				auto* packet2 = FeedbackRtpTransportPacket::Parse(buffer, len);

				REQUIRE(packet2);
				REQUIRE(packet2->GetBaseSequenceNumber() == 1000);
				REQUIRE(packet2->GetPacketStatusCount() == 2);
				REQUIRE(packet2->GetFeedbackPacketCount() == 1);
				REQUIRE(packet2->GetPacketFractionLost() == 0);

				uint8_t buffer2[1024];
				auto len2 = packet2->Serialize(buffer2);

				REQUIRE(len == len2);
				REQUIRE(std::memcmp(buffer, buffer2, len) == 0);
				REQUIRE(packet2->GetSize() == len2);

				delete packet2;
			}
		}

		delete packet;
	}

	SECTION("create two sequential FeedbackRtpTransportPackets")
	{
		/* clang-format off */
		std::vector<TestFeedbackRtpTransportInput> inputs =
		{
			{ 999, 1000000000, RtcpMtu },  // Pre base.
			{ 1000, 1000000000, RtcpMtu }, // Base.
			{ 1001, 1000000003, RtcpMtu },
			{ 1002, 1000000003, RtcpMtu },
			{ 1003, 1000000003, RtcpMtu },
			{ 1004, 1000000004, RtcpMtu },
			{ 1005, 1000000005, RtcpMtu },
			{ 1006, 1000000005, RtcpMtu },
			{ 1007, 1000000007, RtcpMtu }
		};
		/* clang-format on */

		auto* packet = new FeedbackRtpTransportPacket(senderSsrc, mediaSsrc);

		packet->SetFeedbackPacketCount(1);

		for (auto& input : inputs)
			packet->AddPacket(input.sequenceNumber, input.timestamp, input.maxPacketSize);

		packet->Finish();
		validate(inputs, packet->GetPacketResults());

		REQUIRE(packet->GetBaseSequenceNumber() == 1000);
		REQUIRE(packet->GetPacketStatusCount() == 8);
		REQUIRE(packet->GetFeedbackPacketCount() == 1);
		REQUIRE(packet->GetPacketFractionLost() == 0);
		REQUIRE(packet->GetLatestSequenceNumber() == 1007);
		REQUIRE(packet->GetLatestTimestamp() == 1000000007);

		uint8_t buffer[1024];
		auto len = packet->Serialize(buffer);

		REQUIRE(packet->GetSize() == len);

		SECTION("parse serialized buffer")
		{
			auto* packet2 = FeedbackRtpTransportPacket::Parse(buffer, len);

			REQUIRE(packet2);
			REQUIRE(packet2->GetBaseSequenceNumber() == 1000);
			REQUIRE(packet2->GetPacketStatusCount() == 8);
			REQUIRE(packet2->GetFeedbackPacketCount() == 1);
			REQUIRE(packet2->GetPacketFractionLost() == 0);

			uint8_t buffer2[1024];
			auto len2 = packet2->Serialize(buffer2);

			REQUIRE(len == len2);
			REQUIRE(std::memcmp(buffer, buffer2, len) == 0);
			REQUIRE(packet2->GetSize() == len2);

			delete packet2;
		}

		auto latestWideSeqNumber = packet->GetLatestSequenceNumber();
		auto latestTimestamp     = packet->GetLatestTimestamp();

		/* clang-format off */
		std::vector<TestFeedbackRtpTransportInput> inputs2 =
		{
			{ latestWideSeqNumber, latestTimestamp, RtcpMtu },
			{ 1008, 1000000008, RtcpMtu },
			{ 1009, 1000000009, RtcpMtu },
			{ 1010, 1000000010, RtcpMtu },
			{ 1011, 1000000010, RtcpMtu },
			{ 1012, 1000000010, RtcpMtu },
			{ 1013, 1000000014, RtcpMtu },
			{ 1014, 1000000014, RtcpMtu }
		};
		/* clang-format on */

		auto* packet2 = new FeedbackRtpTransportPacket(senderSsrc, mediaSsrc);

		packet2->SetFeedbackPacketCount(2);

		for (auto& input : inputs2)
			packet2->AddPacket(input.sequenceNumber, input.timestamp, input.maxPacketSize);

		packet2->Finish();
		validate(inputs2, packet2->GetPacketResults());

		REQUIRE(packet2->GetBaseSequenceNumber() == 1008);
		REQUIRE(packet2->GetPacketStatusCount() == 7);
		REQUIRE(packet2->GetFeedbackPacketCount() == 2);
		REQUIRE(packet2->GetPacketFractionLost() == 0);
		REQUIRE(packet2->GetLatestSequenceNumber() == 1014);
		REQUIRE(packet2->GetLatestTimestamp() == 1000000014);

		len = packet2->Serialize(buffer);

		REQUIRE(packet2->GetSize() == len);

		SECTION("parse serialized buffer")
		{
			auto* packet3 = FeedbackRtpTransportPacket::Parse(buffer, len);

			REQUIRE(packet3);
			REQUIRE(packet3->GetBaseSequenceNumber() == 1008);
			REQUIRE(packet3->GetPacketStatusCount() == 7);
			REQUIRE(packet3->GetFeedbackPacketCount() == 2);
			REQUIRE(packet3->GetPacketFractionLost() == 0);

			uint8_t buffer2[1024];
			auto len2 = packet3->Serialize(buffer2);

			REQUIRE(len == len2);
			REQUIRE(std::memcmp(buffer, buffer2, len) == 0);
			REQUIRE(packet3->GetSize() == len2);

			delete packet3;
		}

		delete packet2;
		delete packet;
	}

	SECTION("parse FeedbackRtpTransportPacket, one bit vector chunk")
	{
		// clang-format off
		uint8_t data[] =
		{
			0x8F, 0xCD, 0x00, 0x07,
			0xFA, 0x17, 0xFA, 0x17,
			0x09, 0xFA, 0xFF, 0x67,
			0x00, 0x27, 0x00, 0x0D,
			0x5F, 0xC2, 0xF1, 0x03,
			0xBF, 0x8E, 0x10, 0x00,
			0x00, 0x00, 0x00, 0x00,
			0x00, 0x1C, 0x04, 0x00
		};
		// clang-format on

		auto* packet = FeedbackRtpTransportPacket::Parse(data, sizeof(data));

		REQUIRE(packet);
		REQUIRE(packet->GetSize() == sizeof(data));
		REQUIRE(packet->GetBaseSequenceNumber() == 39);
		REQUIRE(packet->GetPacketStatusCount() == 13);
		REQUIRE(packet->GetReferenceTime() == 6275825); // 0x5FC2F1 (signed 24 bits)
		REQUIRE(packet->GetReferenceTimestamp() == 6275825 * 64);
		REQUIRE(packet->GetFeedbackPacketCount() == 3);

		SECTION("serialize packet")
		{
			uint8_t buffer[1024];
			auto len = packet->Serialize(buffer);

			REQUIRE(len == sizeof(data));
			REQUIRE(std::memcmp(data, buffer, len) == 0);
		}

		delete packet;
	}

	SECTION("parse FeedbackRtpTransportPacket with negative reference time")
	{
		// clang-format off
		uint8_t data[] =
		{
			0x8F, 0xCD, 0x00, 0x04,
			0xFA, 0x17, 0xFA, 0x17,
			0x09, 0xFA, 0xFF, 0x67,
			0x00, 0x27, 0x00, 0x00,
			0xFF, 0xFF, 0xFE, 0x01
		};
		// clang-format on

		auto* packet = FeedbackRtpTransportPacket::Parse(data, sizeof(data));

		REQUIRE(packet);
		REQUIRE(packet->GetSize() == sizeof(data));
		REQUIRE(packet->GetBaseSequenceNumber() == 39);
		REQUIRE(packet->GetPacketStatusCount() == 0);
		REQUIRE(packet->GetReferenceTime() == -2); // 0xFFFFFE (signed 24 bits)
		REQUIRE(packet->GetReferenceTimestamp() == -2 * 64);
		REQUIRE(packet->GetFeedbackPacketCount() == 1);

		SECTION("serialize packet")
		{
			uint8_t buffer[1024];
			auto len = packet->Serialize(buffer);

			REQUIRE(len == sizeof(data));
			REQUIRE(std::memcmp(data, buffer, len) == 0);
		}

		delete packet;
	}

	SECTION("parse FeedbackRtpTransportPacket generated by Chrome")
	{
		// clang-format off
		uint8_t data[] =
		{
			0x8F, 0xCD, 0x00, 0x05,
			0xFA, 0x17, 0xFA, 0x17,
			0x39, 0xE9, 0x42, 0x38,
			0x00, 0x01, 0x00, 0x02,
			0xBD, 0x57, 0xAA, 0x00,
			0x20, 0x02, 0x8C, 0x44
		};
		// clang-format on

		auto* packet = FeedbackRtpTransportPacket::Parse(data, sizeof(data));

		REQUIRE(packet);
		REQUIRE(packet->GetSize() == sizeof(data));
		REQUIRE(packet->GetBaseSequenceNumber() == 1);
		REQUIRE(packet->GetPacketStatusCount() == 2);
		REQUIRE(packet->GetReferenceTime() == -4368470);
		REQUIRE(packet->GetReferenceTimestamp() == -4368470 * 64);

		// Let's also test the reference time reported by Wireshark.
		int32_t wiresharkValue{ 12408746 };

		// Constraint it to signed 24 bits.
		wiresharkValue = wiresharkValue << 8;
		wiresharkValue = wiresharkValue >> 8;

		REQUIRE(packet->GetReferenceTime() == wiresharkValue);
		REQUIRE(packet->GetReferenceTimestamp() == wiresharkValue * 64);
		REQUIRE(packet->GetFeedbackPacketCount() == 0);

		SECTION("serialize packet")
		{
			uint8_t buffer[1024];
			auto len = packet->Serialize(buffer);

			REQUIRE(len == sizeof(data));
			REQUIRE(std::memcmp(data, buffer, len) == 0);
		}

		delete packet;
	}
}
