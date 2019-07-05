#define MS_CLASS "RTC::PipeConsumer"
// #define MS_LOG_DEV

#include "RTC/PipeConsumer.hpp"
#include "DepLibUV.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "RTC/Codecs/Codecs.hpp"

namespace RTC
{
	/* Instance methods. */

	PipeConsumer::PipeConsumer(const std::string& id, RTC::Consumer::Listener* listener, json& data)
	  : RTC::Consumer::Consumer(id, listener, data, RTC::RtpParameters::Type::PIPE)
	{
		MS_TRACE();

		// Ensure there are as many encodings as consumable encodings.
		if (this->rtpParameters.encodings.size() != this->consumableRtpEncodings.size())
			MS_THROW_TYPE_ERROR("number of rtpParameters.encodings and consumableRtpEncodings do not match");

		auto& encoding   = this->rtpParameters.encodings[0];
		auto* mediaCodec = this->rtpParameters.GetCodecForEncoding(encoding);

		this->keyFrameSupported = RTC::Codecs::CanBeKeyFrame(mediaCodec->mimeType);

		// Create RtpStreamSend instances.
		CreateRtpStreams();
	}

	PipeConsumer::~PipeConsumer()
	{
		MS_TRACE();

		for (auto& kv : this->mapMappedSsrcRtpStream)
		{
			auto* rtpStream = kv.second;

			delete rtpStream;
		}
		this->mapMappedSsrcRtpStream.clear();
	}

	void PipeConsumer::FillJson(json& jsonObject) const
	{
		MS_TRACE();

		// Call the parent method.
		RTC::Consumer::FillJson(jsonObject);

		// Add rtpStreams.
		jsonObject["rtpStreams"] = json::array();
		auto jsonRtpStreamsIt    = jsonObject.find("rtpStreams");

		for (auto& kv : this->mapMappedSsrcRtpStream)
		{
			jsonRtpStreamsIt->emplace_back(json::value_t::object);

			auto& jsonEntry = (*jsonRtpStreamsIt)[jsonRtpStreamsIt->size() - 1];
			auto* rtpStream = kv.second;

			rtpStream->FillJson(jsonEntry);
		}
	}

	void PipeConsumer::FillJsonStats(json& jsonArray) const
	{
		MS_TRACE();

		// Add stats of our send streams.
		for (auto& kv : this->mapMappedSsrcRtpStream)
		{
			auto* rtpStream = kv.second;

			jsonArray.emplace_back(json::value_t::object);
			rtpStream->FillJsonStats(jsonArray[jsonArray.size() - 1]);
		}
	}

	void PipeConsumer::FillJsonScore(json& /*jsonObject*/) const
	{
		MS_TRACE();

		// Do nothing.
	}

