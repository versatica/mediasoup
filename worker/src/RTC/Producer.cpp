#define MS_CLASS "RTC::Producer"
// #define MS_LOG_DEV

#include "RTC/Producer.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "RTC/RTCP/FeedbackPsFir.hpp"
#include "RTC/RTCP/FeedbackPsPli.hpp"
#include "RTC/RTCP/FeedbackRtp.hpp"
#include "RTC/RTCP/FeedbackRtpNack.hpp"

namespace RTC
{
	/* Static. */

	static uint8_t ClonedPacketBuffer[RTC::RtpBufferSize];

	/* Instance methods. */

	Producer::Producer(
	  const std::string& id,
	  Listener* listener,
	  RTC::Media::Kind kind,
	  RTC::RtpParameters& rtpParameters,
	  struct RtpMapping& rtpMapping)
	  : producerId(producerId), listener(listener), kind(kind), rtpParameters(rtpParameters),
	    rtpMapping(rtpMapping)
	{
		MS_TRACE();

		// The number of encodings in rtpParameters must match the number of enodings
		// in rtpMapping.
		if (rtpParameters.encodings.size() != rtpMapping.encodings.size())
		{
			MS_THROW_TYPE_ERROR("rtpParameters.encodings size does not match rtpMapping.encodings size");
		}

		// Fill RTP header extension ids and their mapped values.
		// This may throw.
		for (auto& exten : this->rtpParameters.headerExtensions)
		{
			auto it = this->rtpMapping.headerExtensions.find(exten.id);

			// This should not happen since rtpMapping has been made reading the rtpParameters.
			if (it == this->rtpMapping.headerExtensions.end())
				MS_THROW_TYPE_ERROR("RTP extension id not present in the RTP mapping");

			auto mappedId = it->second;

			if (this->rtpHeaderExtensionIds.ssrcAudioLevel == 0u && exten.type == RTC::RtpHeaderExtensionUri::Type::SSRC_AUDIO_LEVEL)
			{
				this->rtpHeaderExtensionIds.ssrcAudioLevel       = exten.id;
				this->mappedRtpHeaderExtensionIds.ssrcAudioLevel = mappedId;
			}

			if (this->rtpHeaderExtensionIds.absSendTime == 0u && exten.type == RTC::RtpHeaderExtensionUri::Type::ABS_SEND_TIME)
			{
				this->rtpHeaderExtensionIds.absSendTime       = exten.id;
				this->mappedRtpHeaderExtensionIds.absSendTime = mappedId;
			}

			if (this->rtpHeaderExtensionIds.mid == 0u && exten.type == RTC::RtpHeaderExtensionUri::Type::MID)
			{
				this->rtpHeaderExtensionIds.mid       = exten.id;
				this->mappedRtpHeaderExtensionIds.mid = mappedId;
			}

			if (this->rtpHeaderExtensionIds.rid == 0u && exten.type == RTC::RtpHeaderExtensionUri::Type::RTP_STREAM_ID)
			{
				this->rtpHeaderExtensionIds.rid       = exten.id;
				this->mappedRtpHeaderExtensionIds.rid = mappedId;
			}
		}

		// Set the RTCP report generation interval.
		if (this->kind == RTC::Media::Kind::AUDIO)
			this->maxRtcpInterval = RTC::RTCP::MaxAudioIntervalMs;
		else
			this->maxRtcpInterval = RTC::RTCP::MaxVideoIntervalMs;

		// TODO: Create a new KeyFrameRequestManager.
		// this->keyFrameRequestManager = new RTC::KeyFrameRequestManager(this);
	}

	Producer::~Producer()
	{
		MS_TRACE();

		// Delete all streams.
		for (auto& kv : this->mapSsrcRtpStream)
		{
			auto* rtpStream = kv.second;

			delete rtpStream;
		}

		this->mapSsrcRtpStream.clear();
		this->mapRtpStreamMappedSsrc.clear();
		this->healthyRtpStreams.clear();

		// TODO: Delete the KeyFrameRequestManager().
		// delete this->keyFrameRequestManager;
	}

	// TODO
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
		static const Json::StaticString JsonStringHeaderExtensionIds{ "rtpHeaderExtensionIds" };
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

		if (this->rtpHeaderExtensionIds.ssrcAudioLevel != 0u)
			jsonHeaderExtensionIds[JsonStringSsrcAudioLevel] = this->rtpHeaderExtensionIds.ssrcAudioLevel;

