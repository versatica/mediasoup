#define MS_CLASS "RTC::SimulcastProducerStreamManager"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SimulcastProducerStreamManager.hpp"
#include "Logger.hpp"

namespace RTC
{
	/* Static. */

	static constexpr uint16_t MaxSequenceNumberGap{ 100 };

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
		for (size_t idx{ 0 }; idx < this->consumableRtpEncodings.size(); ++idx)
		{
			auto& encoding = this->consumableRtpEncodings[idx];

			this->mapMappedSsrcSpatialLayer[encoding.ssrc] = static_cast<int16_t>(idx);
		}

		this->producerRtpStreams.resize(this->consumableRtpEncodings.size(), nullptr);
		this->switchableSpatialLayers.resize(this->consumableRtpEncodings.size(), true);
	}

	std::vector<RTC::RTP::RtpStreamRecv*> SimulcastProducerStreamManager::GetProducerRtpStreams() const
	{
		MS_TRACE();

		auto* const producerCurrentRtpStream = GetProducerCurrentRtpStream();

		if (!producerCurrentRtpStream)
		{
			return {};
		}

		return { producerCurrentRtpStream };
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
						(rtpStream->GetScore() > 0 || !rtpStream->HasRtpInactivityCheckEnabled())
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
	  RTC::RTP::RtpStreamRecv* rtpStream, uint8_t score, uint8_t previousScore)
	{
		MS_TRACE();

		const auto* producerTsReferenceRtpStream = GetProducerTsReferenceRtpStream();

		// If the RTP timestamp reference stream dies while another spatial layer is
		// being sent, take the latter as reference with the offset it is being sent
		// with, so nothing depends on the dead stream anymore.
		if (
		  score == 0 && producerTsReferenceRtpStream && rtpStream == producerTsReferenceRtpStream &&
		  this->currentSpatialLayer != -1 && this->currentSpatialLayer != this->tsReferenceSpatialLayer)
		{
			MS_DEBUG_TAG(
			  simulcast,
			  "TS reference spatial layer %" PRIi16 " is dead, using current spatial layer %" PRIi16
			  " as RTP timestamp reference",
			  this->tsReferenceSpatialLayer,
			  this->currentSpatialLayer);

			this->tsReferenceSpatialLayer = this->currentSpatialLayer;
			this->tsReferenceOffset       = this->tsOffset;
		}

		if (this->listener->IsActive())
		{
			// All Producer streams are dead.
			if (!IsActive())
			{
				MS_DEBUG_TAG(
				  simulcast,
				  "Producer stream is dead and there is no other alive one, unsetting target layers [ssrc:%" PRIu32
				  "]",
				  rtpStream->GetSsrc());

				UpdateTargetLayers(-1, -1);
			}
#ifdef MS_USE_BUILTIN_BWE
			// Just check target layers if the stream has died or reborned.
			else if (score == 0 || previousScore == 0)
			{
				MayChangeLayers(/*force*/ false);
			}
#else
			// Just check target layers if the stream has died or reborned or if
			// bitrate is not externally managed.
			else if (!this->externallyManagedBitrate || (score == 0 || previousScore == 0))
			{
				MayChangeLayers(/*force*/ false);
			}
#endif
		}
	}

	void SimulcastProducerStreamManager::ProducerRtcpSenderReport(
	  RTC::RTP::RtpStreamRecv* rtpStream, bool first)
	{
		MS_TRACE();

		// Check whether any spatial layer can be switched to again, which may happen
		// after this Sender Report if the capture instants told before were too far
		// from the RTP timestamps of the streams to be aligned upon.
		// NOTE: A capture instant carried by 'abs-capture-time' may also make it
		// happen, but that is only noticed upon the next Sender Report.
		bool spatialLayerBecameSwitchable{ false };

		for (size_t sIdx{ 0 }; sIdx < this->producerRtpStreams.size(); ++sIdx)
		{
			const bool switchable =
			  this->producerRtpStreams.at(sIdx) && CanSwitchToSpatialLayer(static_cast<int16_t>(sIdx));

			if (switchable && !this->switchableSpatialLayers.at(sIdx))
			{
				spatialLayerBecameSwitchable = true;
			}

			this->switchableSpatialLayers.at(sIdx) = switchable;
		}

		// The first Sender Report of a stream is the one that lets it be aligned to
		// other streams, so it is worth checking layers upon, and so is any other one
		// after which a spatial layer can be switched to again.
		// NOTE: Doing it on every single Sender Report would ask the Transport to
		// redistribute its outgoing bitrate at the pace of the RTCP of the sender and we
		// don't want that.
		if (!first && !spatialLayerBecameSwitchable)
		{
			return;
		}

		if (first)
		{
			MS_DEBUG_TAG(simulcast, "first SenderReport [ssrc:%" PRIu32 "]", rtpStream->GetSsrc());
		}

		if (IsActive())
		{
			MayChangeLayers(/*force*/ false);
		}
	}

