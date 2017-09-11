#define MS_CLASS "RTC::Producer"
// #define MS_LOG_DEV

#include "RTC/Producer.hpp"
#include "Logger.hpp"
#include "MediaSoupError.hpp"
#include "RTC/RTCP/FeedbackPsPli.hpp"
#include "RTC/RTCP/FeedbackRtp.hpp"
#include "RTC/RTCP/FeedbackRtpNack.hpp"

namespace RTC
{
	/* Static. */

	// TODO: Set a proper value.
	static constexpr uint64_t FullFrameRequestBlockTimeout{ 1000 }; // In ms.

	/* Instance methods. */

	Producer::Producer(
	  Channel::Notifier* notifier,
	  uint32_t producerId,
	  RTC::Media::Kind kind,
	  RTC::Transport* transport,
	  RTC::RtpParameters& rtpParameters,
	  struct RtpMapping& rtpMapping,
	  bool paused)
	  : producerId(producerId), kind(kind), notifier(notifier), transport(transport),
	    rtpParameters(rtpParameters), paused(paused)
	{
		MS_TRACE();

		this->outputEncodings = this->rtpParameters.encodings;
		this->rtpMapping      = rtpMapping;

		// Fill ids of well known RTP header extensions with the mapped ids (if any).
		FillHeaderExtensionIds();

		// Set the RTCP report generation interval.
		if (this->kind == RTC::Media::Kind::AUDIO)
			this->maxRtcpInterval = RTC::RTCP::MaxAudioIntervalMs;
		else
			this->maxRtcpInterval = RTC::RTCP::MaxAudioIntervalMs;

		// Set the RTP full frame request block timer.
		this->fullFrameRequestBlockTimer = new Timer(this);
	}

	Producer::~Producer()
	{
		MS_TRACE();

		ClearRtpStreams();
	}

	void Producer::Destroy()
	{
		MS_TRACE();

		for (auto& listener : this->listeners)
		{
			listener->OnProducerClosed(this);
		}

		// Close the RTP full frame request block timer.
		this->fullFrameRequestBlockTimer->Destroy();

		delete this;
	}

	Json::Value Producer::ToJson() const
	{
		MS_TRACE();

		static const Json::StaticString JsonStringProducerId{ "producerId" };
		static const Json::StaticString JsonStringKind{ "kind" };
		static const Json::StaticString JsonStringRtpParameters{ "rtpParameters" };
		static const Json::StaticString JsonStringRtpStreams{ "rtpStreams" };
		static const Json::StaticString JsonStringHeaderExtensionIds{ "headerExtensionIds" };
		static const Json::StaticString JsonStringSsrcAudioLevel{ "ssrcAudioLevel" };
		static const Json::StaticString JsonStringAbsSendTime{ "absSendTime" };
		static const Json::StaticString JsonStringRid{ "rid" };
		static const Json::StaticString JsonStringPaused{ "paused" };
		static const Json::StaticString JsonStringRtpRawEventEnabled{ "rtpRawEventEnabled" };
		static const Json::StaticString JsonStringRtpObjectEventEnabled{ "rtpObjectEventEnabled" };

		Json::Value json(Json::objectValue);
		Json::Value jsonHeaderExtensionIds(Json::objectValue);
		Json::Value jsonRtpStreams(Json::arrayValue);

		json[JsonStringProducerId] = Json::UInt{ this->producerId };

		json[JsonStringKind] = RTC::Media::GetJsonString(this->kind);

		json[JsonStringRtpParameters] = this->rtpParameters.ToJson();

		for (auto& kv : this->rtpStreams)
		{
			auto rtpStream = kv.second;

			jsonRtpStreams.append(rtpStream->ToJson());
		}
		json[JsonStringRtpStreams] = jsonRtpStreams;

		if (this->headerExtensionIds.ssrcAudioLevel != 0u)
			jsonHeaderExtensionIds[JsonStringSsrcAudioLevel] = this->headerExtensionIds.ssrcAudioLevel;

		if (this->headerExtensionIds.absSendTime != 0u)
			jsonHeaderExtensionIds[JsonStringAbsSendTime] = this->headerExtensionIds.absSendTime;

		if (this->headerExtensionIds.rid != 0u)
			jsonHeaderExtensionIds[JsonStringRid] = this->headerExtensionIds.rid;

		json[JsonStringHeaderExtensionIds] = jsonHeaderExtensionIds;

		json[JsonStringPaused] = this->paused;

		json[JsonStringRtpRawEventEnabled] = this->rtpRawEventEnabled;

		json[JsonStringRtpObjectEventEnabled] = this->rtpObjectEventEnabled;

		return json;
	}

