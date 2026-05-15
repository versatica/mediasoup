#define MS_CLASS "RTC::PipeProducerStreamManager"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/PipeProducerStreamManager.hpp"
#include "Logger.hpp"

namespace RTC
{
	/* Instance methods. */

	PipeProducerStreamManager::PipeProducerStreamManager(
	  const std::vector<RTC::RtpEncodingParameters>& consumableRtpEncodings,
	  const RTC::ConsumerTypes::VideoLayers& preferredLayers,
	  std::unique_ptr<RTC::RTP::Codecs::EncodingContext> encodingContext,
	  RTC::Media::Kind kind,
	  bool keyFrameSupported,
	  Listener* listener,
	  SharedInterface* shared)
	  : ProducerStreamManager(
	      consumableRtpEncodings,
	      preferredLayers,
	      std::move(encodingContext),
	      kind,
	      keyFrameSupported,
	      listener,
	      shared)
	{
		MS_TRACE();

		// Initialize per-stream sync state for each consumable encoding.
		for (const auto& encoding : this->consumableRtpEncodings)
		{
			this->mapMappedSsrcSyncRequired[encoding.ssrc] = false;
		}
	}

	bool PipeProducerStreamManager::IsActive() const
	{
		MS_TRACE();

		return this->listener->IsActive();
	}

	void PipeProducerStreamManager::ProducerRtpStream(
	  RTC::RTP::RtpStreamRecv* /*rtpStream*/, uint32_t /*mappedSsrc*/)
	{
		MS_TRACE();

		// Do nothing.
	}

	void PipeProducerStreamManager::ProducerNewRtpStream(
	  RTC::RTP::RtpStreamRecv* /*rtpStream*/, uint32_t /*mappedSsrc*/)
	{
		MS_TRACE();

		// Do nothing.
	}

	void PipeProducerStreamManager::ProducerRtpStreamScore(
	  RTC::RTP::RtpStreamRecv* /*rtpStream*/, uint8_t /*score*/, uint8_t /*previousScore*/)
	{
		MS_TRACE();

		// Do nothing.
	}

	void PipeProducerStreamManager::ProducerRtcpSenderReport(
	  RTC::RTP::RtpStreamRecv* /*rtpStream*/, bool /*first*/)
	{
		MS_TRACE();

		// Do nothing.
	}

	uint32_t PipeProducerStreamManager::IncreaseLayer(
	  uint32_t /*bitrate*/, bool /*considerLoss*/, float /*lossPercentage*/, uint64_t /*nowMs*/)
	{
		MS_TRACE();

		// Pipe does not play the BWE game.
		return 0u;
	}

	void PipeProducerStreamManager::ApplyLayers(uint64_t /*rtpStreamActiveMs*/)
	{
		MS_TRACE();

		// Pipe does not play the BWE game.
	}

	uint32_t PipeProducerStreamManager::GetDesiredBitrate(uint64_t /*nowMs*/) const
	{
		MS_TRACE();

		// Pipe does not play the BWE game.
		return 0u;
	}

	ProducerStreamManager::RtpPacketProcessResult PipeProducerStreamManager::ProcessRtpPacket(
	  RTC::RTP::Packet* packet,
	  bool /*lastSentPacketHasMarker*/,
	  uint32_t /*clockRate*/,
	  uint32_t /*maxPacketTs*/)
	{
		MS_TRACE();

		RtpPacketProcessResult result;

		auto it = this->mapMappedSsrcSyncRequired.find(packet->GetSsrc());

		MS_ASSERT(it != this->mapMappedSsrcSyncRequired.end(), "unknown mapped SSRC");

		bool& syncRequired = it->second;

		// If sync required and key frames supported, buffer non-key-frame packets.
		if (syncRequired && this->keyFrameSupported && !packet->IsKeyFrame())
		{
			result.type = RtpPacketProcessResult::Type::BUFFER;

#ifdef MS_RTC_LOGGER_RTP
			packet->logger.Discarded(RTC::RtcLogger::RtpPacket::DiscardReason::NOT_A_KEYFRAME);
#endif

			return result;
		}

		// Packets with only padding are not forwarded.
		if (packet->GetPayloadLength() == 0)
		{
			result.type = RtpPacketProcessResult::Type::DROP;

#ifdef MS_RTC_LOGGER_RTP
			packet->logger.Discarded(RTC::RtcLogger::RtpPacket::DiscardReason::EMPTY_PAYLOAD);
#endif

			return result;
		}

		// Whether this is the first packet after re-sync.
		const bool isSyncPacket = syncRequired;

		if (isSyncPacket)
		{
			if (packet->IsKeyFrame())
			{
				MS_DEBUG_TAG(
				  rtp,
				  "sync key frame received [ssrc:%" PRIu32 ", seq:%" PRIu16 ", ts:%" PRIu32 "]",
				  packet->GetSsrc(),
				  packet->GetSequenceNumber(),
				  packet->GetTimestamp());

				result.sendBufferedPackets = true;
			}

			result.isSyncPacket = true;
			result.syncSeqValue = packet->GetSequenceNumber() - 1;

			syncRequired = false;
		}

		// Pipe passes timestamp and marker through unchanged.
		result.type     = RtpPacketProcessResult::Type::FORWARD;
		result.tsOffset = 0u;
		result.marker   = packet->HasMarker();

		return result;
	}

	void PipeProducerStreamManager::RequestKeyFrame()
	{
		MS_TRACE();

		if (this->kind != RTC::Media::Kind::VIDEO)
		{
			return;
		}

		for (const auto& encoding : this->consumableRtpEncodings)
		{
			this->listener->OnProducerStreamManagerKeyFrameRequested(encoding.ssrc);
		}
	}

	void PipeProducerStreamManager::RequestKeyFrameForTargetSpatialLayer()
	{
		MS_TRACE();

		RequestKeyFrame();
	}

	void PipeProducerStreamManager::RequestKeyFrameForCurrentSpatialLayer()
	{
		MS_TRACE();

		RequestKeyFrame();
	}

	void PipeProducerStreamManager::UpdateTargetLayers(int16_t /*spatial*/, int16_t /*temporal*/)
	{
		MS_TRACE();

		// No layer management for pipe.
	}

	bool PipeProducerStreamManager::RecalculateTargetLayers(
	  RTC::ConsumerTypes::VideoLayers& /*newTargetLayers*/) const
	{
		MS_TRACE();

		// No layer changes for pipe.
		return false;
	}

	void PipeProducerStreamManager::OnTransportConnected()
	{
		MS_TRACE();

		for (auto& kv : this->mapMappedSsrcSyncRequired)
		{
			kv.second = true;
		}

		if (IsActive())
		{
			RequestKeyFrame();
		}
	}

	void PipeProducerStreamManager::OnTransportDisconnected()
	{
		MS_TRACE();

		for (auto& kv : this->mapMappedSsrcSyncRequired)
		{
			kv.second = false;
		}
	}

	void PipeProducerStreamManager::OnPaused()
	{
		MS_TRACE();

		for (auto& kv : this->mapMappedSsrcSyncRequired)
		{
			kv.second = false;
		}
	}

	void PipeProducerStreamManager::OnResumed()
	{
		MS_TRACE();

		for (auto& kv : this->mapMappedSsrcSyncRequired)
		{
			kv.second = true;
		}

		if (IsActive())
		{
			RequestKeyFrame();
		}
	}

} // namespace RTC
