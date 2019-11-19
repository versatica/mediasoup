#define MS_CLASS "RTC::ShmConsumer"
// #define MS_LOG_DEV

#include "RTC/ShmConsumer.hpp"
#include "DepLibUV.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "Channel/Notifier.hpp"
#include "RTC/Codecs/Codecs.hpp"

namespace RTC
{
	/* Instance methods. */

	ShmConsumer::ShmConsumer(const std::string& id, RTC::Consumer::Listener* listener, json& data)
	  : RTC::Consumer::Consumer(id, listener, data, RTC::RtpParameters::Type::SHM)
	{
		MS_TRACE();

		// Ensure there is a single encoding.
		if (this->consumableRtpEncodings.size() != 1)
			MS_THROW_TYPE_ERROR("invalid consumableRtpEncodings with size != 1");

		auto& encoding   = this->rtpParameters.encodings[0];
		auto* mediaCodec = this->rtpParameters.GetCodecForEncoding(encoding);
		if (!RTC::Codecs::IsValidTypeForCodec(this->type, mediaCodec->mimeType))
		{
			MS_THROW_TYPE_ERROR("%s codec not supported for svc", mediaCodec->mimeType.ToString().c_str());
		}

		this->keyFrameSupported = RTC::Codecs::CanBeKeyFrame(mediaCodec->mimeType);

		// Create RtpStreamSend instance for sending a single stream to the remote.
		CreateRtpStream();
	}

	ShmConsumer::~ShmConsumer()
	{
		MS_TRACE();

		delete this->rtpStream;
	}

	void ShmConsumer::FillJson(json& jsonObject) const
	{
		MS_TRACE();

		// Call the parent method.
		RTC::Consumer::FillJson(jsonObject);

		// Add rtpStream.
		this->rtpStream->FillJson(jsonObject["rtpStream"]);
	}

	void ShmConsumer::FillJsonStats(json& jsonArray) const
	{
		MS_TRACE();

		// Add stats of our send stream.
		jsonArray.emplace_back(json::value_t::object);
		this->rtpStream->FillJsonStats(jsonArray[0]);

		// Add stats of our recv stream.
		if (this->producerRtpStream)
		{
			jsonArray.emplace_back(json::value_t::object);
			this->producerRtpStream->FillJsonStats(jsonArray[1]);
		}
	}

	void ShmConsumer::FillJsonScore(json& jsonObject) const
	{
		MS_TRACE();

		jsonObject["score"] = this->rtpStream->GetScore();

		if (this->producerRtpStream)
			jsonObject["producerScore"] = this->producerRtpStream->GetScore();
		else
			jsonObject["producerScore"] = 0;
	}

	void ShmConsumer::HandleRequest(Channel::Request* request)
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

	void ShmConsumer::ProducerRtpStream(RTC::RtpStream* rtpStream, uint32_t /*mappedSsrc*/)
	{
		MS_TRACE();

		this->producerRtpStream = rtpStream;

		// Emit the score event.
		EmitScore();
	}

	void ShmConsumer::ProducerNewRtpStream(RTC::RtpStream* rtpStream, uint32_t /*mappedSsrc*/)
	{
		MS_TRACE();

		this->producerRtpStream = rtpStream;

		// Emit the score event.
		EmitScore();
	}

	void ShmConsumer::ProducerRtpStreamScore(
	  RTC::RtpStream* /*rtpStream*/, uint8_t /*score*/, uint8_t /*previousScore*/)
	{
		MS_TRACE();

		// Emit the score event.
		EmitScore();
	}

	void ShmConsumer::ProducerRtcpSenderReport(RTC::RtpStream* /*rtpStream*/, bool /*first*/)
	{
		MS_TRACE();

		// Do nothing.
	}

	void ShmConsumer::SendRtpPacket(RTC::RtpPacket* packet)
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

			this->rtpSeqManager.Sync(packet->GetSequenceNumber() - 1);

