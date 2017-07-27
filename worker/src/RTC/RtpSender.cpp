#define MS_CLASS "RTC::RtpSender"
// #define MS_LOG_DEV

#include "RTC/RtpSender.hpp"
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

	RtpSender::RtpSender(
	    Listener* listener, Channel::Notifier* notifier, uint32_t rtpSenderId, RTC::Media::Kind kind)
	    : rtpSenderId(rtpSenderId), kind(kind), listener(listener), notifier(notifier)
	{
		MS_TRACE();

		if (this->kind == RTC::Media::Kind::AUDIO)
			this->maxRtcpInterval = RTC::RTCP::MaxAudioIntervalMs;
		else
			this->maxRtcpInterval = RTC::RTCP::MaxVideoIntervalMs;
	}

	RtpSender::~RtpSender()
	{
		MS_TRACE();

		delete this->rtpParameters;
		delete this->rtpStream;
	}

	void RtpSender::Destroy()
	{
		MS_TRACE();

		static const Json::StaticString JsonStringClass{ "class" };

		Json::Value eventData(Json::objectValue);

		eventData[JsonStringClass] = "RtpSender";
		this->notifier->Emit(this->rtpSenderId, "close", eventData);

		// Notify the listener.
		this->listener->OnRtpSenderClosed(this);

		delete this;
	}

	Json::Value RtpSender::ToJson() const
	{
		MS_TRACE();

		static const Json::StaticString JsonStringRtpSenderId{ "rtpSenderId" };
		static const Json::StaticString JsonStringKind{ "kind" };
		static const Json::StaticString JsonStringRtpParameters{ "rtpParameters" };
		static const Json::StaticString JsonStringHasTransport{ "hasTransport" };
		static const Json::StaticString JsonStringActive{ "active" };
		static const Json::StaticString JsonStringSupportedPayloadTypes{ "supportedPayloadTypes" };
		static const Json::StaticString JsonStringRtpStream{ "rtpStream" };

		Json::Value json(Json::objectValue);

		json[JsonStringRtpSenderId] = Json::UInt{ this->rtpSenderId };

		json[JsonStringKind] = RTC::Media::GetJsonString(this->kind);

		if (this->rtpParameters != nullptr)
			json[JsonStringRtpParameters] = this->rtpParameters->ToJson();
		else
			json[JsonStringRtpParameters] = Json::nullValue;

		json[JsonStringHasTransport] = this->transport != nullptr;

		json[JsonStringActive] = this->GetActive();

		json[JsonStringSupportedPayloadTypes] = Json::arrayValue;

		for (auto payloadType : this->supportedPayloadTypes)
		{
			json[JsonStringSupportedPayloadTypes].append(Json::UInt{ payloadType });
		}

		if (this->rtpStream != nullptr)
			json[JsonStringRtpStream] = this->rtpStream->ToJson();

		return json;
	}

	void RtpSender::HandleRequest(Channel::Request* request)
	{
		MS_TRACE();

		switch (request->methodId)
		{
			case Channel::Request::MethodId::RTP_SENDER_DUMP:
			{
				auto json = ToJson();

				request->Accept(json);

				break;
			}

			case Channel::Request::MethodId::RTP_SENDER_DISABLE:
			{
				static const Json::StaticString JsonStringDisabled{ "disabled" };
				static const Json::StaticString JsonStringEmit{ "emit" };

				if (!request->data[JsonStringDisabled].isBool())
				{
					request->Reject("Request has invalid data.disabled");

					return;
				}

				bool disabled = request->data[JsonStringDisabled].asBool();
				bool emit     = true;

				if (request->data[JsonStringEmit].isBool())
					emit = request->data[JsonStringEmit].asBool();

				// Nothing changed.
				if (this->disabled == disabled)
				{
					request->Accept();

					return;
				}

				bool wasActive = this->GetActive();

				this->disabled = disabled;

				if (wasActive != this->GetActive())
				{
					if (emit)
						EmitActiveChange();

					if (this->GetActive())
						this->listener->OnRtpSenderFullFrameRequired(this);
				}

				request->Accept();

				break;
			}

			default:
			{
				MS_ERROR("unknown method");

				request->Reject("unknown method");
			}
		}
	}

	void RtpSender::SetPeerCapabilities(RTC::RtpCapabilities* peerCapabilities)
	{
		MS_TRACE();

		MS_ASSERT(peerCapabilities, "given peerCapabilities are null");

		this->peerCapabilities = peerCapabilities;
	}

	void RtpSender::Send(RTC::RtpParameters* rtpParameters)
	{
		MS_TRACE();

		static const Json::StaticString JsonStringClass{ "class" };
		static const Json::StaticString JsonStringRtpParameters{ "rtpParameters" };
		static const Json::StaticString JsonStringActive{ "active" };

		MS_ASSERT(this->peerCapabilities, "peer capabilities unset");
		MS_ASSERT(rtpParameters, "no RTP parameters given");

		bool hadParameters = this->rtpParameters != nullptr;

		// Free the previous rtpParameters.
		if (hadParameters)
			delete this->rtpParameters;

		// Delete previous RtpStreamSend (if any).
		if (this->rtpStream != nullptr)
		{
			delete this->rtpStream;
			this->rtpStream = nullptr;
		}

		// Clone given RTP parameters so we manage our own sender parameters.
		this->rtpParameters = new RTC::RtpParameters(rtpParameters);

		// Remove RTP parameters not supported by this Peer.

		// Remove unsupported codecs.
		auto& codecs = this->rtpParameters->codecs;

		for (auto it = codecs.begin(); it != this->rtpParameters->codecs.end();)
		{
			auto& codec = *it;
			auto it2    = codecs.begin();

			for (; it2 != codecs.end(); ++it2)
			{
				auto& codecCapability = *it2;

				if (codecCapability.Matches(codec))
					break;
			}

			if (it2 != codecs.end())
			{
				this->supportedPayloadTypes.insert(codec.payloadType);
				++it;
			}
			else
			{
				it = codecs.erase(it);
			}
		}

		// Remove unsupported encodings.
		auto& encodings = this->rtpParameters->encodings;

		for (auto it = encodings.begin(); it != encodings.end();)
		{
			auto& encoding = *it;

			if (supportedPayloadTypes.find(encoding.codecPayloadType) != supportedPayloadTypes.end())
			{
				++it;
			}
			else
			{
				it = encodings.erase(it);
			}
		}

		// TODO: Temporal. To be refactored.
		// Remove all the encodings but the first one.
		if (encodings.size() > 1)
		{
			RTC::RtpEncodingParameters encoding = encodings[0];

			encodings.clear();
			encodings.push_back(encoding);
		}

		// Remove unsupported header extensions.
		this->rtpParameters->ReduceHeaderExtensions(this->peerCapabilities->headerExtensions);

		// Set a random muxId.
		this->rtpParameters->muxId = Utils::Crypto::GetRandomString(8);

		// If there are no encodings set not available.
		if (!encodings.empty())
		{
			this->available = true;

			// NOTE: We assume a single stream/encoding when sending to remote peers.
			CreateRtpStream(encodings[0]);
		}
		else
		{
			this->available = false;
		}

		// Emit "parameterschange" if these are updated parameters.
		if (hadParameters)
		{
			Json::Value eventData(Json::objectValue);

			eventData[JsonStringClass]         = "RtpSender";
			eventData[JsonStringRtpParameters] = this->rtpParameters->ToJson();
			eventData[JsonStringActive]        = this->GetActive();

			this->notifier->Emit(this->rtpSenderId, "parameterschange", eventData);
		}
	}

	void RtpSender::SendRtpPacket(RTC::RtpPacket* packet)
	{
		MS_TRACE();

		if (!this->GetActive())
			return;

		MS_ASSERT(this->rtpStream, "no RtpStream set");

		// TODO: Must refactor for simulcast.
		// Ignore the packet if the SSRC is not the single one in the sender
		// RTP parameters.
		if (packet->GetSsrc() != this->rtpParameters->encodings[0].ssrc)
		{
			MS_WARN_TAG(rtp, "ignoring packet with unknown SSRC [ssrc:%" PRIu32 "]", packet->GetSsrc());

			return;
		}

		// Map the payload type.
		uint8_t payloadType = packet->GetPayloadType();
		auto it             = this->supportedPayloadTypes.find(payloadType);

		// NOTE: This may happen if this peer supports just some codecs from the
		// given RtpParameters.
		// TODO: We should just ignore those packets as they have non been negotiated.
		if (it == this->supportedPayloadTypes.end())
		{
			MS_DEBUG_TAG(rtp, "payload type not supported [payloadType:%" PRIu8 "]", payloadType);

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

	void RtpSender::GetRtcp(RTC::RTCP::CompoundPacket* packet, uint64_t now)
	{
		if (this->rtpStream == nullptr)
			return;

		if (static_cast<float>((now - this->lastRtcpSentTime) * 1.15) < this->maxRtcpInterval)
			return;

		auto* report = this->rtpStream->GetRtcpSenderReport(now);
		if (report == nullptr)
			return;

		// NOTE: This assumes a single stream.
		uint32_t ssrc     = this->rtpParameters->encodings[0].ssrc;
		std::string cname = this->rtpParameters->rtcp.cname;

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

	void RtpSender::ReceiveNack(RTC::RTCP::FeedbackRtpNackPacket* nackPacket)
	{
		MS_TRACE();

		if (this->rtpStream == nullptr)
		{
			MS_WARN_TAG(rtp, "no RtpStreamSend");

			return;
		}

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

	void RtpSender::ReceiveRtcpReceiverReport(RTC::RTCP::ReceiverReport* report)
	{
		MS_TRACE();

		if (this->rtpStream == nullptr)
		{
			MS_WARN_TAG(rtp, "no RtpStreamSend");

			return;
		}

		this->rtpStream->ReceiveRtcpReceiverReport(report);
	}

	void RtpSender::CreateRtpStream(RTC::RtpEncodingParameters& encoding)
	{
		MS_TRACE();

		uint32_t ssrc = encoding.ssrc;
		// Get the codec of the stream/encoding.
		auto& codec = this->rtpParameters->GetCodecForEncoding(encoding);
		bool useNack{ false };
		bool usePli{ false };
		uint8_t ssrcAudioLevelId{ 0 };
		uint8_t absSendTimeId{ 0 };

		for (auto& fb : codec.rtcpFeedback)
		{
			if (!useNack && fb.type == "nack")
			{
				MS_DEBUG_TAG(rtcp, "enabling NACK reception");

				useNack = true;
			}
			if (!usePli && fb.type == "nack" && fb.parameter == "pli")
			{
				MS_DEBUG_TAG(rtcp, "enabling PLI reception");

				usePli = true;
			}
		}

		for (auto& exten : this->rtpParameters->headerExtensions)
		{
			if (this->kind == RTC::Media::Kind::AUDIO && (ssrcAudioLevelId == 0u) &&
			    exten.type == RTC::RtpHeaderExtensionUri::Type::SSRC_AUDIO_LEVEL)
			{
				ssrcAudioLevelId = exten.id;
			}

			if ((absSendTimeId == 0u) && exten.type == RTC::RtpHeaderExtensionUri::Type::ABS_SEND_TIME)
			{
				absSendTimeId = exten.id;
			}
		}

		// Create stream params.
		RTC::RtpStream::Params params;

		params.ssrc             = ssrc;
		params.payloadType      = codec.payloadType;
		params.mime             = codec.mime;
		params.clockRate        = codec.clockRate;
		params.useNack          = useNack;
		params.usePli           = usePli;
		params.ssrcAudioLevelId = ssrcAudioLevelId;
		params.absSendTimeId    = absSendTimeId;

		// Create a RtpStreamSend for sending a single media stream.
		if (useNack)
			this->rtpStream = new RTC::RtpStreamSend(params, 750);
		else
			this->rtpStream = new RTC::RtpStreamSend(params, 0);

		if (encoding.hasRtx && encoding.rtx.ssrc != 0u)
		{
			auto& codec = this->rtpParameters->GetRtxCodecForEncoding(encoding);

			if (codec.hasPayloadType)
			{
				this->rtpStream->SetRtx(codec.payloadType, encoding.rtx.ssrc);
			}
		}
	}

	void RtpSender::RetransmitRtpPacket(RTC::RtpPacket* packet)
	{
		MS_TRACE();

		if (!this->GetActive())
			return;

		MS_ASSERT(this->rtpStream, "no RtpStream set");

		RTC::RtpPacket* rtxPacket = { nullptr };

		if (this->rtpStream->HasRtx())
		{
			static uint8_t RtxBuffer[MtuSize];

			rtxPacket = packet->Clone(RtxBuffer);
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

	inline void RtpSender::EmitActiveChange() const
	{
		MS_TRACE();

		static const Json::StaticString JsonStringClass{ "class" };
		static const Json::StaticString JsonStringActive{ "active" };

		Json::Value eventData(Json::objectValue);

		eventData[JsonStringClass]  = "RtpSender";
		eventData[JsonStringActive] = this->GetActive();

		this->notifier->Emit(this->rtpSenderId, "activechange", eventData);
	}
} // namespace RTC
