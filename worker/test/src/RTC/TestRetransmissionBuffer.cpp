#include "common.hpp"
#include "RTC/RetransmissionBuffer.hpp"
#include "RTC/RtpPacket.hpp"
#include <catch2/catch.hpp>
#include <vector>

using namespace RTC;

// Class inheriting from RetransmissionBuffer so we can access its protected
// buffer member.
class MyRetransmissionBuffer : public RetransmissionBuffer
{
public:
	struct VerificationItem
	{
		bool isPresent;
		uint16_t sequenceNumber;
		uint32_t timestamp;
	};

public:
	MyRetransmissionBuffer(uint16_t maxItems, uint32_t maxRetransmissionDelayMs, uint32_t clockRate)
	  : RetransmissionBuffer(maxItems, maxRetransmissionDelayMs, clockRate)
	{
	}

public:
	void Insert(uint16_t seq, uint32_t timestamp)
	{
		// clang-format off
		uint8_t rtpBuffer[] =
		{
			0b10000000, 0b01111011, 0b01010010, 0b00001110,
			0b01011011, 0b01101011, 0b11001010, 0b10110101,
			0, 0, 0, 2
		};
		// clang-format on

		auto* packet = RtpPacket::Parse(rtpBuffer, sizeof(rtpBuffer));

		packet->SetSequenceNumber(seq);
		packet->SetTimestamp(timestamp);

		std::shared_ptr<RtpPacket> sharedPacket;

		RetransmissionBuffer::Insert(packet, sharedPacket);
	}

	void AssertBuffer(std::vector<VerificationItem> verificationBuffer)
	{
		REQUIRE(verificationBuffer.size() == this->buffer.size());

		for (size_t idx{ 0u }; idx < verificationBuffer.size(); ++idx)
		{
			auto& verificationItem = verificationBuffer.at(idx);
			auto* item             = this->buffer.at(idx);

			REQUIRE(verificationItem.isPresent == !!item);

			if (item)
			{
				REQUIRE(verificationItem.sequenceNumber == item->sequenceNumber);
				REQUIRE(verificationItem.timestamp == item->timestamp);
			}
		}
	}
};

SCENARIO("RetransmissionBuffer", "[rtp][rtx]")
{
	SECTION("proper packets received in order")
	{
		uint16_t maxItems{ 4 };
		uint32_t maxRetransmissionDelayMs{ 2000u };
		uint32_t clockRate{ 90000 };

		MyRetransmissionBuffer myRetransmissionBuffer(maxItems, maxRetransmissionDelayMs, clockRate);

		myRetransmissionBuffer.Insert(10001, 1000000000);
		myRetransmissionBuffer.Insert(10002, 1000000000);
		myRetransmissionBuffer.Insert(10003, 1000000200);
		myRetransmissionBuffer.Insert(10004, 1000000200);

		myRetransmissionBuffer.AssertBuffer({ { true, 10001, 1000000000 },
		                                      { true, 10002, 1000000000 },
		                                      { true, 10003, 1000000200 },
		                                      { true, 10004, 1000000200 } });
	}

	SECTION("proper packets received out of order")
	{
		uint16_t maxItems{ 4 };
		uint32_t maxRetransmissionDelayMs{ 2000u };
		uint32_t clockRate{ 90000 };

		MyRetransmissionBuffer myRetransmissionBuffer(maxItems, maxRetransmissionDelayMs, clockRate);

		myRetransmissionBuffer.Insert(10004, 1000000200);
		myRetransmissionBuffer.Insert(10001, 1000000000);
		myRetransmissionBuffer.Insert(10003, 1000000200);
		myRetransmissionBuffer.Insert(10002, 1000000000);

		myRetransmissionBuffer.AssertBuffer({ { true, 10001, 1000000000 },
		                                      { true, 10002, 1000000000 },
		                                      { true, 10003, 1000000200 },
		                                      { true, 10004, 1000000200 } });
	}

	SECTION("too new packet makes buffer emptying")
	{
		uint16_t maxItems{ 4 };
		uint32_t maxRetransmissionDelayMs{ 2000u };
		uint32_t clockRate{ 90000 };

		MyRetransmissionBuffer myRetransmissionBuffer(maxItems, maxRetransmissionDelayMs, clockRate);

		myRetransmissionBuffer.Insert(10001, 1000000000);
		myRetransmissionBuffer.Insert(10002, 1000000000);
		myRetransmissionBuffer.Insert(10003, 1000000200);
		myRetransmissionBuffer.Insert(11005, 2000000000);

		myRetransmissionBuffer.AssertBuffer({ { true, 11005, 2000000000 } });
	}
}
