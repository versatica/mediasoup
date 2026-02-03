#include "RTC/RTP/FuzzerRtpStreamSend.hpp"
#include "Utils.hpp"
#include "RTC/RTP/Packet.hpp"
#include "RTC/RTP/RtpStreamSend.hpp"
#include "RTC/RTP/SharedPacket.hpp"

class TestRtpStreamListener : public RTC::RTP::RtpStreamSend::Listener
{
public:
	void OnRtpStreamScore(
	  ::RTC::RTP::RtpStream* /*rtpStream*/, uint8_t /*score*/, uint8_t /*previousScore*/) override
	{
	}

	void OnRtpStreamRetransmitRtpPacket(
	  RTC::RTP::RtpStreamSend* /*rtpStream*/, ::RTC::RTP::Packet* packet) override
	{
	}
};

void Fuzzer::RTC::RTP::RtpStreamSend::Fuzz(const uint8_t* data, size_t len)
{
	// clang-format off
	uint8_t buffer[] =
	{
		0b10000000, 0b01111011, 0b01010010, 0b00001110,
		0b01011011, 0b01101011, 0b11001010, 0b10110101,
		0, 0, 0, 2
	};
	// clang-format on

	// Create base RtpPacket instance.
	auto* packet = ::RTC::RTP::Packet::Parse(buffer, 12);

	// Create a RtpStreamSend instance.
	TestRtpStreamListener testRtpStreamListener;

	// Create RtpStreamSend instance.
	::RTC::RTP::RtpStream::Params params;

	params.ssrc          = 1111;
	params.clockRate     = 90000;
	params.useNack       = true;
	params.mimeType.type = ::RTC::RtpCodecMimeType::Type::VIDEO;

	packet->SetSsrc(params.ssrc);

	std::string mid;
	auto* stream = new ::RTC::RTP::RtpStreamSend(&testRtpStreamListener, params, mid);
	size_t offset{ 0u };

	while (len >= 4u)
	{
		::RTC::RTP::SharedPacket sharedPacket;

		// Set 'random' sequence number and timestamp.
		packet->SetSequenceNumber(Utils::Byte::Get2Bytes(data, offset));
		packet->SetTimestamp(Utils::Byte::Get4Bytes(data, offset));

		stream->ReceivePacket(packet, sharedPacket);

		len -= 4u;
		offset += 4;
	}

	delete stream;
	delete packet;
}
