#define MS_CLASS "RTC::SimulcastConsumer"
// #define MS_LOG_DEV

#include "RTC/SimulcastConsumer.hpp"
#include "DepLibUV.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "Channel/Notifier.hpp"
#include "RTC/Codecs/Codecs.hpp"

namespace RTC
{
	/* Instance methods. */

	SimulcastConsumer::SimulcastConsumer(
	  const std::string& id, RTC::Consumer::Listener* listener, json& data)
	  : RTC::Consumer::Consumer(id, listener, data, RTC::RtpParameters::Type::SIMULCAST)
	{
		MS_TRACE();

		// Ensure there are N > 1 encodings.
		if (this->consumableRtpEncodings.size() <= 1)
			MS_THROW_TYPE_ERROR("invalid consumableRtpEncodings with size <= 1");

		auto& encoding = this->rtpParameters.encodings[0];

		// Ensure there are as many spatial layers as encodings.
		if (encoding.spatialLayers != this->consumableRtpEncodings.size())
		{
			MS_THROW_TYPE_ERROR("encoding.spatialLayers does not match number of consumableRtpEncodings");
		}

		auto jsonPreferredLayersIt = data.find("preferredLayers");

		// Fill mapMappedSsrcSpatialLayer.
		for (size_t idx{ 0 }; idx < this->consumableRtpEncodings.size(); ++idx)
		{
			auto& encoding = this->consumableRtpEncodings[idx];

			this->mapMappedSsrcSpatialLayer[encoding.ssrc] = static_cast<int16_t>(idx);
		}

		// Set preferredLayers (if given).
		if (jsonPreferredLayersIt != data.end() && jsonPreferredLayersIt->is_object())
		{
			auto jsonSpatialLayerIt  = jsonPreferredLayersIt->find("spatialLayer");
			auto jsonTemporalLayerIt = jsonPreferredLayersIt->find("temporalLayer");

			if (jsonSpatialLayerIt == jsonPreferredLayersIt->end() || !jsonSpatialLayerIt->is_number_unsigned())
			{
				MS_THROW_TYPE_ERROR("missing preferredLayers.spatialLayer");
			}

			this->preferredSpatialLayer = jsonSpatialLayerIt->get<int16_t>();

			if (this->preferredSpatialLayer > encoding.spatialLayers - 1)
				this->preferredSpatialLayer = encoding.spatialLayers - 1;

			if (jsonTemporalLayerIt != jsonPreferredLayersIt->end() && jsonTemporalLayerIt->is_number_unsigned())
			{
				this->preferredTemporalLayer = jsonTemporalLayerIt->get<int16_t>();

				if (this->preferredTemporalLayer > encoding.temporalLayers - 1)
					this->preferredTemporalLayer = encoding.temporalLayers - 1;
			}
			else
			{
				this->preferredTemporalLayer = encoding.temporalLayers - 1;
			}
		}
		else
		{
			// Initially set preferreSpatialLayer and preferredTemporalLayer to the
			// maximum value.
			this->preferredSpatialLayer  = encoding.spatialLayers - 1;
			this->preferredTemporalLayer = encoding.temporalLayers - 1;
		}

		// Reserve space for the Producer RTP streams.
		this->producerRtpStreams.insert(
		  this->producerRtpStreams.begin(), this->consumableRtpEncodings.size(), nullptr);

		// Create RtpStreamSend instance for sending a single stream to the remote.
		CreateRtpStream();
	}

	SimulcastConsumer::~SimulcastConsumer()
	{
		MS_TRACE();

		delete this->rtpStream;
	}

	void SimulcastConsumer::FillJson(json& jsonObject) const
	{
		MS_TRACE();

		// Call the parent method.
		RTC::Consumer::FillJson(jsonObject);

		// Add rtpStream.
		this->rtpStream->FillJson(jsonObject["rtpStream"]);

		// Add preferredSpatialLayer.
		jsonObject["preferredSpatialLayer"] = this->preferredSpatialLayer;

		// Add targetSpatialLayer.
		jsonObject["targetSpatialLayer"] = this->targetSpatialLayer;

		// Add currentSpatialLayer.
		jsonObject["currentSpatialLayer"] = this->currentSpatialLayer;

		// Add preferredTemporalLayer.
		jsonObject["preferredTemporalLayer"] = this->preferredTemporalLayer;

		// Add targetTemporalLayer.
		jsonObject["targetTemporalLayer"] = this->targetTemporalLayer;

		// Add currentTemporalLayer.
		jsonObject["currentTemporalLayer"] = this->currentTemporalLayer;
	}

	void SimulcastConsumer::FillJsonStats(json& jsonArray) const
	{
		MS_TRACE();

		// Add stats of our send stream.
		jsonArray.emplace_back(json::value_t::object);
		this->rtpStream->FillJsonStats(jsonArray[0]);

		// Add stats of our recv stream.
		auto* producerCurrentRtpStream = GetProducerCurrentRtpStream();

		if (producerCurrentRtpStream)
		{
			jsonArray.emplace_back(json::value_t::object);
			producerCurrentRtpStream->FillJsonStats(jsonArray[1]);
		}
	}

