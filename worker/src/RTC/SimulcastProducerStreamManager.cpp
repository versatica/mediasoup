#define MS_CLASS "RTC::SimulcastProducerStreamManager"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SimulcastProducerStreamManager.hpp"
#include "Logger.hpp"

namespace RTC
{
	/* Static. */

	static constexpr uint64_t StreamMinActiveMs{ 2000u };
	static constexpr uint64_t BweDowngradeConservativeMs{ 10000u };
	static constexpr uint64_t BweDowngradeMinActiveMs{ 8000u };
	static constexpr uint16_t MaxSequenceNumberGap{ 100u };

	/* Instance methods. */

	SimulcastProducerStreamManager::SimulcastProducerStreamManager(
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

		// Build mapMappedSsrcSpatialLayer and reserve producerRtpStreams.
		for (size_t idx{ 0u }; idx < this->consumableRtpEncodings.size(); ++idx)
		{
			auto& encoding = this->consumableRtpEncodings[idx];

			this->mapMappedSsrcSpatialLayer[encoding.ssrc] = static_cast<int16_t>(idx);
		}

		this->producerRtpStreams.resize(this->consumableRtpEncodings.size(), nullptr);
	}

	RTC::RTP::RtpStreamRecv* SimulcastProducerStreamManager::GetProducerCurrentRtpStream() const
	{
		MS_TRACE();

		if (this->currentSpatialLayer == -1)
		{
			return nullptr;
		}

		// This may return nullptr.
		return this->producerRtpStreams.at(this->currentSpatialLayer);
	}

	RTC::RTP::RtpStreamRecv* SimulcastProducerStreamManager::GetProducerTargetRtpStream() const
	{
		MS_TRACE();

		if (this->targetLayers.spatial == -1)
		{
			return nullptr;
		}

		// This may return nullptr.
		return this->producerRtpStreams.at(this->targetLayers.spatial);
	}

	bool SimulcastProducerStreamManager::IsActive() const
	{
		MS_TRACE();

		// clang-format off
		return (
			this->listener->IsActive() &&
			std::ranges::any_of(
				this->producerRtpStreams,
				[](const RTC::RTP::RtpStreamRecv* rtpStream)
				{
					return (
						rtpStream != nullptr &&
						(rtpStream->GetScore() > 0u || !rtpStream->HasRtpInactivityCheckEnabled())
					);
				}
			)
		);
		// clang-format on
	}

	void SimulcastProducerStreamManager::ProducerRtpStream(
	  RTC::RTP::RtpStreamRecv* rtpStream, uint32_t mappedSsrc)
	{
		MS_TRACE();

		auto it = this->mapMappedSsrcSpatialLayer.find(mappedSsrc);

		MS_ASSERT(it != this->mapMappedSsrcSpatialLayer.end(), "unknown mappedSsrc");

		const int16_t spatialLayer = it->second;

		this->producerRtpStreams[spatialLayer] = rtpStream;
	}

	void SimulcastProducerStreamManager::ProducerNewRtpStream(
	  RTC::RTP::RtpStreamRecv* rtpStream, uint32_t mappedSsrc)
	{
		MS_TRACE();

		auto it = this->mapMappedSsrcSpatialLayer.find(mappedSsrc);

		MS_ASSERT(it != this->mapMappedSsrcSpatialLayer.end(), "unknown mappedSsrc");

		const int16_t spatialLayer = it->second;

		this->producerRtpStreams[spatialLayer] = rtpStream;

		// Emit the score event.
		this->listener->OnProducerStreamManagerScore();

		if (IsActive())
		{
			MayChangeLayers(/*force*/ false);

			// If this stream is the target spatial layer and we are still waiting
			// for a key frame (i.e. the previous RequestKeyFrameForTargetSpatialLayer
			// call was a no-op because the stream wasn't set yet), request it now.
			if (
			  spatialLayer == this->targetLayers.spatial &&
			  this->currentSpatialLayer != this->targetLayers.spatial)
			{
				RequestKeyFrameForTargetSpatialLayer();
			}
		}
	}

	void SimulcastProducerStreamManager::ProducerRtpStreamScore(
	  RTC::RTP::RtpStreamRecv* /*rtpStream*/, uint8_t score, uint8_t previousScore)
	{
		MS_TRACE();

		if (this->listener->IsActive())
		{
			// All Producer streams are dead.
			if (!IsActive())
			{
				UpdateTargetLayers(-1, -1);
			}
			// Just check target layers if the stream has died or reborned or if
			// bitrate is not externally managed.
			else if (!this->externallyManagedBitrate || (score == 0u || previousScore == 0u))
			{
				MayChangeLayers(/*force*/ false);
			}
		}
	}

