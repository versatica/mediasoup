#ifndef MS_RTC_SIMPLE_CONSUMER_HPP
#define MS_RTC_SIMPLE_CONSUMER_HPP

#include "RTC/Consumer.hpp"
#include "RTC/RtpStreamSend.hpp"
#include "RTC/SeqManager.hpp"

namespace RTC
{
	class SimpleConsumer : public RTC::Consumer, public RTC::RtpStreamSend::Listener
	{
	public:
		SimpleConsumer(const std::string& id, RTC::Consumer::Listener* listener, json& data);
		~SimpleConsumer() override;

	public:
		void FillJson(json& jsonObject) const override;
		void FillJsonStats(json& jsonArray) const override;
		void FillJsonScore(json& jsonObject) const override;
		void HandleRequest(Channel::Request* request) override;
		uint32_t UseBandwidth(uint32_t availableBandwidth) override;
		void ProducerRtpStream(RTC::RtpStreamRecv* rtpStream, uint32_t mappedSsrc) override;
		void ProducerNewRtpStream(RTC::RtpStreamRecv* rtpStream, uint32_t mappedSsrc) override;
		void ProducerRtpStreamScore(RTC::RtpStreamRecv* rtpStream, uint8_t score) override;
		void SendRtpPacket(RTC::RtpPacket* packet) override;
		void GetRtcp(RTC::RTCP::CompoundPacket* packet, uint64_t now) override;
		void NeedWorstRemoteFractionLost(uint32_t mappedSsrc, uint8_t& worstRemoteFractionLost) override;
		void ReceiveNack(RTC::RTCP::FeedbackRtpNackPacket* nackPacket) override;
		void ReceiveKeyFrameRequest(RTC::RTCP::FeedbackPs::MessageType messageType) override;
		void ReceiveRtcpReceiverReport(RTC::RTCP::ReceiverReport* report) override;
		uint32_t GetTransmissionRate(uint64_t now) override;
		float GetLossPercentage() const override;

	private:
		void Paused() override;
		void Resumed() override;
		void CreateRtpStream();
		void RequestKeyFrame();
		void EmitScore() const;

		/* Pure virtual methods inherited from RtpStreamSend::Listener. */
	public:
		void OnRtpStreamScore(RTC::RtpStream* rtpStream, uint8_t score) override;
		void OnRtpStreamRetransmitRtpPacket(RTC::RtpStreamSend* rtpStream, RTC::RtpPacket* packet) override;

	private:
		// Allocated by this.
		RTC::RtpStreamSend* rtpStream{ nullptr };
		// Others.
		bool keyFrameSupported{ false };
		bool syncRequired{ true };
		RTC::SeqManager<uint16_t> rtpSeqManager;
		RTC::SeqManager<uint32_t> rtpTimestampManager;
		RTC::RtpStream* producerRtpStream{ nullptr };
	};
} // namespace RTC

#endif
