#ifndef MS_RTC_RTP_STREAM_RECV_HPP
#define MS_RTC_RTP_STREAM_RECV_HPP

#include "RTC/NackGenerator.hpp"
#include "RTC/RtpStream.hpp"
#include "handles/Timer.hpp"
#include <vector>

namespace RTC
{
	class RtpStreamRecv : public RTC::RtpStream,
	                      public RTC::NackGenerator::Listener,
	                      public Timer::Listener
	{
	public:
		class Listener : public RTC::RtpStream::Listener
		{
		public:
			virtual void OnRtpStreamSendRtcpPacket(
			  RTC::RtpStreamRecv* rtpStream, RTC::RTCP::Packet* packet) = 0;
			virtual void OnRtpStreamNeedWorstRemoteFractionLost(
			  RTC::RtpStreamRecv* rtpStream, uint8_t& worstRemoteFractionLost) = 0;
		};

	public:
		RtpStreamRecv(RTC::RtpStreamRecv::Listener* listener, RTC::RtpStream::Params& params);
		~RtpStreamRecv();

		void FillJsonStats(json& jsonObject) override;
		bool ReceivePacket(RTC::RtpPacket* packet) override;
		bool ReceiveRtxPacket(RTC::RtpPacket* packet);
		RTC::RTCP::ReceiverReport* GetRtcpReceiverReport();
		void ReceiveRtcpSenderReport(RTC::RTCP::SenderReport* report);
		void RequestKeyFrame();
		void Pause() override;
		void Resume() override;

	private:
		void CalculateJitter(uint32_t rtpTimestamp);
		void UpdateScore();

		/* Pure virtual methods inherited from Timer. */
	protected:
		void OnTimer(Timer* timer) override;

		/* Pure virtual methods inherited from RTC::NackGenerator. */
	protected:
		void OnNackGeneratorNackRequired(const std::vector<uint16_t>& seqNumbers) override;
		void OnNackGeneratorKeyFrameRequired() override;

	private:
		uint32_t receivedPrior{ 0 };   // Packets received at last interval.
		uint32_t lastSrTimestamp{ 0 }; // The middle 32 bits out of 64 in the NTP
		                               // timestamp received in the most recent
		                               // sender report.
		uint64_t lastSrReceived{ 0 };  // Wallclock time representing the most recent
		                               // sender report arrival.
		uint32_t transit{ 0 };         // Relative transit time for prev packet.
		uint32_t jitter{ 0 };
		uint8_t firSeqNumber{ 0 };
		uint32_t reportedPacketLost{ 0 };
		bool inactive{ false };        // Stream is inactive.
		std::unique_ptr<RTC::NackGenerator> nackGenerator;
		Timer* inactivityCheckPeriodicTimer{ nullptr };
	};
} // namespace RTC

#endif