	void SimulcastConsumer::FillJsonScore(json& jsonObject) const
	{
		MS_TRACE();

		auto* producerCurrentRtpStream = GetProducerCurrentRtpStream();

		jsonObject["score"] = this->rtpStream->GetScore();

		if (producerCurrentRtpStream)
			jsonObject["producerScore"] = producerCurrentRtpStream->GetScore();
		else
			jsonObject["producerScore"] = 0;
	}

	void SimulcastConsumer::HandleRequest(Channel::Request* request)
	{
		MS_TRACE();

		switch (request->methodId)
		{
			case Channel::Request::MethodId::CONSUMER_REQUEST_KEY_FRAME:
			{
				if (IsActive())
					RequestKeyFrames();

				request->Accept();

				break;
			}

			case Channel::Request::MethodId::CONSUMER_SET_PREFERRED_LAYERS:
			{
				auto previousPreferredSpatialLayer  = this->preferredSpatialLayer;
				auto previousPreferredTemporalLayer = this->preferredTemporalLayer;

				auto jsonSpatialLayerIt  = request->data.find("spatialLayer");
				auto jsonTemporalLayerIt = request->data.find("temporalLayer");

				// Spatial layer.
				if (jsonSpatialLayerIt == request->data.end() || !jsonSpatialLayerIt->is_number_unsigned())
				{
					MS_THROW_TYPE_ERROR("missing spatialLayer");
				}

				this->preferredSpatialLayer = jsonSpatialLayerIt->get<int16_t>();

				if (this->preferredSpatialLayer > this->rtpStream->GetSpatialLayers() - 1)
					this->preferredSpatialLayer = this->rtpStream->GetSpatialLayers() - 1;

				// preferredTemporaLayer is optional.
				if (jsonTemporalLayerIt != request->data.end() && jsonTemporalLayerIt->is_number_unsigned())
				{
					this->preferredTemporalLayer = jsonTemporalLayerIt->get<int16_t>();

					if (this->preferredTemporalLayer > this->rtpStream->GetTemporalLayers() - 1)
						this->preferredTemporalLayer = this->rtpStream->GetTemporalLayers() - 1;
				}
				else
				{
					this->preferredTemporalLayer = this->rtpStream->GetTemporalLayers() - 1;
				}

				MS_DEBUG_DEV(
				  "preferred layers changed to [spatial:%" PRIi16 ", temporal:%" PRIi16 ", consumerId:%s]",
				  this->preferredSpatialLayer,
				  this->preferredTemporalLayer,
				  this->id.c_str());

				request->Accept();

				// clang-format off
				if (
					IsActive() &&
					(
						this->preferredSpatialLayer != previousPreferredSpatialLayer ||
						this->preferredTemporalLayer != previousPreferredTemporalLayer
					)
				)
				// clang-format on
				{
					MayChangeLayers(/*force*/ true);
				}

				break;
			}

			default:
			{
				// Pass it to the parent class.
				RTC::Consumer::HandleRequest(request);
			}
		}
	}

	void SimulcastConsumer::ProducerRtpStream(RTC::RtpStream* rtpStream, uint32_t mappedSsrc)
	{
		MS_TRACE();

		auto it = this->mapMappedSsrcSpatialLayer.find(mappedSsrc);

		MS_ASSERT(it != this->mapMappedSsrcSpatialLayer.end(), "unknown mappedSsrc");

		int16_t spatialLayer = it->second;

		this->producerRtpStreams[spatialLayer] = rtpStream;
	}

	void SimulcastConsumer::ProducerNewRtpStream(RTC::RtpStream* rtpStream, uint32_t mappedSsrc)
	{
		MS_TRACE();

		auto it = this->mapMappedSsrcSpatialLayer.find(mappedSsrc);

		MS_ASSERT(it != this->mapMappedSsrcSpatialLayer.end(), "unknown mappedSsrc");

		int16_t spatialLayer = it->second;

		this->producerRtpStreams[spatialLayer] = rtpStream;

		if (IsActive())
			MayChangeLayers();
	}

	void SimulcastConsumer::ProducerRtpStreamScore(
	  RTC::RtpStream* rtpStream, uint8_t score, uint8_t previousScore)
	{
		MS_TRACE();

		// Emit score event only if the stream whose score changed is the current one.
		if (rtpStream == GetProducerCurrentRtpStream())
			EmitScore();

		if (RTC::Consumer::IsActive())
		{
			// Just check target layers if the stream has died or reborned.
			//
			// clang-format off
			if (
				!this->externallyManagedBitrate ||
				(score == 0 || previousScore == 0)
			)
			// clang-format on
			{
				MayChangeLayers();
			}
		}
	}

	void SimulcastConsumer::SetExternallyManagedBitrate()
	{
		MS_TRACE();

		this->externallyManagedBitrate = true;
	}

