#define MS_CLASS "RTC::SimulcastConsumer"
// #define MS_LOG_DEV

#include "RTC/SimulcastConsumer.hpp"
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

		// Set the RTCP report generation interval.
		if (this->kind == RTC::Media::Kind::AUDIO)
			this->maxRtcpInterval = RTC::RTCP::MaxAudioIntervalMs;
		else
			this->maxRtcpInterval = RTC::RTCP::MaxVideoIntervalMs;

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

		if (producerCurrentRtpStream)
			jsonObject["producer"] = producerCurrentRtpStream->GetScore();
		else
			jsonObject["producer"] = 0;

		jsonObject["consumer"] = this->rtpStream->GetScore();
	}

	void SimulcastConsumer::HandleRequest(Channel::Request* request)
	{
		MS_TRACE();

		switch (request->methodId)
		{
			case Channel::Request::MethodId::CONSUMER_REQUEST_KEY_FRAME:
			{
				RequestKeyFrame();

				request->Accept();

				break;
			}

			case Channel::Request::MethodId::CONSUMER_SET_PREFERRED_LAYERS:
			{
				auto jsonSpatialLayerIt  = request->data.find("spatialLayer");
				auto jsonTemporalLayerIt = request->data.find("temporalLayer");

				// Spatial layer.
				if (jsonSpatialLayerIt == request->data.end() || !jsonSpatialLayerIt->is_number_unsigned())
				{
					MS_THROW_TYPE_ERROR("missing spatialLayer");
				}

				auto preferredSpatialLayer = jsonSpatialLayerIt->get<int16_t>();

				if (preferredSpatialLayer > this->rtpStream->GetSpatialLayers() - 1)
					preferredSpatialLayer = this->rtpStream->GetSpatialLayers() - 1;

				// Temporal layer.
				uint16_t preferredTemporalLayer;

				// perferredTemporaLayer is optional.
				if (jsonTemporalLayerIt != request->data.end() && jsonTemporalLayerIt->is_number_unsigned())
				{
					preferredTemporalLayer = jsonTemporalLayerIt->get<int16_t>();
				}
				else
				{
					preferredTemporalLayer = this->preferredTemporalLayer;
				}

				this->preferredSpatialLayer  = preferredSpatialLayer;
				this->preferredTemporalLayer = preferredTemporalLayer;

				this->targetTemporalLayer = preferredTemporalLayer;

				if (this->currentSpatialLayer == this->targetSpatialLayer && this->encodingContext)
					this->encodingContext->preferences.temporalLayer = this->targetTemporalLayer;

				MS_DEBUG_DEV(
				  "preferred layers changed to [spatial:%" PRIi16 ", temporal:%" PRIi16 ", consumerId:%s]",
				  this->preferredSpatialLayer,
				  this->preferredTemporalLayer,
				  this->id.c_str());

				if (IsActive())
					this->listener->OnConsumerNeedBandwidth(this);

				request->Accept();

				break;
			}

			default:
			{
				// Pass it to the parent class.
				RTC::Consumer::HandleRequest(request);
			}
		}
	}

	uint32_t SimulcastConsumer::UseBandwidth(uint32_t availableBandwidth)
	{
		MS_TRACE();

		if (IsActive())
		{
			// TODO: Here we should be able to use the best layer we can.

			RecalculateTargetSpatialLayer();
		}

		// TODO.
		return 0;
	}

	void SimulcastConsumer::ProducerRtpStream(RTC::RtpStreamRecv* rtpStream, uint32_t mappedSsrc)
	{
		MS_TRACE();

		auto it = this->mapMappedSsrcSpatialLayer.find(mappedSsrc);

		MS_ASSERT(it != this->mapMappedSsrcSpatialLayer.end(), "unknown mappedSsrc");

		int16_t spatialLayer = it->second;

		this->producerRtpStreams[spatialLayer] = rtpStream;

		// Emit the score event.
		EmitScore();
	}

	void SimulcastConsumer::ProducerNewRtpStream(RTC::RtpStreamRecv* rtpStream, uint32_t mappedSsrc)
	{
		MS_TRACE();

		auto it = this->mapMappedSsrcSpatialLayer.find(mappedSsrc);

		MS_ASSERT(it != this->mapMappedSsrcSpatialLayer.end(), "unknown mappedSsrc");

		int16_t spatialLayer = it->second;

		this->producerRtpStreams[spatialLayer] = rtpStream;

		// If the Consumer is active and this is the first Producer RtpStream, we
		// need to ask for bandwidth.
		//
		// clang-format off
		if (
			IsActive() &&
			std::none_of(
				this->producerRtpStreams.begin(), this->producerRtpStreams.end(), [](const RTC::RtpStream* rtpStream)
				{
					return rtpStream != nullptr;
				})
		)
		// clang-format on
		{
			this->listener->OnConsumerNeedBandwidth(this);
		}
		// TODO: We should never upgrade layers by ourselves.
		else if (IsActive())
		{
			RecalculateTargetSpatialLayer();
		}

		// Emit the score event.
		EmitScore();
	}

	void SimulcastConsumer::ProducerRtpStreamScore(RTC::RtpStreamRecv* /*rtpStream*/, uint8_t /*score*/)
	{
		MS_TRACE();

		// TODO: NO, we just can downgrade layers.
		if (IsActive())
			RecalculateTargetSpatialLayer();

		// Emit the score event.
		EmitScore();
	}

	void SimulcastConsumer::SendRtpPacket(RTC::RtpPacket* packet)
	{
		MS_TRACE();

		if (!IsActive())
			return;

		// Map the payload type.
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

			// Change current spatial layer.
			UpdateLayers();

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

		// TODO: Not sure how to deal with it, but if this happens (and we drop the packet)
		// we shouldn't have unset the syncRequired flag, etc.
		//
		// Rewrite payload if needed. Drop packet if necessary.
		if (this->encodingContext && !packet->EncodePayload(this->encodingContext.get()))
		{
			this->rtpSeqManager.Drop(packet->GetSequenceNumber());
			this->rtpTimestampManager.Drop(packet->GetTimestamp());

			return;
		}

		// Update temporal layer only if we are using the target spatial layer.
		if (
		  this->currentSpatialLayer == this->targetSpatialLayer &&
		  this->currentTemporalLayer != this->targetTemporalLayer &&
		  packet->GetTemporalLayer() == this->targetTemporalLayer)
		{
			UpdateLayers();
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
			  "] from original [seq:%" PRIu16 ", ts:%" PRIu32 "]",
			  packet->GetSsrc(),
			  packet->GetSequenceNumber(),
			  packet->GetTimestamp(),
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
			  "] from original [seq:%" PRIu16 ", ts:%" PRIu32 "]",
			  packet->GetSsrc(),
			  packet->GetSequenceNumber(),
			  packet->GetTimestamp(),
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

	void SimulcastConsumer::GetRtcp(RTC::RTCP::CompoundPacket* packet, uint64_t now)
	{
		MS_TRACE();

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

	void SimulcastConsumer::ReceiveKeyFrameRequest(RTC::RTCP::FeedbackPs::MessageType messageType)
	{
		MS_TRACE();

		this->rtpStream->ReceiveKeyFrameRequest(messageType);

		RequestKeyFrame();
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

	float SimulcastConsumer::GetLossPercentage() const
	{
		MS_TRACE();

		auto* producerCurrentRtpStream = GetProducerCurrentRtpStream();

		if (!IsActive() || !producerCurrentRtpStream)
			return 0;

		if (producerCurrentRtpStream->GetLossPercentage() >= this->rtpStream->GetLossPercentage())
		{
			return 0;
		}
		else
		{
			return this->rtpStream->GetLossPercentage() - producerCurrentRtpStream->GetLossPercentage();
		}
	}

	void SimulcastConsumer::Paused()
	{
		MS_TRACE();

		this->rtpStream->Pause();

		// Unset current and target layers.
		this->targetSpatialLayer  = -1;
		this->currentSpatialLayer = -1;
	}

	void SimulcastConsumer::Resumed()
	{
		MS_TRACE();

		this->rtpStream->Resume();

		// We need to sync and wait for a key frame (if supported). Otherwise the
		// receiver will request lot of NACKs due to unknown RTP packets.
		this->syncRequired = true;

		// We need to ask the Transport for bandwidth.
		if (IsActive())
			this->listener->OnConsumerNeedBandwidth(this);
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

		// If the Consumer is paused, tell the RtpStreamSend.
		if (IsPaused() || IsProducerPaused())
			this->rtpStream->Pause();

		auto* rtxCodec = this->rtpParameters.GetRtxCodecForEncoding(encoding);

		if (rtxCodec && encoding.hasRtx)
			this->rtpStream->SetRtx(rtxCodec->payloadType, encoding.rtx.ssrc);

		this->keyFrameSupported = Codecs::CanBeKeyFrame(mediaCodec->mimeType);

		this->encodingContext.reset(RTC::Codecs::GetEncodingContext(mediaCodec->mimeType));
	}

	void SimulcastConsumer::RequestKeyFrame()
	{
		MS_TRACE();

		if (!IsActive() || this->kind != RTC::Media::Kind::VIDEO)
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

	inline void SimulcastConsumer::EmitScore() const
	{
		MS_TRACE();

		json data = json::object();

		FillJsonScore(data);

		Channel::Notifier::Emit(this->id, "score", data);
	}

	void SimulcastConsumer::UpdateLayers()
	{
		MS_TRACE();
		if (this->targetSpatialLayer == this->currentSpatialLayer && this->targetTemporalLayer == this->currentTemporalLayer)
			return;

		// Reset the score of our RtpStream to 10 if spatial layer changed.
		if (this->targetSpatialLayer != this->currentSpatialLayer)
			this->rtpStream->ResetScore(10, false);

		this->currentSpatialLayer  = this->targetSpatialLayer;
		this->currentTemporalLayer = this->targetTemporalLayer;

		if (this->encodingContext)
			this->encodingContext->preferences.temporalLayer = this->targetTemporalLayer;

		MS_DEBUG_DEV(
		  "current layers changed to [spatial:%" PRIi16 ", temporal:%" PRIi16 ", consumerId:%s]",
		  this->currentSpatialLayer,
		  this->currentTemporalLayer,
		  this->id.c_str());

		json data(json::object());

		data["spatialLayer"]  = this->currentSpatialLayer;
		data["temporalLayer"] = this->currentTemporalLayer;

		Channel::Notifier::Emit(this->id, "layerschange", data);

		// Emit the score event.
		EmitScore();
	}

	void SimulcastConsumer::RecalculateTargetSpatialLayer()
	{
		MS_TRACE();

		int16_t newTargetSpatialLayer{ -1 };

		// No current spatial layer, select the highest possible.
		if (this->currentSpatialLayer == -1)
		{
			MS_DEBUG_TAG(simulcast, "no current spatial layer, selecting the highest possible");

			uint8_t maxScore = 0;

			// Ignore spatial layers higher than the preferred one.
			for (int idx = this->preferredSpatialLayer; idx >= 0; --idx)
			{
				auto spatialLayer       = static_cast<int16_t>(idx);
				auto* producerRtpStream = this->producerRtpStreams[idx];

				// Ignore spatial layers for non existing Producer streams.
				if (!producerRtpStream)
					continue;

				if (producerRtpStream->GetScore() >= maxScore)
				{
					maxScore              = producerRtpStream->GetScore();
					newTargetSpatialLayer = spatialLayer;

					if (maxScore > 7)
					{
						MS_DEBUG_TAG(simulcast, "found Producer RtpStream with score > 7, keep it");

						break;
					}
				}
			}
		}
		// Downgrade is desired.
		else if (this->preferredSpatialLayer < this->currentSpatialLayer)
		{
			MS_DEBUG_TAG(simulcast, "trying to downgrade current spatial layer to preferred one");

			int16_t idx      = this->preferredSpatialLayer;
			uint8_t maxScore = 0;

			for (; idx < this->currentSpatialLayer; ++idx)
			{
				auto spatialLayer       = static_cast<int16_t>(idx);
				auto* producerRtpStream = this->producerRtpStreams[idx];

				// Ignore spatial layers for non existing Producer streams.
				if (!producerRtpStream)
					continue;

				if (producerRtpStream->GetScore() >= maxScore)
				{
					maxScore              = producerRtpStream->GetScore();
					newTargetSpatialLayer = spatialLayer;

					if (maxScore > 7)
					{
						MS_DEBUG_TAG(simulcast, "found Producer RtpStream with score > 7, keep it");

						break;
					}
				}
			}
		}
		// Downgrade is needed.
		else if (
		  !this->GetProducerCurrentRtpStream() || this->GetProducerCurrentRtpStream()->GetScore() < 7 ||
		  this->rtpStream->GetScore() <= 6)
		{
			MS_DEBUG_TAG(simulcast, "trying to downgrade current spatial layer");

			uint8_t maxScore = 0;

			for (int idx = this->currentSpatialLayer - 1; idx >= 0; --idx)
			{
				auto spatialLayer       = static_cast<int16_t>(idx);
				auto* producerRtpStream = this->producerRtpStreams[idx];

				// Ignore spatial layers for non existing Producer streams.
				if (!producerRtpStream)
					continue;

				if (producerRtpStream->GetScore() >= maxScore)
				{
					maxScore              = producerRtpStream->GetScore();
					newTargetSpatialLayer = spatialLayer;

					if (maxScore > 7)
					{
						MS_DEBUG_TAG(simulcast, "found Producer RtpStream with score > 7, keep it");

						break;
					}
				}
			}
		}
		// Update to the highest possible spatial layer.
		else
		{
			MS_DEBUG_TAG(simulcast, "trying to upgrade current spatial layer");

			int16_t idx = this->currentSpatialLayer == -1 ? 0 : this->currentSpatialLayer + 1;

			// Ignore spatial layers above the preferred one.
			for (; idx <= this->preferredSpatialLayer; ++idx)
			{
				auto spatialLayer       = idx;
				auto* producerRtpStream = this->producerRtpStreams[idx];

				// Ignore spatial layers for non existing Producer streams.
				if (!producerRtpStream)
					continue;

				// Take this as the new target if it is good enough.
				if (producerRtpStream->GetScore() >= 7)
				{
					MS_DEBUG_TAG(simulcast, "good enough spatial layer found: %" PRIi16, spatialLayer);

					newTargetSpatialLayer = spatialLayer;

					break;
				}
			}
		}

		MS_DEBUG_TAG(
		  simulcast,
		  "newTargetSpatialLayer is %" PRIi16 " [consumerId:%s]",
		  newTargetSpatialLayer,
		  this->id.c_str());

		if (newTargetSpatialLayer == -1)
		{
			MS_DEBUG_TAG(simulcast, "newTargetSpatialLayer remains unset");

			return;
		}

		// Nothing changed.
		if (newTargetSpatialLayer == this->targetSpatialLayer)
			return;

		this->targetSpatialLayer = newTargetSpatialLayer;

		// Already using the target layer. Do nothing.
		if (this->targetSpatialLayer == this->currentSpatialLayer)
			return;

		RequestKeyFrame();

		MS_DEBUG_TAG(
		  simulcast,
		  "target spatial layer changed to %" PRIi16 " [consumerId:%s]",
		  this->targetSpatialLayer,
		  this->id.c_str());
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

	inline void SimulcastConsumer::OnRtpStreamScore(RTC::RtpStream* /*rtpStream*/, uint8_t /*score*/)
	{
		MS_TRACE();

		// TODO: NO, we just can downgrade layers.
		if (IsActive())
			RecalculateTargetSpatialLayer();

		// Emit the score event.
		EmitScore();
	}

	inline void SimulcastConsumer::OnRtpStreamRetransmitRtpPacket(
	  RTC::RtpStreamSend* /*rtpStream*/, RTC::RtpPacket* packet)
	{
		MS_TRACE();

		this->listener->OnConsumerRetransmitRtpPacket(this, packet);
	}
} // namespace RTC