			this->syncRequired = false;
		}

		// Update RTP seq number and timestamp.
		uint16_t seq;

		this->rtpSeqManager.Input(packet->GetSequenceNumber(), seq);

		// Save original packet fields.
		auto origSsrc = packet->GetSsrc();
		auto origSeq  = packet->GetSequenceNumber();

		// Rewrite packet.
		packet->SetSsrc(this->rtpParameters.encodings[0].ssrc);
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
			  "] from original [seq:%" PRIu16 "]",
			  packet->GetSsrc(),
			  packet->GetSequenceNumber(),
			  packet->GetTimestamp(),
			  origSeq);
		}

		// Restore packet fields.
		packet->SetSsrc(origSsrc);
		packet->SetSequenceNumber(origSeq);
	}

	void ShmConsumer::SendProbationRtpPacket(uint16_t seq)
	{
		MS_TRACE();

		this->rtpStream->SendProbationRtpPacket(seq);
	}

	void ShmConsumer::GetRtcp(
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

	void ShmConsumer::NeedWorstRemoteFractionLost(
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

	void ShmConsumer::ReceiveNack(RTC::RTCP::FeedbackRtpNackPacket* nackPacket)
	{
		MS_TRACE();

		if (!IsActive())
			return;

		this->rtpStream->ReceiveNack(nackPacket);
	}

	void ShmConsumer::ReceiveKeyFrameRequest(
	  RTC::RTCP::FeedbackPs::MessageType messageType, uint32_t /*ssrc*/)
	{
		MS_TRACE();

		this->rtpStream->ReceiveKeyFrameRequest(messageType);

		if (IsActive())
			RequestKeyFrame();
	}

	void ShmConsumer::ReceiveRtcpReceiverReport(RTC::RTCP::ReceiverReport* report)
	{
		MS_TRACE();
	}

	uint32_t ShmConsumer::GetTransmissionRate(uint64_t now)
	{
		MS_TRACE();

		return 0u;
	}

	void ShmConsumer::UserOnTransportConnected()
	{
		MS_TRACE();

		this->syncRequired = true;

		if (IsActive())
			RequestKeyFrame();
	}

	void ShmConsumer::UserOnTransportDisconnected()
	{
		MS_TRACE();

		this->rtpStream->Pause();
	}

	void ShmConsumer::UserOnPaused()
	{
		MS_TRACE();

		this->rtpStream->Pause();
	}

	void ShmConsumer::UserOnResumed()
	{
		MS_TRACE();

		this->syncRequired = true;

		if (IsActive())
			RequestKeyFrame();
	}

  // TODO: see if we want to communicate back to producer from here, in that case create rtpStream. 
	// Otherwise, don't need to do anything in this function 
	void ShmConsumer::CreateRtpStream()
	{
		MS_TRACE();

		auto& encoding   = this->rtpParameters.encodings[0];
		auto* mediaCodec = this->rtpParameters.GetCodecForEncoding(encoding);

		MS_DEBUG_TAG(
		  rtp, "[ssrc:%" PRIu32 ", payloadType:%" PRIu8 "]", encoding.ssrc, mediaCodec->payloadType);

		// Set stream params.
		RTC::RtpStream::Params params;

		params.ssrc        = encoding.ssrc;
		params.payloadType = mediaCodec->payloadType;
		params.mimeType    = mediaCodec->mimeType;
		params.clockRate   = mediaCodec->clockRate;
		params.cname       = this->rtpParameters.rtcp.cname;

		// Create a RtpStreamSend for sending a single media stream.
		size_t bufferSize = 0;

		this->rtpStream = new RTC::RtpStreamSend(this, params, bufferSize);
		this->rtpStreams.push_back(this->rtpStream);

		// If the Consumer is paused, tell the RtpStreamSend.
		if (IsPaused() || IsProducerPaused())
			this->rtpStream->Pause();
	}

	void ShmConsumer::RequestKeyFrame()
	{
		MS_TRACE();

		if (this->kind != RTC::Media::Kind::VIDEO)
			return;

		auto mappedSsrc = this->consumableRtpEncodings[0].ssrc;

		this->listener->OnConsumerKeyFrameRequested(this, mappedSsrc);
	}

	inline void ShmConsumer::OnRtpStreamRetransmitRtpPacket(
	  RTC::RtpStreamSend* /*rtpStream*/, RTC::RtpPacket* packet, bool probation)
	{
		MS_TRACE();

		this->listener->OnConsumerRetransmitRtpPacket(this, packet, probation);
	}

	inline void ShmConsumer::EmitScore() const
	{
		MS_TRACE();
	}

	inline void ShmConsumer::OnRtpStreamScore(
	  RTC::RtpStream* /*rtpStream*/, uint8_t /*score*/, uint8_t /*previousScore*/)
	{
		MS_TRACE();
	}
} // namespace RTC
