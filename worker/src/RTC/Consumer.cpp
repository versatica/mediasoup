#define MS_CLASS "RTC::Consumer"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/Consumer.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "RTC/PipeProducerStreamManager.hpp"
#include "RTC/RTP/Codecs/Tools.hpp"
#include "RTC/SimpleProducerStreamManager.hpp"
#include "RTC/SimulcastProducerStreamManager.hpp"
#include "RTC/SvcProducerStreamManager.hpp"
#include "Utils.hpp"
#ifdef MS_RTC_LOGGER_RTP
#include "RTC/RtcLogger.hpp"
#endif
#include <limits> // std::numeric_limits

namespace RTC
{
	/* Static. */

	static constexpr size_t TargetLayerRetransmissionBufferSize{ 30u };

	/* Instance methods. */

	Consumer::Consumer(
	  SharedInterface* shared,
	  const std::string& id,
	  const std::string& producerId,
	  RTC::Consumer::Listener* listener,
	  const FBS::Transport::ConsumeRequest* data)
	  : id(id),
	    producerId(producerId),
	    shared(shared),
	    listener(listener),
	    kind(RTC::Media::Kind(data->kind())),
	    type(RTC::RtpParameters::Type(data->type()))
	{
		MS_TRACE();

		this->pipe = this->type == RTC::RtpParameters::Type::PIPE;

		// This may throw.
		this->rtpParameters = RTC::RtpParameters(data->rtpParameters());

		if (this->rtpParameters.encodings.empty())
		{
			MS_THROW_TYPE_ERROR("empty rtpParameters.encodings");
		}

		// All encodings must have SSRCs.
		for (auto& encoding : this->rtpParameters.encodings)
		{
			if (encoding.ssrc == 0)
			{
				MS_THROW_TYPE_ERROR("invalid encoding in rtpParameters (missing ssrc)");
			}
			else if (encoding.hasRtx && encoding.rtx.ssrc == 0)
			{
				MS_THROW_TYPE_ERROR("invalid encoding in rtpParameters (missing rtx.ssrc)");
			}
		}

		if (data->consumableRtpEncodings()->size() == 0)
		{
			MS_THROW_TYPE_ERROR("empty consumableRtpEncodings");
		}

		this->consumableRtpEncodings.reserve(data->consumableRtpEncodings()->size());

		for (size_t i{ 0 }; i < data->consumableRtpEncodings()->size(); ++i)
		{
			const auto* entry = data->consumableRtpEncodings()->Get(i);

			// This may throw due the constructor of RTC::RtpEncodingParameters.
			this->consumableRtpEncodings.emplace_back(entry);

			// Verify that it has ssrc field.
			auto& encoding = this->consumableRtpEncodings[i];

			if (encoding.ssrc == 0u)
			{
				MS_THROW_TYPE_ERROR("wrong encoding in consumableRtpEncodings (missing ssrc)");
			}
		}

		// Ensure there are as many encodings as consumable encodings for pipe.
		if (pipe && this->rtpParameters.encodings.size() != this->consumableRtpEncodings.size())
		{
			MS_THROW_TYPE_ERROR("number of rtpParameters.encodings and consumableRtpEncodings do not match");
		}

		// Fill RTP header extension ids and their mapped values.
		// This may throw.
		for (auto& exten : this->rtpParameters.headerExtensions)
		{
			if (exten.id == 0u)
			{
				MS_THROW_TYPE_ERROR("RTP extension id cannot be 0");
			}

			if (this->rtpHeaderExtensionIds.mid == 0u && exten.type == RTC::RtpHeaderExtensionUri::Type::MID)
			{
				this->rtpHeaderExtensionIds.mid = exten.id;
			}

			if (this->rtpHeaderExtensionIds.rid == 0u && exten.type == RTC::RtpHeaderExtensionUri::Type::RTP_STREAM_ID)
			{
				this->rtpHeaderExtensionIds.rid = exten.id;
			}

			if (this->rtpHeaderExtensionIds.rrid == 0u && exten.type == RTC::RtpHeaderExtensionUri::Type::REPAIRED_RTP_STREAM_ID)
			{
				this->rtpHeaderExtensionIds.rrid = exten.id;
			}

			if (this->rtpHeaderExtensionIds.absSendTime == 0u && exten.type == RTC::RtpHeaderExtensionUri::Type::ABS_SEND_TIME)
			{
				this->rtpHeaderExtensionIds.absSendTime = exten.id;
			}

			if (this->rtpHeaderExtensionIds.transportWideCc01 == 0u && exten.type == RTC::RtpHeaderExtensionUri::Type::TRANSPORT_WIDE_CC_01)
			{
				this->rtpHeaderExtensionIds.transportWideCc01 = exten.id;
			}

			if (this->rtpHeaderExtensionIds.ssrcAudioLevel == 0u && exten.type == RTC::RtpHeaderExtensionUri::Type::SSRC_AUDIO_LEVEL)
			{
				this->rtpHeaderExtensionIds.ssrcAudioLevel = exten.id;
			}

			if (
			  this->rtpHeaderExtensionIds.dependencyDescriptor == 0u &&
			  exten.type == RTC::RtpHeaderExtensionUri::Type::DEPENDENCY_DESCRIPTOR)
			{
				this->rtpHeaderExtensionIds.dependencyDescriptor = exten.id;
			}

			if (this->rtpHeaderExtensionIds.videoOrientation == 0u && exten.type == RTC::RtpHeaderExtensionUri::Type::VIDEO_ORIENTATION)
			{
				this->rtpHeaderExtensionIds.videoOrientation = exten.id;
			}

			if (this->rtpHeaderExtensionIds.absCaptureTime == 0u && exten.type == RTC::RtpHeaderExtensionUri::Type::ABS_CAPTURE_TIME)
			{
				this->rtpHeaderExtensionIds.absCaptureTime = exten.id;
			}

			if (this->rtpHeaderExtensionIds.playoutDelay == 0u && exten.type == RTC::RtpHeaderExtensionUri::Type::PLAYOUT_DELAY)
			{
				this->rtpHeaderExtensionIds.playoutDelay = exten.id;
			}

			if (this->rtpHeaderExtensionIds.mediasoupPacketId == 0u && exten.type == RTC::RtpHeaderExtensionUri::Type::MEDIASOUP_PACKET_ID)
			{
				this->rtpHeaderExtensionIds.mediasoupPacketId = exten.id;
			}
		}

		// paused is set to false by default.
		this->paused = data->paused();

		// Fill supported codec payload types.
		for (auto& codec : this->rtpParameters.codecs)
		{
			if (codec.mimeType.IsMediaCodec())
			{
				this->supportedCodecPayloadTypes[codec.payloadType] = true;
			}
		}

		// Fill media SSRCs vector.
		for (auto& encoding : this->rtpParameters.encodings)
		{
			this->mediaSsrcs.push_back(encoding.ssrc);
		}

		// Fill RTX SSRCs vector.
		for (auto& encoding : this->rtpParameters.encodings)
		{
			if (encoding.hasRtx)
			{
				this->rtxSsrcs.push_back(encoding.rtx.ssrc);
			}
		}

		// Set the RTCP report generation interval.
		if (this->kind == RTC::Media::Kind::AUDIO)
		{
			this->maxRtcpInterval = RTC::RTCP::MaxAudioIntervalMs;
		}
		else
		{
			this->maxRtcpInterval = RTC::RTCP::MaxVideoIntervalMs;
		}

		auto& encoding = this->rtpParameters.encodings[0];

		// Determine keyFrameSupported.
		const auto* mediaCodec       = this->rtpParameters.GetCodecForEncoding(encoding);
		const bool keyFrameSupported = RTC::RTP::Codecs::Tools::CanBeKeyFrame(mediaCodec->mimeType);

		// Build preferred layers from FBS data.
		RTC::ConsumerTypes::VideoLayers preferredLayers;

		// Create the appropriate ProducerStreamManager subclass based on type.
		switch (this->type)
		{
			case RTC::RtpParameters::Type::SIMPLE:
			{
				// Ensure there is a single encoding.
				if (this->consumableRtpEncodings.size() != 1u)
				{
					MS_THROW_TYPE_ERROR("invalid consumableRtpEncodings with size != 1");
				}

				// Create the encoding context for Opus (DTX filtering).
				std::unique_ptr<RTC::RTP::Codecs::EncodingContext> encodingContext;

				if (
				  mediaCodec->mimeType.type == RTC::RtpCodecMimeType::Type::AUDIO &&
				  (mediaCodec->mimeType.subtype == RTC::RtpCodecMimeType::Subtype::OPUS ||
				   mediaCodec->mimeType.subtype == RTC::RtpCodecMimeType::Subtype::MULTIOPUS))
				{
					RTC::RTP::Codecs::EncodingContext::Params params;

					encodingContext.reset(
					  RTC::RTP::Codecs::Tools::GetEncodingContext(mediaCodec->mimeType, params));

					// ignoreDtx is set to false by default.
					encodingContext->SetIgnoreDtx(data->ignoreDtx());
				}

				this->producerStreamManager = std::make_unique<SimpleProducerStreamManager>(
				  this->consumableRtpEncodings,
				  preferredLayers,
				  std::move(encodingContext),
				  this->kind,
				  keyFrameSupported,
				  this,
				  this->shared);

				break;
			}

			case RTC::RtpParameters::Type::SIMULCAST:
			{
				// We allow a single encoding in simulcast (so we can enable temporal
				// layers with a single simulcast stream).

				// Ensure there are as many spatial layers as encodings.
				if (encoding.spatialLayers != this->consumableRtpEncodings.size())
				{
					MS_THROW_TYPE_ERROR(
					  "encoding.spatialLayers does not match number of consumableRtpEncodings");
				}

				// Set preferredLayers.
				if (flatbuffers::IsFieldPresent(data, FBS::Transport::ConsumeRequest::VT_PREFERREDLAYERS))
				{
					const auto* fbsPreferredLayers = data->preferredLayers();

					preferredLayers.spatial = fbsPreferredLayers->spatialLayer();

					if (preferredLayers.spatial > encoding.spatialLayers - 1)
					{
						preferredLayers.spatial = static_cast<int16_t>(encoding.spatialLayers - 1);
					}

					if (
					  auto preferredTemporalLayer = fbsPreferredLayers->temporalLayer();
					  preferredTemporalLayer.has_value())
					{
						preferredLayers.temporal = preferredTemporalLayer.value();

						if (preferredLayers.temporal > encoding.temporalLayers - 1)
						{
							preferredLayers.temporal = static_cast<int16_t>(encoding.temporalLayers - 1);
						}
					}
					else
					{
						preferredLayers.temporal = static_cast<int16_t>(encoding.temporalLayers - 1);
					}
				}
				else
				{
					// Initially set preferredSpatialLayer and preferredTemporalLayer
					// to the maximum value.
					preferredLayers.spatial  = static_cast<int16_t>(encoding.spatialLayers - 1);
					preferredLayers.temporal = static_cast<int16_t>(encoding.temporalLayers - 1);
				}

				// Create the encoding context.
				if (!RTC::RTP::Codecs::Tools::IsValidTypeForCodec(this->type, mediaCodec->mimeType))
				{
					MS_THROW_TYPE_ERROR(
					  "%s codec not supported for simulcast", mediaCodec->mimeType.ToString().c_str());
				}

				RTC::RTP::Codecs::EncodingContext::Params params;

				params.spatialLayers  = encoding.spatialLayers;
				params.temporalLayers = encoding.temporalLayers;

				std::unique_ptr<RTC::RTP::Codecs::EncodingContext> encodingContext(
				  RTC::RTP::Codecs::Tools::GetEncodingContext(mediaCodec->mimeType, params));

				MS_ASSERT(encodingContext, "no encoding context for this codec");

				this->producerStreamManager = std::make_unique<SimulcastProducerStreamManager>(
				  this->consumableRtpEncodings,
				  preferredLayers,
				  std::move(encodingContext),
				  this->kind,
				  keyFrameSupported,
				  this,
				  this->shared);

				break;
			}

			case RTC::RtpParameters::Type::SVC:
			{
				// Ensure there is a single encoding.
				if (this->consumableRtpEncodings.size() != 1u)
				{
					MS_THROW_TYPE_ERROR("invalid consumableRtpEncodings with size != 1");
				}

				// Ensure there are multiple spatial or temporal layers.
				if (encoding.spatialLayers < 2u && encoding.temporalLayers < 2u)
				{
					MS_THROW_TYPE_ERROR("invalid number of layers");
				}

				// Set preferredLayers.
				if (flatbuffers::IsFieldPresent(data, FBS::Transport::ConsumeRequest::VT_PREFERREDLAYERS))
				{
					preferredLayers.spatial = data->preferredLayers()->spatialLayer();

					if (preferredLayers.spatial > encoding.spatialLayers - 1)
					{
						preferredLayers.spatial = static_cast<int16_t>(encoding.spatialLayers - 1);
					}

					// preferredTemporalLayer is optional.
					auto preferredTemporalLayer = data->preferredLayers()->temporalLayer();

					if (preferredTemporalLayer)
					{
						preferredLayers.temporal = preferredTemporalLayer.value();

						if (preferredLayers.temporal > encoding.temporalLayers - 1)
						{
							preferredLayers.temporal = static_cast<int16_t>(encoding.temporalLayers - 1);
						}
					}
					else
					{
						preferredLayers.temporal = static_cast<int16_t>(encoding.temporalLayers - 1);
					}
				}
				else
				{
					// Initially set preferredSpatialLayer and preferredTemporalLayer
					// to the maximum value.
					preferredLayers.spatial  = static_cast<int16_t>(encoding.spatialLayers - 1);
					preferredLayers.temporal = static_cast<int16_t>(encoding.temporalLayers - 1);
				}

				// Create the encoding context.
				if (!RTC::RTP::Codecs::Tools::IsValidTypeForCodec(this->type, mediaCodec->mimeType))
				{
					MS_THROW_TYPE_ERROR(
					  "%s codec not supported for svc", mediaCodec->mimeType.ToString().c_str());
				}

				RTC::RTP::Codecs::EncodingContext::Params params;

				params.spatialLayers  = encoding.spatialLayers;
				params.temporalLayers = encoding.temporalLayers;
				params.ksvc           = encoding.ksvc;

				std::unique_ptr<RTC::RTP::Codecs::EncodingContext> encodingContext(
				  RTC::RTP::Codecs::Tools::GetEncodingContext(mediaCodec->mimeType, params));

				MS_ASSERT(encodingContext, "no encoding context for this codec");

				this->producerStreamManager = std::make_unique<SvcProducerStreamManager>(
				  this->consumableRtpEncodings,
				  preferredLayers,
				  std::move(encodingContext),
				  this->kind,
				  keyFrameSupported,
				  this,
				  this->shared);

				break;
			}

			case RTC::RtpParameters::Type::PIPE:
			{
				// Pipe consumer: no layer management, N streams.
				this->producerStreamManager = std::make_unique<PipeProducerStreamManager>(
				  this->consumableRtpEncodings,
				  preferredLayers,
				  nullptr,
				  this->kind,
				  keyFrameSupported,
				  this,
				  this->shared);

				break;
			}

			default:
			{
				MS_THROW_TYPE_ERROR("invalid consumer type");
			}
		}

		// Create RtpStreamSend instances.
		CreateRtpStreams();

		// NOTE: This may throw.
		this->shared->GetChannelMessageRegistrator()->RegisterHandler(
		  this->id,
		  /*channelRequestHandler*/ this,
		  /*channelNotificationHandler*/ nullptr);
	}

