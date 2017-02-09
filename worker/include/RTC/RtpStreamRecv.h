#ifndef MS_RTC_RTP_STREAM_RECV_H
#define MS_RTC_RTP_STREAM_RECV_H

#include "RTC/RtpStream.h"
#include "RTC/RTCP/ReceiverReport.h"
#include "RTC/RTCP/SenderReport.h"

namespace RTC
{
	class RtpStreamRecv :
		public RtpStream
	{
	public:
		explicit RtpStreamRecv(uint32_t clockRate);
		virtual ~RtpStreamRecv();

		virtual bool ReceivePacket(RTC::RtpPacket* packet);
		RTC::RTCP::ReceiverReport* GetRtcpReceiverReport();
		void ReceiveRtcpSenderReport(RTC::RTCP::SenderReport* report);

	private:
		void CalculateJitter(uint32_t rtpTimestamp);

	private:
		uint32_t transit = 0; // Relative trans time for prev pkt.
		uint32_t jitter = 0; // Estimated jitter.
	};
}

#endif