	void PipeConsumer::HandleRequest(Channel::Request* request)
	{
		MS_TRACE();

		switch (request->methodId)
		{
			case Channel::Request::MethodId::CONSUMER_REQUEST_KEY_FRAME:
			{
				if (IsActive())
					RequestKeyFrame();

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

	void PipeConsumer::ProducerRtpStream(RTC::RtpStream* /*rtpStream*/, uint32_t /*mappedSsrc*/)
	{
		MS_TRACE();

		// Do nothing.
	}

	void PipeConsumer::ProducerNewRtpStream(RTC::RtpStream* /*rtpStream*/, uint32_t /*mappedSsrc*/)
	{
		MS_TRACE();

		// Do nothing.
	}

	void PipeConsumer::ProducerRtpStreamScore(
	  RTC::RtpStream* /*rtpStream*/, uint8_t /*score*/, uint8_t /*previousScore*/)
	{
		MS_TRACE();

		// Do nothing.
	}

	void PipeConsumer::ProducerRtcpSenderReport(RTC::RtpStream* /*rtpStream*/, bool /*first*/)
	{
		MS_TRACE();

		// Do nothing.
	}

	void PipeConsumer::SendRtpPacket(RTC::RtpPacket* packet)
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

		auto* rtpStream     = this->mapMappedSsrcRtpStream.at(packet->GetSsrc());
		auto& syncRequired  = this->mapRtpStreamSyncRequired.at(rtpStream);
		auto& rtpSeqManager = this->mapRtpStreamRtpSeqManager.at(rtpStream);

		// If we need to sync, support key frames and this is not a key frame, ignore
		// the packet.
		if (syncRequired && this->keyFrameSupported && !packet->IsKeyFrame())
			return;

		// Whether this is the first packet after re-sync.
		bool isSyncPacket = syncRequired;

		// Sync sequence number and timestamp if required.
		if (isSyncPacket)
		{
			if (packet->IsKeyFrame())
				MS_DEBUG_TAG(rtp, "sync key frame received");

			rtpSeqManager.Sync(packet->GetSequenceNumber() - 1);

			syncRequired = false;
		}

		// Update RTP seq number and timestamp.
		uint16_t seq;

		rtpSeqManager.Input(packet->GetSequenceNumber(), seq);

		// Save original packet fields.
		auto origSeq = packet->GetSequenceNumber();

		// Rewrite packet.
		// NOTE: Do not override the ssrc because we want to honor the consumable ssrcs.
		packet->SetSequenceNumber(seq);

		if (isSyncPacket)
		{
			MS_DEBUG_TAG(
			  rtp,
			  "sending sync packet [ssrc:%" PRIu32 ", seq:%" PRIu16 ", ts:%" PRIu32
			  "] from original [seq:%" PRIu16 "]",
			  packet->GetSsrc(),
			  packet->GetSequenceNumber(),
			  packet->GetTimestamp(),
			  origSeq);
		}

		// Process the packet.
		if (rtpStream->ReceivePacket(packet))
		{
			// Send the packet.
			this->listener->OnConsumerSendRtpPacket(this, packet);
		}
		else
		{
			MS_WARN_TAG(
			  rtp,
			  "failed to send packet [ssrc:%" PRIu32 ", seq:%" PRIu16 ", ts:%" PRIu32
			  "] from original [seq:%" PRIu16 "]",
			  packet->GetSsrc(),
			  packet->GetSequenceNumber(),
			  packet->GetTimestamp(),
			  origSeq);
		}

		// Restore packet fields.
		packet->SetSequenceNumber(origSeq);
	}

	void PipeConsumer::SendProbationRtpPacket(uint16_t /*seq*/)
	{
		MS_TRACE();

		MS_ABORT("should not call this method");
	}

	void PipeConsumer::GetRtcp(
	  RTC::RTCP::CompoundPacket* packet, RTC::RtpStreamSend* rtpStream, uint64_t now)
	{
		MS_TRACE();

		MS_ASSERT(
		  std::find(this->rtpStreams.begin(), this->rtpStreams.end(), rtpStream) != this->rtpStreams.end(),
		  "RTP stream does exist");

		// Special condition for PipeConsumer since this method will be called in a loop for
		// each stream in this PipeConsumer.
		// clang-format off
		if (
			now != this->lastRtcpSentTime &&
			static_cast<float>((now - this->lastRtcpSentTime) * 1.15) < this->maxRtcpInterval
		)
		// clang-format on
		{
			return;
		}

		auto* report = rtpStream->GetRtcpSenderReport(now);

		if (!report)
			return;

		packet->AddSenderReport(report);

		// Build SDES chunk for this sender.
		auto* sdesChunk = rtpStream->GetRtcpSdesChunk();

		packet->AddSdesChunk(sdesChunk);

		this->lastRtcpSentTime = now;
	}

	void PipeConsumer::NeedWorstRemoteFractionLost(uint32_t /*mappedSsrc*/, uint8_t& worstRemoteFractionLost)
	{
		MS_TRACE();

		if (!IsActive())
			return;

		for (auto& kv : this->mapMappedSsrcRtpStream)
		{
			auto& rtpStream   = kv.second;
			auto fractionLost = rtpStream->GetFractionLost();

			// If our fraction lost is worse than the given one, update it.
			if (fractionLost > worstRemoteFractionLost)
				worstRemoteFractionLost = fractionLost;
		}
	}

	void PipeConsumer::ReceiveNack(RTC::RTCP::FeedbackRtpNackPacket* /*nackPacket*/)
	{
		MS_TRACE();

		// Do nothing since we do not enable NACK.
	}

	void PipeConsumer::ReceiveKeyFrameRequest(RTC::RTCP::FeedbackPs::MessageType messageType, uint32_t ssrc)
	{
		MS_TRACE();

		auto* rtpStream = this->mapMappedSsrcRtpStream.at(ssrc);

		rtpStream->ReceiveKeyFrameRequest(messageType);

		if (IsActive())
			RequestKeyFrame();
	}

	void PipeConsumer::ReceiveRtcpReceiverReport(RTC::RTCP::ReceiverReport* report)
	{
		MS_TRACE();

		auto* rtpStream = this->mapMappedSsrcRtpStream.at(report->GetSsrc());

		rtpStream->ReceiveRtcpReceiverReport(report);
	}

	uint32_t PipeConsumer::GetTransmissionRate(uint64_t now)
	{
		MS_TRACE();

		if (!IsActive())
			return 0u;

		uint32_t rate{ 0 };

		for (auto& kv : this->mapMappedSsrcRtpStream)
		{
			auto& rtpStream = kv.second;

			rate += rtpStream->GetBitrate(now);
		}

		return rate;
	}

	void PipeConsumer::UserOnTransportConnected()
	{
		MS_TRACE();

		for (auto& kv : this->mapRtpStreamSyncRequired)
		{
			kv.second = true;
		}

		if (IsActive())
		{
			for (auto& kv : this->mapMappedSsrcRtpStream)
			{
				auto& rtpStream = kv.second;

				rtpStream->Resume();
			}

			RequestKeyFrame();
		}
	}

	void PipeConsumer::UserOnTransportDisconnected()
	{
		MS_TRACE();

		for (auto& kv : this->mapMappedSsrcRtpStream)
		{
			auto& rtpStream = kv.second;

			rtpStream->Pause();
		}
	}

	void PipeConsumer::UserOnPaused()
	{
		MS_TRACE();

		for (auto& kv : this->mapMappedSsrcRtpStream)
		{
			auto& rtpStream = kv.second;

			rtpStream->Pause();
		}
	}

	void PipeConsumer::UserOnResumed()
	{
		MS_TRACE();

		for (auto& kv : this->mapRtpStreamSyncRequired)
		{
			kv.second = true;
		}

		if (IsActive())
		{
			for (auto& kv : this->mapMappedSsrcRtpStream)
			{
				auto& rtpStream = kv.second;

				rtpStream->Resume();
			}

			RequestKeyFrame();
		}
	}

	void PipeConsumer::CreateRtpStreams()
	{
		MS_TRACE();

		// NOTE: Here we know that SSRCs in Consumer's rtpParameters must be the same
		// as in the given consumableRtpEncodings.
		for (auto& encoding : this->rtpParameters.encodings)
		{
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
				// NOTE: Do not consider NACK in PipeConsumer.

				if (!params.usePli && fb.type == "nack" && fb.parameter == "pli")
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
			// NOTE: PipeConsumer does not support NACK.
			size_t bufferSize{ 0 };
			auto* rtpStream = new RTC::RtpStreamSend(this, params, bufferSize);

			// If the Consumer is paused, tell the RtpStreamSend.
			if (IsPaused() || IsProducerPaused())
				rtpStream->Pause();

			auto* rtxCodec = this->rtpParameters.GetRtxCodecForEncoding(encoding);

			if (rtxCodec && encoding.hasRtx)
				rtpStream->SetRtx(rtxCodec->payloadType, encoding.rtx.ssrc);

			this->mapMappedSsrcRtpStream[encoding.ssrc] = rtpStream;
			this->rtpStreams.push_back(rtpStream);
			this->mapRtpStreamSyncRequired[rtpStream] = false;
			this->mapRtpStreamRtpSeqManager[rtpStream];
		}
	}

	void PipeConsumer::RequestKeyFrame()
	{
		MS_TRACE();

		if (this->kind != RTC::Media::Kind::VIDEO)
			return;

		for (auto& consumableRtpEncoding : this->consumableRtpEncodings)
		{
			auto mappedSsrc = consumableRtpEncoding.ssrc;

			this->listener->OnConsumerKeyFrameRequested(this, mappedSsrc);
		}
	}

	inline void PipeConsumer::OnRtpStreamScore(
	  RTC::RtpStream* /*rtpStream*/, uint8_t /*score*/, uint8_t /*previousScore*/)
	{
		MS_TRACE();

		// Do nothing.
	}

	inline void PipeConsumer::OnRtpStreamRetransmitRtpPacket(
	  RTC::RtpStreamSend* /*rtpStream*/, RTC::RtpPacket* packet, bool probation)
	{
		MS_TRACE();

		this->listener->OnConsumerRetransmitRtpPacket(this, packet, probation);
	}
} // namespace RTC