	int16_t SimulcastConsumer::GetBitratePriority() const
	{
		MS_TRACE();

		if (!RTC::Consumer::IsActive())
			return 0;

		int16_t prioritySpatialLayer{ 0 };

		// Otherwise, take the maximum available spatial layer up to the preferred
		// one.
		for (size_t idx{ 0 }; idx < this->producerRtpStreams.size(); ++idx)
		{
			auto spatialLayer       = static_cast<int16_t>(idx);
			auto* producerRtpStream = this->producerRtpStreams[idx];

			// Ignore spatial layers for non existing Producer streams or for those
			// with score 0.
			if (!producerRtpStream || producerRtpStream->GetScore() == 0)
				continue;

			// Do not choose a layer greater than the preferred one if we already found
			// an available layer equal or less than the preferred one.
			if (spatialLayer > this->preferredSpatialLayer && prioritySpatialLayer >= -1)
				break;

			// Choose this layer for now.
			prioritySpatialLayer = spatialLayer;
		}

		// Return the choosen spatial layer plus one.
		return prioritySpatialLayer + 1;
	}

	uint32_t SimulcastConsumer::UseAvailableBitrate(uint32_t bitrate)
	{
		MS_TRACE();

		MS_ASSERT(this->externallyManagedBitrate == true, "bitrate is not externally managed");

		if (!RTC::Consumer::IsActive())
			return 0;

		// Calculate virtual available bitrate based on given bitrate and our
		// packet lost fraction.
		uint32_t virtualBitrate;
		auto lossPercentage = this->rtpStream->GetLossPercentage();

		// TODO: We may have to not consider fraction lost with Transport-CC.
		if (lossPercentage < 2)
			virtualBitrate = 1.08 * bitrate;
		else if (lossPercentage > 10)
			virtualBitrate = (1 - 0.5 * (lossPercentage / 100)) * bitrate;
		else
			virtualBitrate = bitrate;

		this->provisionalTargetSpatialLayer  = -1;
		this->provisionalTargetTemporalLayer = -1;

		uint32_t usedBitrate{ 0 };
		uint8_t maxProducerScore{ 0 };
		auto now = DepLibUV::GetTime();

		for (size_t idx{ 0 }; idx < this->producerRtpStreams.size(); ++idx)
		{
			auto spatialLayer       = static_cast<int16_t>(idx);
			auto* producerRtpStream = this->producerRtpStreams[idx];
			auto producerScore      = producerRtpStream ? producerRtpStream->GetScore() : 0;

			// Ignore spatial layers for non existing Producer streams or for those
			// with score 0.
			if (producerScore == 0)
				continue;

			if (producerScore >= maxProducerScore || producerScore >= 7)
			{
				// NOTE: If this is the first valid spatial layer take it. This is because,
				// even if there is no enough bitrate for it, we need to take something.
				if (this->provisionalTargetSpatialLayer == -1)
				{
					this->provisionalTargetSpatialLayer  = spatialLayer;
					this->provisionalTargetTemporalLayer = 0;
				}

				int16_t temporalLayer{ 0 };

				// Check bitrate of every layer.
				for (; temporalLayer < producerRtpStream->GetTemporalLayers(); ++temporalLayer)
				{
					auto requiredBitrate = producerRtpStream->GetBitrate(now, 0, temporalLayer);

					MS_DEBUG_DEV(
					  "testing layers %" PRIi16 ":%" PRIi16 " [virtualBitrate:%" PRIu32
					  ", requiredBitrate:%" PRIu32 "]",
					  spatialLayer,
					  temporalLayer,
					  virtualBitrate,
					  requiredBitrate);

					// If this layer requires more bitrate than the given one, abort the loop
					// (so use the previous chosen layers if any).
					if (requiredBitrate > virtualBitrate)
						goto done;

					// Set provisional layers and used bitrate.
					this->provisionalTargetSpatialLayer  = spatialLayer;
					this->provisionalTargetTemporalLayer = temporalLayer;
					usedBitrate                          = requiredBitrate;

					// If this is the preferred spatial and temporal layer, exit the loops.
					// clang-format off
					if (
						this->provisionalTargetSpatialLayer == this->preferredSpatialLayer &&
						this->provisionalTargetTemporalLayer == this->preferredTemporalLayer &&
						producerScore >= 7
					)
					// clang-format on
					{
						goto done;
					}
				}

				// If this is the preferred or higher spatial layer and has good score,
				// take it and exit.
				if (spatialLayer >= this->preferredSpatialLayer && producerScore >= 7)
					break;
			}
		}

	done:

		MS_DEBUG_2TAGS(
		  bwe,
		  simulcast,
		  "choosing layers %" PRIi16 ":%" PRIi16 " [bitrate:%" PRIu32 ", virtualBitrate:%" PRIu32
		  ", usedBitrate:%" PRIu32 ", consumerId:%s]",
		  this->provisionalTargetSpatialLayer,
		  this->provisionalTargetTemporalLayer,
		  bitrate,
		  virtualBitrate,
		  usedBitrate,
		  this->id.c_str());

		// Must recompute usedBitrate based on given bitrate, virtualBitrate and
		// usedBitrate.
		if (usedBitrate <= bitrate)
			return usedBitrate;
		else if (usedBitrate <= virtualBitrate)
			return bitrate;
		else
			return usedBitrate;
	}

