#include "fuzzer/include/RTC/RTP/FuzzerRtpStreamSend.hpp"
#include "RTC/RTCP/packet/AnyReportPacket.hpp"
#include "RTC/RTP/PacketUtils.hpp"
#include "Utils.hpp"
#include "mocks/include/MockShared.hpp"
#include <string>

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
	const std::unique_ptr<RTC::RTP::Packet> packet{ RTC::RTP::Packet::Parse(buffer, 12) };

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
	const auto stream = std::make_unique<RTC::RTP::RtpStreamSend>(
	  std::addressof(testRtpStreamListener), std::addressof(shared), params, mid);
	size_t offset{ 0 };

	while (len >= 12)
	{
		// Set 'random' sequence number and timestamp.
		packet->SetSequenceNumber(Utils::Byte::Get2Bytes(data, offset));
		packet->SetTimestamp(Utils::Byte::Get4Bytes(data, offset + 2));

		const RTC::RTP::RtpStreamSend::ReceivePacketResult result = stream->ReceivePacket(packet.get());

		if (result.retransmissionBufferItem)
		{
			result.retransmissionBufferItem->sharedPacket =
			  RTC::RTP::PacketUtils::ClonePacket(packet.get());
		}

		// Feed a 'random' report block, which is what the loss of the stream is
		// computed from.
		// NOTE: Total lost is signed on purpose, since a duplicate makes a remote
		// endpoint report fewer packets lost than it reported before.
		const RTC::RTCP::AnyReportPacket::ReportBlock reportBlock{
			.ssrc                       = params.ssrc,
			.fractionLost               = 0,
			.totalLost                  = static_cast<int16_t>(Utils::Byte::Get2Bytes(data, offset + 10)),
			.extendedHighestSeq         = Utils::Byte::Get4Bytes(data, offset + 6),
			.jitter                     = 0,
			.lastSenderReport           = 0,
			.delaySinceLastSenderReport = 0
		};

		stream->ReceiveRtcpReportBlock(reportBlock, shared.GetTimeUs());

		static_cast<void>(stream->HasRtcpSenderReport(shared.GetTimeUs()));

		len -= 12;
		offset += 12;
	}
}
