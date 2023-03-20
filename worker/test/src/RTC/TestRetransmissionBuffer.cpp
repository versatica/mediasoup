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

		// clang-format off
		myRetransmissionBuffer.AssertBuffer(
			{
				{ true, 10001, 1000000000 },
				{ true, 10002, 1000000000 },
				{ true, 10003, 1000000200 },
				{ true, 10004, 1000000200 }
			}
		);
		// clang-format on
	}

	SECTION("proper packets received out of order")
	{
		uint16_t maxItems{ 4 };
		uint32_t maxRetransmissionDelayMs{ 2000u };
		uint32_t clockRate{ 90000 };

		MyRetransmissionBuffer myRetransmissionBuffer(maxItems, maxRetransmissionDelayMs, clockRate);

		myRetransmissionBuffer.Insert(20004, 2000000200);
		myRetransmissionBuffer.Insert(20001, 2000000000);
		myRetransmissionBuffer.Insert(20003, 2000000200);
		myRetransmissionBuffer.Insert(20002, 2000000000);

		// clang-format off
		myRetransmissionBuffer.AssertBuffer(
			{
				{ true, 20001, 2000000000 },
				{ true, 20002, 2000000000 },
				{ true, 20003, 2000000200 },
				{ true, 20004, 2000000200 }
			}
		);
		// clang-format on
	}

	SECTION("packet with too new sequence number produces buffer emptying")
	{
		uint16_t maxItems{ 4 };
		uint32_t maxRetransmissionDelayMs{ 2000u };
		uint32_t clockRate{ 90000 };

		MyRetransmissionBuffer myRetransmissionBuffer(maxItems, maxRetransmissionDelayMs, clockRate);

		myRetransmissionBuffer.Insert(30001, 3000000000);
		myRetransmissionBuffer.Insert(30002, 3000000000);
		myRetransmissionBuffer.Insert(30003, 3000000200);
		myRetransmissionBuffer.Insert(40000, 3000003000);

		// clang-format off
		myRetransmissionBuffer.AssertBuffer(
			{
				{ true, 40000, 3000003000 }
			}
		);
		// clang-format on
	}

	SECTION("blank slots are properly created")
	{
		uint16_t maxItems{ 10 };
		uint32_t maxRetransmissionDelayMs{ 2000u };
		uint32_t clockRate{ 90000 };

		MyRetransmissionBuffer myRetransmissionBuffer(maxItems, maxRetransmissionDelayMs, clockRate);

		myRetransmissionBuffer.Insert(40002, 4000000002);
		// Packet must be discarded since its timestamp is lower than in seq 40002.
		myRetransmissionBuffer.Insert(40003, 4000000001);
		// Must produce 1 blank slot.
		myRetransmissionBuffer.Insert(40004, 4000000004);
		// Discarded (duplicated).
		myRetransmissionBuffer.Insert(40002, 4000000002);
		// Must produce 4 blank slot.
		myRetransmissionBuffer.Insert(40008, 4000000008);
		myRetransmissionBuffer.Insert(40006, 4000000006);
		// Must produce 1 blank slot at the front.
		myRetransmissionBuffer.Insert(40000, 4000000000);

		// clang-format off
		myRetransmissionBuffer.AssertBuffer(
			{
				{ true, 40000, 4000000000 },
				{ false, 0, 0 },
				{ true, 40002, 4000000002 },
				{ false, 0, 0 },
				{ true, 40004, 4000000004 },
				{ false, 0, 0 },
				{ true, 40006, 4000000006 },
				{ false, 0, 0 },
				{ true, 40008, 4000000008 }
			}
		);
		// clang-format on
	}

	SECTION("packet with too old sequence number is discarded")
	{
		uint16_t maxItems{ 4 };
		uint32_t maxRetransmissionDelayMs{ 2000u };
		uint32_t clockRate{ 90000 };

		MyRetransmissionBuffer myRetransmissionBuffer(maxItems, maxRetransmissionDelayMs, clockRate);

		myRetransmissionBuffer.Insert(10001, 1000000001);
		myRetransmissionBuffer.Insert(10002, 1000000002);
		myRetransmissionBuffer.Insert(10003, 1000000003);
		// Too old seq.
		myRetransmissionBuffer.Insert(40000, 1000000000);

		// clang-format off
		myRetransmissionBuffer.AssertBuffer(
			{
				{ true, 10001, 1000000001 },
				{ true, 10002, 1000000002 },
				{ true, 10003, 1000000003 }
			}
		);
		// clang-format on
	}

	SECTION("packet with too old timestamp is discarded")
	{
		uint16_t maxItems{ 4 };
		uint32_t maxRetransmissionDelayMs{ 2000u };
		uint32_t clockRate{ 90000 };

		MyRetransmissionBuffer myRetransmissionBuffer(maxItems, maxRetransmissionDelayMs, clockRate);

		auto maxDiffTs = static_cast<uint32_t>(maxRetransmissionDelayMs * clockRate / 1000);

		myRetransmissionBuffer.Insert(10001, 1000000001);
		myRetransmissionBuffer.Insert(10002, 1000000002);
		myRetransmissionBuffer.Insert(10003, 1000000003);
		// Too old timestamp (subtract 100 to avoid math issues).
		myRetransmissionBuffer.Insert(10000, 1000000003 - maxDiffTs - 100);

		// clang-format off
		myRetransmissionBuffer.AssertBuffer(
			{
				{ true, 10001, 1000000001 },
				{ true, 10002, 1000000002 },
				{ true, 10003, 1000000003 }
			}
		);
		// clang-format on
	}
}
