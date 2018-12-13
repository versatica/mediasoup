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

		// Must delete all the RecvRtpStream instances.
		for (auto& kv : this->mapSsrcRtpStreamInfo)
		{
			auto& info      = kv.second;
			auto* rtpStream = info.rtpStream;

			delete rtpStream;
		}
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
		static const Json::StaticString JsonStringRtpStreamInfos{ "rtpStreamInfos" };
		static const Json::StaticString JsonStringRtpStream{ "rtpStream" };
		static const Json::StaticString JsonStringRid{ "rid" };
		static const Json::StaticString JsonStringProfile{ "profile" };
		static const Json::StaticString JsonStringNone{ "none" };
		static const Json::StaticString JsonStringDefault{ "default" };
		static const Json::StaticString JsonStringLow{ "low" };
		static const Json::StaticString JsonStringMedium{ "medium" };
		static const Json::StaticString JsonStringHigh{ "high" };
		static const Json::StaticString JsonStringRtxSsrc{ "rtxSsrc" };
		static const Json::StaticString JsonStringActive{ "active" };
		static const Json::StaticString JsonStringHeaderExtensionIds{ "headerExtensionIds" };
		static const Json::StaticString JsonStringSsrcAudioLevel{ "ssrcAudioLevel" };
		static const Json::StaticString JsonStringAbsSendTime{ "absSendTime" };
		static const Json::StaticString JsonStringPaused{ "paused" };
		static const Json::StaticString JsonStringLossPercentage{ "lossPercentage" };

		Json::Value json(Json::objectValue);
		Json::Value jsonHeaderExtensionIds(Json::objectValue);
		Json::Value jsonRtpStreamInfos(Json::arrayValue);

		json[JsonStringProducerId] = Json::UInt{ this->producerId };

		json[JsonStringKind] = RTC::Media::GetJsonString(this->kind);

		json[JsonStringRtpParameters] = this->rtpParameters.ToJson();

		float lossPercentage = 0;

		for (auto& kv : this->mapSsrcRtpStreamInfo)
		{
			Json::Value jsonRtpStreamInfo(Json::objectValue);

			auto& info = kv.second;

			jsonRtpStreamInfo[JsonStringRtpStream] = info.rtpStream->ToJson();

			jsonRtpStreamInfo[JsonStringRid] = info.rid;

			jsonRtpStreamInfo[JsonStringRtxSsrc] = info.rtxSsrc;
			jsonRtpStreamInfo[JsonStringActive]  = info.active;

			switch (info.profile)
			{
				case RTC::RtpEncodingParameters::Profile::NONE:
					jsonRtpStreamInfo[JsonStringProfile] = JsonStringNone;
					break;
				case RTC::RtpEncodingParameters::Profile::DEFAULT:
					jsonRtpStreamInfo[JsonStringProfile] = JsonStringDefault;
					break;
				case RTC::RtpEncodingParameters::Profile::LOW:
					jsonRtpStreamInfo[JsonStringProfile] = JsonStringLow;
					break;
				case RTC::RtpEncodingParameters::Profile::MEDIUM:
					jsonRtpStreamInfo[JsonStringProfile] = JsonStringMedium;
					break;
				case RTC::RtpEncodingParameters::Profile::HIGH:
					jsonRtpStreamInfo[JsonStringProfile] = JsonStringHigh;
					break;
			}

			jsonRtpStreamInfos.append(jsonRtpStreamInfo);

			lossPercentage += info.rtpStream->GetLossPercentage();
		}

		json[JsonStringRtpStreamInfos] = jsonRtpStreamInfos;

		if (!this->mapSsrcRtpStreamInfo.empty())
			lossPercentage = lossPercentage / this->mapSsrcRtpStreamInfo.size();

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

		for (auto& kv : this->mapSsrcRtpStreamInfo)
		{
			auto& info         = kv.second;
			auto* rtpStream    = info.rtpStream;
			auto jsonRtpStream = rtpStream->GetStats();

			if (this->transport != nullptr)
				jsonRtpStream[JsonStringTransportId] = this->transport->transportId;

			json.append(jsonRtpStream);
		}

		return json;
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

		if (this->kind == RTC::Media::Kind::VIDEO)
		{
			MS_DEBUG_2TAGS(rtcp, rtx, "requesting key frame after resumed");

			RequestKeyFrame(true);
		}
	}

	void Producer::ReceiveRtpPacket(RTC::RtpPacket* packet)
	{
		MS_TRACE();

		// May need to create a new RtpStreamRecv.
		MayNeedNewStream(packet);

		// Find the corresponding RtpStreamRecv.
		uint32_t ssrc = packet->GetSsrc();
		RTC::RtpStreamRecv* rtpStream{ nullptr };
		RTC::RtpEncodingParameters::Profile profile;
		std::unique_ptr<RTC::RtpPacket> clonedPacket;

		// Media RTP stream found.
		if (this->mapSsrcRtpStreamInfo.find(ssrc) != this->mapSsrcRtpStreamInfo.end())
		{
			rtpStream = this->mapSsrcRtpStreamInfo[ssrc].rtpStream;

			auto& info = this->mapSsrcRtpStreamInfo[ssrc];
			rtpStream  = info.rtpStream;
			profile    = info.profile;

			// Let's clone the RTP packet so we can mangle the payload (if needed) and other
			// stuff that would change its size.
			clonedPacket.reset(packet->Clone(ClonedPacketBuffer));
			packet = clonedPacket.get();

			// Process the packet.
			if (!rtpStream->ReceivePacket(packet))
				return;
		}
		// Otherwise look for RTX SSRCs.
		else
		{
			for (auto& kv : this->mapSsrcRtpStreamInfo)
			{
				auto& info = kv.second;

				if (info.rtxSsrc != 0u && info.rtxSsrc == ssrc)
				{
					rtpStream = info.rtpStream;
					profile   = info.profile;

					// Let's clone the RTP packet so we can mangle the payload (if needed) and
					// other stuff that would change its size.
					clonedPacket.reset(packet->Clone(ClonedPacketBuffer));
					packet = clonedPacket.get();

					// Process the packet.
					if (!rtpStream->ReceiveRtxPacket(packet))
						return;

					// Packet repaired after applying RTX.
					rtpStream->packetsRepaired++;

					break;
				}
			}
		}

		// Not found.
		if (!rtpStream)
		{
			MS_WARN_TAG(rtp, "no RtpStream found for given RTP packet [ssrc:%" PRIu32 "]", ssrc);

			return;
		}

		// TODO: Remove this, it's CPU consuming and provides nothing.
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

		for (auto& kv : this->mapSsrcRtpStreamInfo)
		{
			auto& info      = kv.second;
			auto* rtpStream = info.rtpStream;
			auto* report    = rtpStream->GetRtcpReceiverReport();

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

		if (this->kind != RTC::Media::Kind::VIDEO || this->paused)
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

		for (auto& kv : this->mapSsrcRtpStreamInfo)
		{
			auto& info      = kv.second;
			auto* rtpStream = info.rtpStream;

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
		uint8_t midId{ 0 };
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

			if ((midId == 0u) && exten.type == RTC::RtpHeaderExtensionUri::Type::MID)
			{
				if (idMapping.find(exten.id) != idMapping.end())
					midId = idMapping[exten.id];
				else
					midId = exten.id;

				this->headerExtensionIds.mid          = midId;
				this->transportHeaderExtensionIds.mid = exten.id;
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

		// If this is an already known media SSRC, we are done.
		if (this->mapSsrcRtpStreamInfo.find(ssrc) != this->mapSsrcRtpStreamInfo.end())
			return;

		// Otherwise, first look for encodings with ssrc field.
		for (auto& encoding : this->rtpParameters.encodings)
		{
			if (encoding.ssrc == ssrc)
			{
				CreateRtpStream(encoding, ssrc);

				return;
			}
		}

		// If not found, look for an encoding with encodingId (RID) field.
		const uint8_t* ridPtr;
		size_t ridLen;

		if (!packet->ReadRid(&ridPtr, &ridLen))
			return;

		auto* charRidPtr = reinterpret_cast<const char*>(ridPtr);
		std::string rid(charRidPtr, ridLen);

		for (auto& encoding : this->rtpParameters.encodings)
		{
			if (encoding.encodingId != rid)
				continue;

			// Ignore if RTX.
			auto& rtxCodec = this->rtpParameters.GetRtxCodecForEncoding(encoding);

			if (rtxCodec.payloadType != 0u && packet->GetPayloadType() == rtxCodec.payloadType)
			{
				// TODO: Here we should fill rtxSsrc in the corresponding RtpStreamInfo with
				// same profile (if it exists in the map).
				//
				// https://github.com/versatica/mediasoup/issues/237

				return;
			}

			// If the matching encoding.profile is already present in the map, it means
			// that the sender is using a new SSRC for the same profile, so ignore it.
			for (auto& kv : this->mapSsrcRtpStreamInfo)
			{
				auto& info   = kv.second;
				auto profile = info.profile;

				if (profile == encoding.profile)
				{
					MS_WARN_TAG(rtp, "ignoring media packet with RID already handled [ssrc:%" PRIu32 "]", ssrc);

					return;
				}
			}

			CreateRtpStream(encoding, ssrc);

			return;
		}
	}

	void Producer::CreateRtpStream(RTC::RtpEncodingParameters& encoding, uint32_t ssrc)
	{
		MS_TRACE();

		MS_ASSERT(
		  this->mapSsrcRtpStreamInfo.find(ssrc) == this->mapSsrcRtpStreamInfo.end(),
		  "stream already exists");

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

		// Create a RtpStreamInfo struct.
		struct RtpStreamInfo info;

		info.rtpStream = rtpStream;
		info.rid       = encoding.encodingId;
		info.profile   = encoding.profile;
		info.rtxSsrc   = 0;
		info.active    = false;

		// Insert into the map.
		//
		// TODO: Could this be optimized by using some exotic C++ "struct move instead
		// of copy"?
		this->mapSsrcRtpStreamInfo[ssrc] = info;

		// Check RTX capabilities and fill them.
		//
		// TODO: Not valid for RID+RTX:
		// https://github.com/versatica/mediasoup/issues/237
		if (encoding.hasRtx && encoding.rtx.ssrc != 0u)
		{
			auto& rtxCodec = this->rtpParameters.GetRtxCodecForEncoding(encoding);

			rtpStream->SetRtx(rtxCodec.payloadType, encoding.rtx.ssrc);

			// Set the info rtxSsrc field.
			this->mapSsrcRtpStreamInfo[ssrc].rtxSsrc = encoding.rtx.ssrc;
		}

		// Activate the stream.
		ActivateStream(rtpStream);

		// Request a key frame since we may have lost the first packets of this stream.
		RequestKeyFrame(true);
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

	void Producer::ActivateStream(RTC::RtpStreamRecv* rtpStream)
	{
		auto ssrc = rtpStream->GetSsrc();

		MS_ASSERT(
		  this->mapSsrcRtpStreamInfo.find(ssrc) != this->mapSsrcRtpStreamInfo.end(),
		  "stream not present in the map");

		auto& info   = this->mapSsrcRtpStreamInfo[ssrc];
		auto profile = info.profile;

		MS_ASSERT(info.profile != RTC::RtpEncodingParameters::Profile::NONE, "stream has no profile");
		MS_ASSERT(info.active == false, "stream already active");

		info.active = true;

		// Update the active profiles map.
		this->mapActiveProfiles[profile] = rtpStream;

		// Notify about the profile being enabled.
		for (auto& listener : this->listeners)
		{
			listener->OnProducerProfileEnabled(this, profile, rtpStream);
		}
	}

	void Producer::DeactivateStream(RTC::RtpStreamRecv* rtpStream)
	{
		auto ssrc = rtpStream->GetSsrc();

		MS_ASSERT(
		  this->mapSsrcRtpStreamInfo.find(ssrc) != this->mapSsrcRtpStreamInfo.end(),
		  "stream not present in the map");

		auto& info   = this->mapSsrcRtpStreamInfo[ssrc];
		auto profile = info.profile;

		MS_ASSERT(info.profile != RTC::RtpEncodingParameters::Profile::NONE, "stream has no profile");
		MS_ASSERT(info.active == true, "stream already inactive");

		info.active = false;

		// Update the active profiles map.
		this->mapActiveProfiles.erase(profile);

		// Notify about the profile being disabled.
		for (auto& listener : this->listeners)
		{
			listener->OnProducerProfileDisabled(this, profile);
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

			auto* nackItem = new RTC::RTCP::FeedbackRtpNackItem(seq, bitmask);

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

		this->transport->SendRtcpPacket(&packet);

		rtpStream->pliCount++;
	}

	void Producer::OnRtpStreamInactive(RTC::RtpStream* rtpStream)
	{
		MS_TRACE();

		size_t numActiveStreams = 0;
		uint32_t totalBitrate   = 0;
		uint64_t now            = DepLibUV::GetTime();

		for (auto& kv : this->mapSsrcRtpStreamInfo)
		{
			auto& info           = kv.second;
			auto* otherRtpStream = info.rtpStream;

			if (info.active)
			{
				numActiveStreams++;
				totalBitrate += otherRtpStream->GetRate(now);
			}
		}

		// If there is a single active stream, do nothing.
		if (numActiveStreams <= 1)
			return;

		// Simulcast. No RTP is being received at all. Ignore.
		if (totalBitrate == 0)
			return;

		auto ssrc   = rtpStream->GetSsrc();
		auto active = this->mapSsrcRtpStreamInfo[ssrc].active;

		// Deactivate stream if active.
		if (active)
		{
			MS_DEBUG_TAG(rtp, "stream is now inactive [ssrc:%" PRIu32 "]", ssrc);

			auto* rtpStreamRecv = dynamic_cast<RtpStreamRecv*>(rtpStream);

			DeactivateStream(rtpStreamRecv);
		}
	}

	void Producer::OnRtpStreamActive(RTC::RtpStream* rtpStream)
	{
		MS_TRACE();

		auto ssrc   = rtpStream->GetSsrc();
		auto active = this->mapSsrcRtpStreamInfo[ssrc].active;

		// Activate stream if inactive.
		if (!active)
		{
			MS_DEBUG_TAG(rtp, "stream is now active [ssrc:%" PRIu32 "]", ssrc);

			auto* rtpStreamRecv = dynamic_cast<RtpStreamRecv*>(rtpStream);

			ActivateStream(rtpStreamRecv);
		}
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
