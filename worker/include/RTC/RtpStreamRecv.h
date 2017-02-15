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
		uint32_t last_sr_timestamp = 0; // The middle 32 bits out of 64 in the NTP timestamp received in the most recent sender report.
		uint64_t last_sr_received = 0; // Wallclock time representing the most recent sender report arrival.
		uint32_t transit = 0; // Relative trans time for prev pkt.
		uint32_t jitter = 0; // Estimated jitter.
	};
}

#endif
