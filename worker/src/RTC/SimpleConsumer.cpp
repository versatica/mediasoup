#define MS_CLASS "RTC::SimpleConsumer"
// #define MS_LOG_DEV

#include "RTC/SimpleConsumer.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "Channel/Notifier.hpp"
#include "RTC/Codecs/Codecs.hpp"

namespace RTC
{
	/* Static. */

	static uint8_t RtxPacketBuffer[RtpBufferSize];
	static std::vector<RTC::RtpPacket*> RtpRetransmissionContainer(18);

	/* Instance methods. */

	SimpleConsumer::SimpleConsumer(const std::string& id, Listener* listener, json& data)
	  : RTC::Consumer::Consumer(id, listener, data)
	{
		MS_TRACE();

		// TODO: Uncoment!
		//
		// Ensure there is a single encoding.
		// if (this->consumableRtpEncodings.size() != 1)
		// 	MS_THROW_TYPE_ERROR("invalid consumableRtpEncodings with size != 1");

		// Set the RTCP report generation interval.
		if (this->kind == RTC::Media::Kind::AUDIO)
			this->maxRtcpInterval = RTC::RTCP::MaxAudioIntervalMs;
		else
			this->maxRtcpInterval = RTC::RTCP::MaxVideoIntervalMs;

		// Create RtpStreamSend instance for sending a single stream to the remote.
		CreateRtpStream();
	}

	SimpleConsumer::~SimpleConsumer()
	{
		MS_TRACE();

		delete this->rtpStream;
	}

	void SimpleConsumer::FillJson(json& jsonObject) const
	{
		MS_TRACE();

		// Call the parent method.
		RTC::Consumer::FillJson(jsonObject);
	}

	void SimpleConsumer::FillJsonStats(json& jsonArray) const
	{
		MS_TRACE();

		// TODO
	}

	void SimpleConsumer::HandleRequest(Channel::Request* request)
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

