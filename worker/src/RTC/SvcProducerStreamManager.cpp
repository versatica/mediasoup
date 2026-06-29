#define MS_CLASS "RTC::SvcProducerStreamManager"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SvcProducerStreamManager.hpp"
#include "Logger.hpp"

namespace RTC
{
	/* Static. */

	static constexpr uint64_t BweDowngradeConservativeMs{ 10000u };
	static constexpr uint64_t BweDowngradeMinActiveMs{ 8000u };

	/* Instance methods. */

	SvcProducerStreamManager::SvcProducerStreamManager(
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

	RTC::RTP::RtpStreamRecv* SvcProducerStreamManager::GetProducerCurrentRtpStream() const
	{
		MS_TRACE();

		// SVC has a single producer stream. Return it if we have a current layer.
		if (this->encodingContext->GetCurrentSpatialLayer() == -1)
		{
			return nullptr;
		}

		return this->producerRtpStream;
	}

	RTC::RTP::RtpStreamRecv* SvcProducerStreamManager::GetProducerTargetRtpStream() const
	{
		MS_TRACE();

		// SVC has a single producer stream. Return it if we have a target layer.
		if (this->encodingContext->GetTargetSpatialLayer() == -1)
		{
			return nullptr;
		}

		return this->producerRtpStream;
	}

	bool SvcProducerStreamManager::IsActive() const
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

	void SvcProducerStreamManager::ProducerRtpStream(
	  RTC::RTP::RtpStreamRecv* rtpStream, uint32_t /*mappedSsrc*/)
	{
		MS_TRACE();

		this->producerRtpStream = rtpStream;
	}

	void SvcProducerStreamManager::ProducerNewRtpStream(
	  RTC::RTP::RtpStreamRecv* rtpStream, uint32_t /*mappedSsrc*/)
	{
		MS_TRACE();

		this->producerRtpStream = rtpStream;

		// Emit the score event.
		this->listener->OnProducerStreamManagerScore();

		if (IsActive())
		{
			MayChangeLayers(/*force*/ false);
		}
	}

	void SvcProducerStreamManager::ProducerRtpStreamScore(
	  RTC::RTP::RtpStreamRecv* /*rtpStream*/, uint8_t score, uint8_t previousScore)
	{
		MS_TRACE();

		if (this->listener->IsActive())
		{
			// Just check target layers if the stream has died or reborned or if
			// bitrate is not externally managed.
			if (!this->externallyManagedBitrate || (score == 0u || previousScore == 0u))
			{
				MayChangeLayers(/*force*/ false);
			}
		}
	}

	void SvcProducerStreamManager::ProducerRtcpSenderReport(
	  RTC::RTP::RtpStreamRecv* /*rtpStream*/, bool /*first*/)
	{
		MS_TRACE();

		// Do nothing.
	}

	uint32_t SvcProducerStreamManager::IncreaseLayer(
	  uint32_t bitrate, bool considerLoss, float lossPercentage, uint64_t nowMs)
	{
		MS_TRACE();

		if (!this->producerRtpStream || this->producerRtpStream->GetScore() == 0u)
		{
			return 0u;
		}

		// If already in the preferred layers, do nothing.
		if (this->provisionalTargetLayers == this->preferredLayers)
		{
			return 0u;
		}

		uint32_t virtualBitrate;

		if (considerLoss)
		{
			// Calculate virtual available bitrate based on given bitrate and our
			// packet lost.
			if (lossPercentage < 2)
			{
				virtualBitrate = 1.08 * bitrate;
			}
			else if (lossPercentage > 10)
			{
				virtualBitrate = (1 - 0.5 * (lossPercentage / 100)) * bitrate;
			}
			else
			{
				virtualBitrate = bitrate;
			}
		}
		else
		{
			virtualBitrate = bitrate;
		}

		uint32_t requiredBitrate{ 0u };
		int16_t spatialLayer{ 0 };
		int16_t temporalLayer{ 0 };

		for (; std::cmp_less(spatialLayer, this->producerRtpStream->GetSpatialLayers()); ++spatialLayer)
		{
			// If this is higher than current spatial layer and we moved to current
			// spatial layer due to BWE limitations, check how much it has elapsed
			// since then.
			if (nowMs - this->lastBweDowngradeAtMs < BweDowngradeConservativeMs)
			{
				if (this->provisionalTargetLayers.spatial > -1 && spatialLayer > this->encodingContext->GetCurrentSpatialLayer())
				{
					MS_DEBUG_DEV(
					  "avoid upgrading to spatial layer %" PRIi16 " due to recent BWE downgrade", spatialLayer);

					goto done;
				}
			}

			// Ignore spatial layers lower than the one we already have.
			if (spatialLayer < this->provisionalTargetLayers.spatial)
			{
				continue;
			}

			temporalLayer = 0;

			// Check bitrate of every temporal layer.
			for (; std::cmp_less(temporalLayer, this->producerRtpStream->GetTemporalLayers());
			     ++temporalLayer)
			{
				// Ignore temporal layers lower than the one we already have (taking
				// into account the spatial layer too).
				if (
				  spatialLayer == this->provisionalTargetLayers.spatial &&
				  temporalLayer <= this->provisionalTargetLayers.temporal)
				{
					continue;
				}

				requiredBitrate =
				  this->producerRtpStream->GetLayerBitrate(nowMs, spatialLayer, temporalLayer);

				// When using K-SVC we must subtract the bitrate of the current used
				// layer if the new layer is the temporal layer 0 of a higher spatial
				// layer.
				if (
				  this->encodingContext->IsKSvc() && requiredBitrate && temporalLayer == 0 &&
				  this->provisionalTargetLayers.spatial > -1 &&
				  spatialLayer > this->provisionalTargetLayers.spatial)
				{
					auto provisionalRequiredBitrate = this->producerRtpStream->GetSpatialLayerBitrate(
					  nowMs, this->provisionalTargetLayers.spatial);

					if (requiredBitrate > provisionalRequiredBitrate)
					{
						requiredBitrate -= provisionalRequiredBitrate;
					}
					else
					{
						requiredBitrate = 1u; // Don't set 0 since it would be ignored.
					}
				}

				MS_DEBUG_DEV(
				  "testing layers %" PRIi16 ":%" PRIi16 " [virtual bitrate:%" PRIu32
				  ", required bitrate:%" PRIu32 "]",
				  spatialLayer,
				  temporalLayer,
				  virtualBitrate,
				  requiredBitrate);

				// If active layer, end iterations here. Otherwise move to next spatial
				// layer.
				if (requiredBitrate)
				{
					goto done;
				}
				else
				{
					break;
				}
			}

			// If this is the preferred or higher spatial layer, take it and exit.
			if (spatialLayer >= this->preferredLayers.spatial)
			{
				break;
			}
		}

	done:

		// No higher active layers found.
		if (!requiredBitrate)
		{
			return 0u;
		}

		// No luck.
		if (requiredBitrate > virtualBitrate)
		{
			return 0u;
		}

		// Set provisional layers.
		this->provisionalTargetLayers.spatial  = spatialLayer;
		this->provisionalTargetLayers.temporal = temporalLayer;

		MS_DEBUG_DEV(
		  "upgrading to layers %" PRIi16 ":%" PRIi16 " [virtual bitrate:%" PRIu32
		  ", required bitrate:%" PRIu32 "]",
		  this->provisionalTargetLayers.spatial,
		  this->provisionalTargetLayers.temporal,
		  virtualBitrate,
		  requiredBitrate);

		if (requiredBitrate <= bitrate)
		{
			return requiredBitrate;
		}
		else if (requiredBitrate <= virtualBitrate)
		{
			return bitrate;
		}
		else
		{
			return requiredBitrate; // NOTE: This cannot happen.
		}
	}

	void SvcProducerStreamManager::ApplyLayers(uint64_t rtpStreamActiveMs)
	{
		MS_TRACE();

		auto provisionalTargetLayers = this->provisionalTargetLayers;

		// Reset provisional target layers.
		this->provisionalTargetLayers.Reset();

		if (provisionalTargetLayers != this->encodingContext->GetTargetLayers())
		{
			UpdateTargetLayers(provisionalTargetLayers.spatial, provisionalTargetLayers.temporal);

			// If this looks like a spatial layer downgrade due to BWE limitations,
			// set member.
			if (
			  rtpStreamActiveMs > BweDowngradeMinActiveMs &&
			  this->encodingContext->GetTargetSpatialLayer() <
			    this->encodingContext->GetCurrentSpatialLayer() &&
			  this->encodingContext->GetCurrentSpatialLayer() <= this->preferredLayers.spatial)
			{
				MS_DEBUG_DEV(
				  "possible target spatial layer downgrade (from %" PRIi16 " to %" PRIi16
				  ") due to BWE limitation",
				  this->encodingContext->GetCurrentSpatialLayer(),
				  this->encodingContext->GetTargetSpatialLayer());

				this->lastBweDowngradeAtMs = this->shared->GetTimeMs();
			}
		}
	}

	uint32_t SvcProducerStreamManager::GetDesiredBitrate(uint64_t nowMs) const
	{
		MS_TRACE();

		if (!this->producerRtpStream || this->producerRtpStream->GetScore() == 0u)
		{
			return 0u;
		}

		uint32_t desiredBitrate{ 0u };

		// When using K-SVC each spatial layer is independent of the others.
		if (this->encodingContext->IsKSvc())
		{
			// Let's iterate all spatial layers of the Producer (from highest to lowest)
			// and obtain their bitrate. Choose the highest one.
			// NOTE: When the Producer enables a higher spatial layer, initially the
			// bitrate of it could be less than the bitrate of a lower one. That's why
			// we iterate all spatial layers here anyway.
			for (auto spatialLayer{ this->producerRtpStream->GetSpatialLayers() - 1 }; spatialLayer >= 0;
			     --spatialLayer)
			{
				auto spatialLayerBitrate =
				  this->producerRtpStream->GetSpatialLayerBitrate(nowMs, spatialLayer);

				desiredBitrate = std::max(spatialLayerBitrate, desiredBitrate);
			}
		}
		else
		{
			desiredBitrate = this->producerRtpStream->GetBitrate(nowMs);
		}

		return desiredBitrate;
	}

	ProducerStreamManager::RtpPacketProcessResult SvcProducerStreamManager::ProcessRtpPacket(
	  RTC::RTP::Packet* packet,
	  bool /*lastSentPacketHasMarker*/,
	  uint32_t /*clockRate*/,
	  uint32_t /*maxPacketTs*/)
	{
		MS_TRACE();

		RtpPacketProcessResult result;

		if (this->encodingContext->GetTargetSpatialLayer() == -1 || this->encodingContext->GetTargetTemporalLayer() == -1)
		{
			result.type = RtpPacketProcessResult::Type::DROP;

#ifdef MS_RTC_LOGGER_RTP
			packet->logger.Discarded(RTC::RtcLogger::RtpPacket::DiscardReason::INVALID_TARGET_LAYER);
#endif

			return result;
		}

		// If we need to sync and this is not a key frame, ignore the packet.
		if (this->syncRequired && !packet->IsKeyFrame())
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

			result.isSyncPacket              = true;
			result.syncSeqValue              = packet->GetSequenceNumber() - 1;
			result.shouldSyncEncodingContext = true;

			// Sync the encoding context before ProcessPayload runs below.
			this->encodingContext->SyncRequired();

			this->syncRequired = false;
		}

		auto previousLayers = this->encodingContext->GetCurrentLayers();

		bool marker{ false };

		if (!packet->ProcessPayload(this->encodingContext.get(), marker))
		{
			result.type = RtpPacketProcessResult::Type::DROP;

#ifdef MS_RTC_LOGGER_RTP
			packet->logger.Discarded(RTC::RtcLogger::RtpPacket::DiscardReason::DROPPED_BY_CODEC);
#endif

			return result;
		}

		if (previousLayers != this->encodingContext->GetCurrentLayers())
		{
			result.temporalLayerChanged = true;
		}

		// Set forward action.
		result.type                = RtpPacketProcessResult::Type::FORWARD;
		result.tsOffset            = 0u;
		result.marker              = marker;
		result.sendBufferedPackets = sendBufferedPackets;

		return result;
	}

