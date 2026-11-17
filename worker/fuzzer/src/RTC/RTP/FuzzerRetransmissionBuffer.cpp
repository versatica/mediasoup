#include "fuzzer/include/RTC/RTP/FuzzerRetransmissionBuffer.hpp"
#include "DepLibUV.hpp"
#include "RTC/RTP/Packet.hpp"
#include "RTC/RTP/PacketUtils.hpp"
#include "RTC/RTP/RetransmissionBuffer.hpp"
#include "Utils.hpp"

void FuzzerRtcRtpRetransmissionBuffer::Fuzz(const uint8_t* data, size_t len)
{
	constexpr uint16_t MaxItems{ 2500 };
	constexpr int64_t MaxRetransmissionDelayMs{ 2000 };
	constexpr uint32_t ClockRate{ 90000 };

	// Initialize our stuff just once.
	static thread_local RTC::RTP::RetransmissionBuffer retransmissionBuffer(
	  MaxItems, MaxRetransmissionDelayMs, ClockRate);

	// clang-format off
	uint8_t buffer[] =
	{
		0b10000000, 0b01111011, 0b01010010, 0b00001110,
		0b01011011, 0b01101011, 0b11001010, 0b10110101,
		0, 0, 0, 2
	};
	// clang-format on

	// Create base RtpPacket instance.
	const std::unique_ptr<RTC::RTP::Packet> packet{ RTC::RTP::Packet::Parse(buffer, 12) };

	size_t offset{ 0 };
	int64_t nowMs{ DepLibUV::GetTimeMs() };

	while (len >= 4)
	{
		// Set 'random' sequence number and timestamp.
		packet->SetSequenceNumber(Utils::Byte::Get2Bytes(data, offset));
		packet->SetTimestamp(Utils::Byte::Get4Bytes(data, offset));

		if (RTC::RTP::RetransmissionBuffer::Item* const item = retransmissionBuffer.Insert(packet.get(), nowMs))
		{
			item->sharedPacket = RTC::RTP::PacketUtils::ClonePacket(packet.get());
		}

		// Let some 'random' time pass and check whether the packet is too old.
		nowMs += static_cast<int64_t>(Utils::Crypto::GetRandomUInt<uint64_t>(0, 1000));

		const RTC::RTP::RetransmissionBuffer::Item* const item =
		  retransmissionBuffer.Get(packet->GetSequenceNumber());

		if (item)
		{
			retransmissionBuffer.IsTooOld(item, nowMs);
		}

		len -= 4;
		offset += 4;
	}
}