	Consumer::~Consumer()
	{
		MS_TRACE();

		this->shared->GetChannelMessageRegistrator()->UnregisterHandler(this->id);

		for (auto* rtpStream : this->rtpStreams)
		{
			delete rtpStream;
		}

		this->rtpStreams.clear();
		this->mapMappedSsrcSsrc.clear();
		this->mapSsrcRtpStream.clear();
		this->mapRtpStreamRtpSeqManager.clear();
		this->mapRtpStreamTargetLayerRetransmissionBuffer.clear();
	}

	flatbuffers::Offset<FBS::Consumer::DumpResponse> Consumer::FillBuffer(
	  flatbuffers::FlatBufferBuilder& builder) const
	{
		MS_TRACE();

		// Call the base method.
		auto base = FillBufferBase(builder);
		// Add rtpStreams.
		std::vector<flatbuffers::Offset<FBS::RtpStream::Dump>> rtpStreams;
		rtpStreams.reserve(this->rtpStreams.size());

		for (const auto* rtpStream : this->rtpStreams)
		{
			rtpStreams.emplace_back(rtpStream->FillBuffer(builder));
		}

		auto targetLayers = this->producerStreamManager->GetTargetLayers();

		auto dump = FBS::Consumer::CreateConsumerDumpDirect(
		  builder,
		  base,
		  &rtpStreams,
		  this->producerStreamManager->GetPreferredLayers().spatial,
		  targetLayers.spatial,
		  this->producerStreamManager->GetCurrentSpatialLayer(),
		  this->producerStreamManager->GetPreferredLayers().temporal,
		  targetLayers.temporal,
		  this->producerStreamManager->GetCurrentTemporalLayer());

		return FBS::Consumer::CreateDumpResponse(builder, dump);
	}