	void SvcProducerStreamManager::RequestKeyFrame()
	{
		MS_TRACE();

		auto mappedSsrc = this->consumableRtpEncodings[0].ssrc;

		this->listener->OnProducerStreamManagerKeyFrameRequested(mappedSsrc);
	}

	void SvcProducerStreamManager::RequestKeyFrameForTargetSpatialLayer()
	{
		MS_TRACE();

		RequestKeyFrame();
	}

	void SvcProducerStreamManager::RequestKeyFrameForCurrentSpatialLayer()
	{
		MS_TRACE();

		RequestKeyFrame();
	}

	void SvcProducerStreamManager::UpdateTargetLayers(
	  int16_t newTargetSpatialLayer, int16_t newTargetTemporalLayer)
	{
		MS_TRACE();

		if (newTargetSpatialLayer == -1)
		{
			// Unset current and target layers.
			this->encodingContext->SetTargetSpatialLayer(-1);
			this->encodingContext->SetCurrentSpatialLayer(-1);
			this->encodingContext->SetTargetTemporalLayer(-1);
			this->encodingContext->SetCurrentTemporalLayer(-1);

			MS_DEBUG_TAG(svc, "target layers changed [spatial:-1, temporal:-1]");

			this->listener->OnProducerStreamManagerLayersChanged();

			return;
		}

		this->encodingContext->SetTargetSpatialLayer(newTargetSpatialLayer);
		this->encodingContext->SetTargetTemporalLayer(newTargetTemporalLayer);

		MS_DEBUG_TAG(
		  svc,
		  "target layers changed [spatial:%" PRIi16 ", temporal:%" PRIi16 "]",
		  newTargetSpatialLayer,
		  newTargetTemporalLayer);

		// Target spatial layer has changed.
		if (newTargetSpatialLayer != this->encodingContext->GetCurrentSpatialLayer())
		{
			// In K-SVC always ask for a keyframe when changing target spatial layer.
			if (this->encodingContext->IsKSvc())
			{
				MS_DEBUG_DEV("K-SVC: requesting keyframe to target spatial change");

				RequestKeyFrame();
			}
			// In full SVC just ask for a keyframe when upgrading target spatial layer.
			// NOTE: This is because nobody implements RTCP LRR yet.
			else if (newTargetSpatialLayer > this->encodingContext->GetCurrentSpatialLayer())
			{
				MS_DEBUG_DEV("full SVC: requesting keyframe to target spatial upgrade");

				RequestKeyFrame();
			}
		}
	}

