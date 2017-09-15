#ifndef MS_RTC_RTP_STREAM_RECV_HPP
#define MS_RTC_RTP_STREAM_RECV_HPP

#include "RTC/NackGenerator.hpp"
#include "RTC/RTCP/ReceiverReport.hpp"
#include "RTC/RTCP/SenderReport.hpp"
#include "RTC/RtpStream.hpp"

namespace RTC
{
	class RtpStreamRecv : public RtpStream, public RTC::NackGenerator::Listener
	{
	public:
		class Listener : public RtpStream::Listener
		{
		public:
			virtual void OnRtpStreamRecvNackRequired(
			  RTC::RtpStreamRecv* rtpStream, const std::vector<uint16_t>& seqNumbers) = 0;
			virtual void OnRtpStreamRecvPliRequired(RTC::RtpStreamRecv* rtpStream)    = 0;
		};

	public:
		RtpStreamRecv(Listener* listener, RTC::RtpStream::Params& params);
		~RtpStreamRecv() override;

		bool ReceivePacket(RTC::RtpPacket* packet) override;
		bool ReceiveRtxPacket(RTC::RtpPacket* packet);
		RTC::RTCP::ReceiverReport* GetRtcpReceiverReport();
		void ReceiveRtcpSenderReport(RTC::RTCP::SenderReport* report);
		void SetRtx(uint8_t payloadType, uint32_t ssrc);
		void RequestKeyFrame();

	private:
		void CalculateJitter(uint32_t rtpTimestamp);

		/* Pure virtual methods inherited from RtpStream. */
	protected:
		virtual void CheckHealth() override;

		/* Pure virtual methods inherited from RTC::NackGenerator. */
	protected:
		void OnNackGeneratorNackRequired(const std::vector<uint16_t>& seqNumbers) override;
		void OnNackGeneratorKeyFrameRequired() override;

	private:
		// Passed by argument.
		Listener* listener{ nullptr };
		// Others.
		uint32_t expectedPrior{ 0 }; // Packet expected at last interval.
		uint32_t receivedPrior{ 0 }; // Packet received at last interval.
		uint32_t lastSrTimestamp{ 0 }; // The middle 32 bits out of 64 in the NTP timestamp received in
		                               // the most recent sender report.
		uint64_t lastSrReceived{ 0 };  // Wallclock time representing the most recent sender report
		                               // arrival.
		uint32_t transit{ 0 };         // Relative trans time for prev pkt.
		std::unique_ptr<RTC::NackGenerator> nackGenerator;
		// RTX related.
		bool hasRtx{ false };
		uint8_t rtxPayloadType{ 0 };
		uint32_t rtxSsrc{ 0 };
	};
} // namespace RTC

#endif
