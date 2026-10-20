#define MS_CLASS "RTC::SvcProducerStreamManager"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SvcProducerStreamManager.hpp"
#include "Logger.hpp"

namespace RTC
{
	/* Static. */

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

	std::vector<RTC::RTP::RtpStreamRecv*> SvcProducerStreamManager::GetProducerRtpStreams() const
	{
		MS_TRACE();

		auto* const producerCurrentRtpStream = GetProducerCurrentRtpStream();

		if (!producerCurrentRtpStream)
		{
			return {};
		}

		return { producerCurrentRtpStream };
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
			(this->producerRtpStream->GetScore() > 0 || !this->producerRtpStream->HasRtpInactivityCheckEnabled())
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
			MayChangeLayers();
		}
	}

	void SvcProducerStreamManager::ProducerRtpStreamScore(
	  RTC::RTP::RtpStreamRecv* /*rtpStream*/, uint8_t score, uint8_t previousScore)
	{
		MS_TRACE();

		if (this->listener->IsActive())
		{
#ifdef MS_USE_BUILTIN_BWE
			// Just check target layers if the stream has died or reborned.
			if (score == 0 || previousScore == 0)
			{
				MayChangeLayers();
			}
#else
			// Just check target layers if the stream has died or reborned or if
			// bitrate is not externally managed.
			if (!this->externallyManagedBitrate || (score == 0 || previousScore == 0))
			{
				MayChangeLayers();
			}
#endif
		}
	}

	void SvcProducerStreamManager::ProducerRtcpSenderReport(
	  RTC::RTP::RtpStreamRecv* /*rtpStream*/, bool /*first*/)
	{
		MS_TRACE();

		// Do nothing.
	}

	void SvcProducerStreamManager::ProducerSpatialLayerActivityChanged(
	  RTC::RTP::RtpStreamRecv* /*rtpStream*/, uint8_t /*spatialLayer*/, bool /*isActive*/)
	{
		MS_TRACE();

		// All spatial layers share a single RTP stream, so neither its score nor its
		// inactivity check can tell that just one of them started or stopped. This is
		// the only chance to reconsider the target layers.
		if (IsActive())
		{
			MayChangeLayers();
		}
	}

#ifdef MS_USE_BUILTIN_BWE
	RTC::BWE::BitrateAllocator::Candidate::BitrateOptions SvcProducerStreamManager::GetBitrateOptions(
	  int64_t nowMs) const
	{
		MS_TRACE();

		RTC::BWE::BitrateAllocator::Candidate::BitrateOptions bitrateOptions;

		// Sending nothing is always on offer, priced at zero, since there may be no
		// usable spatial layer at all.
		bitrateOptions.options.push_back({ .bitrate = 0, .layers = {} });

		if (!this->producerRtpStream || this->producerRtpStream->GetScore() == 0)
		{
			bitrateOptions.currentOption = GetCurrentBitrateOption(bitrateOptions.options);

			return bitrateOptions;
		}

		// Whether a usable spatial layer has been found, in which case we must not
		// go above the preferred spatial layer. Same criteria as in
		// RecalculateTargetLayers(), so both take the same decision.
		bool usableSpatialLayerFound{ false };

		for (int16_t spatialLayer{ 0 };
		     std::cmp_less(spatialLayer, this->producerRtpStream->GetSpatialLayers());
		     ++spatialLayer)
		{
			// A spatial layer with no bitrate at all is not being received.
			if (this->producerRtpStream->GetSpatialLayerBitrate(nowMs, spatialLayer) == 0)
			{
				continue;
			}

			// Don't go above the preferred spatial layer if we already found a usable
			// lower one.
			if (spatialLayer > this->preferredLayers.spatial && usableSpatialLayerFound)
			{
				break;
			}

			const size_t numOptions = bitrateOptions.options.size();

			// Don't consider temporal layers above the preferred one, nor above the
			// ones this stream has.
			const auto maxTemporalLayer = std::min(
			  static_cast<int16_t>(this->producerRtpStream->GetTemporalLayers() - 1),
			  this->preferredLayers.temporal);

			for (int16_t temporalLayer{ 0 }; temporalLayer <= maxTemporalLayer; ++temporalLayer)
			{
				// A temporal layer with no bitrate is not being received, so running at
				// it would mean sending nothing. Neither is any above it, since a
				// temporal layer cannot arrive while a lower one does not.
				if (this->producerRtpStream->GetTemporalLayerBitrate(nowMs, spatialLayer, temporalLayer) == 0)
				{
					break;
				}

				// In K-SVC only the chosen spatial layer is forwarded, so running at
				// this point costs that layer alone up to this temporal layer. In full
				// SVC the lower spatial layers go along with it.
				const int64_t bitrate =
				  this->encodingContext->IsKSvc()
				    ? this->producerRtpStream->GetCumulativeTemporalLayerBitrate(
				        nowMs, spatialLayer, temporalLayer)
				    : this->producerRtpStream->GetCumulativeBitrate(nowMs, spatialLayer, temporalLayer);

				bitrateOptions.options.push_back(
				  {
				    .bitrate = bitrate, .layers = { spatialLayer, temporalLayer }
        });
			}

			// A spatial layer whose lowest temporal layer is not being received
			// cannot be run at, since every point of it is cumulative from that one,
			// so it does not count as found and the walk goes on.
			if (bitrateOptions.options.size() == numOptions)
			{
				continue;
			}

			usableSpatialLayerFound = true;

			// If this is the preferred or higher spatial layer, take it and exit.
			if (spatialLayer >= this->preferredLayers.spatial)
			{
				break;
			}
		}

		bitrateOptions.currentOption = GetCurrentBitrateOption(bitrateOptions.options);

		return bitrateOptions;
	}

	void SvcProducerStreamManager::SetBitrateOption(
	  const RTC::BWE::BitrateAllocator::Candidate::BitrateOption& option)
	{
		MS_TRACE();

		this->bitrateAllocated = true;

		if (option.layers != this->encodingContext->GetTargetLayers())
		{
			UpdateTargetLayers(option.layers.spatial, option.layers.temporal);
		}
	}
