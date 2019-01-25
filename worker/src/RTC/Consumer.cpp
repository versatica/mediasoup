#define MS_CLASS "RTC::Consumer"
// #define MS_LOG_DEV

#include "RTC/Consumer.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "Utils.hpp"
#include "Channel/Notifier.hpp"
#include "RTC/Codecs/Codecs.hpp"
#include "RTC/RTCP/FeedbackRtpNack.hpp"
#include "RTC/RTCP/SenderReport.hpp"

namespace RTC
{
	/* Static. */

	static uint8_t RtxPacketBuffer[RtpBufferSize];
	static std::vector<RTC::RtpPacket*> RtpRetransmissionContainer(18);

	/* Instance methods. */

	Consumer::Consumer(const std::string& id, Listener* listener, json& data)
	  : id(id), listener(listener)
	{
		MS_TRACE();

		auto jsonKindIt = data.find("kind");

		if (jsonKindIt == data.end() || !jsonKindIt->is_string())
			MS_THROW_TYPE_ERROR("missing kind");

		// This may throw.
		this->kind = RTC::Media::GetKind(jsonKindIt->get<std::string>());

		if (this->kind == RTC::Media::Kind::ALL)
			MS_THROW_TYPE_ERROR("invalid empty kind");

		auto jsonRtpParametersIt = data.find("rtpParameters");

		if (jsonRtpParametersIt == data.end() || !jsonRtpParametersIt->is_object())
			MS_THROW_TYPE_ERROR("missing rtpParameters");

		// This may throw.
		this->rtpParameters = RTC::RtpParameters(*jsonRtpParametersIt);

		if (this->rtpParameters.encodings.empty())
			MS_THROW_TYPE_ERROR("invalid empty rtpParameters.encodings");
		else if (this->rtpParameters.encodings[0].ssrc == 0)
			MS_THROW_TYPE_ERROR("missing rtpParameters.encodings[0].ssrc");

		auto jsonConsumableRtpEncodingsIt = data.find("consumableRtpEncodings");

		if (jsonConsumableRtpEncodingsIt == data.end() || !jsonConsumableRtpEncodingsIt->is_array())
			MS_THROW_TYPE_ERROR("missing consumableRtpEncodings");

		if (jsonConsumableRtpEncodingsIt->size() == 0)
			MS_THROW_TYPE_ERROR("empty consumableRtpEncodings");

		this->consumableRtpEncodings.reserve(jsonConsumableRtpEncodingsIt->size());

		for (auto& entry : *jsonConsumableRtpEncodingsIt)
		{
			// This may throw due the constructor of RTC::RtpEncodingParameters.
			consumableRtpEncodings.emplace_back(entry);
		}

		// Fill supported codec payload types.
		for (auto& codec : this->rtpParameters.codecs)
		{
			this->supportedCodecPayloadTypes.insert(codec.payloadType);
		}

		// Set the RTCP report generation interval.
		if (this->kind == RTC::Media::Kind::AUDIO)
			this->maxRtcpInterval = RTC::RTCP::MaxAudioIntervalMs;
		else
			this->maxRtcpInterval = RTC::RTCP::MaxVideoIntervalMs;

		// Create RtpStreamSend instance for sending a single stream to the remote.
		CreateRtpStream();
	}

	Consumer::~Consumer()
	{
		MS_TRACE();

		delete this->rtpStream;

		delete this->rtpMonitor;
	}

	void Consumer::FillJson(json& jsonObject) const
	{
		MS_TRACE();

		// TODO
	}

	void Consumer::FillJsonStats(json& jsonArray) const
	{
		MS_TRACE();

		// TODO
	}

	void Consumer::HandleRequest(Channel::Request* request)
	{
		MS_TRACE();

		switch (request->methodId)
		{
			case Channel::Request::MethodId::CONSUMER_DUMP:
			{
				json data{ json::object() };

				FillJson(data);

				request->Accept(data);

				break;
			}

			case Channel::Request::MethodId::CONSUMER_GET_STATS:
			{
				json data{ json::array() };

				FillJsonStats(data);

				request->Accept(data);

				break;
			}

			case Channel::Request::MethodId::CONSUMER_PAUSE:
			{
				if (this->paused)
				{
					request->Accept();

					return;
				}

				this->paused = true;

				MS_DEBUG_DEV("Consumer paused [consumerId:%s]", this->id.c_str());

				if (IsStarted() && !this->producerPaused)
				{
					this->rtpMonitor->Reset();
					this->rtpStream->ClearRetransmissionBuffer();
					this->rtpPacketsBeforeProbation = RtpPacketsBeforeProbation;

					// TODO
					// if (IsProbing())
					// 	StopProbation();
				}

				request->Accept();

				break;
			}

			case Channel::Request::MethodId::CONSUMER_RESUME:
			{
				if (!this->paused)
				{
					request->Accept();

					return;
				}

				this->paused = false;

				MS_DEBUG_DEV("Consumer resumed [consumerId:%s]", this->id.c_str());

				if (IsStarted() && !this->producerPaused)
				{
					// We need to sync and wait for a key frame. Otherwise the receiver will
					// request lot of NACKs due to unknown RTP packets.
					this->syncRequired = true;

					RequestKeyFrame();
				}

				request->Accept();

				break;
			}

				// TODO: Much more.

			default:
			{
				MS_THROW_ERROR("unknown method '%s'", request->method.c_str());
			}
		}
	}

