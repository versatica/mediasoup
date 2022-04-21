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
		PipeConsumer(
		  const std::string& id,
		  const std::string& producerId,
		  RTC::Consumer::Listener* listener,
		  json& data);
		~PipeConsumer() override;

	public:
		void FillJson(json& jsonObject) const override;
		void FillJsonStats(json& jsonArray) const override;
		void FillJsonScore(json& jsonObject) const override;
		void HandleRequest(Channel::ChannelRequest* request) override;
		void ProducerRtpStream(RTC::RtpStream* rtpStream, uint32_t mappedSsrc) override;
		void ProducerNewRtpStream(RTC::RtpStream* rtpStream, uint32_t mappedSsrc) override;
		void ProducerRtpStreamScore(RTC::RtpStream* rtpStream, uint8_t score, uint8_t previousScore) override;
		void ProducerRtcpSenderReport(RTC::RtpStream* rtpStream, bool first) override;
		uint8_t GetBitratePriority() const override;
		uint32_t IncreaseLayer(uint32_t bitrate, bool considerLoss) override;
		void ApplyLayers() override;
		uint32_t GetDesiredBitrate() const override;
		void SendRtpPacket(RTC::RtpPacket* packet) override;
		void GetRtcp(RTC::RTCP::CompoundPacket* packet, RTC::RtpStreamSend* rtpStream, uint64_t nowMs) override;
		std::vector<RTC::RtpStreamSend*> GetRtpStreams() override
		{
			return this->rtpStreams;
		}
		void NeedWorstRemoteFractionLost(uint32_t mappedSsrc, uint8_t& worstRemoteFractionLost) override;
		void ReceiveNack(RTC::RTCP::FeedbackRtpNackPacket* nackPacket) override;
		void ReceiveKeyFrameRequest(RTC::RTCP::FeedbackPs::MessageType messageType, uint32_t ssrc) override;
		void ReceiveRtcpReceiverReport(RTC::RTCP::ReceiverReport* report) override;
		uint32_t GetTransmissionRate(uint64_t nowMs) override;
		float GetRtt() const override;

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
		void OnRtpStreamRetransmitRtpPacket(RTC::RtpStreamSend* rtpStream, RTC::RtpPacket* packet) override;

	private:
		// Allocated by this.
		std::vector<RTC::RtpStreamSend*> rtpStreams;
		// Others.
		absl::flat_hash_map<uint32_t, uint32_t> mapMappedSsrcSsrc;
		absl::flat_hash_map<uint32_t, RTC::RtpStreamSend*> mapSsrcRtpStream;
		bool keyFrameSupported{ false };
		absl::flat_hash_map<RTC::RtpStreamSend*, bool> mapRtpStreamSyncRequired;
		absl::flat_hash_map<RTC::RtpStreamSend*, RTC::SeqManager<uint16_t>> mapRtpStreamRtpSeqManager;
	};
} // namespace RTC

#endif