	flatbuffers::Offset<FBS::Consumer::BaseConsumerDump> Consumer::FillBufferBase(
	  flatbuffers::FlatBufferBuilder& builder) const
	{
		MS_TRACE();

		// Add rtpParameters.
		auto rtpParameters = this->rtpParameters.FillBuffer(builder);

		// Add consumableRtpEncodings.
		std::vector<flatbuffers::Offset<FBS::RtpParameters::RtpEncodingParameters>> consumableRtpEncodings;
		consumableRtpEncodings.reserve(this->consumableRtpEncodings.size());

		for (const auto& encoding : this->consumableRtpEncodings)
		{
			consumableRtpEncodings.emplace_back(encoding.FillBuffer(builder));
		}

		// Add supportedCodecPayloadTypes.
		std::vector<uint8_t> supportedCodecPayloadTypes;

		for (auto i = 0; i < 128; ++i)
		{
			if (this->supportedCodecPayloadTypes[i])
			{
				supportedCodecPayloadTypes.push_back(i);
			}
		}

		// Add traceEventTypes.
		std::vector<FBS::Consumer::TraceEventType> traceEventTypes;

		if (this->traceEventTypes.rtp)
		{
			traceEventTypes.emplace_back(FBS::Consumer::TraceEventType::RTP);
		}
		if (this->traceEventTypes.keyframe)
		{
			traceEventTypes.emplace_back(FBS::Consumer::TraceEventType::KEYFRAME);
		}
		if (this->traceEventTypes.nack)
		{
			traceEventTypes.emplace_back(FBS::Consumer::TraceEventType::NACK);
		}
		if (this->traceEventTypes.pli)
		{
			traceEventTypes.emplace_back(FBS::Consumer::TraceEventType::PLI);
		}
		if (this->traceEventTypes.fir)
		{
			traceEventTypes.emplace_back(FBS::Consumer::TraceEventType::FIR);
		}

		return FBS::Consumer::CreateBaseConsumerDumpDirect(
		  builder,
		  this->id.c_str(),
		  RTC::RtpParameters::TypeToFbs(this->type),
		  this->producerId.c_str(),
		  this->kind == RTC::Media::Kind::AUDIO ? FBS::RtpParameters::MediaKind::AUDIO
		                                        : FBS::RtpParameters::MediaKind::VIDEO,
		  rtpParameters,
		  &consumableRtpEncodings,
		  &supportedCodecPayloadTypes,
		  &traceEventTypes,
		  this->paused,
		  this->producerPaused,
		  this->priority);
	}

	flatbuffers::Offset<FBS::Consumer::GetStatsResponse> Consumer::FillBufferStats(
	  flatbuffers::FlatBufferBuilder& builder)
	{
		MS_TRACE();

		std::vector<flatbuffers::Offset<FBS::RtpStream::Stats>> rtpStreams;
		rtpStreams.reserve(this->rtpStreams.size());

		// Add stats of our send streams.
		for (auto* rtpStream : this->rtpStreams)
		{
			rtpStreams.emplace_back(rtpStream->FillBufferStats(builder));
		}

		// Add stats of the current recv stream.
		auto* producerCurrentRtpStream = this->producerStreamManager->GetProducerCurrentRtpStream();

		if (producerCurrentRtpStream)
		{
			rtpStreams.emplace_back(producerCurrentRtpStream->FillBufferStats(builder));
		}

		return FBS::Consumer::CreateGetStatsResponseDirect(builder, &rtpStreams);
	}

	flatbuffers::Offset<FBS::Consumer::ConsumerScore> Consumer::FillBufferScore(
	  flatbuffers::FlatBufferBuilder& builder) const
	{
		MS_TRACE();

		MS_ASSERT(this->producerRtpStreamScores, "producerRtpStreamScores not set");

		// NOTE: Hardcoded values in PipeTransport.
		if (this->pipe)
		{
			return FBS::Consumer::CreateConsumerScoreDirect(builder, 10, 10, this->producerRtpStreamScores);
		}

		auto* rtpStream = this->mapSsrcRtpStream.begin()->second;
		uint8_t producerScore{ 0 };

		auto* producerCurrentRtpStream = this->producerStreamManager->GetProducerCurrentRtpStream();

		if (producerCurrentRtpStream)
		{
			producerScore = producerCurrentRtpStream->GetScore();
		}

		return FBS::Consumer::CreateConsumerScoreDirect(
		  builder, rtpStream->GetScore(), producerScore, this->producerRtpStreamScores);
	}

