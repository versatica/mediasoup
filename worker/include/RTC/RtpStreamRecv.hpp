#ifndef MS_RTC_RTP_STREAM_RECV_HPP
#define MS_RTC_RTP_STREAM_RECV_HPP

#include "RTC/RtpStream.hpp"
#include "RTC/RTCP/ReceiverReport.hpp"
#include "RTC/RTCP/SenderReport.hpp"

namespace RTC
{
	class RtpStreamRecv :
		public RtpStream
	{
	public:
		class Listener
		{
		public:
			virtual void onNackRequired(RTC::RtpStreamRecv* rtpStream, uint16_t seq, uint16_t bitmask) = 0;
		};

	public:
		explicit RtpStreamRecv(Listener* listener, uint32_t ssrc, uint32_t clockRate, bool useNack);
		virtual ~RtpStreamRecv();

		virtual Json::Value toJson() const override;
		virtual bool ReceivePacket(RTC::RtpPacket* packet) override;
		RTC::RTCP::ReceiverReport* GetRtcpReceiverReport();
		void ReceiveRtcpSenderReport(RTC::RTCP::SenderReport* report);

	private:
		void CalculateJitter(uint32_t rtpTimestamp);
		void MayTriggerNack(RTC::RtpPacket* packet);
		void ResetNack();

	/* Pure virtual methods inherited from RtpStream. */
	protected:
		virtual void onInitSeq() override;

	private:
		// Passed by argument.
		Listener* listener = nullptr;
		bool useNack = false;
		uint32_t last_sr_timestamp = 0; // The middle 32 bits out of 64 in the NTP timestamp received in the most recent sender report.
		uint64_t last_sr_received = 0; // Wallclock time representing the most recent sender report arrival.
		uint32_t transit = 0; // Relative trans time for prev pkt.
		uint32_t jitter = 0; // Estimated jitter.
		uint32_t last_seq32 = 0; // Extended seq number of last valid packet.
	};
}

#endif
