#define MS_CLASS "RTC::Consumer"
// #define MS_LOG_DEV

#include "RTC/Consumer.hpp"
#include "Logger.hpp"
#include "MediaSoupError.hpp"
#include "Utils.hpp"
#include "RTC/RTCP/FeedbackRtpNack.hpp"
#include "RTC/RTCP/SenderReport.hpp"
#include <vector>

namespace RTC
{
	/* Static. */

	static std::vector<RTC::RtpPacket*> RtpRetransmissionContainer(18);

	/* Instance methods. */

	Consumer::Consumer(
	  Channel::Notifier* notifier, uint32_t consumerId, RTC::Media::Kind kind, uint32_t sourceProducerId)
	  : consumerId(consumerId), kind(kind), sourceProducerId(sourceProducerId), notifier(notifier)
	{
		MS_TRACE();

		// Set the RTCP report generation interval.
		if (this->kind == RTC::Media::Kind::AUDIO)
			this->maxRtcpInterval = RTC::RTCP::MaxAudioIntervalMs;
		else
			this->maxRtcpInterval = RTC::RTCP::MaxVideoIntervalMs;
	}

	Consumer::~Consumer()
	{
		MS_TRACE();

		if (this->rtpStream != nullptr)
			delete this->rtpStream;
	}

	void Consumer::Destroy()
	{
		MS_TRACE();

		for (auto& listener : this->listeners)
		{
			listener->OnConsumerClosed(this);
		}

		this->notifier->Emit(this->consumerId, "close");

		delete this;
	}

	Json::Value Consumer::ToJson() const
	{
		MS_TRACE();

		static const Json::StaticString JsonStringConsumerId{ "consumerId" };
		static const Json::StaticString JsonStringKind{ "kind" };
		static const Json::StaticString JsonStringSourceProducerId{ "sourceProducerId" };
		static const Json::StaticString JsonStringRtpParameters{ "rtpParameters" };
		static const Json::StaticString JsonStringRtpStream{ "rtpStream" };
		static const Json::StaticString JsonStringEnabled{ "enabled" };
		static const Json::StaticString JsonStringPaused{ "paused" };
		static const Json::StaticString JsonStringSourcePaused{ "sourcePaused" };

		Json::Value json(Json::objectValue);

		json[JsonStringConsumerId] = Json::UInt{ this->consumerId };

		json[JsonStringKind] = RTC::Media::GetJsonString(this->kind);

		json[JsonStringSourceProducerId] = Json::UInt{ this->sourceProducerId };

		if (this->transport != nullptr)
			json[JsonStringRtpParameters] = this->rtpParameters.ToJson();

		if (this->rtpStream != nullptr)
			json[JsonStringRtpStream] = this->rtpStream->ToJson();

		json[JsonStringPaused] = this->paused;

		json[JsonStringSourcePaused] = this->sourcePaused;

		return json;
	}

	void Consumer::HandleRequest(Channel::Request* request)
	{
		MS_TRACE();

		switch (request->methodId)
		{
			case Channel::Request::MethodId::CONSUMER_DUMP:
			{
				auto json = ToJson();

				request->Accept(json);

				break;
			}

			default:
			{
				MS_ERROR("unknown method");

				request->Reject("unknown method");
			}
		}
	}

	/**
	 * A Transport has been assigned, and hence sending RTP parameters.
	 */
	void Consumer::Enable(RTC::Transport* transport, RTC::RtpParameters& rtpParameters)
	{
		MS_TRACE();

		if (IsEnabled())
			Disable();

		this->transport     = transport;
		this->rtpParameters = rtpParameters;

		FillSupportedCodecPayloadTypes();

		// Create RtpStreamSend instance.
		CreateRtpStream(this->rtpParameters.encodings[0]);

		MS_DEBUG_DEV("Consumer enabled [consumerId:%" PRIu32 "]", this->consumerId);
	}

	void Consumer::Pause()
	{
		MS_TRACE();

		if (this->paused)
			return;

		this->paused = true;

		MS_DEBUG_DEV("Consumer paused [consumerId:%" PRIu32 "]", this->consumerId);

		if (IsEnabled() && !this->sourcePaused)
		{
			this->rtpStream->Reset();
		}
	}