	void Consumer::TransportConnected()
	{
		MS_TRACE();

		// TODO: Ask key frames and so on.
	}

	void Consumer::ProducerPaused()
	{
		MS_TRACE();

		if (this->producerPaused)
			return;

		this->producerPaused = true;

		MS_DEBUG_DEV("Producer paused [consumerId:%s]", this->id.c_str());

		Channel::Notifier::Emit(this->id, "producerpause");

		if (IsStarted() && !this->paused)
		{
			this->rtpMonitor->Reset();
			this->rtpStream->ClearRetransmissionBuffer();
			this->rtpPacketsBeforeProbation = RtpPacketsBeforeProbation;

			// TODO
			// if (IsProbing())
			// StopProbation();
		}
	}

	void Consumer::ProducerResumed()
	{
		MS_TRACE();

		if (!this->producerPaused)
			return;

		this->producerPaused = false;

		MS_DEBUG_DEV("Producer resumed [consumerId:%s]", this->id.c_str());

		Channel::Notifier::Emit(this->id, "producerresume");

		if (IsStarted() && !this->paused)
		{
			// We need to sync. However we don't need to request a key frame since the
			// Producer already requested it.
			this->syncRequired = true;
		}
	}

	void Consumer::ProducerRtpStreamHealthy(RTC::RtpStream* rtpStream, uint32_t mappedSsrc)
	{
		MS_TRACE();

		// TODO
	}

	void Consumer::ProducerRtpStreamUnhealthy(RTC::RtpStream* rtpStream, uint32_t mappedSsrc)
	{
		MS_TRACE();

		// TODO
	}

	void Consumer::ProducerClosed()
	{
		MS_TRACE();

		// TODO
	}

	void Consumer::SendRtpPacket(RTC::RtpPacket* packet)
	{
		MS_TRACE();

		if (!IsStarted())
			return;

		// If paused don't forward RTP.
		if (IsPaused())
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

	void Consumer::GetRtcp(RTC::RTCP::CompoundPacket* packet, uint64_t now)
	{
		MS_TRACE();

		if (!IsStarted() || IsPaused())
			return;

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

	void Consumer::ReceiveNack(RTC::RTCP::FeedbackRtpNackPacket* nackPacket)
	{
		MS_TRACE();

		if (!IsStarted())
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

				this->rtpMonitor->RtpPacketRepaired(packet);

				// Packet repaired after applying RTX.
				this->rtpStream->packetsRepaired++;
			}
		}
	}

	void Consumer::ReceiveKeyFrameRequest(RTC::RTCP::FeedbackPs::MessageType messageType)
	{
		MS_TRACE();

		// 	if (!IsStarted())
		// 		return;

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

	void Consumer::ReceiveRtcpReceiverReport(RTC::RTCP::ReceiverReport* report)
	{
		MS_TRACE();

		if (!IsStarted())
			return;

		if (IsPaused())
			return;

		// Ignore reports that do not refer to the main RTP stream. Ie: RTX stream.
		if (report->GetSsrc() != this->rtpStream->GetSsrc())
			return;

		this->rtpStream->ReceiveRtcpReceiverReport(report);

		// TODO: Why just for video???
		if (this->kind == RTC::Media::Kind::VIDEO)
			this->rtpMonitor->ReceiveRtcpReceiverReport(report);
	}

	float Consumer::GetLossPercentage() const
	{
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

	void Consumer::RequestKeyFrame()
	{
		MS_TRACE();

		// if (!IsStarted())
		// 	return;

		// if (this->kind != RTC::Media::Kind::VIDEO || IsPaused())
		// 	return;

		// for (auto* listener : this->listeners)
		// {
		// 	listener->OnConsumerKeyFrameRequired(this);
		// }
	}

	void Consumer::CreateRtpStream()
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

		this->rtpMonitor = new RTC::RtpMonitor(this, this->rtpStream);
	}

	void Consumer::RetransmitRtpPacket(RTC::RtpPacket* packet)
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

	void Consumer::MayRunProbation()
	{
		// // No simulcast or SVC.
		// if (this->mapProfileRtpStream.empty())
		// 	return;

		// // There is an ongoing profile upgrade. Do not interfere.
		// if (this->effectiveProfile != this->targetProfile)
		// 	return;

		// // Ongoing probation.
		// if (IsProbing())
		// 	return;

		// // Current health status is not good.
		// if (!this->rtpMonitor->IsHealthy())
		// 	return;

		// RecalculateTargetProfile();
	}

	void Consumer::OnRtpMonitorScore(uint8_t /*score*/)
	{
		MS_TRACE();

		// RecalculateTargetProfile();
	}
} // namespace RTC
