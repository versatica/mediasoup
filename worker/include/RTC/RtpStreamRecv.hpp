#ifndef MS_RTC_RTP_STREAM_RECV_HPP
#define MS_RTC_RTP_STREAM_RECV_HPP

#include "RTC/RtpStream.hpp"
#include "RTC/RTCP/ReceiverReport.hpp"
#include "RTC/RTCP/SenderReport.hpp"
#include "RTC/NackGenerator.hpp"

namespace RTC
{
	class RtpStreamRecv :
		public RtpStream,
		public RTC::NackGenerator::Listener
	{
	public:
		class Listener
		{
		public:
			virtual void onNackRequired(RTC::RtpStreamRecv* rtpStream, const std::vector<uint16_t>& seqNumbers) = 0;
			virtual void onPliRequired(RTC::RtpStreamRecv* rtpStream) = 0;
		};

	public:
		RtpStreamRecv(Listener* listener, RTC::RtpStream::Params& params);
		virtual ~RtpStreamRecv();

		virtual Json::Value toJson() const override;
		virtual bool ReceivePacket(RTC::RtpPacket* packet) override;
		RTC::RTCP::ReceiverReport* GetRtcpReceiverReport();
		void ReceiveRtcpSenderReport(RTC::RTCP::SenderReport* report);
		void RequestFullFrame();

	private:
		void CalculateJitter(uint32_t rtpTimestamp);

	/* Pure virtual methods inherited from RtpStream. */
	protected:
		virtual void onInitSeq() override;

	/* Pure virtual methods inherited from RTC::NackGenerator. */
	protected:
		virtual void onNackRequired(const std::vector<uint16_t>& seqNumbers) override;
		virtual void onFullFrameRequired() override;

	private:
		// Passed by argument.
		Listener* listener = nullptr;
		// Others.
		uint32_t lastSrTimestamp = 0; // The middle 32 bits out of 64 in the NTP timestamp received in the most recent sender report.
		uint64_t lastSrReceived = 0; // Wallclock time representing the most recent sender report arrival.
		uint32_t transit = 0; // Relative trans time for prev pkt.
		uint32_t jitter = 0; // Estimated jitter.
		std::unique_ptr<RTC::NackGenerator> nackGenerator;
	};
}

#endif