#else
	int64_t SvcProducerStreamManager::IncreaseLayer(
	  int64_t bitrate, bool considerLoss, float lossPercentage, int64_t nowMs)
	{
		MS_TRACE();

		if (!this->producerRtpStream || this->producerRtpStream->GetScore() == 0)
		{
			return 0;
		}

		// If already in the preferred layers, do nothing.
		if (this->provisionalTargetLayers == this->preferredLayers)
		{
			return 0;
		}

		int64_t virtualBitrate;

		if (considerLoss)
		{
			// Calculate virtual available bitrate based on given bitrate and our
			// packet lost.
			if (lossPercentage < 2)
			{
				virtualBitrate = static_cast<int64_t>(1.08 * bitrate);
			}
			else if (lossPercentage > 10)
			{
				virtualBitrate = static_cast<int64_t>((1 - 0.5 * (lossPercentage / 100)) * bitrate);
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

		int64_t requiredBitrate{ 0 };
		int16_t spatialLayer{ 0 };
		int16_t temporalLayer{ 0 };

		for (; std::cmp_less(spatialLayer, this->producerRtpStream->GetSpatialLayers()); ++spatialLayer)
		{
			// Ignore spatial layers lower than the one we already have.
			if (spatialLayer < this->provisionalTargetLayers.spatial)
			{
				continue;
			}

			temporalLayer = 0;

			// Don't consider temporal layers above the preferred one, nor above the
			// ones this stream has.
			const auto maxTemporalLayer = std::min(
			  static_cast<int16_t>(this->producerRtpStream->GetTemporalLayers() - 1),
			  this->preferredLayers.temporal);

			// Check bitrate of every temporal layer.
			for (; temporalLayer <= maxTemporalLayer; ++temporalLayer)
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
				  this->producerRtpStream->GetTemporalLayerBitrate(nowMs, spatialLayer, temporalLayer);

				// When using K-SVC we must subtract the bitrate of the current used
				// layer if the new layer is the temporal layer 0 of a higher spatial
				// layer.
				if (
				  this->encodingContext->IsKSvc() && requiredBitrate > 0 && temporalLayer == 0 &&
				  this->provisionalTargetLayers.spatial > -1 &&
				  spatialLayer > this->provisionalTargetLayers.spatial)
				{
					const int64_t provisionalRequiredBitrate = this->producerRtpStream->GetSpatialLayerBitrate(
					  nowMs, this->provisionalTargetLayers.spatial);

					if (requiredBitrate > provisionalRequiredBitrate)
					{
						requiredBitrate -= provisionalRequiredBitrate;
					}
					else
					{
						requiredBitrate = 1; // Don't set 0 since it would be ignored.
					}
				}

				MS_DEBUG_DEV(
				  "testing layers %" PRIi16 ":%" PRIi16 " [virtual bitrate:%" PRIi64
				  ", required bitrate:%" PRIi64 "]",
				  spatialLayer,
				  temporalLayer,
				  virtualBitrate,
				  requiredBitrate);

				// If active layer, end iterations here. Otherwise move to next spatial
				// layer.
				if (requiredBitrate > 0)
				{
					goto done;
				}
				else
				{
					break;
				}
			}

			// If this is the preferred or higher spatial layer, take it and exit,
			// unless we have not found any usable spatial layer yet.
			if (spatialLayer >= this->preferredLayers.spatial && this->provisionalTargetLayers.spatial != -1)
			{
				break;
			}
		}

	done:

		// No higher active layers found.
		if (requiredBitrate <= 0)
		{
			return 0;
		}

		// No luck.
		if (requiredBitrate > virtualBitrate)
		{
			return 0;
		}

		// Set provisional layers.
		this->provisionalTargetLayers.spatial  = spatialLayer;
		this->provisionalTargetLayers.temporal = temporalLayer;

		MS_DEBUG_DEV(
		  "upgrading to layers %" PRIi16 ":%" PRIi16 " [virtual bitrate:%" PRIi64
		  ", required bitrate:%" PRIi64 "]",
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

	void SvcProducerStreamManager::ApplyLayers()
	{
		MS_TRACE();

		auto provisionalTargetLayers = this->provisionalTargetLayers;

		// Reset provisional target layers.
		this->provisionalTargetLayers.Reset();

		if (provisionalTargetLayers != this->encodingContext->GetTargetLayers())
		{
			UpdateTargetLayers(provisionalTargetLayers.spatial, provisionalTargetLayers.temporal);
		}
	}
#endif

	int64_t SvcProducerStreamManager::GetDesiredBitrate(int64_t nowMs) const
	{
		MS_TRACE();

		if (!this->producerRtpStream || this->producerRtpStream->GetScore() == 0)
		{
			return 0;
		}

		int64_t desiredBitrate{ 0 };

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

		// Preserve the original marker bit. ProcessPayload may update it (VP9/AV1
		// overwrite it based on spatial-layer end-of-frame; VP8/H264 do not, so the
		// original packet marker must survive).
		bool marker{ packet->HasMarker() };

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
		result.tsOffset            = 0;
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

		const int64_t nowMs = this->shared->GetTimeMs();
		int16_t spatialLayer{ 0 };

		if (!this->producerRtpStream)
		{
			goto done;
		}

		if (this->producerRtpStream->GetScore() == 0)
		{
			goto done;
		}

		for (; std::cmp_less(spatialLayer, this->producerRtpStream->GetSpatialLayers()); ++spatialLayer)
		{
			if (!this->producerRtpStream->GetSpatialLayerBitrate(nowMs, spatialLayer))
			{
				continue;
			}

			// Don't go above the preferred spatial layer if we already found a usable
			// lower one.
			if (spatialLayer > this->preferredLayers.spatial && newTargetLayers.spatial != -1)
			{
				break;
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
			// Don't consider temporal layers above the preferred one, nor above the
			// ones this stream has. The Producer stream and the encoding context are
			// told apart because they come from different RTP parameters, the
			// Producer ones and the Consumer ones, and nothing here makes them agree.
			const auto temporalLayers = std::min(
			  this->producerRtpStream->GetTemporalLayers(), this->encodingContext->GetTemporalLayers());

			newTargetLayers.temporal =
			  std::min(this->preferredLayers.temporal, static_cast<int16_t>(temporalLayers - 1));
		}

	done:

		// Return true if any target layer changed.
		return newTargetLayers != this->encodingContext->GetTargetLayers();
	}

	void SvcProducerStreamManager::OnTransportConnected()
	{
		MS_TRACE();

		this->syncRequired = true;

#ifndef MS_USE_BUILTIN_BWE
		if (IsActive())
		{
			MayChangeLayers();
		}
#endif
	}

	void SvcProducerStreamManager::OnTransportDisconnected()
	{
		MS_TRACE();

		UpdateTargetLayers(-1, -1);
	}

	void SvcProducerStreamManager::OnPaused()
	{
		MS_TRACE();

		UpdateTargetLayers(-1, -1);

#ifdef MS_USE_BUILTIN_BWE
		// What this one was putting in the link it is not putting any more, and that
		// goes to whoever can use it. There is nothing to recalculate first, since a
		// paused Consumer offers nothing whatever its streams can do.
		this->listener->OnProducerStreamManagerNeedBitrateChange();
#endif
	}

	void SvcProducerStreamManager::OnResumed()
	{
		MS_TRACE();

		this->syncRequired = true;

		if (IsActive())
		{
#ifdef MS_USE_BUILTIN_BWE
			// Whatever this one may put out again has to be bid for, and the target
			// layers it was left with say nothing about that.
			this->listener->OnProducerStreamManagerNeedBitrateChange();
#else
			MayChangeLayers();
#endif
		}
	}

} // namespace RTC
