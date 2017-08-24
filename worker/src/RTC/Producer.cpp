#define MS_CLASS "RTC::Producer"
// #define MS_LOG_DEV

#include "RTC/Producer.hpp"
#include "Logger.hpp"
#include "MediaSoupError.hpp"
#include "RTC/RTCP/FeedbackPsPli.hpp"
#include "RTC/RTCP/FeedbackRtp.hpp"
#include "RTC/RTCP/FeedbackRtpNack.hpp"
#include "RTC/Transport.hpp"

namespace RTC
{
	/* Instance methods. */

	Producer::Producer(
	    Listener* listener, Channel::Notifier* notifier, uint32_t producerId, RTC::Media::Kind kind)
	    : producerId(producerId), kind(kind), listener(listener), notifier(notifier)
	{
		MS_TRACE();

		if (this->kind == RTC::Media::Kind::AUDIO)
			this->maxRtcpInterval = RTC::RTCP::MaxAudioIntervalMs;
		else
			this->maxRtcpInterval = RTC::RTCP::MaxAudioIntervalMs;
	}

	Producer::~Producer()
	{
		MS_TRACE();

		delete this->rtpParameters;

		ClearRtpStreams();
	}

	void Producer::Destroy()
	{
		MS_TRACE();

		static const Json::StaticString JsonStringClass{ "class" };

		Json::Value eventData(Json::objectValue);

		// Notify.
		eventData[JsonStringClass] = "Producer";
		this->notifier->Emit(this->producerId, "close", eventData);

		// Notify the listener.
		this->listener->OnProducerClosed(this);

		delete this;
	}

	Json::Value Producer::ToJson() const
	{
		MS_TRACE();

		static const Json::StaticString JsonStringProducerId{ "producerId" };
		static const Json::StaticString JsonStringKind{ "kind" };
		static const Json::StaticString JsonStringRtpParameters{ "rtpParameters" };
		static const Json::StaticString JsonStringHasTransport{ "hasTransport" };
		static const Json::StaticString JsonStringRtpRawEventEnabled{ "rtpRawEventEnabled" };
		static const Json::StaticString JsonStringRtpObjectEventEnabled{ "rtpObjectEventEnabled" };
		static const Json::StaticString JsonStringRtpStreams{ "rtpStreams" };
		static const Json::StaticString JsonStringRtpStream{ "rtpStream" };

		Json::Value json(Json::objectValue);
		Json::Value jsonRtpStreams(Json::arrayValue);

		json[JsonStringProducerId] = Json::UInt{ this->producerId };

		json[JsonStringKind] = RTC::Media::GetJsonString(this->kind);

		if (this->rtpParameters != nullptr)
			json[JsonStringRtpParameters] = this->rtpParameters->ToJson();
		else
			json[JsonStringRtpParameters] = Json::nullValue;

		json[JsonStringHasTransport] = this->transport != nullptr;

		json[JsonStringRtpRawEventEnabled] = this->rtpRawEventEnabled;

		json[JsonStringRtpObjectEventEnabled] = this->rtpObjectEventEnabled;

		for (auto& kv : this->rtpStreams)
		{
			auto rtpStream = kv.second;

			jsonRtpStreams.append(rtpStream->ToJson());
		}
		json[JsonStringRtpStreams] = jsonRtpStreams;

		return json;
	}