	uint32_t SimulcastConsumer::IncreaseLayer(uint32_t bitrate)
	{
		MS_TRACE();

		MS_ASSERT(this->externallyManagedBitrate == true, "bitrate is not externally managed");

		if (!RTC::Consumer::IsActive())
			return 0;

		// If no target spatial layer is selected, do nothing.
		if (this->provisionalTargetSpatialLayer == -1)
		{
			return 0;
		}
		// If already in the preferred layers, do nothing.
		// clang-format off
		else if (
			this->provisionalTargetSpatialLayer == this->preferredSpatialLayer &&
			this->provisionalTargetTemporalLayer == this->preferredTemporalLayer
		)
		// clang-format on
		{
			return 0;
		}

		// Calculate virtual available bitrate based on given bitrate and our
		// packet lost fraction.
		uint32_t virtualBitrate;
		auto lossPercentage = this->rtpStream->GetLossPercentage();

		// TODO: We may have to not consider fraction lost with Transport-CC.
		if (lossPercentage < 2)
			virtualBitrate = 1.08 * bitrate;
		else if (lossPercentage > 10)
			virtualBitrate = (1 - 0.5 * (lossPercentage / 100)) * bitrate;
		else
			virtualBitrate = bitrate;

		auto spatialLayer      = this->provisionalTargetSpatialLayer;
		auto temporalLayer     = this->provisionalTargetTemporalLayer;
		auto producerRtpStream = this->producerRtpStreams.at(spatialLayer);

		// Can upgrade temporal layer.
		if (temporalLayer < producerRtpStream->GetTemporalLayers() - 1)
		{
			++temporalLayer;
		}
		// Can upgrade spatial layer.
		else if (static_cast<size_t>(spatialLayer) < this->producerRtpStreams.size() - 1)
		{
			producerRtpStream = this->producerRtpStreams.at(++spatialLayer);

			// Producer stream does not exist or it's dead. Exit.
			if (!producerRtpStream || producerRtpStream->GetScore() < 7)
				return 0;

			// Set temporal layer to 0.
			temporalLayer = 0;
		}

		auto now             = DepLibUV::GetTime();
		auto requiredBitrate = producerRtpStream->GetLayerBitrate(now, 0, temporalLayer);

		// No luck.
		if (requiredBitrate > virtualBitrate)
			return 0;

		// Set provisional layers.
		this->provisionalTargetSpatialLayer  = spatialLayer;
		this->provisionalTargetTemporalLayer = temporalLayer;

		MS_DEBUG_DEV(
		  "upgrading to layers %" PRIi16 ":%" PRIi16 " [virtualBitrate:%" PRIu32
		  ", requiredBitrate:%" PRIu32 "]",
		  this->provisionalTargetSpatialLayer,
		  this->provisionalTargetTemporalLayer,
		  virtualBitrate,
		  requiredBitrate);

		if (requiredBitrate <= bitrate)
			return requiredBitrate;
		else if (requiredBitrate <= virtualBitrate)
			return bitrate;
		else
			return requiredBitrate; // NOTE: This cannot happen.
	}

	void SimulcastConsumer::ApplyLayers()
	{
		MS_TRACE();

		MS_ASSERT(this->externallyManagedBitrate == true, "bitrate is not externally managed");

		if (!RTC::Consumer::IsActive())
			return;

		// clang-format off
		if (
			this->provisionalTargetSpatialLayer != this->targetSpatialLayer ||
			this->provisionalTargetTemporalLayer != this->targetTemporalLayer
		)
		// clang-format on
		{
			UpdateTargetLayers(this->provisionalTargetSpatialLayer, this->provisionalTargetTemporalLayer);
		}
	}