	void Consumer::HandleRequest(Channel::ChannelRequest* request)
	{
		MS_TRACE();

		switch (request->method)
		{
			case Channel::ChannelRequest::Method::CONSUMER_DUMP:
			{
				auto dumpOffset = FillBuffer(request->GetBufferBuilder());

				request->Accept(FBS::Response::Body::Consumer_DumpResponse, dumpOffset);

				break;
			}

			case Channel::ChannelRequest::Method::CONSUMER_REQUEST_KEY_FRAME:
			{
				if (IsActive())
				{
					if (this->pipe)
					{
						if (this->kind != RTC::Media::Kind::VIDEO)
						{
							return;
						}

						for (auto& consumableRtpEncoding : this->consumableRtpEncodings)
						{
							auto mappedSsrc = consumableRtpEncoding.ssrc;

							this->listener->OnConsumerKeyFrameRequested(this, mappedSsrc);
						}
					}
					else
					{
						this->producerStreamManager->RequestKeyFrame();
					}
				}

				request->Accept();

				break;
			}

			case Channel::ChannelRequest::Method::CONSUMER_SET_PREFERRED_LAYERS:
			{
				// Simple consumers and pipes have no layers concept.
				if (this->type == RTC::RtpParameters::Type::SIMPLE || this->pipe)
				{
					// Accept with empty preferred layers object.
					auto responseOffset =
					  FBS::Consumer::CreateSetPreferredLayersResponse(request->GetBufferBuilder());

					request->Accept(FBS::Response::Body::Consumer_SetPreferredLayersResponse, responseOffset);

					break;
				}

				auto* rtpStream              = this->mapSsrcRtpStream.begin()->second;
				auto previousPreferredLayers = this->producerStreamManager->GetPreferredLayers();

				const auto* body = request->data->body_as<FBS::Consumer::SetPreferredLayersRequest>();
				const auto* preferredLayers = body->preferredLayers();

				RTC::ConsumerTypes::VideoLayers newPreferredLayers;

				// Spatial layer.
				newPreferredLayers.spatial = preferredLayers->spatialLayer();

				if (newPreferredLayers.spatial > rtpStream->GetSpatialLayers() - 1)
				{
					newPreferredLayers.spatial = static_cast<int16_t>(rtpStream->GetSpatialLayers() - 1);
				}

				// preferredTemporalLayer is optional.
				auto preferredTemporalLayer = preferredLayers->temporalLayer();

				if (preferredTemporalLayer.has_value())
				{
					newPreferredLayers.temporal = preferredTemporalLayer.value();

					if (newPreferredLayers.temporal > rtpStream->GetTemporalLayers() - 1)
					{
						newPreferredLayers.temporal = static_cast<int16_t>(rtpStream->GetTemporalLayers() - 1);
					}
				}
				else
				{
					newPreferredLayers.temporal = static_cast<int16_t>(rtpStream->GetTemporalLayers() - 1);
				}

				this->producerStreamManager->SetPreferredLayers(newPreferredLayers);

				MS_DEBUG_DEV(
				  "preferred layers changed [spatial:%" PRIi16 ", temporal:%" PRIi16 ", consumerId:%s]",
				  newPreferredLayers.spatial,
				  newPreferredLayers.temporal,
				  this->id.c_str());

				preferredTemporalLayer     = newPreferredLayers.temporal;
				auto preferredLayersOffset = FBS::Consumer::CreateConsumerLayers(
				  request->GetBufferBuilder(), newPreferredLayers.spatial, preferredTemporalLayer);
				auto responseOffset = FBS::Consumer::CreateSetPreferredLayersResponse(
				  request->GetBufferBuilder(), preferredLayersOffset);

				request->Accept(FBS::Response::Body::Consumer_SetPreferredLayersResponse, responseOffset);

				if (IsActive() && newPreferredLayers != previousPreferredLayers)
				{
					this->producerStreamManager->MayChangeLayers(/*force*/ true);
				}

				break;
			}

			case Channel::ChannelRequest::Method::CONSUMER_GET_STATS:
			{
				auto responseOffset = FillBufferStats(request->GetBufferBuilder());

				request->Accept(FBS::Response::Body::Consumer_GetStatsResponse, responseOffset);

				break;
			}

			case Channel::ChannelRequest::Method::CONSUMER_PAUSE:
			{
				if (this->paused)
				{
					request->Accept();

					break;
				}

				const bool wasActive = IsActive();

				this->paused = true;

				MS_DEBUG_DEV("Consumer paused [consumerId:%s]", this->id.c_str());

				if (wasActive)
				{
					UserOnPaused();
				}

				request->Accept();

				break;
			}

			case Channel::ChannelRequest::Method::CONSUMER_RESUME:
			{
				if (!this->paused)
				{
					request->Accept();

					break;
				}

				this->paused = false;

				MS_DEBUG_DEV("Consumer resumed [consumerId:%s]", this->id.c_str());

				if (IsActive())
				{
					UserOnResumed();
				}

				request->Accept();

				break;
			}

			case Channel::ChannelRequest::Method::CONSUMER_SET_PRIORITY:
			{
				const auto* body = request->data->body_as<FBS::Consumer::SetPriorityRequest>();

				if (body->priority() < 1u)
				{
					MS_THROW_TYPE_ERROR("wrong priority (must be higher than 0)");
				}

				this->priority = body->priority();

				auto responseOffset =
				  FBS::Consumer::CreateSetPriorityResponse(request->GetBufferBuilder(), this->priority);

				request->Accept(FBS::Response::Body::Consumer_SetPriorityResponse, responseOffset);

				break;
			}

			case Channel::ChannelRequest::Method::CONSUMER_ENABLE_TRACE_EVENT:
			{
				const auto* body = request->data->body_as<FBS::Consumer::EnableTraceEventRequest>();

				// Reset traceEventTypes.
				struct TraceEventTypes newTraceEventTypes;

				for (const auto& type : *body->events())
				{
					switch (type)
					{
						case FBS::Consumer::TraceEventType::KEYFRAME:
						{
							newTraceEventTypes.keyframe = true;

							break;
						}
						case FBS::Consumer::TraceEventType::FIR:
						{
							newTraceEventTypes.fir = true;

							break;
						}
						case FBS::Consumer::TraceEventType::NACK:
						{
							newTraceEventTypes.nack = true;

							break;
						}
						case FBS::Consumer::TraceEventType::PLI:
						{
							newTraceEventTypes.pli = true;

							break;
						}
						case FBS::Consumer::TraceEventType::RTP:
						{
							newTraceEventTypes.rtp = true;

							break;
						}
					}
				}

				this->traceEventTypes = newTraceEventTypes;

				request->Accept();

				break;
			}

			default:
			{
				MS_THROW_ERROR("unknown method '%s'", request->methodCStr);
			}
		}
	}

	void Consumer::TransportConnected()
	{
		MS_TRACE();

		if (this->transportConnected)
		{
			return;
		}

		this->transportConnected = true;

		MS_DEBUG_DEV("Transport connected [consumerId:%s]", this->id.c_str());

		UserOnTransportConnected();
	}

	void Consumer::TransportDisconnected()
	{
		MS_TRACE();

		if (!this->transportConnected)
		{
			return;
		}

		this->transportConnected = false;

		MS_DEBUG_DEV("Transport disconnected [consumerId:%s]", this->id.c_str());

		UserOnTransportDisconnected();
	}

	void Consumer::ProducerPaused()
	{
		MS_TRACE();

		if (this->producerPaused)
		{
			return;
		}

		const bool wasActive = IsActive();

		this->producerPaused = true;

		MS_DEBUG_DEV("Producer paused [consumerId:%s]", this->id.c_str());

		if (wasActive)
		{
			UserOnPaused();
		}

		this->shared->GetChannelNotifier()->Emit(
		  this->id, FBS::Notification::Event::CONSUMER_PRODUCER_PAUSE);
	}

	void Consumer::ProducerResumed()
	{
		MS_TRACE();

		if (!this->producerPaused)
		{
			return;
		}

		this->producerPaused = false;

		MS_DEBUG_DEV("Producer resumed [consumerId:%s]", this->id.c_str());

		if (IsActive())
		{
			UserOnResumed();
		}

		this->shared->GetChannelNotifier()->Emit(
		  this->id, FBS::Notification::Event::CONSUMER_PRODUCER_RESUME);
	}

	void Consumer::ProducerRtpStream(RTC::RTP::RtpStreamRecv* rtpStream, uint32_t mappedSsrc)
	{
		MS_TRACE();

		this->producerStreamManager->ProducerRtpStream(rtpStream, mappedSsrc);
	}

	void Consumer::ProducerNewRtpStream(RTC::RTP::RtpStreamRecv* rtpStream, uint32_t mappedSsrc)
	{
		MS_TRACE();

		this->producerStreamManager->ProducerNewRtpStream(rtpStream, mappedSsrc);
	}

