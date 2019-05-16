#ifndef MS_RTC_PIPE_CONSUMER_HPP
#define MS_RTC_PIPE_CONSUMER_HPP

#include "RTC/Consumer.hpp"
#include "RTC/RtpStreamSend.hpp"
#include "RTC/SeqManager.hpp"

namespace RTC
{
	class PipeConsumer : public RTC::Consumer, public RTC::RtpStreamSend::Listener
	{
	public:
		PipeConsumer(const std::string& id, RTC::Consumer::Listener* listener, json& data);
		~PipeConsumer() override;

	public:
		void FillJson(json& jsonObject) const override;
		void FillJsonStats(json& jsonArray) const override;
		void FillJsonScore(json& jsonObject) const override;
		void HandleRequest(Channel::Request* request) override;
		void ProducerRtpStream(RTC::RtpStream* rtpStream, uint32_t mappedSsrc) override;
		void ProducerNewRtpStream(RTC::RtpStream* rtpStream, uint32_t mappedSsrc) override;
		void ProducerRtpStreamScore(RTC::RtpStream* rtpStream, uint8_t score, uint8_t previousScore) override;
		void ProducerRtcpSenderReport(RTC::RtpStream* rtpStream, bool first) override;
		void SendRtpPacket(RTC::RtpPacket* packet) override;
		void SendProbationRtpPacket(uint16_t seq) override;
		void GetRtcp(RTC::RTCP::CompoundPacket* packet, RTC::RtpStreamSend* rtpStream, uint64_t now) override;
		std::vector<RTC::RtpStreamSend*> GetRtpStreams() override;
		void NeedWorstRemoteFractionLost(uint32_t mappedSsrc, uint8_t& worstRemoteFractionLost) override;
		void ReceiveNack(RTC::RTCP::FeedbackRtpNackPacket* nackPacket) override;
		void ReceiveKeyFrameRequest(RTC::RTCP::FeedbackPs::MessageType messageType, uint32_t ssrc) override;
		void ReceiveRtcpReceiverReport(RTC::RTCP::ReceiverReport* report) override;
		uint32_t GetTransmissionRate(uint64_t now) override;

	private:
		void UserOnTransportConnected() override;
		void UserOnTransportDisconnected() override;
		void UserOnPaused() override;
		void UserOnResumed() override;
		void CreateRtpStreams();
		void RequestKeyFrame();

		/* Pure virtual methods inherited from RtpStreamSend::Listener. */
	public:
		void OnRtpStreamScore(RTC::RtpStream* rtpStream, uint8_t score, uint8_t previousScore) override;
		void OnRtpStreamRetransmitRtpPacket(
		  RTC::RtpStreamSend* rtpStream, RTC::RtpPacket* packet, bool probation = false) override;

	private:
		// Allocated by this.
		std::unordered_map<uint32_t, RTC::RtpStreamSend*> mapMappedSsrcRtpStream;
		// Others.
		std::vector<RTC::RtpStreamSend*> rtpStreams;
		bool keyFrameSupported{ false };
		std::unordered_map<RTC::RtpStreamSend*, bool> mapRtpStreamSyncRequired;
		std::unordered_map<RTC::RtpStreamSend*, RTC::SeqManager<uint16_t>> mapRtpStreamRtpSeqManager;
	};

	// Inline methods.

	inline std::vector<RTC::RtpStreamSend*> PipeConsumer::GetRtpStreams()
	{
		return this->rtpStreams;
	}
} // namespace RTC

#endif