	void SimulcastConsumer::SendRtpPacket(RTC::RtpPacket* packet)
	{
		MS_TRACE();

		if (!IsActive())
			return;

		auto payloadType = packet->GetPayloadType();

		// NOTE: This may happen if this Consumer supports just some codecs of those
		// in the corresponding Producer.
		if (this->supportedCodecPayloadTypes.find(payloadType) == this->supportedCodecPayloadTypes.end())
		{
			MS_DEBUG_DEV("payload type not supported [payloadType:%" PRIu8 "]", payloadType);

			return;
		}

		auto spatialLayer = this->mapMappedSsrcSpatialLayer.at(packet->GetSsrc());

		// Check whether this is the packet we are waiting for in order to update
		// the current spatial layer.
		if (this->currentSpatialLayer != this->targetSpatialLayer && spatialLayer == this->targetSpatialLayer)
		{
			// Ignore if key frame is supported and this is not one.
			if (this->keyFrameSupported && !packet->IsKeyFrame())
				return;

			// Update current spatial and temporal layers.
			UpdateCurrentLayers();

			// Need to resync the stream.
			this->syncRequired = true;
		}

		// If the packet belongs to different spatial layer than the one being sent,
		// drop it.
		if (spatialLayer != this->currentSpatialLayer)
			return;

		// If we need to sync, support key frames and this is not a key frame, ignore
		// the packet.
		if (this->syncRequired && this->keyFrameSupported && !packet->IsKeyFrame())
			return;

		// Whether this is the first packet after re-sync.
		bool isSyncPacket = this->syncRequired;

		// Sync sequence number and timestamp if required.
		if (isSyncPacket)
		{
			if (packet->IsKeyFrame())
				MS_DEBUG_TAG(rtp, "sync key frame received");

			this->rtpSeqManager.Sync(packet->GetSequenceNumber());
			this->rtpTimestampManager.Sync(packet->GetTimestamp());

			// Calculate RTP timestamp diff between now and last sent RTP packet.
			if (this->rtpStream->GetMaxPacketMs() != 0u)
			{
				auto now    = DepLibUV::GetTime();
				auto diffMs = now - this->rtpStream->GetMaxPacketMs();
				auto diffTs = diffMs * this->rtpStream->GetClockRate() / 1000;

				this->rtpTimestampManager.Offset(diffTs);
			}

			if (this->encodingContext)
				this->encodingContext->SyncRequired();

			this->syncRequired = false;
		}

		// Rewrite payload if needed. Drop packet if necessary.
		if (this->encodingContext && !packet->EncodePayload(this->encodingContext.get()))
		{
			this->rtpSeqManager.Drop(packet->GetSequenceNumber());
			this->rtpTimestampManager.Drop(packet->GetTimestamp());

			return;
		}

		// Update current temporal layer if the packet honors the target temporal layer
		// (just if this packet belongs to the target spatial layer).
		if (
		  this->currentSpatialLayer == this->targetSpatialLayer &&
		  this->currentTemporalLayer != this->targetTemporalLayer &&
		  packet->GetTemporalLayer() == this->targetTemporalLayer)
		{
			UpdateCurrentLayers();
		}

		// Update RTP seq number and timestamp.
		uint16_t seq;
		uint32_t timestamp;

		this->rtpSeqManager.Input(packet->GetSequenceNumber(), seq);
		this->rtpTimestampManager.Input(packet->GetTimestamp(), timestamp);

		// Save original packet fields.
		auto origSsrc      = packet->GetSsrc();
		auto origSeq       = packet->GetSequenceNumber();
		auto origTimestamp = packet->GetTimestamp();

		// Rewrite packet.
		packet->SetSsrc(this->rtpParameters.encodings[0].ssrc);
		packet->SetSequenceNumber(seq);
		packet->SetTimestamp(timestamp);

		if (isSyncPacket)
		{
			MS_DEBUG_TAG(
			  rtp,
			  "sending sync packet [ssrc:%" PRIu32 ", seq:%" PRIu16 ", ts:%" PRIu32
			  "] from original [ssrc:%" PRIu32 ", seq:%" PRIu16 ", ts:%" PRIu32 "]",
			  packet->GetSsrc(),
			  packet->GetSequenceNumber(),
			  packet->GetTimestamp(),
			  origSsrc,
			  origSeq,
			  origTimestamp);
		}

		// Process the packet.
		if (this->rtpStream->ReceivePacket(packet))
		{
			// Send the packet.
			this->listener->OnConsumerSendRtpPacket(this, packet);
		}
		else
		{
			MS_WARN_TAG(
			  rtp,
			  "failed to send packet [ssrc:%" PRIu32 ", seq:%" PRIu16 ", ts:%" PRIu32
			  "] from original [ssrc:%" PRIu32 ", seq:%" PRIu16 ", ts:%" PRIu32 "]",
			  packet->GetSsrc(),
			  packet->GetSequenceNumber(),
			  packet->GetTimestamp(),
			  origSsrc,
			  origSeq,
			  origTimestamp);
		}

		// Restore packet fields.
		packet->SetSsrc(origSsrc);
		packet->SetSequenceNumber(origSeq);
		packet->SetTimestamp(origTimestamp);

		// Restore the original payload if needed.
		if (this->encodingContext)
			packet->RestorePayload();
	}

	void SimulcastConsumer::GetRtcp(
	  RTC::RTCP::CompoundPacket* packet, RTC::RtpStreamSend* rtpStream, uint64_t now)
	{
		MS_TRACE();

		MS_ASSERT(rtpStream == this->rtpStream, "RTP stream does not match");

		if (static_cast<float>((now - this->lastRtcpSentTime) * 1.15) < this->maxRtcpInterval)
			return;

		auto* report = this->rtpStream->GetRtcpSenderReport(now);

		if (!report)
			return;

		packet->AddSenderReport(report);

		// Build SDES chunk for this sender.
		auto* sdesChunk = this->rtpStream->GetRtcpSdesChunk();

		packet->AddSdesChunk(sdesChunk);

		this->lastRtcpSentTime = now;
	}

