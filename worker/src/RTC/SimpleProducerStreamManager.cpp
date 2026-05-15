#define MS_CLASS "RTC::SimpleProducerStreamManager"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SimpleProducerStreamManager.hpp"
#include "Logger.hpp"

namespace RTC
{
	/* Instance methods. */

	SimpleProducerStreamManager::SimpleProducerStreamManager(
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
	}

	RTC::RTP::RtpStreamRecv* SimpleProducerStreamManager::GetProducerCurrentRtpStream() const
	{
		MS_TRACE();

		return this->producerRtpStream;
	}

	RTC::RTP::RtpStreamRecv* SimpleProducerStreamManager::GetProducerTargetRtpStream() const
	{
		MS_TRACE();

		return this->producerRtpStream;
	}

	bool SimpleProducerStreamManager::IsActive() const
	{
		MS_TRACE();

		// clang-format off
		return (
		  this->listener->IsActive() &&
		  this->producerRtpStream &&
		  // If there is no RTP inactivity check do not consider the stream
		  // inactive despite it has score 0.
		  (this->producerRtpStream->GetScore() > 0u || !this->producerRtpStream->HasRtpInactivityCheckEnabled())
		);
		// clang-format on
	}

	void SimpleProducerStreamManager::ProducerRtpStream(
	  RTC::RTP::RtpStreamRecv* rtpStream, uint32_t /*mappedSsrc*/)
	{
		MS_TRACE();

		this->producerRtpStream = rtpStream;
	}

	void SimpleProducerStreamManager::ProducerNewRtpStream(
	  RTC::RTP::RtpStreamRecv* rtpStream, uint32_t /*mappedSsrc*/)
	{
		MS_TRACE();

		this->producerRtpStream = rtpStream;

		// Emit the score event.
		this->listener->OnProducerStreamManagerScore();
	}

	void SimpleProducerStreamManager::ProducerRtpStreamScore(
	  RTC::RTP::RtpStreamRecv* /*rtpStream*/, uint8_t /*score*/, uint8_t /*previousScore*/)
	{
		MS_TRACE();
	}

	void SimpleProducerStreamManager::ProducerRtcpSenderReport(
	  RTC::RTP::RtpStreamRecv* /*rtpStream*/, bool /*first*/)
	{
		MS_TRACE();

		// Do nothing.
	}

	uint32_t SimpleProducerStreamManager::IncreaseLayer(
	  uint32_t bitrate, bool /*considerLoss*/, float /*lossPercentage*/, uint64_t nowMs)
	{
		MS_TRACE();

		MS_ASSERT(this->externallyManagedBitrate, "bitrate is not externally managed");
		MS_ASSERT(this->kind == RTC::Media::Kind::VIDEO, "should be video");
		MS_ASSERT(IsActive(), "should be active");

		// If this is not the first time this method is called within the same
		// iteration, return 0 since a video Simple consumer does not keep state
		// about this.
		if (this->managingBitrate)
		{
			return 0u;
		}

		this->managingBitrate = true;

		if (!this->producerRtpStream)
		{
			return 0u;
		}

		// Video Simple consumer does not really play the BWE game. However, let's
		// be honest and try to be nice.
		auto desiredBitrate = this->producerRtpStream->GetBitrate(nowMs);

		if (desiredBitrate < bitrate)
		{
			return desiredBitrate;
		}
		else
		{
			return bitrate;
		}
	}

	void SimpleProducerStreamManager::ApplyLayers(uint64_t /*rtpStreamActiveMs*/)
	{
		MS_TRACE();

		MS_ASSERT(this->externallyManagedBitrate, "bitrate is not externally managed");
		MS_ASSERT(this->kind == RTC::Media::Kind::VIDEO, "should be video");
		MS_ASSERT(IsActive(), "should be active");

		this->managingBitrate = false;

		// Simple does not play the BWE game (even if video kind).
	}

	uint32_t SimpleProducerStreamManager::GetDesiredBitrate(uint64_t nowMs) const
	{
		MS_TRACE();

		MS_ASSERT(this->externallyManagedBitrate, "bitrate is not externally managed");

		// Audio does not play the BWE game.
		if (this->kind != RTC::Media::Kind::VIDEO)
		{
			return 0u;
		}

		if (!IsActive())
		{
			return 0u;
		}

		return this->producerRtpStream->GetBitrate(nowMs);
	}