	void Producer::HandleRequest(Channel::Request* request)
	{
		MS_TRACE();

		switch (request->methodId)
		{
			case Channel::Request::MethodId::PRODUCER_CLOSE:
			{
#ifdef MS_LOG_DEV
				uint32_t producerId = this->producerId;
#endif

				Destroy();

				MS_DEBUG_DEV("Producer closed [producerId:%" PRIu32 "]", producerId);

				request->Accept();

				break;
			}

			case Channel::Request::MethodId::PRODUCER_DUMP:
			{
				auto json = ToJson();

				request->Accept(json);

				break;
			}

			case Channel::Request::MethodId::PRODUCER_RECEIVE:
			{
				// Keep a reference to the previous rtpParameters.
				auto previousRtpParameters = this->rtpParameters;

				try
				{
					this->rtpParameters = new RTC::RtpParameters(request->data);
				}
				catch (const MediaSoupError& error)
				{
					request->Reject(error.what());

					return;
				}

				// NOTE: this may throw. If so keep the current parameters.
				try
				{
					this->listener->OnProducerParameters(this);
				}
				catch (const MediaSoupError& error)
				{
					// Rollback previous parameters.
					this->rtpParameters = previousRtpParameters;

					request->Reject(error.what());

					return;
				}

				// Free the previous parameters.
				delete previousRtpParameters;

				// Free previous RTP streams.
				ClearRtpStreams();

				auto data = this->rtpParameters->ToJson();

				request->Accept(data);

				// And notify again.
				this->listener->OnProducerParametersDone(this);

				// Create RtpStreamRecv instances.
				for (auto& encoding : this->rtpParameters->encodings)
				{
					CreateRtpStream(encoding);
				}

				break;
			}

			case Channel::Request::MethodId::PRODUCER_SET_RTP_RAW_EVENT:
			{
				static const Json::StaticString JsonStringEnabled{ "enabled" };

				if (!request->data[JsonStringEnabled].isBool())
				{
					request->Reject("Request has invalid data.enabled");

					return;
				}

				this->rtpRawEventEnabled = request->data[JsonStringEnabled].asBool();

				request->Accept();

				// If set, require a full frame.
				if (this->rtpRawEventEnabled)
					this->RequestFullFrame();

				break;
			}

			case Channel::Request::MethodId::PRODUCER_SET_RTP_OBJECT_EVENT:
			{
				static const Json::StaticString JsonStringEnabled{ "enabled" };

				if (!request->data[JsonStringEnabled].isBool())
				{
					request->Reject("Request has invalid data.enabled");

					return;
				}

				this->rtpObjectEventEnabled = request->data[JsonStringEnabled].asBool();

				request->Accept();

				// If set, require a full frame.
				if (this->rtpObjectEventEnabled)
					this->RequestFullFrame();

				break;
			}

			default:
			{
				MS_ERROR("unknown method");

				request->Reject("unknown method");
			}
		}
	}

	void Producer::ReceiveRtpPacket(RTC::RtpPacket* packet)
	{
		MS_TRACE();

		static const Json::StaticString JsonStringClass{ "class" };
		static const Json::StaticString JsonStringObject{ "object" };
		static const Json::StaticString JsonStringPayloadType{ "payloadType" };
		static const Json::StaticString JsonStringMarker{ "marker" };
		static const Json::StaticString JsonStringSequenceNumber{ "sequenceNumber" };
		static const Json::StaticString JsonStringTimestamp{ "timestamp" };
		static const Json::StaticString JsonStringSsrc{ "ssrc" };

		// TODO: Check if stopped, etc (not yet done).

		// Find the corresponding RtpStreamRecv.
		uint32_t ssrc = packet->GetSsrc();

		RTC::RtpStreamRecv* rtpStream{ nullptr };

		if (this->rtpStreams.find(ssrc) == this->rtpStreams.end())
		{
			if (this->rtxStreamMap.find(ssrc) == this->rtxStreamMap.end())
			{
				MS_WARN_TAG(rtp, "no RtpStream found for given RTP packet [ssrc:%" PRIu32 "]", ssrc);

				return;
			}

			else
			{
				rtpStream = this->rtxStreamMap[ssrc];

				// Process the packet.
				if (!rtpStream->ReceiveRtxPacket(packet))
					return;
			}
		}

		else
		{
			rtpStream = this->rtpStreams[ssrc];

			// Process the packet.
			if (!rtpStream->ReceivePacket(packet))
				return;
		}

		// Notify the listener.
		this->listener->OnRtpPacket(this, packet);

		// Emit "rtpraw" if enabled.
		if (this->rtpRawEventEnabled)
		{
			Json::Value eventData(Json::objectValue);

			eventData[JsonStringClass] = "Producer";

			this->notifier->EmitWithBinary(
			    this->producerId, "rtpraw", eventData, packet->GetData(), packet->GetSize());
		}

		// Emit "rtpobject" is enabled.
		if (this->rtpObjectEventEnabled)
		{
			Json::Value eventData(Json::objectValue);
			Json::Value jsonObject(Json::objectValue);

			eventData[JsonStringClass] = "Producer";

			jsonObject[JsonStringPayloadType]    = Json::UInt{ packet->GetPayloadType() };
			jsonObject[JsonStringMarker]         = packet->HasMarker();
			jsonObject[JsonStringSequenceNumber] = Json::UInt{ packet->GetSequenceNumber() };
			jsonObject[JsonStringTimestamp]      = Json::UInt{ packet->GetTimestamp() };
			jsonObject[JsonStringSsrc]           = Json::UInt{ packet->GetSsrc() };

			eventData[JsonStringObject] = jsonObject;

			this->notifier->EmitWithBinary(
			    this->producerId, "rtpobject", eventData, packet->GetPayload(), packet->GetPayloadLength());
		}
	}