	void SimulcastConsumer::NeedWorstRemoteFractionLost(
	  uint32_t /*mappedSsrc*/, uint8_t& worstRemoteFractionLost)
	{
		MS_TRACE();

		if (!IsActive())
			return;

		auto fractionLost = this->rtpStream->GetFractionLost();

		// If our fraction lost is worse than the given one, update it.
		if (fractionLost > worstRemoteFractionLost)
			worstRemoteFractionLost = fractionLost;
	}

	void SimulcastConsumer::ReceiveNack(RTC::RTCP::FeedbackRtpNackPacket* nackPacket)
	{
		MS_TRACE();

		if (!IsActive())
			return;

		this->rtpStream->ReceiveNack(nackPacket);
	}

	void SimulcastConsumer::ReceiveKeyFrameRequest(
	  RTC::RTCP::FeedbackPs::MessageType messageType, uint32_t /*ssrc*/)
	{
		MS_TRACE();

		this->rtpStream->ReceiveKeyFrameRequest(messageType);

		if (IsActive())
			RequestKeyFrameForCurrentSpatialLayer();
	}

	void SimulcastConsumer::ReceiveRtcpReceiverReport(RTC::RTCP::ReceiverReport* report)
	{
		MS_TRACE();

		this->rtpStream->ReceiveRtcpReceiverReport(report);
	}

	uint32_t SimulcastConsumer::GetTransmissionRate(uint64_t now)
	{
		MS_TRACE();

		if (!IsActive())
			return 0u;

		return this->rtpStream->GetBitrate(now);
	}

	void SimulcastConsumer::UserOnTransportConnected()
	{
		MS_TRACE();

		this->syncRequired = true;

		if (IsActive())
			MayChangeLayers();
	}

	void SimulcastConsumer::UserOnTransportDisconnected()
	{
		MS_TRACE();

		this->rtpStream->Pause();

		UpdateTargetLayers(-1, -1);
	}

	void SimulcastConsumer::UserOnPaused()
	{
		MS_TRACE();

		this->rtpStream->Pause();

		UpdateTargetLayers(-1, -1);

		// Tell the transport so it can distribute available bitrate into other
		// consumers.
		if (this->externallyManagedBitrate)
			this->listener->OnConsumerNeedBitrateChange(this);
	}

	void SimulcastConsumer::UserOnResumed()
	{
		MS_TRACE();

		this->syncRequired = true;

		if (IsActive())
			MayChangeLayers();
	}

	void SimulcastConsumer::CreateRtpStream()
	{
		MS_TRACE();

		auto& encoding   = this->rtpParameters.encodings[0];
		auto* mediaCodec = this->rtpParameters.GetCodecForEncoding(encoding);

		MS_DEBUG_TAG(
		  rtp, "[ssrc:%" PRIu32 ", payloadType:%" PRIu8 "]", encoding.ssrc, mediaCodec->payloadType);

		// Set stream params.
		RTC::RtpStream::Params params;

		params.ssrc           = encoding.ssrc;
		params.payloadType    = mediaCodec->payloadType;
		params.mimeType       = mediaCodec->mimeType;
		params.clockRate      = mediaCodec->clockRate;
		params.cname          = this->rtpParameters.rtcp.cname;
		params.spatialLayers  = encoding.spatialLayers;
		params.temporalLayers = encoding.temporalLayers;

		// Check in band FEC in codec parameters.
		if (mediaCodec->parameters.HasInteger("useinbandfec") && mediaCodec->parameters.GetInteger("useinbandfec") == 1)
		{
			MS_DEBUG_TAG(rtp, "in band FEC enabled");

			params.useInBandFec = true;
		}

		// Check DTX in codec parameters.
		if (mediaCodec->parameters.HasInteger("usedtx") && mediaCodec->parameters.GetInteger("usedtx") == 1)
		{
			MS_DEBUG_TAG(rtp, "DTX enabled");

			params.useDtx = true;
		}

		// Check DTX in the encoding.
		if (encoding.dtx)
		{
			MS_DEBUG_TAG(rtp, "DTX enabled");

			params.useDtx = true;
		}

		for (auto& fb : mediaCodec->rtcpFeedback)
		{
			if (!params.useNack && fb.type == "nack" && fb.parameter == "")
			{
				MS_DEBUG_2TAGS(rtp, rtcp, "NACK supported");

				params.useNack = true;
			}
			else if (!params.usePli && fb.type == "nack" && fb.parameter == "pli")
			{
				MS_DEBUG_2TAGS(rtp, rtcp, "PLI supported");

				params.usePli = true;
			}
			else if (!params.useFir && fb.type == "ccm" && fb.parameter == "fir")
			{
				MS_DEBUG_2TAGS(rtp, rtcp, "FIR supported");

				params.useFir = true;
			}
		}

		// Create a RtpStreamSend for sending a single media stream.
		size_t bufferSize = params.useNack ? 1500 : 0;

		this->rtpStream = new RTC::RtpStreamSend(this, params, bufferSize);
		this->rtpStreams.push_back(this->rtpStream);

		// If the Consumer is paused, tell the RtpStreamSend.
		if (IsPaused() || IsProducerPaused())
			this->rtpStream->Pause();

		auto* rtxCodec = this->rtpParameters.GetRtxCodecForEncoding(encoding);

		if (rtxCodec && encoding.hasRtx)
			this->rtpStream->SetRtx(rtxCodec->payloadType, encoding.rtx.ssrc);

		this->keyFrameSupported = RTC::Codecs::CanBeKeyFrame(mediaCodec->mimeType);

		this->encodingContext.reset(RTC::Codecs::GetEncodingContext(mediaCodec->mimeType));
	}

