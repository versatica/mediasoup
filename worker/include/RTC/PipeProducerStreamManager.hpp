#ifndef MS_RTC_PIPE_PRODUCER_STREAM_MANAGER_HPP
#define MS_RTC_PIPE_PRODUCER_STREAM_MANAGER_HPP

#include "RTC/ProducerStreamManager.hpp"
#include <ankerl/unordered_dense.h>

namespace RTC
{
	class PipeProducerStreamManager : public ProducerStreamManager
	{
	public:
		PipeProducerStreamManager(
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
			return -1;
		}
		int16_t GetCurrentTemporalLayer() const override
		{
			return -1;
		}
		RTC::RTP::RtpStreamRecv* GetProducerCurrentRtpStream() const override
		{
			return nullptr;
		}
		RTC::RTP::RtpStreamRecv* GetProducerTargetRtpStream() const override
		{
			return nullptr;
		}
		bool IsPacketForCurrentStream(const RTC::RTP::Packet* /*packet*/) const override
		{
			// Pipe has no concept of "current" stream — all packets belong.
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
		// Per-stream sync state, keyed by mapped SSRC.
		ankerl::unordered_dense::map<uint32_t, bool> mapMappedSsrcSyncRequired;
	};
} // namespace RTC

#endif
