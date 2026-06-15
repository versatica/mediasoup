#ifndef MS_RTC_SIMPLE_CONSUMER_STREAM_HPP
#define MS_RTC_SIMPLE_CONSUMER_STREAM_HPP

#include "RTC/ProducerStreamManager.hpp"

namespace RTC
{
	class SimpleProducerStreamManager : public ProducerStreamManager
	{
	public:
		SimpleProducerStreamManager(
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
			return {};
		}
		int16_t GetCurrentSpatialLayer() const override
		{
			return 0;
		}
		int16_t GetCurrentTemporalLayer() const override
		{
			return 0;
		}
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
		void UpdateTargetLayers(int16_t spatial, int16_t temporal) override;
		bool RecalculateTargetLayers(RTC::ConsumerTypes::VideoLayers& newTargetLayers) const override;
		void OnTransportConnected() override;
		void OnTransportDisconnected() override;
		void OnPaused() override;
		void OnResumed() override;

	private:
		// Producer RTP stream (single stream for Simple).
		RTC::RTP::RtpStreamRecv* producerRtpStream{ nullptr };
		// Prevents double IncreaseLayer in one BWE round.
		bool managingBitrate{ false };
	};
} // namespace RTC

#endif
