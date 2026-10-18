#ifndef MS_RTC_SVC_CONSUMER_STREAM_HPP
#define MS_RTC_SVC_CONSUMER_STREAM_HPP

#include "common.hpp"
#include "RTC/ProducerStreamManager.hpp"
#include <vector>

namespace RTC
{
	class SvcProducerStreamManager : public ProducerStreamManager
	{
	public:
		SvcProducerStreamManager(
		  const std::vector<RTC::RtpEncodingParameters>& consumableRtpEncodings,
		  const RTC::ConsumerTypes::VideoLayers& preferredLayers,
		  std::unique_ptr<RTC::RTP::Codecs::EncodingContext> encodingContext,
		  RTC::Media::Kind kind,
		  bool keyFrameSupported,
		  Listener* listener,
		  SharedInterface* shared);

	public:
		RTC::ConsumerTypes::VideoLayers GetTargetLayers() const override
		{
			return this->encodingContext->GetTargetLayers();
		}
		int16_t GetCurrentSpatialLayer() const override
		{
			return this->encodingContext->GetCurrentSpatialLayer();
		}
		int16_t GetCurrentTemporalLayer() const override
		{
			return this->encodingContext->GetCurrentTemporalLayer();
		}
		std::vector<RTC::RTP::RtpStreamRecv*> GetProducerRtpStreams() const override;
		RTC::RTP::RtpStreamRecv* GetProducerCurrentRtpStream() const override;
		RTC::RTP::RtpStreamRecv* GetProducerTargetRtpStream() const override;
		bool IsPacketForCurrentStream(const RTC::RTP::Packet* /*packet*/) const override
		{
			return true;
		}
		bool IsActive() const override;
		void ProducerRtpStream(RTC::RTP::RtpStreamRecv* rtpStream, uint32_t mappedSsrc) override;
		void ProducerNewRtpStream(RTC::RTP::RtpStreamRecv* rtpStream, uint32_t mappedSsrc) override;
		void ProducerRtpStreamScore(
		  RTC::RTP::RtpStreamRecv* rtpStream, uint8_t score, uint8_t previousScore) override;
		void ProducerRtcpSenderReport(RTC::RTP::RtpStreamRecv* rtpStream, bool first) override;
		void ProducerSpatialLayerActivityChanged(
		  RTC::RTP::RtpStreamRecv* rtpStream, uint8_t spatialLayer, bool isActive) override;
#ifdef MS_USE_BUILTIN_BWE
		RTC::BWE::BitrateAllocator::Candidate::BitrateOptions GetBitrateOptions(int64_t nowMs) const override;
		void SetBitrateOption(const RTC::BWE::BitrateAllocator::Candidate::BitrateOption& option) override;
#else
		int64_t IncreaseLayer(int64_t bitrate, bool considerLoss, float lossPercentage, int64_t nowMs) override;
		void ApplyLayers() override;
#endif
		int64_t GetDesiredBitrate(int64_t nowMs) const override;
		RtpPacketProcessResult ProcessRtpPacket(
		  RTC::RTP::Packet* packet,
		  bool lastSentPacketHasMarker,
		  uint32_t clockRate,
		  uint32_t maxPacketTs) override;
		void RequestKeyFrame() override;
		void RequestKeyFrameForTargetSpatialLayer() override;
		void RequestKeyFrameForCurrentSpatialLayer() override;
		void UpdateTargetLayers(int16_t newTargetSpatialLayer, int16_t newTargetTemporalLayer) override;
		bool RecalculateTargetLayers(RTC::ConsumerTypes::VideoLayers& newTargetLayers) const override;
		void OnTransportConnected() override;
		void OnTransportDisconnected() override;
		void OnPaused() override;
		void OnResumed() override;

	private:
		// Producer RTP stream (single stream for SVC).
		RTC::RTP::RtpStreamRecv* producerRtpStream{ nullptr };
	};
} // namespace RTC

#endif