	void Producer::GetRtcp(RTC::RTCP::CompoundPacket* packet, uint64_t now)
	{
		if (static_cast<float>((now - this->lastRtcpSentTime) * 1.15) < this->maxRtcpInterval)
			return;

		for (auto& kv : this->rtpStreams)
		{
			auto rtpStream = kv.second;
			auto* report   = rtpStream->GetRtcpReceiverReport();

			report->SetSsrc(rtpStream->GetSsrc());
			packet->AddReceiverReport(report);
		}

		this->lastRtcpSentTime = now;
	}

	void Producer::ReceiveRtcpFeedback(RTC::RTCP::FeedbackPsPacket* packet) const
	{
		MS_TRACE();

		if (this->transport == nullptr)
			return;

		// Ensure that the RTCP packet fits into the RTCP buffer.
		if (packet->GetSize() > RTC::RTCP::BufferSize)
		{
			MS_WARN_TAG(rtcp, "cannot send RTCP packet, size too big (%zu bytes)", packet->GetSize());

			return;
		}

		packet->Serialize(RTC::RTCP::Buffer);
		this->transport->SendRtcpPacket(packet);
	}

	void Producer::ReceiveRtcpFeedback(RTC::RTCP::FeedbackRtpPacket* packet) const
	{
		MS_TRACE();

		if (this->transport == nullptr)
			return;

		// Ensure that the RTCP packet fits into the RTCP buffer.
		if (packet->GetSize() > RTC::RTCP::BufferSize)
		{
			MS_WARN_TAG(rtcp, "cannot send RTCP packet, size too big (%zu bytes)", packet->GetSize());

			return;
		}

		packet->Serialize(RTC::RTCP::Buffer);
		this->transport->SendRtcpPacket(packet);
	}

	void Producer::RequestFullFrame() const
	{
		MS_TRACE();

		for (auto& kv : this->rtpStreams)
		{
			auto rtpStream = kv.second;

			rtpStream->RequestFullFrame();
		}
	}

