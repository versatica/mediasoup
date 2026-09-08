#include "RTC/RTP/FuzzerRetransmissionBuffer.hpp"
#include "RTC/RTP/Packet.hpp"
#include "RTC/RTP/RetransmissionBuffer.hpp"
#include "RTC/RTP/SharedPacket.hpp"
#include "Utils.hpp"

void FuzzerRtcRtpRetransmissionBuffer::Fuzz(const uint8_t* data, size_t len)
{
	const uint16_t maxItems{ 2500 };
	const int64_t maxRetransmissionDelayMs{ 2000 };
	const uint32_t clockRate{ 90000 };

	// Trick to initialize our stuff just once (use static).
	static RTC::RTP::RetransmissionBuffer retransmissionBuffer(
	  maxItems, maxRetransmissionDelayMs, clockRate);

	// clang-format off
	uint8_t buffer[] =
	{
		0b10000000, 0b01111011, 0b01010010, 0b00001110,
		0b01011011, 0b01101011, 0b11001010, 0b10110101,
		0, 0, 0, 2
	};
	// clang-format on

	// Create base RtpPacket instance.
	auto* packet = RTC::RTP::Packet::Parse(buffer, 12);
	size_t offset{ 0 };

	while (len >= 4)
	{
		const RTC::RTP::SharedPacket sharedPacket;

		// Set 'random' sequence number and timestamp.
		packet->SetSequenceNumber(Utils::Byte::Get2Bytes(data, offset));
		packet->SetTimestamp(Utils::Byte::Get4Bytes(data, offset));

		retransmissionBuffer.Insert(packet, sharedPacket);

		len -= 4;
		offset += 4;
	}

	delete packet;
}