	void Consumer::Resume()
	{
		MS_TRACE();

		if (!this->paused)
			return;

		this->paused = false;

		MS_DEBUG_DEV("Consumer resumed [consumerId:%" PRIu32 "]", this->consumerId);

		if (IsEnabled() && !this->sourcePaused)
		{
			RequestFullFrame();
		}
	}

	void Consumer::SourcePause()
	{
		MS_TRACE();

		if (this->sourcePaused)
			return;

		this->sourcePaused = true;

		MS_DEBUG_DEV("Consumer source paused [consumerId:%" PRIu32 "]", this->consumerId);

		this->notifier->Emit(this->consumerId, "sourcepaused");

		if (IsEnabled() && !this->paused)
		{
			this->rtpStream->Reset();
		}
	}

	void Consumer::SourceResume()
	{
		MS_TRACE();

		if (!this->sourcePaused)
			return;

		this->sourcePaused = false;

		MS_DEBUG_DEV("Consumer source resumed [consumerId:%" PRIu32 "]", this->consumerId);

		this->notifier->Emit(this->consumerId, "sourceresumed");

		if (IsEnabled() && !this->paused)
		{
			RequestFullFrame();
		}
	}

	void Consumer::SourceRtpParametersUpdated()
	{
		if (!IsEnabled())
			return;

		// TODO: Set special flag to be ready for random seq numbers.
	}

	/**
	 * Called when the Transport assigned to this Consumer has been closed, so this
	 * Consumer becomes unhandled.
	 */
	void Consumer::Disable()
	{
		MS_TRACE();

		this->transport = nullptr;

		this->supportedCodecPayloadTypes.clear();

		if (this->rtpStream != nullptr)
		{
			delete this->rtpStream;
			this->rtpStream = nullptr;
		}

		// Reset RTCP and RTP counter stuff.
		this->lastRtcpSentTime = 0;
		this->transmittedCounter.Reset();
		this->retransmittedCounter.Reset();
	}

	void Consumer::SendRtpPacket(RTC::RtpPacket* packet)
	{
		MS_TRACE();

		if (!IsEnabled())
			return;

		// If paused don't forward RTP.
		if (IsPaused())
			return;

		// Ignore the packet if the SSRC is not the single one in the sender
		// RTP parameters.
		if (packet->GetSsrc() != this->rtpParameters.encodings[0].ssrc)
		{
			MS_WARN_TAG(rtp, "ignoring packet with unknown SSRC [ssrc:%" PRIu32 "]", packet->GetSsrc());

			return;
		}

		// Map the payload type.
		auto payloadType = packet->GetPayloadType();

		// NOTE: This may happen if this Consumer supports just some codecs of those
		// in the corresponding Producer.
		if (this->supportedCodecPayloadTypes.find(payloadType) == this->supportedCodecPayloadTypes.end())
		{
			MS_DEBUG_DEV("payload type not supported [payloadType:%" PRIu8 "]", payloadType);

			return;
		}

		// Process the packet.
		if (!this->rtpStream->ReceivePacket(packet))
			return;

		// Send the packet.
		this->transport->SendRtpPacket(packet);

		// Update transmitted RTP data counter.
		this->transmittedCounter.Update(packet);
	}

	void Consumer::GetRtcp(RTC::RTCP::CompoundPacket* packet, uint64_t now)
	{
		MS_TRACE();

		if (static_cast<float>((now - this->lastRtcpSentTime) * 1.15) < this->maxRtcpInterval)
			return;

		auto* report = this->rtpStream->GetRtcpSenderReport(now);

		if (report == nullptr)
			return;

		// NOTE: This assumes a single stream.
		uint32_t ssrc     = this->rtpParameters.encodings[0].ssrc;
		std::string cname = this->rtpParameters.rtcp.cname;

		report->SetSsrc(ssrc);
		packet->AddSenderReport(report);

		// Build SDES chunk for this sender.
		auto sdesChunk = new RTC::RTCP::SdesChunk(ssrc);
		auto sdesItem =
		  new RTC::RTCP::SdesItem(RTC::RTCP::SdesItem::Type::CNAME, cname.size(), cname.c_str());

		sdesChunk->AddItem(sdesItem);
		packet->AddSdesChunk(sdesChunk);
		this->lastRtcpSentTime = now;
	}