	ProducerStreamManager::RtpPacketProcessResult SimpleProducerStreamManager::ProcessRtpPacket(
	  RTC::RTP::Packet* packet,
	  bool /*lastSentPacketHasMarker*/,
	  uint32_t /*clockRate*/,
	  uint32_t /*maxPacketTs*/)
	{
		MS_TRACE();

		RtpPacketProcessResult result;

		// If we need to sync, support key frames and this is not a key frame,
		// ignore the packet.
		if (this->syncRequired && this->keyFrameSupported && !packet->IsKeyFrame())
		{
			// NOTE: No need to drop the packet in the RTP sequence manager since here
			// we are blocking all packets but the key frame that would trigger sync
			// below.

			// Store the packet for the scenario in which this packet is part of the
			// key frame and it arrived before the first packet of the key frame.
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

		// Preserve the original marker bit. ProcessPayload may update it.
		bool marker{ packet->HasMarker() };

		// Process the payload if needed. Drop packet if necessary.
		if (this->encodingContext && !packet->ProcessPayload(this->encodingContext.get(), marker))
		{
			MS_DEBUG_DEV(
			  "discarding packet [ssrc:%" PRIu32 ", seq:%" PRIu16 ", ts:%" PRIu32 "]",
			  packet->GetSsrc(),
			  packet->GetSequenceNumber(),
			  packet->GetTimestamp());

			result.type = RtpPacketProcessResult::Type::DROP;

#ifdef MS_RTC_LOGGER_RTP
			packet->logger.Discarded(RTC::RtcLogger::RtpPacket::DiscardReason::DROPPED_BY_CODEC);
#endif

			return result;
		}

		// Whether this is the first packet after re-sync.
		const bool isSyncPacket = this->syncRequired;

		// Whether packets stored in the target layer retransmission buffer must be
		// sent once this packet is sent.
		bool sendBufferedPackets{ false };

		// Sync sequence number and timestamp if required.
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

				sendBufferedPackets = true;
			}

			result.isSyncPacket = true;
			result.syncSeqValue = packet->GetSequenceNumber() - 1;

			this->syncRequired = false;
		}

		// Set forward action.
		result.type                = RtpPacketProcessResult::Type::FORWARD;
		result.tsOffset            = 0u;
		result.marker              = marker;
		result.sendBufferedPackets = sendBufferedPackets;

		return result;
	}

	void SimpleProducerStreamManager::RequestKeyFrame()
	{
		MS_TRACE();

		if (this->kind != RTC::Media::Kind::VIDEO)
		{
			return;
		}

		auto mappedSsrc = this->consumableRtpEncodings[0].ssrc;

		this->listener->OnProducerStreamManagerKeyFrameRequested(mappedSsrc);
	}

	void SimpleProducerStreamManager::RequestKeyFrameForTargetSpatialLayer()
	{
		MS_TRACE();

		RequestKeyFrame();
	}

	void SimpleProducerStreamManager::RequestKeyFrameForCurrentSpatialLayer()
	{
		MS_TRACE();

		RequestKeyFrame();
	}

	void SimpleProducerStreamManager::UpdateTargetLayers(int16_t /*spatial*/, int16_t /*temporal*/)
	{
		MS_TRACE();
	}

	bool SimpleProducerStreamManager::RecalculateTargetLayers(
	  RTC::ConsumerTypes::VideoLayers& /*newTargetLayers*/) const
	{
		MS_TRACE();

		// No layer changes possible for Simple.
		return false;
	}

	void SimpleProducerStreamManager::OnTransportConnected()
	{
		MS_TRACE();

		this->syncRequired = true;

		if (IsActive())
		{
			RequestKeyFrame();
		}
	}

	void SimpleProducerStreamManager::OnTransportDisconnected()
	{
		MS_TRACE();

		// Nothing specific for Simple.
	}

	void SimpleProducerStreamManager::OnPaused()
	{
		MS_TRACE();

		// Nothing specific for Simple.
	}

	void SimpleProducerStreamManager::OnResumed()
	{
		MS_TRACE();

		this->syncRequired = true;

		if (IsActive())
		{
			RequestKeyFrame();
		}
	}

} // namespace RTC