	void SimulcastConsumer::RequestKeyFrames()
	{
		MS_TRACE();

		if (this->kind != RTC::Media::Kind::VIDEO)
			return;

		auto* producerTargetRtpStream  = GetProducerTargetRtpStream();
		auto* producerCurrentRtpStream = GetProducerCurrentRtpStream();

		if (producerTargetRtpStream)
		{
			auto mappedSsrc = this->consumableRtpEncodings[this->targetSpatialLayer].ssrc;

			this->listener->OnConsumerKeyFrameRequested(this, mappedSsrc);
		}

		if (producerCurrentRtpStream && producerCurrentRtpStream != producerTargetRtpStream)
		{
			auto mappedSsrc = this->consumableRtpEncodings[this->currentSpatialLayer].ssrc;

			this->listener->OnConsumerKeyFrameRequested(this, mappedSsrc);
		}
	}

	void SimulcastConsumer::RequestKeyFrameForTargetSpatialLayer()
	{
		MS_TRACE();

		if (this->kind != RTC::Media::Kind::VIDEO)
			return;

		auto* producerTargetRtpStream = GetProducerTargetRtpStream();

		if (!producerTargetRtpStream)
			return;

		auto mappedSsrc = this->consumableRtpEncodings[this->targetSpatialLayer].ssrc;

		this->listener->OnConsumerKeyFrameRequested(this, mappedSsrc);
	}

	void SimulcastConsumer::RequestKeyFrameForCurrentSpatialLayer()
	{
		MS_TRACE();

		if (this->kind != RTC::Media::Kind::VIDEO)
			return;

		auto* producerCurrentRtpStream = GetProducerCurrentRtpStream();

		if (!producerCurrentRtpStream)
			return;

		auto mappedSsrc = this->consumableRtpEncodings[this->currentSpatialLayer].ssrc;

		this->listener->OnConsumerKeyFrameRequested(this, mappedSsrc);
	}

	void SimulcastConsumer::MayChangeLayers(bool force)
	{
		MS_TRACE();

		int16_t newTargetSpatialLayer;
		int16_t newTargetTemporalLayer;

		if (RecalculateTargetLayers(newTargetSpatialLayer, newTargetTemporalLayer))
		{
			// If bitrate externally managed, don't bother the transport unless
			// the newTargetSpatialLayer has changed (or force is true).
			// This is because, if bitrate is externally managed, the target temporal
			// layer is managed by the available given bitrate so the transport
			// will let us change it when it considers.
			if (this->externallyManagedBitrate)
			{
				if (newTargetSpatialLayer != this->targetSpatialLayer || force)
					this->listener->OnConsumerNeedBitrateChange(this);
			}
			else
			{
				UpdateTargetLayers(newTargetSpatialLayer, newTargetTemporalLayer);
			}
		}
	}

	bool SimulcastConsumer::RecalculateTargetLayers(
	  int16_t& newTargetSpatialLayer, int16_t& newTargetTemporalLayer) const
	{
		MS_TRACE();

		// Start with no layers.
		newTargetSpatialLayer  = -1;
		newTargetTemporalLayer = -1;

		uint8_t maxProducerScore{ 0 };

		for (size_t idx{ 0 }; idx < this->producerRtpStreams.size(); ++idx)
		{
			auto spatialLayer       = static_cast<int16_t>(idx);
			auto* producerRtpStream = this->producerRtpStreams[idx];
			auto producerScore      = producerRtpStream ? producerRtpStream->GetScore() : 0;

			// Ignore spatial layers for non existing Producer streams or for those
			// with score 0.
			if (producerScore == 0)
				continue;

			if (producerScore >= maxProducerScore || producerScore >= 7)
			{
				newTargetSpatialLayer = spatialLayer;
				maxProducerScore      = producerScore;

				// If this is the preferred or higher spatial layer and has good score,
				// take it and exit.
				if (spatialLayer >= this->preferredSpatialLayer && producerScore >= 7)
					break;
			}
		}

		if (newTargetSpatialLayer != -1)
		{
			if (newTargetSpatialLayer == this->preferredSpatialLayer)
				newTargetTemporalLayer = this->preferredTemporalLayer;
			else if (newTargetSpatialLayer < this->preferredSpatialLayer)
				newTargetTemporalLayer = this->rtpStream->GetTemporalLayers() - 1;
			else
				newTargetTemporalLayer = 0;
		}

		// Return true if any target layer changed.
		// clang-format off
		return (
			newTargetSpatialLayer != this->targetSpatialLayer ||
			newTargetTemporalLayer != this->targetTemporalLayer
		);
		// clang-format on
	}