	void Producer::HandleRequest(Channel::Request* request)
	{
		MS_TRACE();

		switch (request->methodId)
		{
			case Channel::Request::MethodId::PRODUCER_DUMP:
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

	void Producer::UpdateRtpParameters(RTC::RtpParameters& rtpParameters)
	{
		MS_TRACE();

		this->rtpParameters = rtpParameters;

		// Clear previous RtpStreamRecv instances.
		ClearRtpStreams();

		for (auto& listener : this->listeners)
		{
			// NOTE: This may throw.
			listener->OnProducerRtpParametersUpdated(this);
		}

		MS_DEBUG_DEV("Producer RTP parameters updated [producerId:%" PRIu32 "]", this->producerId);
	}

	void Producer::Pause()
	{
		MS_TRACE();

		if (this->paused)
			return;

		this->paused = true;

		MS_DEBUG_DEV("Producer paused [producerId:%" PRIu32 "]", this->producerId);

		for (auto& listener : this->listeners)
		{
			listener->OnProducerPaused(this);
		}
	}

	void Producer::Resume()
	{
		MS_TRACE();

		if (!this->paused)
			return;

		this->paused = false;

		MS_DEBUG_DEV("Producer resumed [producerId:%" PRIu32 "]", this->producerId);

		for (auto& listener : this->listeners)
		{
			listener->OnProducerResumed(this);
		}

		RequestFullFrame(true);
	}

	void Producer::SetRtpRawEvent(bool enabled)
	{
		MS_TRACE();

		if (enabled == this->rtpRawEventEnabled)
			return;

		this->rtpRawEventEnabled = enabled;

		if (enabled)
		{
			MS_DEBUG_DEV("Producer rtpraw event enabled [producerId:%" PRIu32 "]", this->producerId);
		}
		else
		{
			MS_DEBUG_DEV("Producer rtpraw event disabled [producerId:%" PRIu32 "]", this->producerId);
		}

		// If set (and not paused), require a full frame.
		if (this->rtpRawEventEnabled && !this->paused)
			RequestFullFrame(true);
	}

	void Producer::SetRtpObjectEvent(bool enabled)
	{
		MS_TRACE();

		if (enabled == this->rtpObjectEventEnabled)
			return;

		this->rtpObjectEventEnabled = enabled;

		if (enabled)
		{
			MS_DEBUG_DEV("Producer rtpobject event enabled [producerId:%" PRIu32 "]", this->producerId);
		}
		else
		{
			MS_DEBUG_DEV("Producer rtpobject event disabled [producerId:%" PRIu32 "]", this->producerId);
		}

		// If set (and not paused), require a full frame.
		if (this->rtpObjectEventEnabled && !this->paused)
			RequestFullFrame(true);
	}

	void Producer::ReceiveRtpPacket(RTC::RtpPacket* packet)
	{
		MS_TRACE();

		static const Json::StaticString JsonStringObject{ "object" };
		static const Json::StaticString JsonStringPayloadType{ "payloadType" };
		static const Json::StaticString JsonStringMarker{ "marker" };
		static const Json::StaticString JsonStringSequenceNumber{ "sequenceNumber" };
		static const Json::StaticString JsonStringTimestamp{ "timestamp" };
		static const Json::StaticString JsonStringSsrc{ "ssrc" };

		// Find the corresponding RtpStreamRecv.
		uint32_t ssrc = packet->GetSsrc();

		if (
		  this->rtpStreams.find(ssrc) == this->rtpStreams.end() &&
		  this->mapRtxStreams.find(ssrc) == this->mapRtxStreams.end())
		{
			for (auto& encoding : this->rtpParameters.encodings)
			{
				// TODO: This is not ready for RID usage (without SSRC in encodings).
				if (
				  encoding.ssrc == ssrc || (encoding.hasRtx && encoding.rtx.ssrc == ssrc) ||
				  (encoding.hasFec && encoding.fec.ssrc == ssrc))
				{
					CreateRtpStream(encoding);

					// If this is the first media packet for this stream request a full frame.
					if (encoding.ssrc == ssrc)
						RequestFullFrame(true);

					break;
				}
			}
		}

		RTC::RtpStreamRecv* rtpStream{ nullptr };

		if (this->rtpStreams.find(ssrc) != this->rtpStreams.end())
		{
			rtpStream = this->rtpStreams[ssrc];

			// Process the packet.
			if (!rtpStream->ReceivePacket(packet))
				return;
		}
		else if (this->mapRtxStreams.find(ssrc) != this->mapRtxStreams.end())
		{
			rtpStream = this->mapRtxStreams[ssrc];

			// Process the packet.
			if (!rtpStream->ReceiveRtxPacket(packet))
				return;
		}
		else
		{
			MS_WARN_TAG(rtp, "no RtpStream found for given RTP packet [ssrc:%" PRIu32 "]", ssrc);

			return;
		}

		// If paused stop here.
		if (this->paused)
			return;

		RTC::RtpEncodingParameters::Profile profile;

		try
		{
			profile = GetProfile(rtpStream, packet);
		}
		catch (const MediaSoupError& error)
		{
			return;
		}

		// Apply the Producer codec payload type and extension header mapping before
		// dispatching the packet.
		ApplyRtpMapping(packet);

		for (auto& listener : this->listeners)
		{
			listener->OnProducerRtpPacket(this, packet, profile);
		}

		// Emit "rtpraw" if enabled.
		if (this->rtpRawEventEnabled)
		{
			this->notifier->EmitWithBinary(this->producerId, "rtpraw", packet->GetData(), packet->GetSize());
		}

		// Emit "rtpobject" is enabled.
		if (this->rtpObjectEventEnabled)
		{
			Json::Value eventData(Json::objectValue);
			Json::Value jsonObject(Json::objectValue);

			jsonObject[JsonStringPayloadType]    = Json::UInt{ packet->GetPayloadType() };
			jsonObject[JsonStringMarker]         = packet->HasMarker();
			jsonObject[JsonStringSequenceNumber] = Json::UInt{ packet->GetSequenceNumber() };
			jsonObject[JsonStringTimestamp]      = Json::UInt{ packet->GetTimestamp() };
			jsonObject[JsonStringSsrc]           = Json::UInt{ packet->GetSsrc() };

			eventData[JsonStringObject] = jsonObject;

			this->notifier->EmitWithBinary(
			  this->producerId, "rtpobject", packet->GetPayload(), packet->GetPayloadLength(), eventData);
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

		// Ensure that the RTCP packet fits into the RTCP buffer.
		if (packet->GetSize() > RTC::RTCP::BufferSize)
		{
			MS_WARN_TAG(rtcp, "cannot send RTCP packet, size too big (%zu bytes)", packet->GetSize());

			return;
		}

		packet->Serialize(RTC::RTCP::Buffer);
		this->transport->SendRtcpPacket(packet);
	}

	void Producer::RequestFullFrame(bool force)
	{
		MS_TRACE();

		if (this->kind == RTC::Media::Kind::AUDIO || this->paused)
			return;

		if (force)
		{
			// Stop the timer.
			this->fullFrameRequestBlockTimer->Stop();
		}
		else if (this->fullFrameRequestBlockTimer->IsActive())
		{
			MS_DEBUG_TAG(rtcp, "blocking full frame request until timer expires");

			// Set flag.
			this->isFullFrameRequested = true;

			return;
		}

		// Run the timer.
		this->fullFrameRequestBlockTimer->Start(FullFrameRequestBlockTimeout);

		for (auto& kv : this->rtpStreams)
		{
			auto rtpStream = kv.second;

			rtpStream->RequestFullFrame();
		}

		// Reset flag.
		this->isFullFrameRequested = false;
	}

	void Producer::FillHeaderExtensionIds()
	{
		MS_TRACE();

		auto& idMapping = this->rtpMapping.headerExtensionIds;
		uint8_t ssrcAudioLevelId{ 0 };
		uint8_t absSendTimeId{ 0 };
		uint8_t ridId{ 0 };

		for (auto& exten : this->rtpParameters.headerExtensions)
		{
			if (
			  this->kind == RTC::Media::Kind::AUDIO && (ssrcAudioLevelId == 0u) &&
			  exten.type == RTC::RtpHeaderExtensionUri::Type::SSRC_AUDIO_LEVEL)
			{
				if (idMapping.find(exten.id) != idMapping.end())
					ssrcAudioLevelId = idMapping[exten.id];
				else
					ssrcAudioLevelId = exten.id;

				this->headerExtensionIds.ssrcAudioLevel = ssrcAudioLevelId;
			}

			if ((absSendTimeId == 0u) && exten.type == RTC::RtpHeaderExtensionUri::Type::ABS_SEND_TIME)
			{
				if (idMapping.find(exten.id) != idMapping.end())
					absSendTimeId = idMapping[exten.id];
				else
					absSendTimeId = exten.id;

				this->headerExtensionIds.absSendTime = absSendTimeId;
			}

			if ((ridId == 0u) && exten.type == RTC::RtpHeaderExtensionUri::Type::RTP_STREAM_ID)
			{
				if (idMapping.find(exten.id) != idMapping.end())
					ridId = idMapping[exten.id];
				else
					ridId = exten.id;

				this->headerExtensionIds.rid = ridId;
			}
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
		auto& codec = this->rtpParameters.GetCodecForEncoding(encoding);
		bool useNack{ false };
		bool usePli{ false };
		bool useRemb{ false };

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
			else if (!useRemb && fb.type == "goog-remb")
			{
				MS_DEBUG_TAG(rbe, "REMB supported");

				useRemb = true;
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

		// Create a RtpStreamRecv for receiving a media stream.
		this->rtpStreams[ssrc] = new RTC::RtpStreamRecv(this, params);

		// Enable REMB in the transport if requested.
		if (useRemb)
			this->transport->EnableRemb();

		// Check rtx capabilities.
		if (encoding.hasRtx && encoding.rtx.ssrc != 0u)
		{
			if (this->mapRtxStreams.find(encoding.rtx.ssrc) != this->mapRtxStreams.end())
				return;

			auto& codec    = this->rtpParameters.GetRtxCodecForEncoding(encoding);
			auto rtpStream = this->rtpStreams[ssrc];

			rtpStream->SetRtx(codec.payloadType, encoding.rtx.ssrc);
			this->mapRtxStreams[encoding.rtx.ssrc] = rtpStream;
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
		this->mapRtxStreams.clear();
		this->profiles.clear();
	}

	void Producer::ApplyRtpMapping(RTC::RtpPacket* packet) const
	{
		MS_TRACE();

		auto& codecPayloadTypeMap = this->rtpMapping.codecPayloadTypes;
		auto payloadType          = packet->GetPayloadType();

		if (codecPayloadTypeMap.find(payloadType) != codecPayloadTypeMap.end())
		{
			packet->SetPayloadType(codecPayloadTypeMap.at(payloadType));
		}

		auto& headerExtensionIdMap = this->rtpMapping.headerExtensionIds;

		packet->MangleExtensionHeaderIds(headerExtensionIdMap);

		if (this->headerExtensionIds.ssrcAudioLevel != 0u)
		{
			packet->AddExtensionMapping(
			  RtpHeaderExtensionUri::Type::SSRC_AUDIO_LEVEL, this->headerExtensionIds.ssrcAudioLevel);
		}

		if (this->headerExtensionIds.absSendTime != 0u)
		{
			packet->AddExtensionMapping(
			  RtpHeaderExtensionUri::Type::ABS_SEND_TIME, this->headerExtensionIds.absSendTime);
		}

		if (this->headerExtensionIds.rid != 0u)
		{
			packet->AddExtensionMapping(
			  RtpHeaderExtensionUri::Type::RTP_STREAM_ID, this->headerExtensionIds.rid);
		}
	}

	RTC::RtpEncodingParameters::Profile Producer::GetProfile(
	  RTC::RtpStreamRecv* rtpStream, RTC::RtpPacket* packet)
	{
		// The stream is already mapped to a profile.
		if (this->profiles.find(rtpStream) != this->profiles.end())
		{
			auto it      = this->profiles[rtpStream].begin();
			auto profile = *it;

			return profile;
		}

		auto ssrc = packet->GetSsrc();

		// Look for the encoding associated with the SSRC.
		// NOTE: This is specific to simulcast with no temporal layers.
		for (auto& encoding : this->rtpParameters.encodings)
		{
			if (encoding.ssrc == ssrc)
			{
				auto profile = encoding.profile;

				this->profiles[rtpStream].insert(profile);

				// Don't announce default profile, but just those for simulcast/SVC.
				if (profile != RTC::RtpEncodingParameters::Profile::DEFAULT)
				{
					for (auto& listener : this->listeners)
					{
						listener->OnProducerProfileEnabled(this, profile);
					}
				}

				return profile;
			}
		}

		MS_THROW_ERROR("unknown RTP packet received [ssrc:%" PRIu32 "]", ssrc);
	}

	void Producer::OnRtpStreamRecvNackRequired(
	  RTC::RtpStreamRecv* rtpStream, const std::vector<uint16_t>& seqNumbers)
	{
		MS_TRACE();

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

	void Producer::OnRtpStreamRecvPliRequired(RTC::RtpStreamRecv* rtpStream)
	{
		MS_TRACE();

		MS_DEBUG_TAG(rtcp, "sending PLI");

		RTC::RTCP::FeedbackPsPliPacket packet(0, rtpStream->GetSsrc());

		packet.Serialize(RTC::RTCP::Buffer);

		// Send two, because it's free.
		this->transport->SendRtcpPacket(&packet);
		this->transport->SendRtcpPacket(&packet);
	}

	void Producer::OnRtpStreamHealthReport(RTC::RtpStream* rtpStream, bool healthy)
	{
		MS_TRACE();

		auto rtpStreamRecv = static_cast<RtpStreamRecv*>(rtpStream);
		auto& profiles     = this->profiles[rtpStreamRecv];

		// The stream has transitioned to non healthy.
		if (rtpStream->IsHealthy() && !healthy)
		{
			MS_DEBUG_TAG(rtp, "stream is now unhealthy [ssrc:%" PRIu32 "]", rtpStream->GetSsrc());

			// Notify about the profiles being disabled.
			for (auto& profile : profiles)
			{
				// Don't announce default profile, but just those for simulcast/SVC.
				if (profile == RTC::RtpEncodingParameters::Profile::DEFAULT)
					break;

				for (auto& listener : this->listeners)
				{
					listener->OnProducerProfileDisabled(this, profile);
				}
			}
		}

		// The stream has transitioned to healthy.
		if (!rtpStream->IsHealthy() && healthy)
		{
			MS_DEBUG_TAG(rtp, "stream is now healthy [ssrc:%" PRIu32 "]", rtpStream->GetSsrc());

			// Notify about the profiles being disabled.
			for (auto& profile : profiles)
			{
				// Don't announce default profile, but just those for simulcast/SVC.
				if (profile == RTC::RtpEncodingParameters::Profile::DEFAULT)
					break;

				for (auto& listener : this->listeners)
				{
					listener->OnProducerProfileEnabled(this, profile);
				}
			}
		}
	}

	inline void Producer::OnTimer(Timer* timer)
	{
		MS_TRACE();

		if (timer == this->fullFrameRequestBlockTimer)
		{
			// Nobody asked for a full frame since the timer was started.
			if (!this->isFullFrameRequested)
				return;

			MS_DEBUG_TAG(rtcp, "requesting full frame after timer expires");

			RequestFullFrame();
		}
	}
} // namespace RTC