	void Producer::CreateRtpStream(RTC::RtpEncodingParameters& encoding)
	{
		MS_TRACE();

		// Don't create an RtpStreamRecv if the encoding has no SSRC.
		// TODO: For simulcast or, if not announced, this would be done
		// dynamicall by the RtpListener when matching a RID with its SSRC.
		if (encoding.ssrc == 0u)
			return;

		uint32_t ssrc = encoding.ssrc;

		// Don't create a RtpStreamRecv if there is already one for the same SSRC.
		// TODO: This may not work for SVC codecs.
		if (this->rtpStreams.find(ssrc) != this->rtpStreams.end())
			return;

		// Get the codec of the stream/encoding.
		auto& codec = this->rtpParameters->GetCodecForEncoding(encoding);
		bool useNack{ false };
		bool usePli{ false };
		bool useRemb{ false };
		uint8_t ssrcAudioLevelId{ 0 };
		uint8_t absSendTimeId{ 0 };

		for (auto& fb : codec.rtcpFeedback)
		{
			if (!useNack && fb.type == "nack")
			{
				MS_DEBUG_TAG(rtcp, "enabling NACK generation");

				useNack = true;
			}
			if (!usePli && fb.type == "nack" && fb.parameter == "pli")
			{
				MS_DEBUG_TAG(rtcp, "enabling PLI generation");

				usePli = true;
			}
			else if (!useRemb && fb.type == "goog-remb")
			{
				MS_DEBUG_TAG(rbe, "enabling REMB");

				useRemb = true;
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

		// Create a RtpStreamRecv for receiving a media stream.
		this->rtpStreams[ssrc] = new RTC::RtpStreamRecv(this, params);

		// Enable REMB in the transport if requested.
		if (useRemb)
			this->transport->EnableRemb();

		// Check rtx capabilities.
		if (encoding.hasRtx && encoding.rtx.ssrc != 0u)
		{
			if (this->rtxStreamMap.find(encoding.rtx.ssrc) != this->rtxStreamMap.end())
				return;

			auto& codec    = this->rtpParameters->GetRtxCodecForEncoding(encoding);
			auto rtpStream = this->rtpStreams[ssrc];

			rtpStream->SetRtx(codec.payloadType, encoding.rtx.ssrc);
			this->rtxStreamMap[encoding.rtx.ssrc] = rtpStream;
		}
	}

	void Producer::ClearRtpStreams()
	{
		MS_TRACE();

		for (auto& kv : this->rtpStreams)
		{
			auto rtpStream = kv.second;

			delete rtpStream;
		}

		this->rtpStreams.clear();
	}

	void Producer::OnNackRequired(RTC::RtpStreamRecv* rtpStream, const std::vector<uint16_t>& seqNumbers)
	{
		if (this->transport == nullptr)
			return;

		RTC::RTCP::FeedbackRtpNackPacket packet(0, rtpStream->GetSsrc());
		auto it        = seqNumbers.begin();
		const auto end = seqNumbers.end();

		while (it != end)
		{
			uint16_t seq;
			uint16_t bitmask{ 0 };

			seq = *it;
			++it;

			while (it != end)
			{
				uint16_t shift = *it - seq - 1;

				if (shift <= 15)
				{
					bitmask |= (1 << shift);
					++it;
				}
				else
				{
					break;
				}
			}

			auto nackItem = new RTC::RTCP::FeedbackRtpNackItem(seq, bitmask);

			packet.AddItem(nackItem);
		}

		// Ensure that the RTCP packet fits into the RTCP buffer.
		if (packet.GetSize() > RTC::RTCP::BufferSize)
		{
			MS_WARN_TAG(rtx, "cannot send RTCP NACK packet, size too big (%zu bytes)", packet.GetSize());

			return;
		}

		packet.Serialize(RTC::RTCP::Buffer);
		this->transport->SendRtcpPacket(&packet);
	}

	void Producer::OnPliRequired(RTC::RtpStreamRecv* rtpStream)
	{
		if (this->transport == nullptr)
			return;

		RTC::RTCP::FeedbackPsPliPacket packet(0, rtpStream->GetSsrc());

		packet.Serialize(RTC::RTCP::Buffer);

		// Send two, because it's free.
		this->transport->SendRtcpPacket(&packet);
		this->transport->SendRtcpPacket(&packet);
	}
} // namespace RTC