		if (this->rtpHeaderExtensionIds.absSendTime != 0u)
			jsonHeaderExtensionIds[JsonStringAbsSendTime] = this->rtpHeaderExtensionIds.absSendTime;

		if (this->rtpHeaderExtensionIds.rid != 0u)
			jsonHeaderExtensionIds[JsonStringRid] = this->rtpHeaderExtensionIds.rid;

		json[JsonStringHeaderExtensionIds] = jsonHeaderExtensionIds;

		json[JsonStringPaused] = this->paused;

		return json;
	}

	// TODO
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

		MS_DEBUG_DEV("Producer paused [producerId:%s]", this->producerId.c_str());

		this->listener->OnProducerPaused(this);
	}

	void Producer::Resume()
	{
		MS_TRACE();

		if (!this->paused)
			return;

		this->paused = false;

		MS_DEBUG_DEV("Producer resumed [producerId:%s]", this->producerId.c_str());

		this->listener->OnProducerResumed(this);

		if (this->kind == RTC::Media::Kind::VIDEO)
		{
			MS_DEBUG_2TAGS(rtcp, rtx, "requesting forced key frame after resumed");

			RequestKeyFrame(true);
		}
	}

	void Producer::ReceiveRtpPacket(RTC::RtpPacket* packet)
	{
		MS_TRACE();

		auto* rtpStream = GetRtpStream(packet);

		if (!rtpStream)
		{
			MS_WARN_TAG(rtp, "no RtpStream found for received RTP packet [ssrc:%" PRIu32 "]", ssrc);

			return;
		}

		// TODO: Here we mut check if ssrc is packet->GetSsrc() or packet->GetRtxSsrc().

		// Find the corresponding RtpStreamRecv.
		uint32_t ssrc = packet->GetSsrc();
		RTC::RtpStreamRecv* rtpStream{ nullptr };
		std::unique_ptr<RTC::RtpPacket> clonedPacket;

		// TODO: use iterators!!!
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

		this->listener->OnProducerRtpPacketReceived(this, packet);
	}

	void Producer::ReceiveRtcpSenderReport(RTC::RTCP::SenderReport* report)
	{
		// TODO
		auto it = this->mapSsrcRtpStreamInfo.find(report->GetSsrc());

		if (it != this->mapSsrcRtpStreamInfo.end())
		{
			auto& info      = it->second;
			auto* rtpStream = info.rtpStream;

			rtpStream->ReceiveRtcpSenderReport(report);
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

	void Producer::RequestKeyFrame(bool force)
	{
		MS_TRACE();

		if (this->kind != RTC::Media::Kind::VIDEO || this->paused)
			return;

		// TODO: Use the new KeyFrameRequestManager.
	}

	RTC::RtpStreamRecv* Producer::GetRtpStream(RTC::RtpPacket* packet)
	{
		MS_TRACE();

		uint32_t ssrc       = packet->GetSsrc();
		uint8_t payloadType = packet->GetPayloadType();

		// If stream found, return it.
		auto it = this->mapSsrcRtpStream.find(ssrc);

		if (it != this->mapSsrcRtpStream.end())
		{
			auto* rtpStream = it->second;

			return rtpStream;
		}

		// Otherwise, ensure packet has the announced media payloadType or RTX payloadType.

		bool isMediaPacket = false;
		bool isRtxPacket   = false;
		auto* mediaCodec   = this->rtpParameters.GetCodecForEncoding(encoding);
		auto* rtxCodec     = this->rtpParameters.GetRtxCodecForEncoding(encoding);

		if (mediaCodec->payloadType == payloadType)
		{
			isMediaPacket = true;
		}
		else if (rtxCodec && rtxCodec->payloadType == payloadType)
		{
			isRtxPacket = true;
		}
		else
		{
			MS_WARN_TAG(rtp, "ignoring packet with unknown media/RTX payloadType");

			return nullptr;
		}

		// Otherwise check our encodings and, if appropriate, create a new stream.

		// First, look for an encoding with matching media or RTX ssrc value.
		for (size_t i = 0; i < this->rtpParameters.encodings.size(); ++i)
		{
			auto& encoding = this->rtpParameters.encodings[i];

			if (isMediaPacket && encoding.ssrc == ssrc)
			{
				auto* rtpStream = CreateRtpStream(ssrc, *mediaCodec, i);

				return rtpStream;
			}
			else if (isRtxPacket && encoding.hasRtx && encoding.rtx.ssrc == ssrc)
			{
				auto it = this->mapSsrcRtpStream.find(encoding.ssrc);

				// Ignore if no stream has been created yet for the corresponding encoding.
				if (it == this->mapSsrcRtpStream.end())
				{
					MS_DEBUG_2TAGS(rtp, rtx, "ignoring RTX packet for not yet created stream (ssrc lookup)");

					return nullptr;
				}

				auto* rtpStream = it->second;

				// Update the stream RTX data.
				rtpStream->SetRtx(payloadType, ssrc);

				// Insert the new RTX SSRC into the map.
				this->mapSsrcRtpStream[ssrc] = rtpStream;

				return rtpStream;
			}
		}

		// If not found, look for an encoding matching the packet RID value.
		const uint8_t* ridPtr;
		size_t ridLen;

		if (packet->ReadRid(&ridPtr, &ridLen))
		{
			auto* charRidPtr = reinterpret_cast<const char*>(ridPtr);
			std::string rid(charRidPtr, ridLen);

			for (size_t i = 0; i < this->rtpParameters.encodings.size(); ++i)
			{
				auto& encoding = this->rtpParameters.encodings[i];

				if (encoding.rid != rid)
					continue;

				if (isMediaPacket)
				{
					// Ensure no other stream already exists with same RID.
					for (auto& kv : this->mapSsrcRtpStream)
					{
						auto* rtpStream = kv.second;

						if (rtpStream->GetRid() == rid)
						{
							MS_WARN_TAG(
							  rtp, "ignoring packet with unknown ssrc but already handled RID (RID lookup)");

							return nullptr;
						}
					}

					auto* rtpStream = CreateRtpStream(ssrc, *mediaCodec, i);

					return rtpStream;
				}
				else if (isRtxPacket)
				{
					// Ensure an rtpStream already exists with same RID.
					for (auto& kv : this->mapSsrcRtpStream)
					{
						auto* rtpStream = kv.second;

						if (rtpStream->GetRid() == rid)
						{
							// Update the stream RTX data.
							rtpStream->SetRtx(payloadType, ssrc);

							// Insert the new RTX SSRC into the map.
							this->mapSsrcRtpStream[ssrc] = rtpStream;

							return rtpStream;
						}
					}

					MS_DEBUG_2TAGS(rtp, rtx, "ignoring RTX packet for not yet created stream (RID lookup)");

					return nullptr;
				}
			}

			MS_WARN_TAG(rtp, "ignoring packet with unknown RID (RID lookup)");

			return nullptr;
		}

		return nullptr;
	}

	void Producer::CreateRtpStream(uint32_t ssrc, RTC::RtpCodecParameters& codec, size_t encodingIdx)
	{
		MS_TRACE();

		MS_ASSERT(
		  this->mapSsrcRtpStream.find(ssrc) == this->mapSsrcRtpStream.end(), "stream already exists");

		auto& encoding        = this->rtpParameters.encodings[encodingIdx];
		auto& encodingMapping = this->rtpMapping.encodings[encodingIdx];

		// Set stream params.
		RTC::RtpStream::Params params;

		params.ssrc        = ssrc;
		params.payloadType = codec.payloadType;
		params.mimeType    = codec.mimeType;
		params.clockRate   = codec.clockRate;
		params.rid         = encoding.rid;

		for (auto& fb : codec.rtcpFeedback)
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
			else if (!params.useRemb && fb.type == "goog-remb")
			{
				MS_DEBUG_TAG(rbe, "REMB supported");

				params.useRemb = true;
			}
		}

		// Create a RtpStreamRecv for receiving a media stream.
		auto* rtpStream = new RTC::RtpStreamRecv(this, params);

		// Insert into the map.
		this->mapSsrcRtpStream[ssrc] = rtpStream;

		// Set the mapped SSRC.
		this->mapRtpStreamMappedSsrc[rtpStream] = encodingMapping.mappedSsrc;

		// Set stream as healthy.
		SetHealthyStream(rtpStream);

		// Request a key frame since we may have lost the first packets of this stream.
		RequestKeyFrame(true);
	}

	void Producer::SetHealthyStream(RTC::RtpStreamRecv* rtpStream)
	{
		MS_TRACE();

		if (this->healthyRtpStreams.find(rtpStream) != this->healthyRtpStreams.end())
			return;

		this->healthyRtpStreams.insert(rtpStream);

		uint32_t mappedSsrc = this->mapRtpStreamMappedSsrc.at(rtpStream);

		// Notify the listener.
		this->listener->OnProducerRtpStreamHealthy(this, rtpStream, mappedSsrc);
	}

	void Producer::SetUnhealthyStream(RTC::RtpStreamRecv* rtpStream)
	{
		MS_TRACE();

		if (this->healthyRtpStreams.find(rtpStream) == this->healthyRtpStreams.end())
			return;

		this->healthyRtpStreams.erase(rtpStream);

		uint32_t mappedSsrc = this->mapRtpStreamMappedSsrc.at(rtpStream);

		// Notify the listener.
		this->listener->OnProducerRtpStreamUnhealthy(this, rtpStream, mappedSsrc);
	}

	// TODO: Must set mapped ssrc!
	void Producer::MangleRtpRtpPacket(RTC::RtpPacket* packet) const
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

		if (this->rtpHeaderExtensionIds.ssrcAudioLevel != 0u)
		{
			packet->AddExtensionMapping(
			  RtpHeaderExtensionUri::Type::SSRC_AUDIO_LEVEL, this->rtpHeaderExtensionIds.ssrcAudioLevel);
		}

		if (this->rtpHeaderExtensionIds.absSendTime != 0u)
		{
			packet->AddExtensionMapping(
			  RtpHeaderExtensionUri::Type::ABS_SEND_TIME, this->rtpHeaderExtensionIds.absSendTime);
		}

		if (this->rtpHeaderExtensionIds.rid != 0u)
		{
			packet->AddExtensionMapping(
			  RtpHeaderExtensionUri::Type::RTP_STREAM_ID, this->rtpHeaderExtensionIds.rid);
		}
	}

	void Producer::OnRtpStreamRecvNackRequired(
	  RTC::RtpStreamRecv* rtpStream, const std::vector<uint16_t>& seqNumbers)
	{
		MS_TRACE();

		RTC::RTCP::FeedbackRtpNackPacket packet(0, rtpStream->GetSsrc());

		auto it        = seqNumbers.begin();
		const auto end = seqNumbers.end();
		size_t numPacketsRequested{ 0 };

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

			numPacketsRequested += nackItem->CountRequestedPackets();
		}

		// Ensure that the RTCP packet fits into the RTCP buffer.
		if (packet.GetSize() > RTC::RTCP::BufferSize)
		{
			MS_WARN_TAG(rtx, "cannot send RTCP NACK packet, size too big (%zu bytes)", packet.GetSize());

			return;
		}

		packet.Serialize(RTC::RTCP::Buffer);

		this->listener->OnProducerSendRtcpPacket(&packet);

		rtpStream->nackCount++;
		rtpStream->nackRtpPacketCount += numPacketsRequested;
	}

	void Producer::OnRtpStreamRecvPliRequired(RTC::RtpStreamRecv* rtpStream)
	{
		MS_TRACE();

		MS_DEBUG_2TAGS(rtcp, rtx, "sending PLI [ssrc:%" PRIu32 "]", rtpStream->GetSsrc());

		RTC::RTCP::FeedbackPsPliPacket packet(0, rtpStream->GetSsrc());

		packet.Serialize(RTC::RTCP::Buffer);

		this->listener->OnProducerSendRtcpPacket(&packet);

		rtpStream->pliCount++;
	}

	void Producer::OnRtpStreamRecvFirRequired(RTC::RtpStreamRecv* rtpStream)
	{
		MS_TRACE();

		MS_DEBUG_2TAGS(rtcp, rtx, "sending FIR [ssrc:%" PRIu32 "]", rtpStream->GetSsrc());

		RTC::RTCP::FeedbackPsFirPacket packet(0, rtpStream->GetSsrc());
		FeedbackPsFirItem* item =
		  new FeedbackPsFirItem(rtpStream->GetSsrc(), rtpStream->GetFirSeqNumber());

		packet.AddItem(item);
		packet.Serialize(RTC::RTCP::Buffer);

		this->listener->OnProducerSendRtcpPacket(&packet);

		rtpStream->pliCount++;
	}

	void Producer::OnRtpStreamHealthy(RTC::RtpStream* rtpStream)
	{
		MS_TRACE();

		auto* rtpStreamRecv = dynamic_cast<RtpStreamRecv*>(rtpStream);

		SetHealthyStream(rtpStreamRecv);
	}

	void Producer::OnRtpStreamUnhealthy(RTC::RtpStream* rtpStream)
	{
		MS_TRACE();

		auto* rtpStreamRecv = dynamic_cast<RtpStreamRecv*>(rtpStream);

		SetUnhealthyStream(rtpStreamRecv);
	}
} // namespace RTC