	void Consumer::ProducerRtpStreamScores(const std::vector<uint8_t>* scores)
	{
		MS_TRACE();

		// This is gonna be a constant pointer.
		this->producerRtpStreamScores = scores;
	}

	void Consumer::ProducerRtpStreamScore(
	  RTC::RTP::RtpStreamRecv* rtpStream, uint8_t score, uint8_t previousScore)
	{
		MS_TRACE();

		EmitScore();

		this->producerStreamManager->ProducerRtpStreamScore(rtpStream, score, previousScore);
	}

	void Consumer::ProducerRtcpSenderReport(RTC::RTP::RtpStreamRecv* rtpStream, bool first)
	{
		MS_TRACE();

		this->producerStreamManager->ProducerRtcpSenderReport(rtpStream, first);
	}

	// The caller (Router) is supposed to proceed with the deletion of this Consumer
	// right after calling this method. Otherwise ugly things may happen.
	void Consumer::ProducerClosed()
	{
		MS_TRACE();

		this->producerClosed = true;

		MS_DEBUG_DEV("Producer closed [consumerId:%s]", this->id.c_str());

		this->shared->GetChannelNotifier()->Emit(
		  this->id, FBS::Notification::Event::CONSUMER_PRODUCER_CLOSE);

		this->listener->OnConsumerProducerClosed(this);
	}

	uint8_t Consumer::GetBitratePriority() const
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

