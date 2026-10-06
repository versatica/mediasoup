#include "RTC/RTP/FuzzerRtpStreamSend.hpp"
#include "RTC/RTCP/ReceiverReport.hpp"
#include "RTC/RTP/SharedPacket.hpp"
#include "Utils.hpp"
#include "mocks/include/MockShared.hpp"

namespace
{
	// NOLINTNEXTLINE(readability-identifier-naming)
	thread_local mocks::MockShared shared(/*getTimeUs*/
	                                      []() -> int64_t
	                                      {
		                                      return 1000 * 1000;
	                                      });
} // namespace

void FuzzerRtcRtpStreamSend::Fuzz(const uint8_t* data, size_t len)
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
	auto* packet = RTC::RTP::Packet::Parse(buffer, 12);

	// Create a RtpStreamSend instance.
	TestRtpStreamListener testRtpStreamListener;

	// Create RtpStreamSend instance.
	RTC::RTP::RtpStream::Params params;

	params.ssrc          = 1111;
	params.clockRate     = 90000;
	params.useNack       = true;
	params.mimeType.type = RTC::RtpCodecMimeType::Type::VIDEO;

	packet->SetSsrc(params.ssrc);

	std::string mid;
	auto* stream = new RTC::RTP::RtpStreamSend(
	  std::addressof(testRtpStreamListener), std::addressof(shared), params, mid);
	size_t offset{ 0u };

	while (len >= 12u)
	{
		const RTC::RTP::SharedPacket sharedPacket;

		// Set 'random' sequence number and timestamp.
		packet->SetSequenceNumber(Utils::Byte::Get2Bytes(data, offset));
		packet->SetTimestamp(Utils::Byte::Get4Bytes(data, offset + 2));

		stream->ReceivePacket(packet, sharedPacket);

		// Feed a 'random' Receiver Report, which is what the loss of the stream is
		// worked out from.
		RTC::RTCP::ReceiverReport report;

		report.SetSsrc(params.ssrc);
		report.SetLastSeq(Utils::Byte::Get4Bytes(data, offset + 6));
		// NOTE: Signed on purpose, since a duplicate makes a remote endpoint report
		// fewer packets lost than it reported before.
		report.SetTotalLost(static_cast<int16_t>(Utils::Byte::Get2Bytes(data, offset + 10)));

		stream->ReceiveRtcpReceiverReport(std::addressof(report), shared.GetTimeUs());

		len -= 12u;
		offset += 12;
	}

	delete stream;
	delete packet;
}
