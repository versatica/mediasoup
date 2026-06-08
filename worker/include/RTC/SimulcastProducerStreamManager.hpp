#ifndef MS_RTC_SIMULCAST_CONSUMER_STREAM_HPP
#define MS_RTC_SIMULCAST_CONSUMER_STREAM_HPP

#include "RTC/ProducerStreamManager.hpp"
#include <ankerl/unordered_dense.h>

namespace RTC
{
	class SimulcastProducerStreamManager : public ProducerStreamManager
	{
	public:
		SimulcastProducerStreamManager(
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
			return this->targetLayers;
		}
		int16_t GetCurrentSpatialLayer() const override
		{
			return this->currentSpatialLayer;
		}
		int16_t GetCurrentTemporalLayer() const override
		{
			return this->encodingContext->GetCurrentTemporalLayer();
		}
		RTC::RTP::RtpStreamRecv* GetProducerCurrentRtpStream() const override;
		RTC::RTP::RtpStreamRecv* GetProducerTargetRtpStream() const override;
		bool IsPacketForCurrentStream(const RTC::RTP::Packet* packet) const override
		{
			const auto it = this->mapMappedSsrcSpatialLayer.find(packet->GetSsrc());
			if (it == this->mapMappedSsrcSpatialLayer.end())
			{
				return false;
			}
			return it->second == this->currentSpatialLayer;
		}
		bool IsActive() const override;
		void ProducerRtpStream(RTC::RTP::RtpStreamRecv* rtpStream, uint32_t mappedSsrc) override;
		void ProducerNewRtpStream(RTC::RTP::RtpStreamRecv* rtpStream, uint32_t mappedSsrc) override;
		void ProducerRtpStreamScore(
		  RTC::RTP::RtpStreamRecv* rtpStream, uint8_t score, uint8_t previousScore) override;
		void ProducerRtcpSenderReport(RTC::RTP::RtpStreamRecv* rtpStream, bool first) override;
		uint32_t IncreaseLayer(
		  uint32_t bitrate, bool considerLoss, float lossPercentage, uint64_t nowMs) override;
		void ApplyLayers(uint64_t rtpStreamActiveMs) override;
		uint32_t GetDesiredBitrate(uint64_t nowMs) const override;
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
		bool CanSwitchToSpatialLayer(int16_t spatialLayer) const;
		RTC::RTP::RtpStreamRecv* GetProducerTsReferenceRtpStream() const;

	private:
		// Producer RTP streams (multiple for Simulcast).
		std::vector<RTC::RTP::RtpStreamRecv*> producerRtpStreams;
		ankerl::unordered_dense::map<uint32_t, int16_t> mapMappedSsrcSpatialLayer;
		RTC::ConsumerTypes::VideoLayers targetLayers;
		int16_t currentSpatialLayer{ -1 };
		int16_t spatialLayerToSync{ -1 };
		// Timestamp synchronization.
		int16_t tsReferenceSpatialLayer{ -1 };
		uint32_t tsOffset{ 0u };
		bool keyFrameForTsOffsetRequested{ false };
		// Old-packet filtering after spatial switch.
		uint16_t snReferenceSpatialLayer{ 0u };
		bool checkingForOldPacketsInSpatialLayer{ false };
		// BWE downgrade tracking.
		uint64_t lastBweDowngradeAtMs{ 0u };
	};
} // namespace RTC

#endif
