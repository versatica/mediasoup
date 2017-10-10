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

	static uint8_t ClonedPacketBuffer[RTC::RtpBufferSize];
	static constexpr uint64_t KeyFrameRequestBlockTimeout{ 1000 }; // In ms.

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
	    rtpParameters(rtpParameters), rtpMapping(rtpMapping), paused(paused)
	{
		MS_TRACE();

		this->outputEncodings = this->rtpParameters.encodings;

		// Fill ids of well known RTP header extensions with the mapped ids (if any).
		FillHeaderExtensionIds();

		// Set the RTCP report generation interval.
		if (this->kind == RTC::Media::Kind::AUDIO)
			this->maxRtcpInterval = RTC::RTCP::MaxAudioIntervalMs;
		else
			this->maxRtcpInterval = RTC::RTCP::MaxVideoIntervalMs;

		// Set the RTP key frame request block timer.
		this->keyFrameRequestBlockTimer = new Timer(this);
	}

	Producer::~Producer()
	{
		MS_TRACE();

		this->mapRtpStreamProfiles.clear();

		ClearRtpStreams();
	}

	void Producer::Destroy()
	{
		MS_TRACE();

		for (auto& listener : this->listeners)
		{
			listener->OnProducerClosed(this);
		}

		// Close the RTP key frame request block timer.
		this->keyFrameRequestBlockTimer->Destroy();

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
		static const Json::StaticString JsonStringLossPercentage{ "lossPercentage" };

		Json::Value json(Json::objectValue);
		Json::Value jsonHeaderExtensionIds(Json::objectValue);
		Json::Value jsonRtpStreams(Json::arrayValue);

		json[JsonStringProducerId] = Json::UInt{ this->producerId };

		json[JsonStringKind] = RTC::Media::GetJsonString(this->kind);

		json[JsonStringRtpParameters] = this->rtpParameters.ToJson();

		float lossPercentage = 0;

		for (auto& kv : this->rtpStreams)
		{
			auto rtpStream = kv.second;

			jsonRtpStreams.append(rtpStream->ToJson());
			lossPercentage += rtpStream->GetLossPercentage();
		}

		json[JsonStringRtpStreams] = jsonRtpStreams;

		if (!this->rtpStreams.empty())
			lossPercentage = lossPercentage / this->rtpStreams.size();

		json[JsonStringLossPercentage] = lossPercentage;

		if (this->headerExtensionIds.ssrcAudioLevel != 0u)
			jsonHeaderExtensionIds[JsonStringSsrcAudioLevel] = this->headerExtensionIds.ssrcAudioLevel;

		if (this->headerExtensionIds.absSendTime != 0u)
			jsonHeaderExtensionIds[JsonStringAbsSendTime] = this->headerExtensionIds.absSendTime;

		if (this->headerExtensionIds.rid != 0u)
			jsonHeaderExtensionIds[JsonStringRid] = this->headerExtensionIds.rid;

		json[JsonStringHeaderExtensionIds] = jsonHeaderExtensionIds;

		json[JsonStringPaused] = this->paused;

		return json;
	}

	Json::Value Producer::GetStats() const
	{
		MS_TRACE();

		static const Json::StaticString JsonStringTransportId{ "transportId" };

		Json::Value json(Json::arrayValue);

		for (auto it : this->rtpStreams)
		{
			auto rtpStream     = it.second;
			auto jsonRtpStream = rtpStream->GetStats();

			if (this->transport != nullptr)
			{
				jsonRtpStream[JsonStringTransportId] = this->transport->transportId;
			}

			json.append(jsonRtpStream);
		}

		return json;
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

		MS_DEBUG_2TAGS(rtcp, rtx, "requesting full frame after resumed");

		RequestKeyFrame(true);
	}

	void Producer::ReceiveRtpPacket(RTC::RtpPacket* packet)
	{
		MS_TRACE();

		// May need to create a new RtpStreamRecv.
		MayNeedNewStream(packet);

		// Find the corresponding RtpStreamRecv.
		uint32_t ssrc = packet->GetSsrc();
		RTC::RtpStreamRecv* rtpStream{ nullptr };
		std::unique_ptr<RTC::RtpPacket> clonedPacket;

		if (this->rtpStreams.find(ssrc) != this->rtpStreams.end())
		{
			rtpStream = this->rtpStreams[ssrc];

			// Let's clone the RTP packet so we can mangle the payload (if needed) and other
			// stuff that would change its size.
			clonedPacket.reset(packet->Clone(ClonedPacketBuffer));
			packet = clonedPacket.get();

			// Process the packet.
			if (!rtpStream->ReceivePacket(packet))
				return;
		}
		else if (this->mapRtxStreams.find(ssrc) != this->mapRtxStreams.end())
		{
			rtpStream = this->mapRtxStreams[ssrc];

			// Let's clone the RTP packet so we can mangle the payload (if needed) and other
			// stuff that would change its size.
			clonedPacket.reset(packet->Clone(ClonedPacketBuffer));
			packet = clonedPacket.get();

			// Process the packet.
			if (!rtpStream->ReceiveRtxPacket(packet))
				return;

			// Packet repaired after applying RTX.
			rtpStream->packetsRepaired++;
		}
		else
		{
			MS_WARN_TAG(rtp, "no RtpStream found for given RTP packet [ssrc:%" PRIu32 "]", ssrc);

			return;
		}

		RTC::RtpEncodingParameters::Profile profile;

		try
		{
			profile = GetProfile(rtpStream, packet);
		}
		catch (const MediaSoupError& error)
		{
			return;
		}

		if (packet->IsKeyFrame())
		{
			MS_DEBUG_TAG(
			  rtp,
			  "key frame received [ssrc:%" PRIu32 ", seq:%" PRIu16 ", profile:%s]",
			  packet->GetSsrc(),
			  packet->GetSequenceNumber(),
			  RTC::RtpEncodingParameters::profile2String[profile].c_str());
		}

		// If paused stop here.
		if (this->paused)
			return;

		// Apply the Producer codec payload type and extension header mapping before
		// dispatching the packet.
		ApplyRtpMapping(packet);

		for (auto& listener : this->listeners)
		{
			listener->OnProducerRtpPacket(this, packet, profile);
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

	void Producer::RequestKeyFrame(bool force)
	{
		MS_TRACE();

		if (this->kind == RTC::Media::Kind::AUDIO || this->paused)
			return;

		if (force)
		{
			// Stop the timer.
			this->keyFrameRequestBlockTimer->Stop();
		}
		else if (this->keyFrameRequestBlockTimer->IsActive())
		{
			MS_DEBUG_2TAGS(rtcp, rtx, "blocking key frame request due to flood protection");

			// Set flag.
			this->isKeyFrameRequested = true;

			return;
		}

		// Run the timer.
		this->keyFrameRequestBlockTimer->Start(KeyFrameRequestBlockTimeout);

		for (auto& kv : this->rtpStreams)
		{
			auto rtpStream = kv.second;

			rtpStream->RequestKeyFrame();
		}

		// Reset flag.
		this->isKeyFrameRequested = false;
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

				this->headerExtensionIds.absSendTime          = absSendTimeId;
				this->transportHeaderExtensionIds.absSendTime = exten.id;
			}

			if ((ridId == 0u) && exten.type == RTC::RtpHeaderExtensionUri::Type::RTP_STREAM_ID)
			{
				if (idMapping.find(exten.id) != idMapping.end())
					ridId = idMapping[exten.id];
				else
					ridId = exten.id;

				this->headerExtensionIds.rid          = ridId;
				this->transportHeaderExtensionIds.rid = exten.id;
			}
		}
	}

	void Producer::MayNeedNewStream(RTC::RtpPacket* packet)
	{
		MS_TRACE();

		uint32_t ssrc = packet->GetSsrc();

		// If already exists, do nothing.
		if (
		  this->rtpStreams.find(ssrc) != this->rtpStreams.end() ||
		  this->mapRtxStreams.find(ssrc) != this->mapRtxStreams.end())
		{
			return;
		}

		// First look for encodings with ssrc field.
		{
			for (auto& encoding : this->rtpParameters.encodings)
			{
				if (encoding.ssrc == ssrc)
				{
					CreateRtpStream(encoding, ssrc);

					return;
				}
			}
		}

		// TODO: Look for muxId.

		// If not found, look for encodings with encodingId (RID) field.
		{
			const uint8_t* ridPtr;
			size_t ridLen;

			if (packet->ReadRid(&ridPtr, &ridLen))
			{
				auto* charRidPtr = const_cast<char*>(reinterpret_cast<const char*>(ridPtr));
				std::string rid(charRidPtr, ridLen);

				for (auto& encoding : this->rtpParameters.encodings)
				{
					if (encoding.encodingId == rid)
					{
						CreateRtpStream(encoding, ssrc);

						return;
					}
				}
			}
		}
	}

	void Producer::CreateRtpStream(RTC::RtpEncodingParameters& encoding, uint32_t ssrc)
	{
		MS_TRACE();

		MS_ASSERT(this->rtpStreams.find(ssrc) == this->rtpStreams.end(), "stream already exists");

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
		params.mimeType    = codec.mimeType;
		params.clockRate   = codec.clockRate;
		params.useNack     = useNack;
		params.usePli      = usePli;

		// Create a RtpStreamRecv for receiving a media stream.
		auto* rtpStream = new RTC::RtpStreamRecv(this, params);

		this->rtpStreams[ssrc] = rtpStream;

		// Check RTX capabilities.
		if (encoding.hasRtx && encoding.rtx.ssrc != 0u)
		{
			if (this->mapRtxStreams.find(encoding.rtx.ssrc) != this->mapRtxStreams.end())
				return;

			auto& codec    = this->rtpParameters.GetRtxCodecForEncoding(encoding);
			auto rtpStream = this->rtpStreams[ssrc];

			rtpStream->SetRtx(codec.payloadType, encoding.rtx.ssrc);
			this->mapRtxStreams[encoding.rtx.ssrc] = rtpStream;
		}

		// TODO: For SVC, the dependencyEncodingIds must be checked and their respective profiles must
		// be also added into mapRtpStreamProfiles.
		auto profile = encoding.profile;

		// Add the stream to the profiles map.
		this->mapRtpStreamProfiles[rtpStream].insert(profile);

		// Add new profile/s into healthyProfiles and notify the listener.
		AddHealthyProfiles(rtpStream);

		// Request a key frame since we may have lost the first packets of this stream.
		RequestKeyFrame(true);
	}

	void Producer::ClearRtpStream(RTC::RtpStreamRecv* rtpStream)
	{
		MS_TRACE();

		// Remove the profiles related to this stream from healthyProfiles and notify the listener.
		RemoveHealthyProfiles(rtpStream);

		this->rtpStreams.erase(rtpStream->GetSsrc());

		for (auto& kv : this->mapRtxStreams)
		{
			auto rtxSsrc      = kv.first;
			auto* mediaStream = kv.second;

			if (mediaStream == rtpStream)
			{
				this->mapRtxStreams.erase(rtxSsrc);

				break;
			}
		}

		this->mapRtpStreamProfiles.erase(rtpStream);

		delete rtpStream;
	}

	void Producer::ClearRtpStreams()
	{
		MS_TRACE();

		for (auto& kv : this->rtpStreams)
		{
			auto rtpStream = kv.second;

			delete rtpStream;
		}

		// Notify about all profiles being disabled.
		for (auto& kv : this->mapRtpStreamProfiles)
		{
			auto& profiles = kv.second;

			for (auto profile : profiles)
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

		this->rtpStreams.clear();
		this->mapRtxStreams.clear();
		this->mapRtpStreamProfiles.clear();
		this->healthyProfiles.clear();
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
		if (this->mapRtpStreamProfiles.find(rtpStream) != this->mapRtpStreamProfiles.end())
		{
			auto it      = this->mapRtpStreamProfiles[rtpStream].begin();
			auto profile = *it;

			return profile;
		}

		MS_THROW_ERROR("unknown RTP packet received [ssrc:%" PRIu32 "]", packet->GetSsrc());
	}

	void Producer::AddHealthyProfiles(RTC::RtpStreamRecv* rtpStream)
	{
		auto& profiles = this->mapRtpStreamProfiles[rtpStream];

		// Notify about the profiles being enabled.
		for (auto& profile : profiles)
		{
			MS_ASSERT(
			  this->healthyProfiles.find(profile) == this->healthyProfiles.end(),
			  "profile already in headltyProfiles set");

			// Add the profile to the healthy profiles set.
			this->healthyProfiles[profile] = rtpStream;

			for (auto& listener : this->listeners)
			{
				listener->OnProducerProfileEnabled(this, profile, rtpStream);
			}
		}
	}

	void Producer::RemoveHealthyProfiles(RTC::RtpStreamRecv* rtpStream)
	{
		auto& profiles = this->mapRtpStreamProfiles[rtpStream];

		// Notify about the profiles being disabled.
		for (auto& profile : profiles)
		{
			MS_ASSERT(
			  this->healthyProfiles.find(profile) != this->healthyProfiles.end(),
			  "profile not in headltyProfiles");

			// Remove the profile from the healthy profiles set.
			this->healthyProfiles.erase(profile);

			for (auto& listener : this->listeners)
			{
				listener->OnProducerProfileDisabled(this, profile);
			}
		}
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

		rtpStream->nackCount++;
	}

	void Producer::OnRtpStreamRecvPliRequired(RTC::RtpStreamRecv* rtpStream)
	{
		MS_TRACE();

		MS_DEBUG_2TAGS(rtcp, rtx, "sending PLI [ssrc:%" PRIu32 "]", rtpStream->GetSsrc());

		RTC::RTCP::FeedbackPsPliPacket packet(0, rtpStream->GetSsrc());

		packet.Serialize(RTC::RTCP::Buffer);

		// Send two, because it's free.
		this->transport->SendRtcpPacket(&packet);
		this->transport->SendRtcpPacket(&packet);

		rtpStream->pliCount++;
	}

	void Producer::OnRtpStreamInactivity(RTC::RtpStream* rtpStream)
	{
		MS_TRACE();

		auto rtpStreamRecv = dynamic_cast<RtpStreamRecv*>(rtpStream);

		MS_ASSERT(
		  this->mapRtpStreamProfiles.find(rtpStreamRecv) != this->mapRtpStreamProfiles.end(),
		  "stream not present in mapRtpStreamProfiles");

		// Single healthy profile present. Ignore.
		if (this->healthyProfiles.size() == 1)
			return;

		// Simulcast. Check whether any RTP is being received at all.
		uint32_t totalBitrate = 0;
		uint64_t now = DepLibUV::GetTime();

		for (auto it : this->healthyProfiles)
		{
			auto healthyRtpStream = it.second;
			auto ssrc = healthyRtpStream->GetSsrc();

			totalBitrate += this->rtpStreams[ssrc]->GetRate(now);
		}

		// No RTP is being received at all. Ignore.
		if (totalBitrate == 0)
			return;

		// Simulcast. Remove the stream from healthy profiles.
		MS_DEBUG_TAG(rtp,
			"rtp inactivity detected [ssrc:%" PRIu32,
			rtpStream->GetSsrc());

		rtpStream->SetUnhealthy();
		RemoveHealthyProfiles(rtpStreamRecv);

		// Reset health check timer on every healthy stream.
		for (auto it : this->healthyProfiles)
		{
			auto healthyRtpStream = it.second;
			auto ssrc = healthyRtpStream->GetSsrc();

			this->rtpStreams[ssrc]->ResetHealthCheckTimer(RTC::RtpStream::HealthCheckPeriod * 2);
		}
	}

	void Producer::OnRtpStreamHealthy(RTC::RtpStream* rtpStream)
	{
		MS_TRACE();

		auto rtpStreamRecv = dynamic_cast<RtpStreamRecv*>(rtpStream);

		MS_ASSERT(
		  this->mapRtpStreamProfiles.find(rtpStreamRecv) != this->mapRtpStreamProfiles.end(),
		  "stream not present in mapRtpStreamProfiles");

		MS_DEBUG_TAG(rtp, "stream is now healthy [ssrc:%" PRIu32 "]", rtpStream->GetSsrc());

		// Add the profiles related to this stream into healthyProfiles and notify the listener.
		AddHealthyProfiles(rtpStreamRecv);
	}

	void Producer::OnRtpStreamUnhealthy(RTC::RtpStream* rtpStream)
	{
		MS_TRACE();

		auto rtpStreamRecv = dynamic_cast<RtpStreamRecv*>(rtpStream);

		MS_ASSERT(
		  this->mapRtpStreamProfiles.find(rtpStreamRecv) != this->mapRtpStreamProfiles.end(),
		  "stream not present in mapRtpStreamProfiles");

		MS_DEBUG_TAG(rtp, "stream is now unhealthy [ssrc:%" PRIu32 "]", rtpStream->GetSsrc());

		// Remove the profiles related to this stream from healthyProfiles and notify the listener.
		RemoveHealthyProfiles(rtpStreamRecv);
	}

	inline void Producer::OnTimer(Timer* timer)
	{
		MS_TRACE();

		if (timer == this->keyFrameRequestBlockTimer)
		{
			// Nobody asked for a key frame since the timer was started.
			if (!this->isKeyFrameRequested)
				return;

			MS_DEBUG_2TAGS(rtcp, rtx, "key frame requested during flood protection, requesting it now");

			RequestKeyFrame();
		}
	}
} // namespace RTC