	void SimulcastProducerStreamManager::ProducerRtcpSenderReport(
	  RTC::RTP::RtpStreamRecv* rtpStream, bool first)
	{
		MS_TRACE();

		// Just interested if this is the first Sender Report for a RTP stream.
		if (!first)
		{
			return;
		}

		MS_DEBUG_TAG(simulcast, "first SenderReport [ssrc:%" PRIu32 "]", rtpStream->GetSsrc());

		// If our RTP timestamp reference stream does not yet have SR, do nothing
		// since we know we won't be able to switch.
		auto* producerTsReferenceRtpStream = GetProducerTsReferenceRtpStream();

		if (!producerTsReferenceRtpStream || !producerTsReferenceRtpStream->GetSenderReportNtpMs())
		{
			return;
		}

		if (IsActive())
		{
			MayChangeLayers(/*force*/ false);
		}
	}

	uint32_t SimulcastProducerStreamManager::IncreaseLayer(
	  uint32_t bitrate, bool considerLoss, float lossPercentage, uint64_t nowMs)
	{
		MS_TRACE();

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

		for (size_t sIdx{ 0u }; sIdx < this->producerRtpStreams.size(); ++sIdx)
		{
			spatialLayer = static_cast<int16_t>(sIdx);

			// If this is higher than current spatial layer and we moved to current
			// spatial layer due to BWE limitations, check how much it has elapsed
			// since then.
			if (nowMs - this->lastBweDowngradeAtMs < BweDowngradeConservativeMs)
			{
				if (this->provisionalTargetLayers.spatial > -1 && spatialLayer > this->currentSpatialLayer)
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
			// If this is higher than preferred spatial layer, abort.
			else if (spatialLayer > this->preferredLayers.spatial)
			{
				MS_DEBUG_DEV(
				  "avoid upgrading to spatial layer %" PRIi16
				  " since it's higher than preferred spatial layer %" PRIi16,
				  spatialLayer,
				  this->preferredLayers.spatial);

				goto done;
			}

			// This can be null.
			auto* producerRtpStream = this->producerRtpStreams.at(spatialLayer);

			// Producer stream does not exist. Ignore.
			if (!producerRtpStream)
			{
				continue;
			}

			// Ignore spatial layers (streams) with score 0.
			if (producerRtpStream->GetScore() == 0)
			{
				continue;
			}

			// If the stream has not been active time enough and we have an active one
			// already, move to the next spatial layer.
			if (
			  spatialLayer != this->provisionalTargetLayers.spatial &&
			  this->provisionalTargetLayers.spatial != -1 &&
			  producerRtpStream->GetActiveMs() < StreamMinActiveMs)
			{
				const auto* provisionalProducerRtpStream =
				  this->producerRtpStreams.at(this->provisionalTargetLayers.spatial);

				// The stream for the current provisional spatial layer has been active
				// for enough time, move to the next spatial layer.
				if (provisionalProducerRtpStream->GetActiveMs() >= StreamMinActiveMs)
				{
					continue;
				}
			}

			// We may not yet switch to this spatial layer.
			if (!CanSwitchToSpatialLayer(spatialLayer))
			{
				continue;
			}

			temporalLayer = 0;

			// Check bitrate of every temporal layer.
			for (; std::cmp_less(temporalLayer, producerRtpStream->GetTemporalLayers()); ++temporalLayer)
			{
				// Ignore temporal layers lower than the one we already have (taking
				// into account the spatial layer too).
				if (
				  spatialLayer == this->provisionalTargetLayers.spatial &&
				  temporalLayer <= this->provisionalTargetLayers.temporal)
				{
					continue;
				}

				requiredBitrate = producerRtpStream->GetLayerBitrate(nowMs, 0, temporalLayer);

				// This is simulcast so we must subtract the bitrate of the current
				// temporal spatial layer if this is the temporal layer 0 of a higher
				// spatial layer.
				if (
				  requiredBitrate && temporalLayer == 0 && this->provisionalTargetLayers.spatial > -1 &&
				  spatialLayer > this->provisionalTargetLayers.spatial)
				{
					auto* provisionalProducerRtpStream =
					  this->producerRtpStreams.at(this->provisionalTargetLayers.spatial);
					auto provisionalRequiredBitrate = provisionalProducerRtpStream->GetBitrate(
					  nowMs, 0, this->provisionalTargetLayers.temporal);

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

			// If this is the preferred spatial layer or higher, take it and exit.
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
		  "setting provisional layers to %" PRIi16 ":%" PRIi16 " [virtual bitrate:%" PRIu32
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

	void SimulcastProducerStreamManager::ApplyLayers(uint64_t rtpStreamActiveMs)
	{
		MS_TRACE();

		auto provisionalTargetLayers = this->provisionalTargetLayers;

		// Reset provisional target layers.
		this->provisionalTargetLayers.Reset();

		if (provisionalTargetLayers != this->targetLayers)
		{
			UpdateTargetLayers(provisionalTargetLayers.spatial, provisionalTargetLayers.temporal);

			// If this looks like a spatial layer downgrade due to BWE limitations, set
			// member.
			if (
			  rtpStreamActiveMs > BweDowngradeMinActiveMs &&
			  this->targetLayers.spatial < this->currentSpatialLayer &&
			  this->currentSpatialLayer <= this->preferredLayers.spatial)
			{
				MS_DEBUG_DEV(
				  "possible target spatial layer downgrade (from %" PRIi16 " to %" PRIi16
				  ") due to BWE limitation",
				  this->currentSpatialLayer,
				  this->targetLayers.spatial);

				this->lastBweDowngradeAtMs = this->shared->GetTimeMs();
			}
		}
	}

	uint32_t SimulcastProducerStreamManager::GetDesiredBitrate(uint64_t nowMs) const
	{
		MS_TRACE();

		uint32_t desiredBitrate{ 0u };

		// Let's iterate all streams of the Producer (from highest to lowest) and
		// obtain their bitrate. Choose the highest one.
		// NOTE: When the Producer enables a higher stream, initially the bitrate of
		// it could be less than the bitrate of a lower stream. That's why we
		// iterate all streams here anyway.
		for (auto sIdx{ static_cast<int16_t>(this->producerRtpStreams.size() - 1) }; sIdx >= 0; --sIdx)
		{
			auto* producerRtpStream = this->producerRtpStreams.at(sIdx);

			if (!producerRtpStream)
			{
				continue;
			}

			auto streamBitrate = producerRtpStream->GetBitrate(nowMs);

			desiredBitrate = std::max(streamBitrate, desiredBitrate);
		}

		return desiredBitrate;
	}

	ProducerStreamManager::RtpPacketProcessResult SimulcastProducerStreamManager::ProcessRtpPacket(
	  RTC::RTP::Packet* packet, bool lastSentPacketHasMarker, uint32_t clockRate, uint32_t maxPacketTs)
	{
		MS_TRACE();

		RtpPacketProcessResult result;

		auto spatialLayer = this->mapMappedSsrcSpatialLayer.at(packet->GetSsrc());

		// Check target layers validity.
		if (this->targetLayers.temporal == -1)
		{
#ifdef MS_RTC_LOGGER_RTP
			packet->logger.Discarded(RTC::RtcLogger::RtpPacket::DiscardReason::INVALID_TARGET_LAYER);
#endif

			if (spatialLayer == this->currentSpatialLayer)
			{
				result.type = RtpPacketProcessResult::Type::DROP;
			}
			else
			{
				result.type = RtpPacketProcessResult::Type::SILENT_DROP;
			}

			return result;
		}

		bool shouldSwitchCurrentSpatialLayer{ false };

		// Check whether this is the packet we are waiting for in order to update
		// the current spatial layer.
		if (
		  this->currentSpatialLayer != this->targetLayers.spatial &&
		  spatialLayer == this->targetLayers.spatial)
		{
			// Ignore if not a key frame.
			if (this->keyFrameSupported && !packet->IsKeyFrame())
			{
				result.type = RtpPacketProcessResult::Type::BUFFER;

#ifdef MS_RTC_LOGGER_RTP
				packet->logger.Discarded(RTC::RtcLogger::RtpPacket::DiscardReason::NOT_A_KEYFRAME);
#endif

				return result;
			}

			shouldSwitchCurrentSpatialLayer = true;

			// Need to resync the stream.
			this->syncRequired       = true;
			this->spatialLayerToSync = spatialLayer;
		}
		// If the packet belongs to different spatial layer than the one being sent,
		// drop it.
		else if (spatialLayer != this->currentSpatialLayer)
		{
#ifdef MS_RTC_LOGGER_RTP
			packet->logger.Discarded(RTC::RtcLogger::RtpPacket::DiscardReason::SPATIAL_LAYER_MISMATCH);
#endif
			result.type = RtpPacketProcessResult::Type::SILENT_DROP;

			return result;
		}

		// If we need to sync and this is not a key frame, ignore the packet.
		if (this->syncRequired && this->keyFrameSupported && !packet->IsKeyFrame())
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
#ifdef MS_RTC_LOGGER_RTP
			packet->logger.Discarded(RTC::RtcLogger::RtpPacket::DiscardReason::EMPTY_PAYLOAD);
#endif
			if (spatialLayer == this->currentSpatialLayer)
			{
				result.type = RtpPacketProcessResult::Type::DROP;

#ifdef MS_RTC_LOGGER_RTP
				packet->logger.Discarded(RTC::RtcLogger::RtpPacket::DiscardReason::EMPTY_PAYLOAD);
#endif
			}
			else
			{
				result.type = RtpPacketProcessResult::Type::SILENT_DROP;
			}

			return result;
		}

		// Whether this is the first packet after re-sync.
		const bool isSyncPacket = this->syncRequired;

		// Sync sequence number and timestamp if required.
		if (isSyncPacket && (this->spatialLayerToSync == -1 || spatialLayer == this->spatialLayerToSync))
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

			uint32_t tsOffset{ 0u };

			// Sync our RTP stream's RTP timestamp.
			if (spatialLayer == this->tsReferenceSpatialLayer)
			{
				tsOffset = 0u;
			}
			// If this is not the RTP stream we use as TS reference, do NTP based RTP
			// TS synchronization.
			else
			{
				auto* producerTsReferenceRtpStream = GetProducerTsReferenceRtpStream();
				auto* producerTargetRtpStream      = GetProducerTargetRtpStream();

				// NOTE: If we are here is because we have Sender Reports for both the
				// TS reference stream and the target one.
				MS_ASSERT(
				  producerTsReferenceRtpStream->GetSenderReportNtpMs(),
				  "no Sender Report for TS reference RTP stream");
				MS_ASSERT(
				  producerTargetRtpStream->GetSenderReportNtpMs(), "no Sender Report for current RTP stream");

				// Calculate NTP and TS stuff.
				auto ntpMs1 = producerTsReferenceRtpStream->GetSenderReportNtpMs();
				auto ts1    = producerTsReferenceRtpStream->GetSenderReportTs();
				auto ntpMs2 = producerTargetRtpStream->GetSenderReportNtpMs();
				auto ts2    = producerTargetRtpStream->GetSenderReportTs();
				int64_t diffMs;

				if (ntpMs2 >= ntpMs1)
				{
					diffMs = ntpMs2 - ntpMs1;
				}
				else
				{
					diffMs = -1 * (ntpMs1 - ntpMs2);
				}

				const int64_t diffTs  = diffMs * clockRate / 1000;
				const uint32_t newTs2 = ts2 - diffTs;

				// Apply offset. This is the difference that later must be removed from
				// the sending RTP packet.
				tsOffset = newTs2 - ts1;
			}

			// When switching to a new stream it may happen that the timestamp of this
			// key frame is lower than the highest timestamp sent to the remote endpoint.
			// If so, apply an extra offset to "fix" it for the whole life of this
			// selected Producer stream.
			if (shouldSwitchCurrentSpatialLayer && (packet->GetTimestamp() - tsOffset <= maxPacketTs))
			{
				// Max delay in ms we allow for the stream when switching.
				// https://en.wikipedia.org/wiki/Audio-to-video_synchronization#Recommendations
				static constexpr uint32_t MaxExtraOffsetMs{ 75 };

				// Outgoing packet matches the highest timestamp seen in the previous
				// stream. Apply an expected offset for a new frame in a 30fps stream.
				static constexpr uint8_t MsOffset{ 33 }; // (1 / 30 * 1000).

				const int64_t maxTsExtraOffset = MaxExtraOffsetMs * clockRate / 1000;
				uint32_t tsExtraOffset =
				  maxPacketTs - packet->GetTimestamp() + tsOffset + (MsOffset * clockRate / 1000);

				// NOTE: Don't ask for a key frame if already done.
				if (this->keyFrameForTsOffsetRequested)
				{
					// Give up and use the theoretical offset.
					if (std::cmp_greater(tsExtraOffset, maxTsExtraOffset))
					{
						MS_WARN_TAG(
						  simulcast,
						  "giving up on proper stream switching after got a requested keyframe for "
						  "which still too high RTP timestamp extra offset is needed (%" PRIu32 ")",
						  tsExtraOffset);

						tsExtraOffset = 1u;
					}
				}
				else if (std::cmp_greater(tsExtraOffset, maxTsExtraOffset))
				{
					MS_WARN_TAG(
					  simulcast,
					  "cannot switch stream due to too high RTP timestamp extra offset needed "
					  "(%" PRIu32 "), requesting keyframe",
					  tsExtraOffset);

					RequestKeyFrameForTargetSpatialLayer();

					this->keyFrameForTsOffsetRequested = true;

					// Reset flags since we are discarding this key frame.
					this->syncRequired       = false;
					this->spatialLayerToSync = -1;

					result.type = RtpPacketProcessResult::Type::SILENT_DROP;

#ifdef MS_RTC_LOGGER_RTP
					packet->logger.Discarded(
					  RTC::RtcLogger::RtpPacket::DiscardReason::TOO_HIGH_TIMESTAMP_EXTRA_NEEDED);
#endif

					return result;
				}

				if (tsExtraOffset > 0u)
				{
					MS_DEBUG_TAG(
					  simulcast,
					  "RTP timestamp extra offset generated for stream switching: %" PRIu32,
					  tsExtraOffset);

					// Increase the timestamp offset for the whole life of this Producer
					// stream (until switched to a different one).
					tsOffset -= tsExtraOffset;
				}
			}

			this->tsOffset = tsOffset;

			// Sync our RTP stream's sequence number.
			// If previous frame has not been sent completely when we switch layer,
			// we can tell libwebrtc that previous frame is incomplete by skipping
			// one RTP sequence number.
			// https://github.com/versatica/mediasoup/issues/408
			result.isSyncPacket = true;
			result.syncSeqValue = packet->GetSequenceNumber() - (lastSentPacketHasMarker ? 1 : 2);
			result.shouldSyncEncodingContext = true;

			// Sync the encoding context now, before ProcessPayload runs below.
			// This must happen before ProcessPayload so the codec handler resets
			// its state before processing the first (key frame) packet.
			if (this->encodingContext)
			{
				this->encodingContext->SyncRequired();
			}

			this->syncRequired                 = false;
			this->spatialLayerToSync           = -1;
			this->keyFrameForTsOffsetRequested = false;
		}

		// Old-packet filtering after spatial layer switch.
		if (!shouldSwitchCurrentSpatialLayer && this->checkingForOldPacketsInSpatialLayer)
		{
			if (SeqManager<uint16_t>::IsSeqLowerThan(packet->GetSequenceNumber(), this->snReferenceSpatialLayer))
			{
				result.type = RtpPacketProcessResult::Type::DROP;

#ifdef MS_RTC_LOGGER_RTP
				packet->logger.Discarded(
				  RTC::RtcLogger::RtpPacket::DiscardReason::PACKET_PREVIOUS_TO_SPATIAL_LAYER_SWITCH);
#endif

				return result;
			}
			else if (
			  SeqManager<uint16_t>::IsSeqHigherThan(
			    packet->GetSequenceNumber(), this->snReferenceSpatialLayer + MaxSequenceNumberGap))
			{
				this->checkingForOldPacketsInSpatialLayer = false;
			}
		}

		// Preserve the original marker bit. ProcessPayload may update it.
		bool marker{ packet->HasMarker() };

		if (shouldSwitchCurrentSpatialLayer)
		{
			// Update current spatial layer.
			this->currentSpatialLayer = this->targetLayers.spatial;

			this->snReferenceSpatialLayer             = packet->GetSequenceNumber();
			this->checkingForOldPacketsInSpatialLayer = true;

			// Update target and current temporal layer.
			this->encodingContext->SetTargetTemporalLayer(this->targetLayers.temporal);
			this->encodingContext->SetCurrentTemporalLayer(packet->GetTemporalLayer());

			// Rewrite payload if needed.
			packet->ProcessPayload(this->encodingContext.get(), marker);

			result.spatialLayerSwitched = true;
		}
		else
		{
			auto previousTemporalLayer = this->encodingContext->GetCurrentTemporalLayer();

			// Rewrite payload if needed. Drop packet if necessary.
			if (!packet->ProcessPayload(this->encodingContext.get(), marker))
			{
				result.type = RtpPacketProcessResult::Type::DROP;

#ifdef MS_RTC_LOGGER_RTP
				packet->logger.Discarded(RTC::RtcLogger::RtpPacket::DiscardReason::DROPPED_BY_CODEC);
#endif

				return result;
			}

			if (previousTemporalLayer != this->encodingContext->GetCurrentTemporalLayer())
			{
				result.temporalLayerChanged = true;
			}
		}

		// Set forward action.
		result.type     = RtpPacketProcessResult::Type::FORWARD;
		result.tsOffset = this->tsOffset;
		result.marker   = marker;

		return result;
	}

	void SimulcastProducerStreamManager::RequestKeyFrame()
	{
		MS_TRACE();

		auto* producerTargetRtpStream  = GetProducerTargetRtpStream();
		auto* producerCurrentRtpStream = GetProducerCurrentRtpStream();

		if (producerTargetRtpStream)
		{
			auto mappedSsrc = this->consumableRtpEncodings[this->targetLayers.spatial].ssrc;

			this->listener->OnProducerStreamManagerKeyFrameRequested(mappedSsrc);
		}

		if (producerCurrentRtpStream && producerCurrentRtpStream != producerTargetRtpStream)
		{
			auto mappedSsrc = this->consumableRtpEncodings[this->currentSpatialLayer].ssrc;

			this->listener->OnProducerStreamManagerKeyFrameRequested(mappedSsrc);
		}
	}

	void SimulcastProducerStreamManager::RequestKeyFrameForTargetSpatialLayer()
	{
		MS_TRACE();

		auto* producerTargetRtpStream = GetProducerTargetRtpStream();

		if (!producerTargetRtpStream)
		{
			return;
		}

		auto mappedSsrc = this->consumableRtpEncodings[this->targetLayers.spatial].ssrc;

		this->listener->OnProducerStreamManagerKeyFrameRequested(mappedSsrc);
	}

	void SimulcastProducerStreamManager::RequestKeyFrameForCurrentSpatialLayer()
	{
		MS_TRACE();

		auto* producerCurrentRtpStream = GetProducerCurrentRtpStream();

		if (!producerCurrentRtpStream)
		{
			return;
		}

		auto mappedSsrc = this->consumableRtpEncodings[this->currentSpatialLayer].ssrc;

		this->listener->OnProducerStreamManagerKeyFrameRequested(mappedSsrc);
	}

	void SimulcastProducerStreamManager::UpdateTargetLayers(
	  int16_t newTargetSpatialLayer, int16_t newTargetTemporalLayer)
	{
		MS_TRACE();

		// If we don't have yet a RTP timestamp reference, set it now.
		if (
		  newTargetSpatialLayer != -1 && (this->tsReferenceSpatialLayer == -1 ||
		                                  !GetProducerTsReferenceRtpStream()->GetSenderReportNtpMs()))
		{
			MS_DEBUG_TAG(
			  simulcast, "using spatial layer %" PRIi16 " as RTP timestamp reference", newTargetSpatialLayer);

			this->tsReferenceSpatialLayer = newTargetSpatialLayer;
		}

		// If the new target spatial layer doesn't match the current one, clear the
		// target layer retransmission buffer.
		if (newTargetSpatialLayer != this->targetLayers.spatial)
		{
			this->listener->OnProducerStreamManagerClearRetransmissionBuffer();
		}

		if (newTargetSpatialLayer == -1)
		{
			// Unset current and target layers.
			this->targetLayers.spatial  = -1;
			this->targetLayers.temporal = -1;
			this->currentSpatialLayer   = -1;

			this->encodingContext->SetTargetTemporalLayer(-1);
			this->encodingContext->SetCurrentTemporalLayer(-1);

			MS_DEBUG_TAG(simulcast, "target layers changed [spatial:-1, temporal:-1]");

			this->listener->OnProducerStreamManagerLayersChanged();

			return;
		}

		this->targetLayers.spatial  = newTargetSpatialLayer;
		this->targetLayers.temporal = newTargetTemporalLayer;

		// If the new target spatial layer matches the current one, apply the new
		// target temporal layer now.
		if (this->targetLayers.spatial == this->currentSpatialLayer)
		{
			this->encodingContext->SetTargetTemporalLayer(this->targetLayers.temporal);
		}

		MS_DEBUG_TAG(
		  simulcast,
		  "target layers changed [spatial:%" PRIi16 ", temporal:%" PRIi16 "]",
		  this->targetLayers.spatial,
		  this->targetLayers.temporal);

		// If the target spatial layer is different than the current one, request
		// a key frame.
		if (this->targetLayers.spatial != this->currentSpatialLayer)
		{
			RequestKeyFrameForTargetSpatialLayer();
		}
	}

	bool SimulcastProducerStreamManager::RecalculateTargetLayers(
	  RTC::ConsumerTypes::VideoLayers& newTargetLayers) const
	{
		MS_TRACE();

		// Start with no layers.
		newTargetLayers.Reset();

		auto nowMs = this->shared->GetTimeMs();

		for (size_t sIdx{ 0u }; sIdx < this->producerRtpStreams.size(); ++sIdx)
		{
			auto spatialLayer       = static_cast<int16_t>(sIdx);
			auto* producerRtpStream = this->producerRtpStreams.at(sIdx);
			auto producerScore      = producerRtpStream ? producerRtpStream->GetScore() : 0u;

			// If this is higher than current spatial layer and we moved to current
			// spatial layer due to BWE limitations, check how much it has elapsed
			// since then.
			if (nowMs - this->lastBweDowngradeAtMs < BweDowngradeConservativeMs)
			{
				if (newTargetLayers.spatial > -1 && spatialLayer > this->currentSpatialLayer)
				{
					continue;
				}
			}

			// Ignore spatial layers for non existing Producer streams or for those
			// with score 0.
			if (producerScore == 0u)
			{
				continue;
			}

			// If the stream has not been active time enough and we have an active one
			// already, move to the next spatial layer.
			// NOTE: Require bitrate externally managed for this.
			if (this->externallyManagedBitrate && newTargetLayers.spatial != -1 && producerRtpStream->GetActiveMs() < StreamMinActiveMs)
			{
				continue;
			}

			// We may not yet switch to this spatial layer.
			if (!CanSwitchToSpatialLayer(spatialLayer))
			{
				continue;
			}

			newTargetLayers.spatial = spatialLayer;

			// If this is the preferred or higher spatial layer take it and exit.
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

		// Return true if any target layer changed.
		return (newTargetLayers != this->targetLayers);
	}

	void SimulcastProducerStreamManager::OnTransportConnected()
	{
		MS_TRACE();

		this->syncRequired                 = true;
		this->spatialLayerToSync           = -1;
		this->keyFrameForTsOffsetRequested = false;

		if (IsActive())
		{
			MayChangeLayers(/*force*/ false);
		}
	}

	void SimulcastProducerStreamManager::OnTransportDisconnected()
	{
		MS_TRACE();

		this->lastBweDowngradeAtMs = 0u;

		UpdateTargetLayers(-1, -1);
	}

	void SimulcastProducerStreamManager::OnPaused()
	{
		MS_TRACE();

		this->lastBweDowngradeAtMs = 0u;

		UpdateTargetLayers(-1, -1);
	}

	void SimulcastProducerStreamManager::OnResumed()
	{
		MS_TRACE();

		this->syncRequired                        = true;
		this->spatialLayerToSync                  = -1;
		this->keyFrameForTsOffsetRequested        = false;
		this->checkingForOldPacketsInSpatialLayer = false;

		if (IsActive())
		{
			MayChangeLayers(/*force*/ false);
		}
	}

	bool SimulcastProducerStreamManager::CanSwitchToSpatialLayer(int16_t spatialLayer) const
	{
		MS_TRACE();

		MS_ASSERT(
		  this->producerRtpStreams.at(spatialLayer),
		  "no Producer RtpStream for the given spatialLayer:%" PRIi16,
		  spatialLayer);

		return (
		  this->tsReferenceSpatialLayer == -1 || spatialLayer == this->tsReferenceSpatialLayer ||
		  this->producerRtpStreams.at(spatialLayer)->GetSenderReportNtpMs());
	}

	RTC::RTP::RtpStreamRecv* SimulcastProducerStreamManager::GetProducerTsReferenceRtpStream() const
	{
		MS_TRACE();

		if (this->tsReferenceSpatialLayer == -1)
		{
			return nullptr;
		}

		// This may return nullptr.
		return this->producerRtpStreams.at(this->tsReferenceSpatialLayer);
	}
} // namespace RTC
