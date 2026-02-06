#ifndef MS_FUZZER_RTC_RTP_STREAM_SEND_HPP
#define MS_FUZZER_RTC_RTP_STREAM_SEND_HPP

#include "common.hpp"
#include "RTC/RTP/Packet.hpp"
#include "RTC/RTP/RtpStreamSend.hpp"

namespace FuzzerRtcRtpStreamSend
{
	class TestRtpStreamListener : public RTC::RTP::RtpStreamSend::Listener
	{
	public:
		void OnRtpStreamScore(
		  RTC::RTP::RtpStream* /*rtpStream*/, uint8_t /*score*/, uint8_t /*previousScore*/) override
		{
		}

		void OnRtpStreamRetransmitRtpPacket(
		  RTC::RTP::RtpStreamSend* /*rtpStream*/, RTC::RTP::Packet* packet) override
		{
		}
	};

	void Fuzz(const uint8_t* data, size_t len);
} // namespace FuzzerRtcRtpStreamSend

#endif
