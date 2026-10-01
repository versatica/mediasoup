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
		int64_t IncreaseLayer(int64_t bitrate, bool considerLoss, float lossPercentage, int64_t nowMs) override;
		void ApplyLayers(int64_t rtpStreamActiveMs) override;
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
		/**
		 * Whether the given spatial layer may be chosen as the target one.
		 *
		 * @remarks
		 * - The endpoint is sent a single RTP timeline, the one of the RTP timestamp
		 *   reference spatial layer, so forwarding packets of any other spatial layer
		 *   requires knowing the RTP timestamp offset between both, which is what their
		 *   capture instants tell.
		 * - A spatial layer whose offset cannot be told is still chosen when the RTP
		 *   timestamp reference one is not sending media anymore, since it then takes over
		 *   as reference and its own RTP timestamps become the ones sent.
		 *
		 * @param spatialLayer - Spatial layer being considered.
		 */
		bool CanSwitchToSpatialLayer(int16_t spatialLayer) const;
		/**
		 * Whether the capture instant of the given spatial layer is known, which is what
		 * tells the offset between its RTP timestamps and the ones of the RTP timestamp
		 * reference spatial layer.
		 *
		 * @param spatialLayer - Spatial layer being considered.
		 *
		 * @returns False if there is no Producer RtpStream for it yet.
		 */
		bool HasSpatialLayerCaptureMapping(int16_t spatialLayer) const;
		/**
		 * Whether the RTP timestamp reference spatial layer is still sending media, so it
		 * may become the current spatial layer again.
		 *
		 * @remarks
		 * - Same criteria RecalculateTargetLayers() applies to candidate spatial layers,
		 *   so a spatial layer is not chosen as the target one while it is not alive.
		 *
		 * @returns False if there is no RTP timestamp reference spatial layer yet.
		 */
		bool IsTsReferenceSpatialLayerAlive() const;
		/**
		 * Whether the given spatial layer, which is about to become the target one, must
		 * take over as RTP timestamp reference.
		 *
		 * @remarks
		 * - Replacing the reference re-bases the RTP timeline sent to the endpoint onto the
		 *   RTP timestamps of the given spatial layer, which the endpoint sees as a
		 *   discontinuity, so it is only done when the current reference is of no use:
		 *   there is none yet, its capture instant cannot be told or it stopped sending
		 *   media and the given spatial layer cannot be aligned to it.
		 * - A reference that stopped sending media is kept while other spatial layers can
		 *   still be aligned to it, since its capture instant does not expire.
		 *
		 * @param spatialLayer - Spatial layer about to become the target one, so it has
		 * already passed CanSwitchToSpatialLayer(). Never -1.
		 */
		bool ShouldReplaceTsReferenceSpatialLayer(int16_t spatialLayer) const;
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
		// Spatial layer that was the RTP timestamp reference the last time its capture
		// instant was known upon a received Sender Report.
		int16_t tsReferenceSpatialLayerWithCaptureMapping{ -1 };
		uint32_t tsOffset{ 0 };
		bool keyFrameForTsOffsetRequested{ false };
		// Old-packet filtering after spatial switch.
		uint16_t snReferenceSpatialLayer{ 0 };
		bool checkingForOldPacketsInSpatialLayer{ false };
		// BWE downgrade tracking.
		int64_t lastBweDowngradeAtMs{ 0 };
	};
} // namespace RTC

#endif
