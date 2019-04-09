#ifndef MS_RTC_SIMULCAST_CONSUMER_HPP
#define MS_RTC_SIMULCAST_CONSUMER_HPP

#include "RTC/Codecs/PayloadDescriptorHandler.hpp"
#include "RTC/Consumer.hpp"
#include "RTC/RtpStreamSend.hpp"
#include "RTC/SeqManager.hpp"

namespace RTC
{
	class SimulcastConsumer : public RTC::Consumer, public RTC::RtpStreamSend::Listener
	{
	public:
		SimulcastConsumer(const std::string& id, RTC::Consumer::Listener* listener, json& data);
		~SimulcastConsumer() override;

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
		void RetransmitRtpPacket(RTC::RtpPacket* packet);
		void EmitScore() const;
		void UpdateLayers();
		void RecalculateTargetSpatialLayer();
		RTC::RtpStream* GetProducerCurrentRtpStream() const;
		RTC::RtpStream* GetProducerTargetRtpStream() const;

		/* Pure virtual methods inherited from RtpStreamSend::Listener. */
	public:
		void OnRtpStreamScore(RTC::RtpStream* rtpStream, uint8_t score) override;
		void OnRtpStreamRetransmitRtpPacket(RTC::RtpStreamSend* rtpStream, RTC::RtpPacket* packet) override;

	private:
		// Allocated by this.
		RTC::RtpStreamSend* rtpStream{ nullptr };
		std::unordered_map<uint32_t, int16_t> mapMappedSsrcSpatialLayer;
		std::vector<RTC::RtpStream*> producerRtpStreams; // Indexed by spatial layer.
		// Others.
		bool keyFrameSupported{ false };
		bool syncRequired{ true };
		RTC::SeqManager<uint16_t> rtpSeqManager;
		RTC::SeqManager<uint32_t> rtpTimestampManager;
		std::unique_ptr<RTC::Codecs::EncodingContext> encodingContext;
		int16_t preferredSpatialLayer{ -1 };
		int16_t targetSpatialLayer{ -1 };
		int16_t currentSpatialLayer{ -1 };
		int16_t preferredTemporalLayer{ -1 };
		int16_t targetTemporalLayer{ -1 };
		int16_t currentTemporalLayer{ -1 };
	};
} // namespace RTC

#endif
