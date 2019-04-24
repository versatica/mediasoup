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
		bool IsActive() const override;
		void ProducerRtpStream(RTC::RtpStream* rtpStream, uint32_t mappedSsrc) override;
		void ProducerNewRtpStream(RTC::RtpStream* rtpStream, uint32_t mappedSsrc) override;
		void ProducerRtpStreamScore(RTC::RtpStream* rtpStream, uint8_t score, uint8_t previousScore) override;
		void SetExternallyManagedBitrate() override;
		int16_t GetBitratePriority() const override;
		uint32_t UseAvailableBitrate(uint32_t bitrate) override;
		uint32_t IncreaseLayer(uint32_t bitrate) override;
		void ApplyLayers() override;
		void SendRtpPacket(RTC::RtpPacket* packet) override;
		void GetRtcp(RTC::RTCP::CompoundPacket* packet, uint64_t now) override;
		void NeedWorstRemoteFractionLost(uint32_t mappedSsrc, uint8_t& worstRemoteFractionLost) override;
		void ReceiveNack(RTC::RTCP::FeedbackRtpNackPacket* nackPacket) override;
		void ReceiveKeyFrameRequest(RTC::RTCP::FeedbackPs::MessageType messageType) override;
		void ReceiveRtcpReceiverReport(RTC::RTCP::ReceiverReport* report) override;
		uint32_t GetTransmissionRate(uint64_t now) override;

	private:
		void UserOnTransportConnected() override;
		void UserOnTransportDisconnected() override;
		void UserOnPaused() override;
		void UserOnResumed() override;
		void CreateRtpStream();
		void RequestKeyFrame();
		void MayChangeLayers(bool force = false);
		bool RecalculateTargetLayers(int16_t& newTargetSpatialLayer, int16_t& newTargetTemporalLayer) const;
		void UpdateTargetLayers(int16_t newTargetSpatialLayer, int16_t newTargetTemporalLayer);
		void UpdateCurrentLayers();
		void EmitScore() const;
		void EmitLayersChange() const;
		RTC::RtpStream* GetProducerCurrentRtpStream() const;
		RTC::RtpStream* GetProducerTargetRtpStream() const;

		/* Pure virtual methods inherited from RtpStreamSend::Listener. */
	public:
		void OnRtpStreamScore(RTC::RtpStream* rtpStream, uint8_t score, uint8_t previousScore) override;
		void OnRtpStreamRetransmitRtpPacket(RTC::RtpStreamSend* rtpStream, RTC::RtpPacket* packet) override;

	private:
		// Allocated by this.
		RTC::RtpStreamSend* rtpStream{ nullptr };
		std::unordered_map<uint32_t, int16_t> mapMappedSsrcSpatialLayer;
		std::vector<RTC::RtpStream*> producerRtpStreams; // Indexed by spatial layer.
		// Others.
		bool keyFrameSupported{ false };
		bool syncRequired{ false };
		RTC::SeqManager<uint16_t> rtpSeqManager;
		RTC::SeqManager<uint32_t> rtpTimestampManager;
		int16_t preferredSpatialLayer{ -1 };
		int16_t preferredTemporalLayer{ -1 };
		int16_t provisionalTargetSpatialLayer{ -1 };
		int16_t provisionalTargetTemporalLayer{ -1 };
		int16_t targetSpatialLayer{ -1 };
		int16_t targetTemporalLayer{ -1 };
		int16_t currentSpatialLayer{ -1 };
		int16_t currentTemporalLayer{ -1 };
		std::unique_ptr<RTC::Codecs::EncodingContext> encodingContext;
		bool externallyManagedBitrate{ false };
	};

	/* Inline methods. */

	inline bool SimulcastConsumer::IsActive() const
	{
		// clang-format off
		return (
			RTC::Consumer::IsActive() &&
			std::any_of(
				this->producerRtpStreams.begin(),
				this->producerRtpStreams.end(),
				[](const RTC::RtpStream* rtpStream)
				{
					return (rtpStream != nullptr && rtpStream->GetScore() > 0);
				}
			)
		);
		// clang-format on
	}
} // namespace RTC

#endif