			default:
			{
				// Pass it to the parent class.
				RTC::Consumer::HandleRequest(request);
			}
		}
	}

	void SimpleConsumer::TransportConnected()
	{
		MS_TRACE();

		RequestKeyFrame();
	}

	void SimpleConsumer::ProducerNewRtpStream(RTC::RtpStream* rtpStream, uint32_t mappedSsrc)
	{
		MS_TRACE();

		this->producerRtpStream = rtpStream;
	}

	void SimpleConsumer::ProducerRtpStreamScore(RTC::RtpStream* rtpStream, uint8_t score)
	{
		MS_TRACE();

		// Do nothing.
	}

	void SimpleConsumer::SendRtpPacket(RTC::RtpPacket* packet)
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

		bool isKeyFrame = false;

		// Just check if the packet contains a key frame when we need to sync.
		if (this->syncRequired && packet->IsKeyFrame())
			isKeyFrame = true;

		// If we are waiting for a key frame and this is not one, ignore the packet.
		if (this->syncRequired && this->keyFrameSupported && !isKeyFrame)
			return;

		// Whether this is the first packet after re-sync.
		bool isSyncPacket = this->syncRequired;

		// Sync sequence number and timestamp if required.
		if (isSyncPacket)
		{
			if (isKeyFrame)
			{
				MS_DEBUG_TAG(rtp, "awaited key frame received");

				// Clear RTP retransmission buffer to avoid congesting the receiver by
				// sending useless retransmissions (now that we are sending a newer key
				// frame).
				this->rtpStream->ClearRetransmissionBuffer();
			}

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

	void SimpleConsumer::GetRtcp(RTC::RTCP::CompoundPacket* packet, uint64_t now)
	{
		MS_TRACE();

		if (static_cast<float>((now - this->lastRtcpSentTime) * 1.15) < this->maxRtcpInterval)
			return;

		auto* report = this->rtpStream->GetRtcpSenderReport(now);

		if (!report)
			return;

		uint32_t ssrc     = this->rtpParameters.encodings[0].ssrc;
		std::string cname = this->rtpParameters.rtcp.cname;

		report->SetSsrc(ssrc);
		packet->AddSenderReport(report);

		// Build SDES chunk for this sender.
		auto* sdesChunk = new RTC::RTCP::SdesChunk(ssrc);
		auto* sdesItem =
		  new RTC::RTCP::SdesItem(RTC::RTCP::SdesItem::Type::CNAME, cname.size(), cname.c_str());

		sdesChunk->AddItem(sdesItem);
		packet->AddSdesChunk(sdesChunk);
		this->lastRtcpSentTime = now;
	}

	void SimpleConsumer::ReceiveNack(RTC::RTCP::FeedbackRtpNackPacket* nackPacket)
	{
		MS_TRACE();

		if (!IsActive())
			return;

		this->rtpStream->nackCount++;

		for (auto it = nackPacket->Begin(); it != nackPacket->End(); ++it)
		{
			RTC::RTCP::FeedbackRtpNackItem* item = *it;

			this->rtpStream->nackRtpPacketCount += item->CountRequestedPackets();

			this->rtpStream->RequestRtpRetransmission(
			  item->GetPacketId(), item->GetLostPacketBitmask(), RtpRetransmissionContainer);

			auto it2 = RtpRetransmissionContainer.begin();

			for (; it2 != RtpRetransmissionContainer.end(); ++it2)
			{
				RTC::RtpPacket* packet = *it2;

				if (packet == nullptr)
					break;

				RetransmitRtpPacket(packet);

				// Packet repaired after applying RTX.
				this->rtpStream->RtpPacketRepaired(packet);
			}
		}
	}

	void SimpleConsumer::ReceiveKeyFrameRequest(RTC::RTCP::FeedbackPs::MessageType messageType)
	{
		MS_TRACE();

		if (!IsActive())
			return;

		switch (messageType)
		{
			case RTC::RTCP::FeedbackPs::MessageType::PLI:
				this->rtpStream->pliCount++;
				break;

			case RTC::RTCP::FeedbackPs::MessageType::FIR:
				this->rtpStream->firCount++;
				break;

			default:
				MS_ASSERT(false, "invalid messageType");
		}

		RequestKeyFrame();
	}

	void SimpleConsumer::ReceiveRtcpReceiverReport(RTC::RTCP::ReceiverReport* report)
	{
		MS_TRACE();

		if (!IsActive())
			return;

		// Ignore reports that do not refer to the main RTP stream. Ie: RTX stream.
		if (report->GetSsrc() != this->rtpStream->GetSsrc())
			return;

		this->rtpStream->ReceiveRtcpReceiverReport(report);
	}

	uint32_t SimpleConsumer::GetTransmissionRate(uint64_t now)
	{
		MS_TRACE();

		if (!IsActive())
			return 0u;

		return this->rtpStream->GetRate(now) + this->retransmittedCounter.GetRate(now);
	}

	float SimpleConsumer::GetLossPercentage() const
	{
		if (!IsActive() || !this->producerRtpStream)
			return 0;

		if (this->producerRtpStream->GetLossPercentage() >= this->rtpStream->GetLossPercentage())
		{
			return 0;
		}
		else
		{
			return this->rtpStream->GetLossPercentage() - this->producerRtpStream->GetLossPercentage();
		}
	}

	void SimpleConsumer::Started()
	{
		MS_TRACE();

		RequestKeyFrame();
	}

	void SimpleConsumer::Paused(bool /*wasProducer*/)
	{
		MS_TRACE();

		this->rtpStream->ClearRetransmissionBuffer();
	}

	void SimpleConsumer::Resumed(bool wasProducer)
	{
		MS_TRACE();

		// We need to sync and wait for a key frame (if supported). Otherwise the
		// receiver will request lot of NACKs due to unknown RTP packets.
		this->syncRequired = true;

		// If we have been resumed due to the Producer becoming resumed, we don't
		// need to request a key frame since the Producer already requested it.
		if (!wasProducer)
			RequestKeyFrame();
	}

	void SimpleConsumer::CreateRtpStream()
	{
		MS_TRACE();

		auto& encoding   = this->rtpParameters.encodings[0];
		auto* mediaCodec = this->rtpParameters.GetCodecForEncoding(encoding);

		// Set stream params.
		RTC::RtpStream::Params params;

		params.ssrc        = encoding.ssrc;
		params.payloadType = mediaCodec->payloadType;
		params.mimeType    = mediaCodec->mimeType;
		params.clockRate   = mediaCodec->clockRate;

		for (auto& fb : mediaCodec->rtcpFeedback)
		{
			if (!params.useNack && fb.type == "nack")
			{
				MS_DEBUG_2TAGS(rtcp, rtx, "NACK supported");

				params.useNack = true;
			}
			if (!params.usePli && fb.type == "nack" && fb.parameter == "pli")
			{
				MS_DEBUG_TAG(rtcp, "PLI supported");

				params.usePli = true;
			}
			if (!params.useFir && fb.type == "ccm" && fb.parameter == "fir")
			{
				MS_DEBUG_TAG(rtcp, "FIR supported");

				params.useFir = true;
			}
		}

		// Create a RtpStreamSend for sending a single media stream.
		if (params.useNack)
			this->rtpStream = new RTC::RtpStreamSend(this, params, 1500);
		else
			this->rtpStream = new RTC::RtpStreamSend(this, params, 0);

		auto* rtxCodec = this->rtpParameters.GetRtxCodecForEncoding(encoding);

		if (rtxCodec && encoding.hasRtx)
			this->rtpStream->SetRtx(rtxCodec->payloadType, encoding.rtx.ssrc);

		this->keyFrameSupported = Codecs::CanBeKeyFrame(mediaCodec->mimeType);

		this->encodingContext.reset(RTC::Codecs::GetEncodingContext(mediaCodec->mimeType));
	}

	void SimpleConsumer::RequestKeyFrame()
	{
		MS_TRACE();

		if (!IsActive() || !this->producerRtpStream)
			return;

		if (this->kind != RTC::Media::Kind::VIDEO)
			return;

		auto mappedSsrc = this->consumableRtpEncodings[0].ssrc;

		this->listener->OnConsumerKeyFrameRequired(this, mappedSsrc);
	}

	void SimpleConsumer::RetransmitRtpPacket(RTC::RtpPacket* packet)
	{
		MS_TRACE();

		RTC::RtpPacket* rtxPacket{ nullptr };

		if (this->rtpStream->HasRtx())
		{
			rtxPacket = packet->Clone(RtxPacketBuffer);

			this->rtpStream->RtxEncode(rtxPacket);

			MS_DEBUG_TAG(
			  rtx,
			  "sending RTX packet [ssrc:%" PRIu32 ", seq:%" PRIu16 "] recovering original [ssrc:%" PRIu32
			  ", seq:%" PRIu16 "]",
			  rtxPacket->GetSsrc(),
			  rtxPacket->GetSequenceNumber(),
			  packet->GetSsrc(),
			  packet->GetSequenceNumber());
		}
		else
		{
			rtxPacket = packet;

			MS_DEBUG_TAG(
			  rtx,
			  "retransmitting packet [ssrc:%" PRIu32 ", seq:%" PRIu16 "]",
			  rtxPacket->GetSsrc(),
			  rtxPacket->GetSequenceNumber());
		}

		// Update retransmitted RTP data counter.
		this->retransmittedCounter.Update(rtxPacket);

		// Send the packet.
		this->listener->OnConsumerSendRtpPacket(this, rtxPacket);

		// Delete the RTX RtpPacket if it was cloned.
		if (rtxPacket != packet)
			delete rtxPacket;
	}
} // namespace RTC