	void SimulcastConsumer::UpdateTargetLayers(int16_t newTargetSpatialLayer, int16_t newTargetTemporalLayer)
	{
		MS_TRACE();

		if (newTargetSpatialLayer == -1)
		{
			// Unset current and target layers.
			this->targetSpatialLayer   = -1;
			this->targetTemporalLayer  = -1;
			this->currentSpatialLayer  = -1;
			this->currentTemporalLayer = -1;

			if (this->encodingContext)
			{
				this->encodingContext->preferences.temporalLayer = this->rtpStream->GetTemporalLayers() - 1;
			}

			MS_DEBUG_TAG(
			  simulcast,
			  "target layers changed [spatial:%" PRIi16 ", temporal:%" PRIi16 ", consumerId:%s]",
			  this->targetSpatialLayer,
			  this->targetTemporalLayer,
			  this->id.c_str());

			EmitLayersChange();

			return;
		}

		this->targetSpatialLayer  = newTargetSpatialLayer;
		this->targetTemporalLayer = newTargetTemporalLayer;

		// If the new target spatial layer matches the current one, apply the new
		// target temporal layer now.
		if (this->encodingContext && this->targetSpatialLayer == this->currentSpatialLayer)
		{
			this->encodingContext->preferences.temporalLayer = this->targetTemporalLayer;
		}

		MS_DEBUG_TAG(
		  simulcast,
		  "target layers changed [spatial:%" PRIi16 ", temporal:%" PRIi16 ", consumerId:%s]",
		  this->targetSpatialLayer,
		  this->targetTemporalLayer,
		  this->id.c_str());

		// If the target spatial layer is different than the current one, request
		// a key frame.
		if (this->targetSpatialLayer != this->currentSpatialLayer)
			RequestKeyFrameForTargetSpatialLayer();
	}

	void SimulcastConsumer::UpdateCurrentLayers()
	{
		MS_TRACE();

		bool emitScore{ false };

		// Reset the score of our RtpStream to 10 if spatial layer changed.
		if (this->targetSpatialLayer != this->currentSpatialLayer)
		{
			this->rtpStream->ResetScore(10, false);

			emitScore = true;
		}

		this->currentSpatialLayer  = this->targetSpatialLayer;
		this->currentTemporalLayer = this->targetTemporalLayer;

		if (this->encodingContext)
			this->encodingContext->preferences.temporalLayer = this->targetTemporalLayer;

		MS_DEBUG_DEV(
		  "current layers changed to [spatial:%" PRIi16 ", temporal:%" PRIi16 ", consumerId:%s]",
		  this->currentSpatialLayer,
		  this->currentTemporalLayer,
		  this->id.c_str());

		// Emit the layersChange event.
		EmitLayersChange();

		// Emit the score event (just if spatial layer changed).
		if (emitScore)
			EmitScore();
	}

	inline void SimulcastConsumer::EmitScore() const
	{
		MS_TRACE();

		json data = json::object();

		FillJsonScore(data);

		Channel::Notifier::Emit(this->id, "score", data);
	}

	inline void SimulcastConsumer::EmitLayersChange() const
	{
		MS_TRACE();

		json data = json::object();

		if (this->currentSpatialLayer >= 0 && this->currentTemporalLayer >= 0)
		{
			data["spatialLayer"]  = this->currentSpatialLayer;
			data["temporalLayer"] = this->currentTemporalLayer;
		}
		else
		{
			data = nullptr;
		}

		Channel::Notifier::Emit(this->id, "layerschange", data);
	}

	inline RTC::RtpStream* SimulcastConsumer::GetProducerCurrentRtpStream() const
	{
		MS_TRACE();

		if (this->currentSpatialLayer == -1)
			return nullptr;

		// This may return nullptr.
		return this->producerRtpStreams.at(this->currentSpatialLayer);
	}

	inline RTC::RtpStream* SimulcastConsumer::GetProducerTargetRtpStream() const
	{
		MS_TRACE();

		if (this->targetSpatialLayer == -1)
			return nullptr;

		// This may return nullptr.
		return this->producerRtpStreams.at(this->targetSpatialLayer);
	}

	inline void SimulcastConsumer::OnRtpStreamScore(
	  RTC::RtpStream* /*rtpStream*/, uint8_t /*score*/, uint8_t /*previousScore*/)
	{
		MS_TRACE();

		// Emit the score event.
		EmitScore();

		if (IsActive())
		{
			// Just check target layers if our bitrate is not externally managed.
			// NOTE: For now this is a bit useless since, when locally managed, we do
			// not check the Consumer score at all.
			if (!this->externallyManagedBitrate)
				MayChangeLayers();
		}
	}

	inline void SimulcastConsumer::OnRtpStreamRetransmitRtpPacket(
	  RTC::RtpStreamSend* /*rtpStream*/, RTC::RtpPacket* packet)
	{
		MS_TRACE();

		this->listener->OnConsumerRetransmitRtpPacket(this, packet);
	}
} // namespace RTC
