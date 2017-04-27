#define MS_CLASS "RTC::RtpReceiver"
// #define MS_LOG_DEV

#include "RTC/RtpReceiver.hpp"
#include "Logger.hpp"
#include "MediaSoupError.hpp"
#include "RTC/RTCP/FeedbackPsPli.hpp"
#include "RTC/RTCP/FeedbackRtp.hpp"
#include "RTC/RTCP/FeedbackRtpNack.hpp"
#include "RTC/Transport.hpp"

namespace RTC
{
	/* Instance methods. */

	RtpReceiver::RtpReceiver(
	    Listener* listener, Channel::Notifier* notifier, uint32_t rtpReceiverId, RTC::Media::Kind kind)
	    : rtpReceiverId(rtpReceiverId), kind(kind), listener(listener), notifier(notifier)
	{
		MS_TRACE();

		if (this->kind == RTC::Media::Kind::AUDIO)
			this->maxRtcpInterval = RTC::RTCP::MaxAudioIntervalMs;
		else
			this->maxRtcpInterval = RTC::RTCP::MaxAudioIntervalMs;
	}

	RtpReceiver::~RtpReceiver()
	{
		MS_TRACE();

		delete this->rtpParameters;

		ClearRtpStreams();
	}

	void RtpReceiver::Destroy()
	{
		MS_TRACE();

		static const Json::StaticString JsonStringClass{ "class" };

		Json::Value eventData(Json::objectValue);

		// Notify.
		eventData[JsonStringClass] = "RtpReceiver";
		this->notifier->Emit(this->rtpReceiverId, "close", eventData);

		// Notify the listener.
		this->listener->OnRtpReceiverClosed(this);

		delete this;
	}

