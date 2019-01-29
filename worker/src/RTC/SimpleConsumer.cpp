#define MS_CLASS "RTC::SimpleConsumer"
// #define MS_LOG_DEV

#include "RTC/SimpleConsumer.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
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

		// TODO
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
				// TODO: Add methods such as CONSUMER_REQUEST_KEY_FRAME.

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

		if (!IsActive())
			return;

		// TODO: Ask key frame if video and so on.
	}

	void SimpleConsumer::ProducerRtpStreamHealthy(RTC::RtpStream* rtpStream, uint32_t mappedSsrc)
	{
		MS_TRACE();

		// TODO

		if (!IsActive())
			return;
	}

	void SimpleConsumer::ProducerRtpStreamUnhealthy(RTC::RtpStream* rtpStream, uint32_t mappedSsrc)
	{
		MS_TRACE();

		// TODO

		if (!IsActive())
			return;
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

		// TODO
	}

	void SimpleConsumer::GetRtcp(RTC::RTCP::CompoundPacket* packet, uint64_t now)
	{
		MS_TRACE();

		if (static_cast<float>((now - this->lastRtcpSentTime) * 1.15) < this->maxRtcpInterval)
			return;

		auto* report = this->rtpStream->GetRtcpSenderReport(now);

		if (report == nullptr)
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
				this->rtpStream->packetsRepaired++;
			}
		}
	}

	void SimpleConsumer::ReceiveKeyFrameRequest(RTC::RTCP::FeedbackPs::MessageType messageType)
	{
		MS_TRACE();

		if (!IsActive())
			return;

		// 	switch (messageType)
		// 	{
		// 		case RTC::RTCP::FeedbackPs::MessageType::PLI:
		// 			this->rtpStream->pliCount++;
		// 			break;

		// 		case RTC::RTCP::FeedbackPs::MessageType::FIR:
		// 			this->rtpStream->firCount++;
		// 			break;

		// 		default:
		// 			MS_ASSERT(false, "invalid messageType");
		// 	}

		// 	RequestKeyFrame();
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
		if (!IsActive())
			return 0;

		float lossPercentage = 0;

		// TODO: Buff...

		// if (this->effectiveProfile != RTC::RtpEncodingParameters::Profile::NONE)
		// {
		// 	auto it = this->mapProfileRtpStream.find(this->effectiveProfile);

		// 	MS_ASSERT(it != this->mapProfileRtpStream.end(), "no RtpStream associated with current profile");

		// 	auto* rtpStream = it->second;

		// 	if (rtpStream->GetLossPercentage() >= this->rtpStream->GetLossPercentage())
		// 		lossPercentage = 0;
		// 	else
		// 		lossPercentage = (this->rtpStream->GetLossPercentage() - rtpStream->GetLossPercentage());
		// }

		return lossPercentage;
	}

	void SimpleConsumer::Started()
	{
		MS_TRACE();

		// TODO
	}

	void SimpleConsumer::Paused()
	{
		MS_TRACE();

		// TODO
	}

	void SimpleConsumer::Resumed()
	{
		MS_TRACE();

		// TODO
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
			this->rtpStream = new RTC::RtpStreamSend(params, 1500);
		else
			this->rtpStream = new RTC::RtpStreamSend(params, 0);

		auto* rtxCodec = this->rtpParameters.GetRtxCodecForEncoding(encoding);

		if (rtxCodec && encoding.hasRtx && encoding.rtx.ssrc != 0u)
			this->rtpStream->SetRtx(rtxCodec->payloadType, encoding.rtx.ssrc);

		this->encodingContext.reset(RTC::Codecs::GetEncodingContext(mediaCodec->mimeType));
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

		// Delete the RTX RtpPacket if it was created.
		if (rtxPacket != packet)
			delete rtxPacket;
	}
} // namespace RTC
