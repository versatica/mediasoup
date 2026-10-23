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

	RTC::BWE::BitrateAllocator::Candidate::BitrateOptions PipeProducerStreamManager::GetBitrateOptions(
	  int64_t nowMs) const
	{
		MS_TRACE();

		// Pipe forwards every stream it is given, always, so it offers the single
		// point it is putting out.
		int64_t outputBitrate{ 0 };

		for (const auto& kv : this->mapMappedSsrcProducerRtpStream)
		{
			auto* const producerRtpStream = kv.second;

			outputBitrate += producerRtpStream->GetBitrate(nowMs);
		}

		return { .options = { { .bitrate = outputBitrate, .layers = {} } }, .currentOption = 0 };
	}

	void PipeProducerStreamManager::SetBitrateOption(
	  const RTC::BWE::BitrateAllocator::Candidate::BitrateOption& /*option*/)
	{
		MS_TRACE();

		MS_ABORT("a single point to run at, so it is never given any");
	}

	int64_t PipeProducerStreamManager::GetDesiredBitrate(int64_t nowMs) const
	{
		MS_TRACE();

		// What it is putting out is what it wants, since it has no way of wanting
		// anything else. Leaving it out would ask the estimator for less than what
		// is going to be sent, while the allocation takes its bitrate off the top
		// all the same.
		int64_t desiredBitrate{ 0 };

		for (const auto& kv : this->mapMappedSsrcProducerRtpStream)
		{
			auto* const producerRtpStream = kv.second;

			desiredBitrate += producerRtpStream->GetBitrate(nowMs);
		}

		return desiredBitrate;
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

		// This one never adapts, so the bitrate allocation takes what it puts out off
		// the top instead of bidding for it. Now that it puts out nothing, that whole
		// amount is there for the others.
		if (this->bweManaged)
		{
			this->listener->OnProducerStreamManagerNeedBitrateChange();
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

			// It is taking that amount off the top again, so what the others were given
			// while it was quiet no longer fits.
			if (this->bweManaged)
			{
				this->listener->OnProducerStreamManagerNeedBitrateChange();
			}
		}
	}

} // namespace RTC
