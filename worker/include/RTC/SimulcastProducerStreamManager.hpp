#ifndef MS_RTC_SIMULCAST_CONSUMER_STREAM_HPP
#define MS_RTC_SIMULCAST_CONSUMER_STREAM_HPP

#include "common.hpp"
#include "RTC/ProducerStreamManager.hpp"
#include <ankerl/unordered_dense.h>
#include <vector>

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
		std::vector<RTC::RTP::RtpStreamRecv*> GetProducerRtpStreams() const override;
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
#ifdef MS_USE_BUILTIN_BWE
		bool IsBitrateAdaptable() const override
		{
			return true;
		}
		int64_t GetOutputBitrate(int64_t nowMs) override;
		int64_t TakeBitrate(int64_t bitrate, int64_t nowMs) override;
		void ApplyBitrateAllocation(int64_t rtpStreamActiveMs) override;
#else
		int64_t IncreaseLayer(int64_t bitrate, bool considerLoss, float lossPercentage, int64_t nowMs) override;
		void ApplyLayers(int64_t rtpStreamActiveMs) override;
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
		bool CanSwitchToSpatialLayer(int16_t spatialLayer) const;
		RTC::RTP::RtpStreamRecv* GetProducerTsReferenceRtpStream() const;
		/**
		 * Whether the RTP timestamp reference stream can be given up without aligning
		 * other streams to it, which is when it is dead, since the capture instants it
		 * told cannot be relied on anymore.
		 */
		bool CanGiveUpTsReferenceSpatialLayer() const;
		/**
		 * RTP timestamp offset that places the RTP timeline of the given Producer
		 * stream on the one of the RTP timestamp reference stream.
		 *
		 * @remarks
		 * - It is computed upon the capture instants of the given RTP timestamp and of
		 *   the highest RTP timestamp of the RTP timestamp reference stream, both in
		 *   the clock of the remote sender and both taken from the same source,
		 *   'abs-capture-time' if both streams can tell it or Sender Reports otherwise.
		 *
		 * @param producerRtpStream - Producer stream to align.
		 * @param ts - RTP timestamp of the Producer stream to align.
		 *
		 * @returns No value if any of both capture instants cannot be told.
		 */
		std::optional<uint32_t> GetTimestampOffset(
		  const RTC::RTP::RtpStreamRecv* producerRtpStream, uint32_t ts) const;

	private:
		// Producer RTP streams (multiple for Simulcast).
		std::vector<RTC::RTP::RtpStreamRecv*> producerRtpStreams;
		ankerl::unordered_dense::map<uint32_t, int16_t> mapMappedSsrcSpatialLayer;
		RTC::ConsumerTypes::VideoLayers targetLayers;
		int16_t currentSpatialLayer{ -1 };
		int16_t spatialLayerToSync{ -1 };
		// Timestamp synchronization.
		int16_t tsReferenceSpatialLayer{ -1 };
		// Offset of the RTP timeline of the RTP timestamp reference stream from the one
		// of our stream. Not zero once the reference has been given up after packets
		// had been forwarded.
		uint32_t tsReferenceOffset{ 0 };
		// Whether each spatial layer could be switched to as of the last received
		// Sender Report, so a later one after which it can be again is noticed.
		std::vector<bool> switchableSpatialLayers;
		uint32_t tsOffset{ 0 };
		bool keyFrameForTsOffsetRequested{ false };
		// When the last packet was forwarded, or no value if none has been. Once any
		// has been, the RTP timeline of our stream is tied to the one of the RTP
		// timestamp reference stream.
		std::optional<int64_t> lastPacketForwardedAtMs;
		// Old-packet filtering after spatial switch.
		uint16_t snReferenceSpatialLayer{ 0 };
		bool checkingForOldPacketsInSpatialLayer{ false };
		// BWE downgrade tracking.
		int64_t lastBweDowngradeAtMs{ 0 };
	};
} // namespace RTC

#endif