	Json::Value RtpReceiver::ToJson() const
	{
		MS_TRACE();

		static const Json::StaticString JsonStringRtpReceiverId{ "rtpReceiverId" };
		static const Json::StaticString JsonStringKind{ "kind" };
		static const Json::StaticString JsonStringRtpParameters{ "rtpParameters" };
		static const Json::StaticString JsonStringHasTransport{ "hasTransport" };
		static const Json::StaticString JsonStringRtpRawEventEnabled{ "rtpRawEventEnabled" };
		static const Json::StaticString JsonStringRtpObjectEventEnabled{ "rtpObjectEventEnabled" };
		static const Json::StaticString JsonStringRtpStreams{ "rtpStreams" };
		static const Json::StaticString JsonStringRtpStream{ "rtpStream" };

		Json::Value json(Json::objectValue);
		Json::Value jsonRtpStreams(Json::arrayValue);

		json[JsonStringRtpReceiverId] = Json::UInt{ this->rtpReceiverId };

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

	void RtpReceiver::HandleRequest(Channel::Request* request)
	{
		MS_TRACE();

		switch (request->methodId)
		{
			case Channel::Request::MethodId::RTP_RECEIVER_CLOSE:
			{
#ifdef MS_LOG_DEV
				uint32_t rtpReceiverId = this->rtpReceiverId;
#endif

				Destroy();

				MS_DEBUG_DEV("RtpReceiver closed [rtpReceiverId:%" PRIu32 "]", rtpReceiverId);

				request->Accept();

				break;
			}

			case Channel::Request::MethodId::RTP_RECEIVER_DUMP:
			{
				auto json = ToJson();

				request->Accept(json);

				break;
			}

			case Channel::Request::MethodId::RTP_RECEIVER_RECEIVE:
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
					this->listener->OnRtpReceiverParameters(this);
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
				this->listener->OnRtpReceiverParametersDone(this);

				// Create RtpStreamRecv instances.
				for (auto& encoding : this->rtpParameters->encodings)
				{
					CreateRtpStream(encoding);
				}

				break;
			}

			case Channel::Request::MethodId::RTP_RECEIVER_SET_RTP_RAW_EVENT:
			{
				static const Json::StaticString JsonStringEnabled{ "enabled" };

				if (!request->data[JsonStringEnabled].isBool())
				{
					request->Reject("Request has invalid data.enabled");

					return;
				}

				this->rtpRawEventEnabled = request->data[JsonStringEnabled].asBool();

				request->Accept();

				break;
			}

			case Channel::Request::MethodId::RTP_RECEIVER_SET_RTP_OBJECT_EVENT:
			{
				static const Json::StaticString JsonStringEnabled{ "enabled" };

				if (!request->data[JsonStringEnabled].isBool())
				{
					request->Reject("Request has invalid data.enabled");

					return;
				}

				this->rtpObjectEventEnabled = request->data[JsonStringEnabled].asBool();

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

	void RtpReceiver::ReceiveRtpPacket(RTC::RtpPacket* packet)
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

		if (this->rtpStreams.find(ssrc) == this->rtpStreams.end())
		{
			MS_WARN_TAG(rtp, "no RtpStream found for given RTP packet [ssrc:%" PRIu32 "]", ssrc);

			return;
		}

		auto rtpStream = this->rtpStreams[ssrc];

		// Process the packet.
		// TODO: Must check what kind of packet we are checking. For example, RTX
		// packets (once implemented) should have a different handling.
		if (!rtpStream->ReceivePacket(packet))
			return;

		// Notify the listener.
		this->listener->OnRtpPacket(this, packet);

		// Emit "rtpraw" if enabled.
		if (this->rtpRawEventEnabled)
		{
			Json::Value eventData(Json::objectValue);

			eventData[JsonStringClass] = "RtpReceiver";

			this->notifier->EmitWithBinary(
			    this->rtpReceiverId, "rtpraw", eventData, packet->GetData(), packet->GetSize());
		}

		// Emit "rtpobject" is enabled.
		if (this->rtpObjectEventEnabled)
		{
			Json::Value eventData(Json::objectValue);
			Json::Value jsonObject(Json::objectValue);

			eventData[JsonStringClass] = "RtpReceiver";

			jsonObject[JsonStringPayloadType]    = Json::UInt{ packet->GetPayloadType() };
			jsonObject[JsonStringMarker]         = packet->HasMarker();
			jsonObject[JsonStringSequenceNumber] = Json::UInt{ packet->GetSequenceNumber() };
			jsonObject[JsonStringTimestamp]      = Json::UInt{ packet->GetTimestamp() };
			jsonObject[JsonStringSsrc]           = Json::UInt{ packet->GetSsrc() };

			eventData[JsonStringObject] = jsonObject;

			this->notifier->EmitWithBinary(
			    this->rtpReceiverId, "rtpobject", eventData, packet->GetPayload(), packet->GetPayloadLength());
		}
	}

	void RtpReceiver::GetRtcp(RTC::RTCP::CompoundPacket* packet, uint64_t now)
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

	void RtpReceiver::ReceiveRtcpFeedback(RTC::RTCP::FeedbackPsPacket* packet) const
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

	void RtpReceiver::ReceiveRtcpFeedback(RTC::RTCP::FeedbackRtpPacket* packet) const
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

	void RtpReceiver::RequestFullFrame() const
	{
		MS_TRACE();

		for (auto& kv : this->rtpStreams)
		{
			auto rtpStream = kv.second;

			rtpStream->RequestFullFrame();
		}
	}

	void RtpReceiver::CreateRtpStream(RTC::RtpEncodingParameters& encoding)
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
	}

	void RtpReceiver::ClearRtpStreams()
	{
		MS_TRACE();

		for (auto& kv : this->rtpStreams)
		{
			auto rtpStream = kv.second;

			delete rtpStream;
		}

		this->rtpStreams.clear();
	}

	void RtpReceiver::OnNackRequired(RTC::RtpStreamRecv* rtpStream, const std::vector<uint16_t>& seqNumbers)
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

	void RtpReceiver::OnPliRequired(RTC::RtpStreamRecv* rtpStream)
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
