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

	void PipeProducerStreamManager::ProducerRtpStream(RTC::RTP::RtpStreamRecv* rtpStream, uint32_t mappedSsrc)
	{
		MS_TRACE();

		this->mapMappedSsrcProducerRtpStream[mappedSsrc] = rtpStream;
	}

	void PipeProducerStreamManager::ProducerNewRtpStream(
	  RTC::RTP::RtpStreamRecv* rtpStream, uint32_t mappedSsrc)
	{
		MS_TRACE();

		this->mapMappedSsrcProducerRtpStream[mappedSsrc] = rtpStream;
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

#ifdef MS_USE_BUILTIN_BWE
	int64_t PipeProducerStreamManager::GetOutputBitrate(int64_t nowMs) const
	{
		MS_TRACE();

		int64_t outputBitrate{ 0 };

		for (const auto& kv : this->mapMappedSsrcProducerRtpStream)
		{
			auto* const producerRtpStream = kv.second;

			outputBitrate += producerRtpStream->GetBitrate(nowMs);
		}

		return outputBitrate;
	}

	int64_t PipeProducerStreamManager::TakeBitrate(int64_t /*bitrate*/, int64_t /*nowMs*/)
	{
		MS_TRACE();

		MS_ABORT("not an adaptable bitrate");
	}

	void PipeProducerStreamManager::ApplyBitrateAllocation(int64_t /*rtpStreamActiveMs*/)
	{
		MS_TRACE();

		MS_ABORT("not an adaptable bitrate");
	}
#else
	int64_t PipeProducerStreamManager::IncreaseLayer(
	  int64_t /*bitrate*/, bool /*considerLoss*/, float /*lossPercentage*/, int64_t /*nowMs*/)
	{
		MS_TRACE();

		// Pipe does not play the BWE game.
		return 0;
	}

	void PipeProducerStreamManager::ApplyLayers(int64_t /*rtpStreamActiveMs*/)
	{
		MS_TRACE();

		// Pipe does not play the BWE game.
	}
#endif

#ifdef MS_USE_BUILTIN_BWE
	int64_t PipeProducerStreamManager::GetDesiredBitrate(int64_t /*nowMs*/) const
	{
		MS_TRACE();

		MS_ABORT("not an adaptable bitrate");
	}
#else
	int64_t PipeProducerStreamManager::GetDesiredBitrate(int64_t /*nowMs*/) const
	{
		MS_TRACE();

		// Pipe does not play the BWE game.
		return 0;
	}
#endif

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
		result.tsOffset = 0;
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