	void Consumer::ReceiveNack(RTC::RTCP::FeedbackRtpNackPacket* nackPacket)
	{
		MS_TRACE();

		if (!IsEnabled())
			return;

		for (auto it = nackPacket->Begin(); it != nackPacket->End(); ++it)
		{
			RTC::RTCP::FeedbackRtpNackItem* item = *it;

			this->rtpStream->RequestRtpRetransmission(
			  item->GetPacketId(), item->GetLostPacketBitmask(), RtpRetransmissionContainer);

			auto it2 = RtpRetransmissionContainer.begin();
			for (; it2 != RtpRetransmissionContainer.end(); ++it2)
			{
				RTC::RtpPacket* packet = *it2;

				if (packet == nullptr)
					break;

				RetransmitRtpPacket(packet);
			}
		}
	}

	void Consumer::ReceiveRtcpReceiverReport(RTC::RTCP::ReceiverReport* report)
	{
		MS_TRACE();

		if (!IsEnabled())
			return;

		this->rtpStream->ReceiveRtcpReceiverReport(report);
	}

	void Consumer::RequestFullFrame()
	{
		MS_TRACE();

		if (!IsEnabled())
			return;

		if (this->kind == RTC::Media::Kind::AUDIO || IsPaused())
			return;

		for (auto& listener : this->listeners)
		{
			listener->OnConsumerFullFrameRequired(this);
		}
	}

	void Consumer::FillSupportedCodecPayloadTypes()
	{
		MS_TRACE();

		for (auto& codec : this->rtpParameters.codecs)
		{
			this->supportedCodecPayloadTypes.insert(codec.payloadType);
		}
	}

	void Consumer::CreateRtpStream(RTC::RtpEncodingParameters& encoding)
	{
		MS_TRACE();

		uint32_t ssrc = encoding.ssrc;
		// Get the codec of the stream/encoding.
		auto& codec = this->rtpParameters.GetCodecForEncoding(encoding);
		bool useNack{ false };
		bool usePli{ false };

		for (auto& fb : codec.rtcpFeedback)
		{
			if (!useNack && fb.type == "nack")
			{
				MS_DEBUG_2TAGS(rtcp, rtx, "NACK supported");

				useNack = true;
			}
			if (!usePli && fb.type == "nack" && fb.parameter == "pli")
			{
				MS_DEBUG_TAG(rtcp, "PLI supported");

				usePli = true;
			}
		}

		// Create stream params.
		RTC::RtpStream::Params params;

		params.ssrc        = ssrc;
		params.payloadType = codec.payloadType;
		params.mime        = codec.mime;
		params.clockRate   = codec.clockRate;
		params.useNack     = useNack;
		params.usePli      = usePli;

		// Create a RtpStreamSend for sending a single media stream.
		if (useNack)
			this->rtpStream = new RTC::RtpStreamSend(params, 750);
		else
			this->rtpStream = new RTC::RtpStreamSend(params, 0);

		if (encoding.hasRtx && encoding.rtx.ssrc != 0u)
		{
			auto& codec = this->rtpParameters.GetRtxCodecForEncoding(encoding);

			this->rtpStream->SetRtx(codec.payloadType, encoding.rtx.ssrc);
		}
	}

	void Consumer::RetransmitRtpPacket(RTC::RtpPacket* packet)
	{
		MS_TRACE();

		RTC::RtpPacket* rtxPacket{ nullptr };

		if (this->rtpStream->HasRtx())
		{
			static uint8_t rtxBuffer[MtuSize];

			rtxPacket = packet->Clone(rtxBuffer);
			this->rtpStream->RtxEncode(rtxPacket);

			MS_DEBUG_TAG(
			  rtx,
			  "sending rtx packet [ssrc: %" PRIu32 " seqnr: %" PRIu16
			  "] recovering original [ssrc: %" PRIu32 " seqnr: %" PRIu16 "]",
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
			  "retransmitting packet [ssrc: %" PRIu32 " seqnr: %" PRIu16 "]",
			  rtxPacket->GetSsrc(),
			  rtxPacket->GetSequenceNumber());
		}

		// Update retransmitted RTP data counter.
		this->retransmittedCounter.Update(rtxPacket);

		// Send the packet.
		this->transport->SendRtpPacket(rtxPacket);

		// Delete the RTX RtpPacket if it was created.
		if (rtxPacket != packet)
			delete rtxPacket;
	}
} // namespace RTC