	bool SvcProducerStreamManager::RecalculateTargetLayers(
	  RTC::ConsumerTypes::VideoLayers& newTargetLayers) const
	{
		MS_TRACE();

		// Start with no layers.
		newTargetLayers.Reset();

		auto nowMs = this->shared->GetTimeMs();
		int16_t spatialLayer{ 0 };

		if (!this->producerRtpStream)
		{
			goto done;
		}

		if (this->producerRtpStream->GetScore() == 0u)
		{
			goto done;
		}

		for (; std::cmp_less(spatialLayer, this->producerRtpStream->GetSpatialLayers()); ++spatialLayer)
		{
			// If this is higher than current spatial layer and we moved to current
			// spatial layer due to BWE limitations, check how much it has elapsed
			// since then.
			if (nowMs - this->lastBweDowngradeAtMs < BweDowngradeConservativeMs)
			{
				if (newTargetLayers.spatial > -1 && spatialLayer > this->encodingContext->GetCurrentSpatialLayer())
				{
					continue;
				}
			}

			if (!this->producerRtpStream->GetSpatialLayerBitrate(nowMs, spatialLayer))
			{
				continue;
			}

			newTargetLayers.spatial = spatialLayer;

			// If this is the preferred or higher spatial layer and has bitrate,
			// take it and exit.
			if (spatialLayer >= this->preferredLayers.spatial)
			{
				break;
			}
		}

		if (newTargetLayers.spatial != -1)
		{
			if (newTargetLayers.spatial == this->preferredLayers.spatial)
			{
				newTargetLayers.temporal = this->preferredLayers.temporal;
			}
			else if (newTargetLayers.spatial < this->preferredLayers.spatial)
			{
				newTargetLayers.temporal =
				  static_cast<int16_t>(this->encodingContext->GetTemporalLayers() - 1);
			}
			else
			{
				newTargetLayers.temporal = 0;
			}
		}

	done:

		// Return true if any target layer changed.
		return newTargetLayers != this->encodingContext->GetTargetLayers();
	}

	void SvcProducerStreamManager::OnTransportConnected()
	{
		MS_TRACE();

		this->syncRequired = true;

		if (IsActive())
		{
			MayChangeLayers(/*force*/ false);
		}
	}

	void SvcProducerStreamManager::OnTransportDisconnected()
	{
		MS_TRACE();

		this->lastBweDowngradeAtMs = 0u;

		UpdateTargetLayers(-1, -1);
	}

	void SvcProducerStreamManager::OnPaused()
	{
		MS_TRACE();

		this->lastBweDowngradeAtMs = 0u;

		UpdateTargetLayers(-1, -1);
	}

	void SvcProducerStreamManager::OnResumed()
	{
		MS_TRACE();

		this->syncRequired = true;

		if (IsActive())
		{
			MayChangeLayers(/*force*/ false);
		}
	}

} // namespace RTC