		return this->priority;
	}

	uint32_t Consumer::IncreaseLayer(uint32_t bitrate, bool considerLoss)
	{
		MS_TRACE();

		MS_ASSERT(this->externallyManagedBitrate, "bitrate is not externally managed");
		MS_ASSERT(IsActive(), "should be active");

		// Pipe does not play the BWE game.
		if (this->pipe)
		{
			return 0u;
		}

		float lossPercentage{ 0.0f };

		auto* rtpStream = this->mapSsrcRtpStream.begin()->second;

		if (considerLoss)
		{
			lossPercentage = rtpStream->GetLossPercentage();
		}

		auto nowMs = DepLibUV::GetTimeMs();

		return this->producerStreamManager->IncreaseLayer(bitrate, considerLoss, lossPercentage, nowMs);
	}

	void Consumer::ApplyLayers()
	{
		MS_TRACE();

		MS_ASSERT(this->externallyManagedBitrate, "bitrate is not externally managed");
		MS_ASSERT(IsActive(), "should be active");

		// Pipe does not play the BWE game.
		if (this->pipe)
		{
			return;
		}

		auto* rtpStream = this->mapSsrcRtpStream.begin()->second;

		this->producerStreamManager->ApplyLayers(rtpStream->GetActiveMs());
	}

	uint32_t Consumer::GetDesiredBitrate() const
	{
		MS_TRACE();

		MS_ASSERT(this->externallyManagedBitrate, "bitrate is not externally managed");

		// Pipe does not play the BWE game.
		if (this->pipe)
		{
			return 0u;
		}

		// Audio does not play the BWE game.
		if (this->kind != RTC::Media::Kind::VIDEO)
		{
			return 0u;
		}

		if (!IsActive())
		{
			return 0u;
		}

		auto nowMs          = DepLibUV::GetTimeMs();
		auto desiredBitrate = this->producerStreamManager->GetDesiredBitrate(nowMs);

		// If consumer.rtpParameters.encodings[0].maxBitrate was given and it's
		// greater than computed one, then use it.
		auto maxBitrate = this->rtpParameters.encodings[0].maxBitrate;

		desiredBitrate = std::max(maxBitrate, desiredBitrate);

		return desiredBitrate;
	}

	// NOLINTNEXTLINE(misc-no-recursion)
	void Consumer::SendRtpPacket(RTC::RTP::Packet* packet, RTC::RTP::SharedPacket& sharedPacket)
	{
		MS_TRACE();

#ifdef MS_RTC_LOGGER_RTP
		packet->logger.consumerId = this->id;
#endif

		RTC::RTP::RtpStreamSend* rtpStream;
		RTC::SeqManager<uint16_t>* rtpSeqManager;
		RetransmissionBuffer* targetLayerRetransmissionBuffer;

		if (this->pipe)
		{
			auto ssrc     = this->mapMappedSsrcSsrc.at(packet->GetSsrc());
			rtpStream     = this->mapSsrcRtpStream.at(ssrc);
			rtpSeqManager = &this->mapRtpStreamRtpSeqManager.at(rtpStream);
			targetLayerRetransmissionBuffer =
			  &this->mapRtpStreamTargetLayerRetransmissionBuffer.at(rtpStream);
		}
		else
		{
			rtpStream     = this->mapSsrcRtpStream.begin()->second;
			rtpSeqManager = &this->mapRtpStreamRtpSeqManager.at(rtpStream);
			targetLayerRetransmissionBuffer =
			  &this->mapRtpStreamTargetLayerRetransmissionBuffer.at(rtpStream);
		}

		if (!IsActive())
		{
			if (this->producerStreamManager->IsPacketForCurrentStream(packet))
			{
#ifdef MS_RTC_LOGGER_RTP
				packet->logger.Discarded(RTC::RtcLogger::RtpPacket::DiscardReason::CONSUMER_INACTIVE);
#endif
				rtpSeqManager->Drop(packet->GetSequenceNumber());
			}

			return;
		}

		auto payloadType = packet->GetPayloadType();

		// NOTE: This may happen if this Consumer supports just some codecs of those
		// in the corresponding Producer.
		if (!this->supportedCodecPayloadTypes[payloadType])
		{
			if (this->producerStreamManager->IsPacketForCurrentStream(packet))
			{
				MS_WARN_DEV("payload type not supported [payloadType:%" PRIu8 "]", payloadType);

#ifdef MS_RTC_LOGGER_RTP
				packet->logger.Discarded(RTC::RtcLogger::RtpPacket::DiscardReason::UNSUPPORTED_PAYLOAD_TYPE);
#endif
				rtpSeqManager->Drop(packet->GetSequenceNumber());
			}

			return;
		}

		// Ask the ProducerStreamManager to process the packet.
		auto action = this->producerStreamManager->ProcessRtpPacket(
		  packet, this->lastSentPacketHasMarker, rtpStream->GetClockRate(), rtpStream->GetMaxPacketTs());

		switch (action.type)
		{
			case ProducerStreamManager::RtpPacketProcessResult::Type::DROP:
			{
				rtpSeqManager->Drop(packet->GetSequenceNumber());
				return;
			}

			case ProducerStreamManager::RtpPacketProcessResult::Type::SILENT_DROP:
			{
				// Don't account in seq manager.
				return;
			}

			case ProducerStreamManager::RtpPacketProcessResult::Type::BUFFER:
			{
				StorePacketInTargetLayerRetransmissionBuffer(
				  *targetLayerRetransmissionBuffer, packet, sharedPacket);

				return;
			}

			case ProducerStreamManager::RtpPacketProcessResult::Type::FORWARD:
			{
				// Continue below.
				break;
			}
		}

		// Handle sync.
		if (action.isSyncPacket)
		{
			rtpSeqManager->Sync(action.syncSeqValue);
		}

		// Handle spatial layer switch.
		if (action.spatialLayerSwitched)
		{
			// Reset the score of our RtpStream to 10.
			rtpStream->ResetScore(10u, /*notify*/ false);

			// Emit the layersChange event.
			EmitLayersChange();

			// Emit the score event.
			EmitScore();
		}

		// Handle temporal layer change.
		if (action.temporalLayerChanged)
		{
			EmitLayersChange();
		}

		// Update RTP seq number and timestamp based on offset.
		uint16_t seq;
		const uint32_t timestamp = packet->GetTimestamp() - action.tsOffset;

		rtpSeqManager->Input(packet->GetSequenceNumber(), seq);

		// Save original packet fields.
		auto origSsrc         = packet->GetSsrc();
		auto origSeq          = packet->GetSequenceNumber();
		auto origTimestamp    = packet->GetTimestamp();
		const bool origMarker = packet->HasMarker();

		// Rewrite packet.
		// For pipe each stream has its own SSRC; for non-pipe always encodings[0].
		packet->SetSsrc(this->pipe ? rtpStream->GetSsrc() : this->rtpParameters.encodings[0].ssrc);
		packet->SetSequenceNumber(seq);
		packet->SetTimestamp(timestamp);
		packet->SetMarker(action.marker);

#ifdef MS_RTC_LOGGER_RTP
		packet->logger.sendRtpTimestamp = timestamp;
		packet->logger.sendSeqNumber    = seq;
#endif

		if (action.isSyncPacket)
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

		const RTC::RTP::RtpStreamSend::ReceivePacketResult result =
		  rtpStream->ReceivePacket(packet, sharedPacket);

		if (result != RTC::RTP::RtpStreamSend::ReceivePacketResult::DISCARDED)
		{
			if (rtpSeqManager->GetMaxOutput() == packet->GetSequenceNumber())
			{
				this->lastSentPacketHasMarker = packet->HasMarker();
			}

			// Send the packet.
			this->listener->OnConsumerSendRtpPacket(this, packet);

			// May emit 'trace' event.
			EmitTraceEventRtpAndKeyFrameTypes(packet);
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

#ifdef MS_RTC_LOGGER_RTP
			packet->logger.Discarded(RTC::RtcLogger::RtpPacket::DiscardReason::SEND_RTP_STREAM_DISCARDED);
#endif
		}

		// Restore packet fields.
		packet->SetSsrc(origSsrc);
		packet->SetSequenceNumber(origSeq);
		packet->SetTimestamp(origTimestamp);
		packet->SetMarker(origMarker);

		// Restore the original payload if needed.
		packet->RestorePayload();

		// If sharedPacket doesn't have a packet inside and it has been stored we
		// need to clone the packet into it.
		if (!sharedPacket.HasPacket() && result == RTC::RTP::RtpStreamSend::ReceivePacketResult::ACCEPTED_AND_STORED)
		{
			sharedPacket.Assign(packet);
		}

		// If sent packet was the first packet of a key frame, let's send buffered
		// packets belonging to the same key frame that arrived earlier due to
		// packet misorder.
		if (action.sendBufferedPackets)
		{
			// NOTE: Only send buffered packets if the first packet containing the
			// key frame was sent.
			if (result != RTC::RTP::RtpStreamSend::ReceivePacketResult::DISCARDED)
			{
				for (auto& kv : *targetLayerRetransmissionBuffer)
				{
					auto& bufferedSharedPacket = kv.second;
					auto* bufferedPacket       = bufferedSharedPacket.GetPacket();

					if (bufferedPacket->GetSequenceNumber() > origSeq)
					{
						MS_DEBUG_DEV(
						  "sending packet buffered in the target layer retransmission buffer "
						  "[ssrc:%" PRIu32 ", seq:%" PRIu16 ", ts:%" PRIu32
						  "] after sending first packet of the key frame [ssrc:%" PRIu32 ", seq:%" PRIu16
						  ", ts:%" PRIu32 "]",
						  bufferedPacket->GetSsrc(),
						  bufferedPacket->GetSequenceNumber(),
						  bufferedPacket->GetTimestamp(),
						  packet->GetSsrc(),
						  packet->GetSequenceNumber(),
						  packet->GetTimestamp());

						SendRtpPacket(bufferedPacket, bufferedSharedPacket);

						// Be sure that the target layer retransmission buffer has not
						// been emptied as a result of sending this packet. If so, exit
						// the loop.
						if (targetLayerRetransmissionBuffer->empty())
						{
							MS_DEBUG_DEV(
							  "target layer retransmission buffer emptied while iterating "
							  "it, exiting the loop");

							break;
						}
					}
				}
			}

			targetLayerRetransmissionBuffer->clear();
		}
	}

	bool Consumer::GetRtcp(RTC::RTCP::CompoundPacket* packet, uint64_t nowMs)
	{
		MS_TRACE();

		// Special condition for pipe consumer since this method will be called in a
		// loop for each stream.
		if (this->pipe)
		{
			if (
			  nowMs != this->lastRtcpSentTime &&
			  static_cast<float>((nowMs - this->lastRtcpSentTime) * 1.15) < this->maxRtcpInterval)
			{
				return true;
			}
		}
		else if (static_cast<float>((nowMs - this->lastRtcpSentTime) * 1.15) < this->maxRtcpInterval)
		{
			return true;
		}

		std::vector<RTCP::SenderReport*> senderReports;
		std::vector<RTCP::SdesChunk*> sdesChunks;
		std::vector<RTCP::DelaySinceLastRr::SsrcInfo*> delaySinceLastRrSsrcInfos;

		for (auto* rtpStream : this->rtpStreams)
		{
			auto* report = rtpStream->GetRtcpSenderReport(nowMs);

			if (!report)
			{
				continue;
			}

			senderReports.push_back(report);

			// Build SDES chunk for this sender.
			auto* sdesChunk = rtpStream->GetRtcpSdesChunk();
			sdesChunks.push_back(sdesChunk);

			auto* delaySinceLastRrSsrcInfo = rtpStream->GetRtcpXrDelaySinceLastRrSsrcInfo(nowMs);

			if (delaySinceLastRrSsrcInfo)
			{
				delaySinceLastRrSsrcInfos.push_back(delaySinceLastRrSsrcInfo);
			}
		}

		// RTCP Compound packet buffer cannot hold the data.
		if (!packet->Add(senderReports, sdesChunks, delaySinceLastRrSsrcInfos))
		{
			return false;
		}

		this->lastRtcpSentTime = nowMs;

		return true;
	}

	void Consumer::NeedWorstRemoteFractionLost(uint32_t /*mappedSsrc*/, uint8_t& worstRemoteFractionLost)
	{
		MS_TRACE();

		if (!IsActive())
		{
			return;
		}

		for (auto* rtpStream : this->rtpStreams)
		{
			auto fractionLost = rtpStream->GetFractionLost();

			// If our fraction lost is worse than the given one, update it.
			worstRemoteFractionLost = std::max(fractionLost, worstRemoteFractionLost);
		}
	}

	void Consumer::ReceiveNack(RTC::RTCP::FeedbackRtpNackPacket* nackPacket)
	{
		MS_TRACE();

		if (!IsActive())
		{
			return;
		}

		// May emit 'trace' event.
		EmitTraceEventNackType();

		RTC::RTP::RtpStreamSend* rtpStream;

		if (this->pipe)
		{
			auto ssrc = nackPacket->GetMediaSsrc();
			rtpStream = this->mapSsrcRtpStream.at(ssrc);
		}
		else
		{
			rtpStream = this->mapSsrcRtpStream.begin()->second;
		}

		rtpStream->ReceiveNack(nackPacket);
	}

	void Consumer::ReceiveKeyFrameRequest(RTC::RTCP::FeedbackPs::MessageType messageType, uint32_t ssrc)
	{
		MS_TRACE();

		if (this->kind != RTC::Media::Kind::VIDEO)
		{
			return;
		}

		switch (messageType)
		{
			case RTC::RTCP::FeedbackPs::MessageType::PLI:
			{
				EmitTraceEventPliType(ssrc);

				break;
			}

			case RTC::RTCP::FeedbackPs::MessageType::FIR:
			{
				EmitTraceEventFirType(ssrc);

				break;
			}

			default:;
		}

		auto* rtpStream = this->mapSsrcRtpStream.at(ssrc);

		rtpStream->ReceiveKeyFrameRequest(messageType);

		if (IsActive())
		{
			if (this->pipe)
			{
				for (auto& consumableRtpEncoding : this->consumableRtpEncodings)
				{
					auto mappedSsrc = consumableRtpEncoding.ssrc;

					this->listener->OnConsumerKeyFrameRequested(this, mappedSsrc);
				}
			}
			else
			{
				this->producerStreamManager->RequestKeyFrameForCurrentSpatialLayer();
			}
		}
	}

	void Consumer::ReceiveRtcpReceiverReport(RTC::RTCP::ReceiverReport* report)
	{
		MS_TRACE();

		auto* rtpStream = this->mapSsrcRtpStream.at(report->GetSsrc());

		rtpStream->ReceiveRtcpReceiverReport(report);
	}

	void Consumer::ReceiveRtcpXrReceiverReferenceTime(RTC::RTCP::ReceiverReferenceTime* report)
	{
		MS_TRACE();

		for (auto* rtpStream : this->rtpStreams)
		{
			rtpStream->ReceiveRtcpXrReceiverReferenceTime(report);
		}
	}

	uint32_t Consumer::GetTransmissionRate(uint64_t nowMs)
	{
		MS_TRACE();

		if (!IsActive())
		{
			return 0u;
		}

		uint32_t rate{ 0u };

		for (auto* rtpStream : this->rtpStreams)
		{
			rate += rtpStream->GetBitrate(nowMs);
		}

		return rate;
	}

	float Consumer::GetRtt() const
	{
		MS_TRACE();

		float rtt{ 0 };

		for (auto* rtpStream : this->rtpStreams)
		{
			rtt = std::max(rtpStream->GetRtt(), rtt);
		}

		return rtt;
	}

	void Consumer::UserOnTransportConnected()
	{
		MS_TRACE();

		this->producerStreamManager->OnTransportConnected();
	}

	void Consumer::UserOnTransportDisconnected()
	{
		MS_TRACE();

		for (auto* rtpStream : this->rtpStreams)
		{
			rtpStream->Pause();
		}

		for (auto& kv : this->mapRtpStreamTargetLayerRetransmissionBuffer)
		{
			auto& targetLayerRetransmissionBuffer = kv.second;

			targetLayerRetransmissionBuffer.clear();
		}

		this->producerStreamManager->OnTransportDisconnected();
	}

	void Consumer::UserOnPaused()
	{
		MS_TRACE();

		for (auto* rtpStream : this->rtpStreams)
		{
			rtpStream->Pause();
		}

		for (auto& kv : this->mapRtpStreamTargetLayerRetransmissionBuffer)
		{
			auto& targetLayerRetransmissionBuffer = kv.second;

			targetLayerRetransmissionBuffer.clear();
		}

		this->producerStreamManager->OnPaused();

		if (this->externallyManagedBitrate)
		{
			// Audio does not play the BWE game.
			if (this->kind != RTC::Media::Kind::VIDEO)
			{
				return;
			}

			this->listener->OnConsumerNeedZeroBitrate(this);
		}
	}

	void Consumer::UserOnResumed()
	{
		MS_TRACE();

		this->producerStreamManager->OnResumed();
	}

	void Consumer::CreateRtpStreams()
	{
		MS_TRACE();

		// NOTE: For non-pipe consumers, all spatial layers are multiplexed through
		// a single RtpStreamSend (encodings[0]). Pipe consumers need one stream per
		// encoding. Here we know that SSRCs in Consumer's rtpParameters must be the
		// same as in the given consumableRtpEncodings.
		const size_t numStreams = this->pipe ? this->rtpParameters.encodings.size() : 1u;

		for (size_t idx{ 0u }; idx < numStreams; ++idx)
		{
			auto& encoding           = this->rtpParameters.encodings[idx];
			const auto* mediaCodec   = this->rtpParameters.GetCodecForEncoding(encoding);
			auto& consumableEncoding = this->consumableRtpEncodings[idx];

			MS_DEBUG_TAG(
			  rtp, "[ssrc:%" PRIu32 ", payloadType:%" PRIu8 "]", encoding.ssrc, mediaCodec->payloadType);

			// Set stream params.
			RTC::RTP::RtpStream::Params params;

			params.encodingIdx    = idx;
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

			for (const auto& fb : mediaCodec->rtcpFeedback)
			{
				if (!params.useNack && fb.type == "nack" && fb.parameter.empty())
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

			auto* rtpStream =
			  new RTC::RTP::RtpStreamSend(this, this->shared, params, this->rtpParameters.mid);

			// If the Consumer is paused, tell the RtpStreamSend.
			if (IsPaused() || IsProducerPaused())
			{
				rtpStream->Pause();
			}

			const auto* rtxCodec = this->rtpParameters.GetRtxCodecForEncoding(encoding);

			if (rtxCodec && encoding.hasRtx)
			{
				rtpStream->SetRtx(rtxCodec->payloadType, encoding.rtx.ssrc);
			}

			this->rtpStreams.push_back(rtpStream);
			this->mapMappedSsrcSsrc[consumableEncoding.ssrc] = encoding.ssrc;
			this->mapSsrcRtpStream[encoding.ssrc]            = rtpStream;

			// Let's choose an initial output seq number between 1000 and 32768 to avoid
			// libsrtp bug:
			// https://github.com/versatica/mediasoup/issues/1437
			const auto initialOutputSeq =
			  Utils::Crypto::GetRandomUInt<uint16_t>(1000u, std::numeric_limits<uint16_t>::max() / 2);

			this->mapRtpStreamRtpSeqManager[rtpStream] = RTC::SeqManager<uint16_t>(initialOutputSeq);

			this->mapRtpStreamTargetLayerRetransmissionBuffer[rtpStream];
		}
	}

	void Consumer::StorePacketInTargetLayerRetransmissionBuffer(
	  std::map<uint16_t, RTC::RTP::SharedPacket, RTC::SeqManager<uint16_t>::SeqLowerThan>&
	    targetLayerRetransmissionBuffer,
	  RTC::RTP::Packet* packet,
	  RTC::RTP::SharedPacket& sharedPacket)
	{
		MS_TRACE();

		MS_DEBUG_DEV(
		  "storing packet in target layer retransmission buffer [ssrc:%" PRIu32 ", seq:%" PRIu16
		  ", ts:%" PRIu32 "]",
		  packet->GetSsrc(),
		  packet->GetSequenceNumber(),
		  packet->GetTimestamp());

		// Store original packet into the buffer. Only clone once and only if
		// necessary.
		if (!sharedPacket.HasPacket())
		{
			sharedPacket.Assign(packet);
		}
		// Assert that, if sharedPacket was already filled, both packet and
		// sharedPacket are the very same RTP packet.
		else
		{
			sharedPacket.AssertSamePacket(packet);
		}

		targetLayerRetransmissionBuffer[packet->GetSequenceNumber()] = sharedPacket;

		if (targetLayerRetransmissionBuffer.size() > TargetLayerRetransmissionBufferSize)
		{
			targetLayerRetransmissionBuffer.erase(targetLayerRetransmissionBuffer.begin());
		}
	}

	void Consumer::EmitScore() const
	{
		MS_TRACE();

		// Pipe consumers never emit score events: score is always a constant 10/10.
		if (this->pipe)
		{
			return;
		}

		auto scoreOffset = FillBufferScore(this->shared->GetChannelNotifier()->GetBufferBuilder());

		auto notificationOffset = FBS::Consumer::CreateScoreNotification(
		  this->shared->GetChannelNotifier()->GetBufferBuilder(), scoreOffset);

		this->shared->GetChannelNotifier()->Emit(
		  this->id,
		  FBS::Notification::Event::CONSUMER_SCORE,
		  FBS::Notification::Body::Consumer_ScoreNotification,
		  notificationOffset);
	}

	void Consumer::EmitLayersChange() const
	{
		MS_TRACE();

		MS_DEBUG_DEV(
		  "current layers changed to [spatial:%" PRIi16 ", temporal:%" PRIi16 ", consumerId:%s]",
		  this->producerStreamManager->GetCurrentSpatialLayer(),
		  this->producerStreamManager->GetCurrentTemporalLayer(),
		  this->id.c_str());

		flatbuffers::Offset<FBS::Consumer::ConsumerLayers> layersOffset;

		if (this->producerStreamManager->GetCurrentSpatialLayer() >= 0)
		{
			layersOffset = FBS::Consumer::CreateConsumerLayers(
			  this->shared->GetChannelNotifier()->GetBufferBuilder(),
			  this->producerStreamManager->GetCurrentSpatialLayer(),
			  this->producerStreamManager->GetCurrentTemporalLayer());
		}

		auto notificationOffset = FBS::Consumer::CreateLayersChangeNotification(
		  this->shared->GetChannelNotifier()->GetBufferBuilder(), layersOffset);

		this->shared->GetChannelNotifier()->Emit(
		  this->id,
		  FBS::Notification::Event::CONSUMER_LAYERS_CHANGE,
		  FBS::Notification::Body::Consumer_LayersChangeNotification,
		  notificationOffset);
	}

	void Consumer::EmitTraceEventRtpAndKeyFrameTypes(const RTC::RTP::Packet* packet, bool isRtx) const
	{
		MS_TRACE();

		if (this->traceEventTypes.keyframe && packet->IsKeyFrame())
		{
			auto rtpPacketDump = packet->FillBuffer(this->shared->GetChannelNotifier()->GetBufferBuilder());
			auto traceInfo = FBS::Consumer::CreateKeyFrameTraceInfo(
			  this->shared->GetChannelNotifier()->GetBufferBuilder(), rtpPacketDump, isRtx);

			auto notification = FBS::Consumer::CreateTraceNotification(
			  this->shared->GetChannelNotifier()->GetBufferBuilder(),
			  FBS::Consumer::TraceEventType::KEYFRAME,
			  this->shared->GetTimeMs(),
			  FBS::Common::TraceDirection::DIRECTION_OUT,
			  FBS::Consumer::TraceInfo::KeyFrameTraceInfo,
			  traceInfo.Union());

			EmitTraceEvent(notification);
		}
		else if (this->traceEventTypes.rtp)
		{
			auto rtpPacketDump = packet->FillBuffer(this->shared->GetChannelNotifier()->GetBufferBuilder());
			auto traceInfo = FBS::Consumer::CreateRtpTraceInfo(
			  this->shared->GetChannelNotifier()->GetBufferBuilder(), rtpPacketDump, isRtx);

			auto notification = FBS::Consumer::CreateTraceNotification(
			  this->shared->GetChannelNotifier()->GetBufferBuilder(),
			  FBS::Consumer::TraceEventType::RTP,
			  this->shared->GetTimeMs(),
			  FBS::Common::TraceDirection::DIRECTION_OUT,
			  FBS::Consumer::TraceInfo::RtpTraceInfo,
			  traceInfo.Union());

			EmitTraceEvent(notification);
		}
	}

	void Consumer::EmitTraceEventPliType(uint32_t ssrc) const
	{
		MS_TRACE();

		if (!this->traceEventTypes.pli)
		{
			return;
		}

		auto traceInfo = FBS::Consumer::CreatePliTraceInfo(
		  this->shared->GetChannelNotifier()->GetBufferBuilder(), ssrc);

		auto notification = FBS::Consumer::CreateTraceNotification(
		  this->shared->GetChannelNotifier()->GetBufferBuilder(),
		  FBS::Consumer::TraceEventType::PLI,
		  this->shared->GetTimeMs(),
		  FBS::Common::TraceDirection::DIRECTION_IN,
		  FBS::Consumer::TraceInfo::PliTraceInfo,
		  traceInfo.Union());

		EmitTraceEvent(notification);
	}

	void Consumer::EmitTraceEventFirType(uint32_t ssrc) const
	{
		MS_TRACE();

		if (!this->traceEventTypes.fir)
		{
			return;
		}

		auto traceInfo = FBS::Consumer::CreateFirTraceInfo(
		  this->shared->GetChannelNotifier()->GetBufferBuilder(), ssrc);

		auto notification = FBS::Consumer::CreateTraceNotification(
		  this->shared->GetChannelNotifier()->GetBufferBuilder(),
		  FBS::Consumer::TraceEventType::FIR,
		  this->shared->GetTimeMs(),
		  FBS::Common::TraceDirection::DIRECTION_IN,
		  FBS::Consumer::TraceInfo::FirTraceInfo,
		  traceInfo.Union());

		EmitTraceEvent(notification);
	}

	void Consumer::EmitTraceEventNackType() const
	{
		MS_TRACE();

		if (!this->traceEventTypes.nack)
		{
			return;
		}

		auto notification = FBS::Consumer::CreateTraceNotification(
		  this->shared->GetChannelNotifier()->GetBufferBuilder(),
		  FBS::Consumer::TraceEventType::NACK,
		  this->shared->GetTimeMs(),
		  FBS::Common::TraceDirection::DIRECTION_IN);

		EmitTraceEvent(notification);
	}

	void Consumer::EmitTraceEvent(flatbuffers::Offset<FBS::Consumer::TraceNotification>& notification) const
	{
		MS_TRACE();

		this->shared->GetChannelNotifier()->Emit(
		  this->id,
		  FBS::Notification::Event::CONSUMER_TRACE,
		  FBS::Notification::Body::Consumer_TraceNotification,
		  notification);
	}

	void Consumer::OnRtpStreamScore(
	  RTC::RTP::RtpStream* /*rtpStream*/, uint8_t /*score*/, uint8_t /*previousScore*/)
	{
		MS_TRACE();

		// Emit the score event.
		EmitScore();

		// NOTE @jmillan: Previously present in Simulcast and SVC. Does it make sense to move it here?
		// Previously present in Simulcast and SVC. Does it make sense to move it here?
		if (IsActive())
		{
			// Just check target layers if our bitrate is not externally managed.
			if (!this->externallyManagedBitrate)
			{
				this->producerStreamManager->MayChangeLayers(/*force*/ false);
			}
		}
	}

	void Consumer::OnRtpStreamRetransmitRtpPacket(
	  RTC::RTP::RtpStreamSend* rtpStream, RTC::RTP::Packet* packet)
	{
		MS_TRACE();

		this->listener->OnConsumerRetransmitRtpPacket(this, packet);

		// May emit 'trace' event.
		EmitTraceEventRtpAndKeyFrameTypes(packet, rtpStream->HasRtx());
	}

	/* ProducerStreamManager::Listener methods. */

	void Consumer::OnProducerStreamManagerKeyFrameRequested(uint32_t mappedSsrc)
	{
		MS_TRACE();

		this->listener->OnConsumerKeyFrameRequested(this, mappedSsrc);
	}

	void Consumer::OnProducerStreamManagerNeedBitrateChange()
	{
		MS_TRACE();

		this->listener->OnConsumerNeedBitrateChange(this);
	}

	void Consumer::OnProducerStreamManagerLayersChanged()
	{
		MS_TRACE();

		EmitLayersChange();
	}

	void Consumer::OnProducerStreamManagerClearRetransmissionBuffer()
	{
		MS_TRACE();

		for (auto& kv : this->mapRtpStreamTargetLayerRetransmissionBuffer)
		{
			kv.second.clear();
		}
	}

	void Consumer::OnProducerStreamManagerScore()
	{
		MS_TRACE();

		EmitScore();
	}
} // namespace RTC
