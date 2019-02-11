#ifndef MS_RTC_SIMULCAST_CONSUMER_HPP
#define MS_RTC_SIMULCAST_CONSUMER_HPP

#include "RTC/Codecs/PayloadDescriptorHandler.hpp"
#include "RTC/Consumer.hpp"
#include "RTC/SeqManager.hpp"

namespace RTC
{
	class SimulcastConsumer : public Consumer
	{
	public:
		SimulcastConsumer(const std::string& id, Listener* listener, json& data);
		~SimulcastConsumer() override;

	public:
		void FillJson(json& jsonObject) const override;
		void FillJsonStats(json& jsonArray) const override;
		void HandleRequest(Channel::Request* request) override;
		void TransportConnected() override;
		void ProducerNewRtpStream(RTC::RtpStream* rtpStream, uint32_t mappedSsrc) override;
		void ProducerRtpStreamScore(RTC::RtpStream* rtpStream, uint8_t score) override;
		void SendRtpPacket(RTC::RtpPacket* packet) override;
		void GetRtcp(RTC::RTCP::CompoundPacket* packet, uint64_t now) override;
		void ReceiveNack(RTC::RTCP::FeedbackRtpNackPacket* nackPacket) override;
		void ReceiveKeyFrameRequest(RTC::RTCP::FeedbackPs::MessageType messageType) override;
		void ReceiveRtcpReceiverReport(RTC::RTCP::ReceiverReport* report) override;
		uint32_t GetTransmissionRate(uint64_t now) override;
		float GetLossPercentage() const override;
		json GetScore() const override;

	protected:
		void Paused(bool wasProducer = false) override;
		void Resumed(bool wasProducer = false) override;

	private:
		void CreateRtpStream();
		void RequestKeyFrame();
		void RetransmitRtpPacket(RTC::RtpPacket* packet);
		void EmitScore() const;
		void RecalculateTargetSpatialLayer(bool force = false);
		bool IsProbing() const;
		void StartProbation(int16_t spatialLayer);
		void StopProbation();
		void SendProbationPacket(RTC::RtpPacket* packet);

		/* Pure virtual methods inherited from RtpStream::Listener. */
	public:
		void OnRtpStreamSendRtcpPacket(RTC::RtpStream* rtpStream, RTC::RTCP::Packet* packet) override;
		void OnRtpStreamRetransmitRtpPacket(RTC::RtpStream* rtpStream, RTC::RtpPacket* packet) override;
		void OnRtpStreamScore(RTC::RtpStream* rtpStream, uint8_t score) override;

	private:
		// Allocated by this.
		RTC::RtpStreamSend* rtpStream{ nullptr };
		std::unordered_map<uint32_t, int16_t> mapMappedSsrcSpatialLayer;
		std::vector<RTC::RtpStream*> producerRtpStreams;
		// Others.
		bool keyFrameSupported{ false };
		bool syncRequired{ true };
		uint64_t lastRtcpSentTime{ 0 };
		uint16_t maxRtcpInterval{ 0 };
		RTC::SeqManager<uint16_t> rtpSeqManager;
		RTC::SeqManager<uint32_t> rtpTimestampManager;
		std::unique_ptr<RTC::Codecs::EncodingContext> encodingContext;
		RTC::RtpStream* producerRtpStream{ nullptr }; // TODO: REMOVE
		int16_t preferredSpatialLayer{ -1 };
		int16_t targetSpatialLayer{ -1 };
		int16_t currentSpatialLayer{ -1 };
		int16_t probationSpatialLayer{ -1 };
		uint16_t packetsBeforeProbation{ 0 };
		uint16_t probationPackets{ 0 };
	};
} // namespace RTC

#endif