#ifdef MS_USE_BUILTIN_BWE
	RTC::BWE::BitrateAllocator::Candidate::BitrateOptions SimulcastProducerStreamManager::GetBitrateOptions(
	  int64_t nowMs) const
	{
		MS_TRACE();

		RTC::BWE::BitrateAllocator::Candidate::BitrateOptions bitrateOptions;

		// Sending nothing is always on offer, priced at zero, since there may be no
		// usable spatial layer at all.
		bitrateOptions.options.push_back({ .bitrate = 0, .layers = {} });

		// Whether a usable spatial layer has been found, in which case we must not
		// go above the preferred spatial layer. Same criteria as in
		// RecalculateTargetLayers(), so both take the same decision no matter the
		// bitrate of the layer.
		bool usableSpatialLayerFound{ false };

		for (size_t sIdx{ 0 }; sIdx < this->producerRtpStreams.size(); ++sIdx)
		{
			const auto spatialLayer = static_cast<int16_t>(sIdx);

			// This can be null.
			auto* producerRtpStream = this->producerRtpStreams.at(sIdx);

			// Ignore spatial layers for non existing Producer streams or for those
			// with score 0.
			if (!producerRtpStream || producerRtpStream->GetScore() == 0)
			{
				continue;
			}

			// We may not yet switch to this spatial layer.
			if (!CanSwitchToSpatialLayer(spatialLayer))
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
			  static_cast<int16_t>(producerRtpStream->GetTemporalLayers() - 1),
			  this->preferredLayers.temporal);

			for (int16_t temporalLayer{ 0 }; temporalLayer <= maxTemporalLayer; ++temporalLayer)
			{
				// A temporal layer with no bitrate is not being received, so running at
				// it would mean sending nothing. Neither is any above it, since a
				// temporal layer cannot arrive while a lower one does not.
				if (producerRtpStream->GetTemporalLayerBitrate(nowMs, 0, temporalLayer) == 0)
				{
					break;
				}

				// Each spatial layer is a stream of its own, so running at this point
				// costs its temporal layers up to this one and nothing else.
				bitrateOptions.options.push_back(
				  {
				    .bitrate = producerRtpStream->GetCumulativeTemporalLayerBitrate(nowMs, 0, temporalLayer),
				    .layers = { spatialLayer, temporalLayer }
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

			// If this is the preferred spatial layer or higher, take it and exit.
			if (spatialLayer >= this->preferredLayers.spatial)
			{
				break;
			}
		}

		bitrateOptions.currentOption = GetCurrentBitrateOption(bitrateOptions.options);

		return bitrateOptions;
	}

	void SimulcastProducerStreamManager::SetBitrateOption(
	  const RTC::BWE::BitrateAllocator::Candidate::BitrateOption& option)
	{
		MS_TRACE();

		this->bitrateAllocated = true;

		if (option.layers != this->targetLayers)
		{
			UpdateTargetLayers(option.layers.spatial, option.layers.temporal);
		}
	}
#else
	int64_t SimulcastProducerStreamManager::IncreaseLayer(
	  int64_t bitrate, bool considerLoss, float lossPercentage, int64_t nowMs)
	{
		MS_TRACE();

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
		// Whether a usable spatial layer has been found, in which case we must not
		// go above the preferred spatial layer. Same criteria as in
		// RecalculateTargetLayers(), so both take the same decision no matter the
		// bitrate of the layer.
		bool usableSpatialLayerFound{ this->provisionalTargetLayers.spatial != -1 };

		for (size_t sIdx{ 0 }; sIdx < this->producerRtpStreams.size(); ++sIdx)
		{
			spatialLayer = static_cast<int16_t>(sIdx);

			// Ignore spatial layers lower than the one we already have.
			if (spatialLayer < this->provisionalTargetLayers.spatial)
			{
				continue;
			}
			// If this is higher than preferred spatial layer, abort unless no lower
			// spatial layer is usable or this is the provisional one (so we may still
			// increase its temporal layer).
			else if (
			  spatialLayer > this->preferredLayers.spatial && usableSpatialLayerFound &&
			  spatialLayer != this->provisionalTargetLayers.spatial)
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

			// We may not yet switch to this spatial layer.
			if (!CanSwitchToSpatialLayer(spatialLayer))
			{
				continue;
			}

			// This spatial layer is usable even if it has no bitrate at all.
			usableSpatialLayerFound = true;

			temporalLayer = 0;

			// Don't consider temporal layers above the preferred one, nor above the
			// ones this stream has.
			const auto maxTemporalLayer = std::min(
			  static_cast<int16_t>(producerRtpStream->GetTemporalLayers() - 1),
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

				requiredBitrate = producerRtpStream->GetTemporalLayerBitrate(nowMs, 0, temporalLayer);

				// This is simulcast so we must subtract the bitrate of the current
				// temporal spatial layer if this is the temporal layer 0 of a higher
				// spatial layer.
				if (
				  requiredBitrate > 0 && temporalLayer == 0 && this->provisionalTargetLayers.spatial > -1 &&
				  spatialLayer > this->provisionalTargetLayers.spatial)
				{
					auto* provisionalProducerRtpStream =
					  this->producerRtpStreams.at(this->provisionalTargetLayers.spatial);
					const int64_t provisionalRequiredBitrate =
					  provisionalProducerRtpStream->GetCumulativeTemporalLayerBitrate(
					    nowMs, 0, this->provisionalTargetLayers.temporal);

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

			// If this is the preferred spatial layer or higher, take it and exit.
			if (spatialLayer >= this->preferredLayers.spatial)
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
		  "setting provisional layers to %" PRIi16 ":%" PRIi16 " [virtual bitrate:%" PRIi64
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

	void SimulcastProducerStreamManager::ApplyLayers()
	{
		MS_TRACE();

		auto provisionalTargetLayers = this->provisionalTargetLayers;

		// Reset provisional target layers.
		this->provisionalTargetLayers.Reset();

		if (provisionalTargetLayers != this->targetLayers)
		{
			UpdateTargetLayers(provisionalTargetLayers.spatial, provisionalTargetLayers.temporal);
		}
	}
#endif

	int64_t SimulcastProducerStreamManager::GetDesiredBitrate(int64_t nowMs) const
	{
		MS_TRACE();

		int64_t desiredBitrate{ 0 };

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

			uint32_t tsOffset{ 0 };

			// Sync our RTP stream's RTP timestamp.
			if (spatialLayer == this->tsReferenceSpatialLayer)
			{
				tsOffset = this->tsReferenceOffset;
			}
			// If this is not the RTP stream we use as TS reference, synchronize its RTP
			// timestamps based on the capture instant of the media.
			else
			{
				// NOTE: The stream of the spatial layer of this packet, which is not always
				// the one of the target spatial layer. A resync may be pending for the
				// current spatial layer while the target one is a different one.
				const auto* producerRtpStream = this->producerRtpStreams.at(spatialLayer);
				const auto alignedTsOffset = GetTimestampOffset(producerRtpStream, packet->GetTimestamp());

				if (alignedTsOffset.has_value())
				{
					// Apply offset. This is the difference that later must be removed from
					// the sending RTP packet.
					tsOffset = alignedTsOffset.value() + this->tsReferenceOffset;
				}
				// The TS reference stream is dead, so it cannot be relied on to align this
				// one. Take this spatial layer as the new TS reference and make its RTP
				// timeline go on after the highest RTP timestamp sent so far as much time
				// as elapsed since the last packet was forwarded, which is the best guess
				// of how far this packet was captured from it.
				else if (CanGiveUpTsReferenceSpatialLayer())
				{
					MS_DEBUG_TAG(
					  simulcast,
					  "TS reference spatial layer %" PRIi16 " is dead and spatial layer %" PRIi16
					  " cannot be aligned to it, using the latter"
					  " as RTP timestamp reference",
					  this->tsReferenceSpatialLayer,
					  spatialLayer);

					// NOTE: Capped below 2^31 ticks, beyond which RTP timestamps that wrap
					// around would take the jump forward as a jump backwards.
					static constexpr int64_t MaxElapsedTs{ (int64_t{ 1 } << 31) - 1 };

					const int64_t nowMs     = this->shared->GetTimeMs();
					const int64_t elapsedMs = nowMs - this->lastPacketForwardedAtMs.value_or(nowMs);
					const auto elapsedTs =
					  static_cast<uint32_t>(std::clamp<int64_t>(elapsedMs * clockRate / 1000, 1, MaxElapsedTs));

					this->tsReferenceSpatialLayer = spatialLayer;
					this->tsReferenceOffset       = packet->GetTimestamp() - maxPacketTs - elapsedTs;

					tsOffset = this->tsReferenceOffset;
				}
				// The capture instant of this stream or the one of the TS reference stream
				// cannot be told yet, so there is no way to align it. The TS reference stream
				// cannot be given up either, since it is alive. Discard the packet and wait,
				// since the Sender Report that is missing is on its way.
				else
				{
					MS_DEBUG_TAG(
					  simulcast,
					  "cannot tell yet the capture instant of spatial layer %" PRIi16
					  " or the one of the TS reference spatial layer %" PRIi16
					  ", discarding packet [ssrc:%" PRIu32 ", seq:%" PRIu16 "]",
					  spatialLayer,
					  this->tsReferenceSpatialLayer,
					  packet->GetSsrc(),
					  packet->GetSequenceNumber());

					// Ask for another key frame since this one is being discarded.
					if (packet->IsKeyFrame())
					{
						RequestKeyFrame();
					}

					// Reset flags since we are discarding the key frame of the target spatial
					// layer, so packets of the current one go on being forwarded meanwhile.
					// NOTE: Not when resyncing the current spatial layer, whose packets must
					// not be forwarded until it is synced.
					if (shouldSwitchCurrentSpatialLayer)
					{
						this->syncRequired       = false;
						this->spatialLayerToSync = -1;
					}

					result.type = RtpPacketProcessResult::Type::SILENT_DROP;

					return result;
				}
			}

			// When switching to a new stream it may happen that the timestamp of this
			// key frame is not higher than the highest timestamp sent to the remote
			// endpoint. If so, apply the minimum extra offset that places it after it
			// for the whole life of this selected Producer stream.
			// NOTE: Nothing has been sent while no packet has been forwarded, so the
			// highest timestamp sent means nothing then.
			if (
			  shouldSwitchCurrentSpatialLayer && this->lastPacketForwardedAtMs.has_value() &&
			  !Utils::Number::IsHigherThan<uint32_t>(packet->GetTimestamp() - tsOffset, maxPacketTs))
			{
				// Max delay in ms we allow for the stream when switching.
				// https://en.wikipedia.org/wiki/Audio-to-video_synchronization#Recommendations
				static constexpr int64_t MaxExtraOffsetMs{ 75 };

				const int64_t maxTsExtraOffset = MaxExtraOffsetMs * clockRate / 1000;

				// NOTE: RTP timestamps wrap around, so the sum is truncated on purpose.
				auto tsExtraOffset =
				  static_cast<uint32_t>(maxPacketTs - (packet->GetTimestamp() - tsOffset) + 1);

				// NOTE: Don't ask for a key frame if already done.
				if (this->keyFrameForTsOffsetRequested)
				{
					// Give up on the capture instant but not on the RTP timeline.
					if (std::cmp_greater(tsExtraOffset, maxTsExtraOffset))
					{
						MS_WARN_TAG(
						  simulcast,
						  "giving up on proper stream switching after got a requested keyframe for "
						  "which still too high RTP timestamp extra offset is needed (%" PRIu32
						  "), applying it anyway",
						  tsExtraOffset);
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

				if (tsExtraOffset > 0)
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

			// If the RTP timestamp reference stream is dead, take the new current
			// spatial layer as reference with the offset it is being sent with, so
			// nothing depends on the dead stream anymore.
			if (this->currentSpatialLayer != this->tsReferenceSpatialLayer && CanGiveUpTsReferenceSpatialLayer())
			{
				MS_DEBUG_TAG(
				  simulcast,
				  "TS reference spatial layer %" PRIi16 " is dead, using current spatial layer %" PRIi16
				  " as RTP timestamp reference",
				  this->tsReferenceSpatialLayer,
				  this->currentSpatialLayer);

				this->tsReferenceSpatialLayer = this->currentSpatialLayer;
				this->tsReferenceOffset       = this->tsOffset;
			}

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

		this->lastPacketForwardedAtMs = this->shared->GetTimeMs();

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

		// While no packet has been forwarded there is no RTP timeline of our stream to
		// keep, so the new target spatial layer becomes the RTP timestamp reference.
		if (newTargetSpatialLayer != -1 && !this->lastPacketForwardedAtMs.has_value())
		{
			MS_DEBUG_TAG(
			  simulcast, "using spatial layer %" PRIi16 " as RTP timestamp reference", newTargetSpatialLayer);

			this->tsReferenceSpatialLayer = newTargetSpatialLayer;
			this->tsReferenceOffset       = 0;
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

		for (size_t sIdx{ 0 }; sIdx < this->producerRtpStreams.size(); ++sIdx)
		{
			auto spatialLayer       = static_cast<int16_t>(sIdx);
			auto* producerRtpStream = this->producerRtpStreams.at(sIdx);
			auto producerScore      = producerRtpStream ? producerRtpStream->GetScore() : 0;

			// Ignore spatial layers for non existing Producer streams or for those
			// with score 0.
			if (producerScore == 0)
			{
				continue;
			}

			// We may not yet switch to this spatial layer.
			if (!CanSwitchToSpatialLayer(spatialLayer))
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

			// If this is the preferred or higher spatial layer take it and exit.
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
			const auto* producerRtpStream = this->producerRtpStreams.at(newTargetLayers.spatial);

			const auto temporalLayers =
			  std::min(producerRtpStream->GetTemporalLayers(), this->encodingContext->GetTemporalLayers());

			newTargetLayers.temporal =
			  std::min(this->preferredLayers.temporal, static_cast<int16_t>(temporalLayers - 1));
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

		UpdateTargetLayers(-1, -1);
	}

	void SimulcastProducerStreamManager::OnPaused()
	{
		MS_TRACE();

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

		// While no packet has been forwarded this spatial layer would become the TS
		// reference one, so there is nothing to align it to.
		if (!this->lastPacketForwardedAtMs.has_value() || spatialLayer == this->tsReferenceSpatialLayer)
		{
			return true;
		}

		const auto* producerRtpStream = this->producerRtpStreams.at(spatialLayer);

		return (
		  GetTimestampOffset(producerRtpStream, producerRtpStream->GetMaxPacketTs()).has_value() ||
		  CanGiveUpTsReferenceSpatialLayer());
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

	bool SimulcastProducerStreamManager::CanGiveUpTsReferenceSpatialLayer() const
	{
		MS_TRACE();

		const auto* producerTsReferenceRtpStream = GetProducerTsReferenceRtpStream();

		if (!producerTsReferenceRtpStream)
		{
			return true;
		}

		return producerTsReferenceRtpStream->GetScore() == 0;
	}

	std::optional<uint32_t> SimulcastProducerStreamManager::GetTimestampOffset(
	  const RTC::RTP::RtpStreamRecv* producerRtpStream, uint32_t ts) const
	{
		MS_TRACE();

		const auto* producerTsReferenceRtpStream = GetProducerTsReferenceRtpStream();

		if (!producerTsReferenceRtpStream)
		{
			return std::nullopt;
		}

		const auto tsReferenceTs = producerTsReferenceRtpStream->GetMaxPacketTs();

		// Both capture instants must come from the same source, so take them from
		// 'abs-capture-time' if both streams can tell it and from Sender Reports
		// otherwise.
		auto tsReferenceCaptureAtUs =
		  producerTsReferenceRtpStream->GetRemoteCaptureAtUsFromAbsCaptureTime(tsReferenceTs);
		auto captureAtUs = producerRtpStream->GetRemoteCaptureAtUsFromAbsCaptureTime(ts);

		if (!tsReferenceCaptureAtUs.has_value() || !captureAtUs.has_value())
		{
			tsReferenceCaptureAtUs =
			  producerTsReferenceRtpStream->GetRemoteCaptureAtUsFromSenderReport(tsReferenceTs);
			captureAtUs = producerRtpStream->GetRemoteCaptureAtUsFromSenderReport(ts);
		}

		if (!tsReferenceCaptureAtUs.has_value() || !captureAtUs.has_value())
		{
			return std::nullopt;
		}

		const int64_t diffUs = captureAtUs.value() - tsReferenceCaptureAtUs.value();
		const int64_t diffTs = (diffUs * producerRtpStream->GetClockRate()) / 1000000;
		const uint32_t newTs = ts - diffTs;

		return newTs - tsReferenceTs;
	}
} // namespace RTC
