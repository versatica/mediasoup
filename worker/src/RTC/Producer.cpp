#define MS_CLASS "RTC::Producer"
// #define MS_LOG_DEV

#include "RTC/Producer.hpp"
#include "DepLibUV.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "Utils.hpp"
#include "Channel/Notifier.hpp"
#include <cstring> // std::memcpy()

namespace RTC
{
	/* Instance methods. */

	Producer::Producer(const std::string& id, RTC::Producer::Listener* listener, json& data)
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

		// Evaluate type.
		this->type = RTC::RtpParameters::GetType(this->rtpParameters);

		auto jsonRtpMappingIt = data.find("rtpMapping");

		if (jsonRtpMappingIt == data.end() || !jsonRtpMappingIt->is_object())
			MS_THROW_TYPE_ERROR("missing rtpMapping");

		auto jsonCodecsIt = jsonRtpMappingIt->find("codecs");

		if (jsonCodecsIt == jsonRtpMappingIt->end() || !jsonCodecsIt->is_array())
			MS_THROW_TYPE_ERROR("missing rtpMapping.codecs");

		for (auto& codec : *jsonCodecsIt)
		{
			if (!codec.is_object())
				MS_THROW_TYPE_ERROR("wrong entry in rtpMapping.codecs (not an object)");

			auto jsonPayloadTypeIt = codec.find("payloadType");

			if (jsonPayloadTypeIt == codec.end() || !jsonPayloadTypeIt->is_number_unsigned())
				MS_THROW_TYPE_ERROR("wrong entry in rtpMapping.codecs (missing payloadType)");

			auto jsonMappedPayloadTypeIt = codec.find("mappedPayloadType");

			if (jsonMappedPayloadTypeIt == codec.end() || !jsonMappedPayloadTypeIt->is_number_unsigned())
				MS_THROW_TYPE_ERROR("wrong entry in rtpMapping.codecs (missing mappedPayloadType)");

			this->rtpMapping.codecs[jsonPayloadTypeIt->get<uint8_t>()] =
			  jsonMappedPayloadTypeIt->get<uint8_t>();
		}

		auto jsonEncodingsIt = jsonRtpMappingIt->find("encodings");

		if (jsonEncodingsIt == jsonRtpMappingIt->end() || !jsonEncodingsIt->is_array())
			MS_THROW_TYPE_ERROR("missing rtpMapping.encodings");

		this->rtpMapping.encodings.reserve(jsonEncodingsIt->size());

		for (auto& encoding : *jsonEncodingsIt)
		{
			if (!encoding.is_object())
				MS_THROW_TYPE_ERROR("wrong entry in rtpMapping.encodings");

			this->rtpMapping.encodings.emplace_back();

			auto& encodingMapping = this->rtpMapping.encodings.back();

			// ssrc is optional.
			auto jsonSsrcIt = encoding.find("ssrc");

			if (jsonSsrcIt != encoding.end() && jsonSsrcIt->is_number_unsigned())
				encodingMapping.ssrc = jsonSsrcIt->get<uint32_t>();

			// rid is optional.
			auto jsonRidIt = encoding.find("rid");

			if (jsonRidIt != encoding.end() && jsonRidIt->is_string())
				encodingMapping.rid = jsonRidIt->get<std::string>();

			// However ssrc or rid must be present.
			if (jsonSsrcIt == encoding.end() && jsonRidIt == encoding.end())
				MS_THROW_TYPE_ERROR("wrong entry in rtpMapping.encodings (missing ssrc or rid)");

			// mappedSsrc is mandatory.
			auto jsonMappedSsrcIt = encoding.find("mappedSsrc");

			if (jsonMappedSsrcIt == encoding.end() || !jsonMappedSsrcIt->is_number_unsigned())
				MS_THROW_TYPE_ERROR("wrong entry in rtpMapping.encodings (missing mappedSsrc)");

			encodingMapping.mappedSsrc = jsonMappedSsrcIt->get<uint32_t>();
		}

		auto jsonPausedIt = data.find("paused");

		if (jsonPausedIt != data.end() && jsonPausedIt->is_boolean())
			this->paused = jsonPausedIt->get<bool>();

		// The number of encodings in rtpParameters must match the number of encodings
		// in rtpMapping.
		if (this->rtpParameters.encodings.size() != this->rtpMapping.encodings.size())
		{
			MS_THROW_TYPE_ERROR("rtpParameters.encodings size does not match rtpMapping.encodings size");
		}

		// Fill RTP header extension ids.
		// This may throw.
		for (auto& exten : this->rtpParameters.headerExtensions)
		{
			if (exten.id == 0u)
				MS_THROW_TYPE_ERROR("RTP extension id cannot be 0");

			if (this->rtpHeaderExtensionIds.mid == 0u && exten.type == RTC::RtpHeaderExtensionUri::Type::MID)
			{
				this->rtpHeaderExtensionIds.mid = exten.id;
			}

			if (this->rtpHeaderExtensionIds.rid == 0u && exten.type == RTC::RtpHeaderExtensionUri::Type::RTP_STREAM_ID)
			{
				this->rtpHeaderExtensionIds.rid = exten.id;
			}

			if (this->rtpHeaderExtensionIds.rrid == 0u && exten.type == RTC::RtpHeaderExtensionUri::Type::REPAIRED_RTP_STREAM_ID)
			{
				this->rtpHeaderExtensionIds.rrid = exten.id;
			}

			if (this->rtpHeaderExtensionIds.absSendTime == 0u && exten.type == RTC::RtpHeaderExtensionUri::Type::ABS_SEND_TIME)
			{
				this->rtpHeaderExtensionIds.absSendTime = exten.id;
			}

			if (this->rtpHeaderExtensionIds.ssrcAudioLevel == 0u && exten.type == RTC::RtpHeaderExtensionUri::Type::SSRC_AUDIO_LEVEL)
			{
				this->rtpHeaderExtensionIds.ssrcAudioLevel = exten.id;
			}

			if (this->rtpHeaderExtensionIds.videoOrientation == 0u && exten.type == RTC::RtpHeaderExtensionUri::Type::VIDEO_ORIENTATION)
			{
				this->rtpHeaderExtensionIds.videoOrientation = exten.id;
			}

			if (this->rtpHeaderExtensionIds.toffset == 0u && exten.type == RTC::RtpHeaderExtensionUri::Type::TOFFSET)
			{
				this->rtpHeaderExtensionIds.toffset = exten.id;
			}
		}

		// Set the RTCP report generation interval.
		if (this->kind == RTC::Media::Kind::AUDIO)
			this->maxRtcpInterval = RTC::RTCP::MaxAudioIntervalMs;
		else
			this->maxRtcpInterval = RTC::RTCP::MaxVideoIntervalMs;

		// Create a KeyFrameRequestManager.
		if (this->kind == RTC::Media::Kind::VIDEO)
			this->keyFrameRequestManager = new RTC::KeyFrameRequestManager(this);
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
		this->mapRtxSsrcRtpStream.clear();
		this->mapRtpStreamMappedSsrc.clear();
		this->mapMappedSsrcSsrc.clear();

		// Delete the KeyFrameRequestManager.
		delete this->keyFrameRequestManager;
	}

	void Producer::FillJson(json& jsonObject) const
	{
		MS_TRACE();

		// Add id.
		jsonObject["id"] = this->id;

		// Add kind.
		jsonObject["kind"] = RTC::Media::GetString(this->kind);

		// Add rtpParameters.
		this->rtpParameters.FillJson(jsonObject["rtpParameters"]);

		// Add type.
		jsonObject["type"] = RTC::RtpParameters::GetTypeString(this->type);

		// Add rtpMapping.
		jsonObject["rtpMapping"] = json::object();
		auto jsonRtpMappingIt    = jsonObject.find("rtpMapping");

		// Add rtpMapping.codecs.
		{
			(*jsonRtpMappingIt)["codecs"] = json::array();
			auto jsonCodecsIt             = jsonRtpMappingIt->find("codecs");
			size_t idx{ 0 };

			for (auto& kv : this->rtpMapping.codecs)
			{
				jsonCodecsIt->emplace_back(json::value_t::object);

				auto& jsonEntry        = (*jsonCodecsIt)[idx];
				auto payloadType       = kv.first;
				auto mappedPayloadType = kv.second;

				jsonEntry["payloadType"]       = payloadType;
				jsonEntry["mappedPayloadType"] = mappedPayloadType;

				++idx;
			}
		}

		// Add rtpMapping.encodings.
		{
			(*jsonRtpMappingIt)["encodings"] = json::array();
			auto jsonEncodingsIt             = jsonRtpMappingIt->find("encodings");

			for (size_t i{ 0 }; i < this->rtpMapping.encodings.size(); ++i)
			{
				jsonEncodingsIt->emplace_back(json::value_t::object);

				auto& jsonEntry       = (*jsonEncodingsIt)[i];
				auto& encodingMapping = this->rtpMapping.encodings[i];

				if (!encodingMapping.rid.empty())
					jsonEntry["rid"] = encodingMapping.rid;
				else
					jsonEntry["rid"] = nullptr;

				if (encodingMapping.ssrc != 0u)
					jsonEntry["ssrc"] = encodingMapping.ssrc;
				else
					jsonEntry["ssrc"] = nullptr;

				jsonEntry["mappedSsrc"] = encodingMapping.mappedSsrc;
			}
		}

		// Add rtpStreams.
		jsonObject["rtpStreams"] = json::array();
		auto jsonRtpStreamsIt    = jsonObject.find("rtpStreams");
		size_t rtpStreamIdx{ 0 };

		for (auto& kv : this->mapSsrcRtpStream)
		{
			jsonRtpStreamsIt->emplace_back(json::value_t::object);

			auto& jsonEntry = (*jsonRtpStreamsIt)[rtpStreamIdx];
			auto* rtpStream = kv.second;

			rtpStream->FillJson(jsonEntry);

			++rtpStreamIdx;
		}

		// Add paused.
		jsonObject["paused"] = this->paused;
	}

	void Producer::FillJsonStats(json& jsonArray) const
	{
		MS_TRACE();

		size_t rtpStreamIdx{ 0 };

		for (auto& kv : this->mapSsrcRtpStream)
		{
			jsonArray.emplace_back(json::value_t::object);

			auto& jsonEntry = jsonArray[rtpStreamIdx];
			auto* rtpStream = kv.second;

			rtpStream->FillJsonStats(jsonEntry);

			++rtpStreamIdx;
		}
	}

	void Producer::HandleRequest(Channel::Request* request)
	{
		MS_TRACE();

		switch (request->methodId)
		{
			case Channel::Request::MethodId::PRODUCER_DUMP:
			{
				json data(json::object());

				FillJson(data);

				request->Accept(data);

				break;
			}

			case Channel::Request::MethodId::PRODUCER_GET_STATS:
			{
				json data(json::array());

				FillJsonStats(data);

				request->Accept(data);

				break;
			}

			case Channel::Request::MethodId::PRODUCER_PAUSE:
			{
				if (this->paused)
				{
					request->Accept();

					return;
				}

				// Pause all streams.
				for (auto& kv : this->mapSsrcRtpStream)
				{
					auto* rtpStream = kv.second;

					rtpStream->Pause();
				}

				this->paused = true;

				MS_DEBUG_DEV("Producer paused [producerId:%s]", this->id.c_str());

				this->listener->OnProducerPaused(this);

				request->Accept();

				break;
			}

			case Channel::Request::MethodId::PRODUCER_RESUME:
			{
				if (!this->paused)
				{
					request->Accept();

					return;
				}

				// Resume all streams.
				for (auto& kv : this->mapSsrcRtpStream)
				{
					auto* rtpStream = kv.second;

					rtpStream->Resume();
				}

				this->paused = false;

				MS_DEBUG_DEV("Producer resumed [producerId:%s]", this->id.c_str());

				this->listener->OnProducerResumed(this);

				if (this->keyFrameRequestManager)
				{
					MS_DEBUG_2TAGS(rtcp, rtx, "requesting forced key frame(s) after resumed");

					// Request a key frame for all streams.
					for (auto& kv : this->mapSsrcRtpStream)
					{
						auto ssrc = kv.first;

						this->keyFrameRequestManager->ForceKeyFrameNeeded(ssrc);
					}
				}

				request->Accept();

				break;
			}

			default:
			{
				MS_THROW_ERROR("unknown method '%s'", request->method.c_str());
			}
		}
	}

	void Producer::ReceiveRtpPacket(RTC::RtpPacket* packet)
	{
		MS_TRACE();

		auto* rtpStream = GetRtpStream(packet);

		if (rtpStream == nullptr)
		{
			MS_WARN_TAG(rtp, "no stream found for received packet [ssrc:%" PRIu32 "]", packet->GetSsrc());

			return;
		}

		// Media packet.
		if (packet->GetSsrc() == rtpStream->GetSsrc())
		{
			// Process the packet.
			if (!rtpStream->ReceivePacket(packet))
				return;
		}
		// RTX packet.
		else if (packet->GetSsrc() == rtpStream->GetRtxSsrc())
		{
			// Process the packet.
			if (!rtpStream->ReceiveRtxPacket(packet))
				return;
		}
		// Should not happen.
		else
		{
			MS_ABORT("found stream does not match received packet");
		}

		if (packet->IsKeyFrame())
		{
			MS_DEBUG_TAG(
			  rtp,
			  "key frame received [ssrc:%" PRIu32 ", seq:%" PRIu16 "]",
			  packet->GetSsrc(),
			  packet->GetSequenceNumber());

			// Tell the keyFrameRequestManager.
			if (this->keyFrameRequestManager)
				this->keyFrameRequestManager->KeyFrameReceived(packet->GetSsrc());
		}

		// If paused stop here.
		if (this->paused)
			return;

		// Mangle the packet before providing the listener with it.
		if (!MangleRtpPacket(packet, rtpStream))
			return;

		// Post-process the packet.
		PostProcessRtpPacket(packet);

		this->listener->OnProducerRtpPacketReceived(this, packet);
	}

	void Producer::ReceiveRtcpSenderReport(RTC::RTCP::SenderReport* report)
	{
		MS_TRACE();

		auto it = this->mapSsrcRtpStream.find(report->GetSsrc());

		if (it == this->mapSsrcRtpStream.end())
		{
			MS_WARN_TAG(rtcp, "RtpStream not found [ssrc:%" PRIu32 "]", report->GetSsrc());

			return;
		}

		auto* rtpStream = it->second;

		rtpStream->ReceiveRtcpSenderReport(report);
	}

	void Producer::GetRtcp(RTC::RTCP::CompoundPacket* packet, uint64_t now)
	{
		MS_TRACE();

		if (static_cast<float>((now - this->lastRtcpSentTime) * 1.15) < this->maxRtcpInterval)
			return;

		for (auto& kv : this->mapSsrcRtpStream)
		{
			auto* rtpStream = kv.second;
			auto* report    = rtpStream->GetRtcpReceiverReport();

			packet->AddReceiverReport(report);
		}

		this->lastRtcpSentTime = now;
	}

	void Producer::RequestKeyFrame(uint32_t mappedSsrc)
	{
		MS_TRACE();

		if (!this->keyFrameRequestManager || this->paused)
			return;

		auto it = this->mapMappedSsrcSsrc.find(mappedSsrc);

		if (it == this->mapMappedSsrcSsrc.end())
		{
			MS_WARN_2TAGS(rtcp, rtx, "given mappedSsrc not found, ignoring");

			return;
		}

		uint32_t ssrc = it->second;

		this->keyFrameRequestManager->KeyFrameNeeded(ssrc);
	}

	RTC::RtpStreamRecv* Producer::GetRtpStream(RTC::RtpPacket* packet)
	{
		MS_TRACE();

		uint32_t ssrc       = packet->GetSsrc();
		uint8_t payloadType = packet->GetPayloadType();

		// If stream found in media ssrcs map, return it.
		{
			auto it = this->mapSsrcRtpStream.find(ssrc);

			if (it != this->mapSsrcRtpStream.end())
			{
				auto* rtpStream = it->second;

				return rtpStream;
			}
		}

		// If stream found in RTX ssrcs map, return it.
		{
			auto it = this->mapRtxSsrcRtpStream.find(ssrc);

			if (it != this->mapRtxSsrcRtpStream.end())
			{
				auto* rtpStream = it->second;

				return rtpStream;
			}
		}

		// Otherwise check our encodings and, if appropriate, create a new stream.

		// First, look for an encoding with matching media or RTX ssrc value.
		for (size_t i{ 0 }; i < this->rtpParameters.encodings.size(); ++i)
		{
			auto& encoding     = this->rtpParameters.encodings[i];
			auto* mediaCodec   = this->rtpParameters.GetCodecForEncoding(encoding);
			auto* rtxCodec     = this->rtpParameters.GetRtxCodecForEncoding(encoding);
			bool isMediaPacket = (mediaCodec->payloadType == payloadType);
			bool isRtxPacket   = (rtxCodec && rtxCodec->payloadType == payloadType);

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
					MS_DEBUG_2TAGS(rtp, rtx, "ignoring RTX packet for not yet created RtpStream (ssrc lookup)");

					return nullptr;
				}

				auto* rtpStream = it->second;

				// Ensure no RTX SSRC was previously detected.
				if (rtpStream->HasRtx())
				{
					MS_DEBUG_2TAGS(rtp, rtx, "ignoring RTX packet with new ssrc (ssrc lookup)");

					return nullptr;
				}

				// Update the stream RTX data.
				rtpStream->SetRtx(payloadType, ssrc);

				// Insert the new RTX SSRC into the map.
				this->mapRtxSsrcRtpStream[ssrc] = rtpStream;

				return rtpStream;
			}
		}

		// If not found, look for an encoding matching the packet RID value.
		std::string rid;

		if (packet->ReadRid(rid))
		{
			for (size_t i{ 0 }; i < this->rtpParameters.encodings.size(); ++i)
			{
				auto& encoding = this->rtpParameters.encodings[i];

				if (encoding.rid != rid)
					continue;

				auto* mediaCodec   = this->rtpParameters.GetCodecForEncoding(encoding);
				auto* rtxCodec     = this->rtpParameters.GetRtxCodecForEncoding(encoding);
				bool isMediaPacket = (mediaCodec->payloadType == payloadType);
				bool isRtxPacket   = (rtxCodec && rtxCodec->payloadType == payloadType);

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
					// Ensure a stream already exists with same RID.
					for (auto& kv : this->mapSsrcRtpStream)
					{
						auto* rtpStream = kv.second;

						if (rtpStream->GetRid() == rid)
						{
							// Ensure no RTX SSRC was previously detected.
							if (rtpStream->HasRtx())
							{
								MS_DEBUG_2TAGS(rtp, rtx, "ignoring RTX packet with new SSRC (RID lookup)");

								return nullptr;
							}

							// Update the stream RTX data.
							rtpStream->SetRtx(payloadType, ssrc);

							// Insert the new RTX SSRC into the map.
							this->mapRtxSsrcRtpStream[ssrc] = rtpStream;

							return rtpStream;
						}
					}

					MS_DEBUG_2TAGS(rtp, rtx, "ignoring RTX packet for not yet created RtpStream (RID lookup)");

					return nullptr;
				}
			}

			MS_WARN_TAG(rtp, "ignoring packet with unknown RID (RID lookup)");

			return nullptr;
		}

		return nullptr;
	}

	RTC::RtpStreamRecv* Producer::CreateRtpStream(
	  uint32_t ssrc, const RTC::RtpCodecParameters& mediaCodec, size_t encodingIdx)
	{
		MS_TRACE();

		MS_ASSERT(
		  this->mapSsrcRtpStream.find(ssrc) == this->mapSsrcRtpStream.end(), "RtpStream already exists");

		auto& encoding        = this->rtpParameters.encodings[encodingIdx];
		auto& encodingMapping = this->rtpMapping.encodings[encodingIdx];

		// Set stream params.
		RTC::RtpStream::Params params;

		params.ssrc        = ssrc;
		params.payloadType = mediaCodec.payloadType;
		params.mimeType    = mediaCodec.mimeType;
		params.clockRate   = mediaCodec.clockRate;
		params.rid         = encoding.rid;
		params.cname       = this->rtpParameters.rtcp.cname;

		// Check in band FEC in codec parameters.
		if (mediaCodec.parameters.HasInteger("useinbandfec") && mediaCodec.parameters.GetInteger("useinbandfec") == 1)
		{
			MS_DEBUG_TAG(rtcp, "in band FEC enabled");

			params.useInBandFec = true;
		}

		// Check DTX in codec parameters.
		if (mediaCodec.parameters.HasInteger("usedtx") && mediaCodec.parameters.GetInteger("usedtx") == 1)
		{
			MS_DEBUG_TAG(rtcp, "DTX enabled");

			params.useDtx = true;
		}

		// Check DTX in the encoding.
		if (encoding.dtx)
		{
			MS_DEBUG_TAG(rtcp, "DTX enabled");

			params.useDtx = true;
		}

		for (auto& fb : mediaCodec.rtcpFeedback)
		{
			if (!params.useNack && fb.type == "nack" && fb.parameter == "")
			{
				MS_DEBUG_2TAGS(rtcp, rtx, "NACK supported");

				params.useNack = true;
			}
			else if (!params.usePli && fb.type == "nack" && fb.parameter == "pli")
			{
				MS_DEBUG_TAG(rtcp, "PLI supported");

				params.usePli = true;
			}
			else if (!params.useFir && fb.type == "ccm" && fb.parameter == "fir")
			{
				MS_DEBUG_TAG(rtcp, "FIR supported");

				params.useFir = true;
			}
		}

		// Create a RtpStreamRecv for receiving a media stream.
		auto* rtpStream = new RTC::RtpStreamRecv(this, params);

		// Insert into the map.
		this->mapSsrcRtpStream[ssrc] = rtpStream;

		// Set the mapped SSRC.
		this->mapRtpStreamMappedSsrc[rtpStream]             = encodingMapping.mappedSsrc;
		this->mapMappedSsrcSsrc[encodingMapping.mappedSsrc] = ssrc;

		// If the Producer is paused tell it to the new RtpStreamRecv.
		if (this->paused)
			rtpStream->Pause();

		// Notify to the listener.
		this->listener->OnProducerNewRtpStream(this, rtpStream, encodingMapping.mappedSsrc);

		// Request a key frame for this stream since we may have lost the first packets.
		if (this->keyFrameRequestManager && !this->paused)
			this->keyFrameRequestManager->ForceKeyFrameNeeded(ssrc);

		// Emit the first score event right now.
		EmitScore();

		return rtpStream;
	}

	inline bool Producer::MangleRtpPacket(RTC::RtpPacket* packet, RTC::RtpStreamRecv* rtpStream) const
	{
		MS_TRACE();

		// Mangle the payload type.
		{
			uint8_t payloadType = packet->GetPayloadType();
			auto it             = this->rtpMapping.codecs.find(payloadType);

			if (it == this->rtpMapping.codecs.end())
			{
				MS_WARN_TAG(rtp, "unknown payload type [payloadType:%" PRIu8 "]", payloadType);

				return false;
			}

			uint8_t mappedPayloadType = it->second;

			packet->SetPayloadType(mappedPayloadType);
		}

		// Mangle the SSRC.
		{
			uint32_t mappedSsrc = this->mapRtpStreamMappedSsrc.at(rtpStream);

			packet->SetSsrc(mappedSsrc);
		}

		// Mangle RTP header extensions.
		{
			static uint8_t buffer[4096];
			static std::vector<RTC::RtpPacket::GenericExtension> extensions;

			// This happens just once.
			if (extensions.capacity() != 24)
				extensions.reserve(24);

			extensions.clear();

			uint8_t* extenValue;
			uint8_t extenLen;
			uint8_t* bufferPtr{ buffer };

			if (this->kind == RTC::Media::Kind::AUDIO)
			{
				// Proxy urn:ietf:params:rtp-hdrext:ssrc-audio-level.
				{
					extenValue = packet->GetExtension(this->rtpHeaderExtensionIds.ssrcAudioLevel, extenLen);

					if (extenValue)
					{
						std::memcpy(bufferPtr, extenValue, extenLen);

						extensions.emplace_back(
						  static_cast<uint8_t>(RTC::RtpHeaderExtensionUri::Type::SSRC_AUDIO_LEVEL),
						  extenLen,
						  bufferPtr);

						bufferPtr += extenLen;
					}
				}
			}
			else if (this->kind == RTC::Media::Kind::VIDEO)
			{
				// Proxy urn:3gpp:video-orientation.
				{
					extenValue = packet->GetExtension(this->rtpHeaderExtensionIds.videoOrientation, extenLen);

					if (extenValue)
					{
						std::memcpy(bufferPtr, extenValue, extenLen);

						extensions.emplace_back(
						  static_cast<uint8_t>(RTC::RtpHeaderExtensionUri::Type::VIDEO_ORIENTATION),
						  extenLen,
						  bufferPtr);

						bufferPtr += extenLen;
					}
				}

				// Proxy urn:ietf:params:rtp-hdrext:toffset.
				{
					extenValue = packet->GetExtension(this->rtpHeaderExtensionIds.toffset, extenLen);

					if (extenValue)
					{
						std::memcpy(bufferPtr, extenValue, extenLen);

						extensions.emplace_back(
						  static_cast<uint8_t>(RTC::RtpHeaderExtensionUri::Type::TOFFSET), extenLen, bufferPtr);

						bufferPtr += extenLen;
					}
				}

				// Add http://www.webrtc.org/experiments/rtp-hdrext/abs-send-time.
				{
					extenLen = 3u;

					auto now         = DepLibUV::GetTime();
					auto absSendTime = static_cast<uint32_t>(((now << 18) + 500) / 1000) & 0x00FFFFFF;

					Utils::Byte::Set3Bytes(bufferPtr, 0, absSendTime);

					extensions.emplace_back(
					  static_cast<uint8_t>(RTC::RtpHeaderExtensionUri::Type::ABS_SEND_TIME), extenLen, bufferPtr);

					bufferPtr += extenLen;
				}
			}

			// Set the new extensions into the packet using One-Byte format.
			packet->SetExtensions(1, extensions);

			// Assign mediasoup RTP header extension ids (just those that mediasoup may
			// be interested in after passing it to the Router).
			packet->SetAbsSendTimeExtensionId(
			  static_cast<uint8_t>(RTC::RtpHeaderExtensionUri::Type::ABS_SEND_TIME));
			packet->SetSsrcAudioLevelExtensionId(
			  static_cast<uint8_t>(RTC::RtpHeaderExtensionUri::Type::SSRC_AUDIO_LEVEL));
			packet->SetVideoOrientationExtensionId(
			  static_cast<uint8_t>(RTC::RtpHeaderExtensionUri::Type::VIDEO_ORIENTATION));
		}

		return true;
	}

	inline void Producer::PostProcessRtpPacket(RTC::RtpPacket* packet)
	{
		MS_TRACE();

		if (this->kind == RTC::Media::Kind::VIDEO)
		{
			bool camera;
			bool flip;
			uint16_t rotation;

			if (packet->ReadVideoOrientation(camera, flip, rotation))
			{
				// If video orientation was not yet detected or any value has changed,
				// emit event.
				if (
				  !this->videoOrientationDetected || camera != this->videoOrientation.camera ||
				  flip != this->videoOrientation.flip || rotation != this->videoOrientation.rotation)
				{
					this->videoOrientationDetected  = true;
					this->videoOrientation.camera   = camera;
					this->videoOrientation.flip     = flip;
					this->videoOrientation.rotation = rotation;

					json data = json::object();

					data["camera"]   = this->videoOrientation.camera;
					data["flip"]     = this->videoOrientation.flip;
					data["rotation"] = this->videoOrientation.rotation;

					Channel::Notifier::Emit(this->id, "videoorientationchange", data);
				}
			}
		}
	}

	inline void Producer::EmitScore() const
	{
		MS_TRACE();

		json data = json::array();

		// Iterate all streams and notify their scores.
		for (auto& kv : this->mapSsrcRtpStream)
		{
			auto* rtpStream = kv.second;

			data.emplace_back(json::value_t::object);

			auto& jsonEntry = data[data.size() - 1];

			jsonEntry["ssrc"] = rtpStream->GetSsrc();

			if (!rtpStream->GetRid().empty())
				jsonEntry["rid"] = rtpStream->GetRid();

			jsonEntry["score"] = rtpStream->GetScore();
		}

		Channel::Notifier::Emit(this->id, "score", data);
	}

	inline void Producer::OnRtpStreamScore(RTC::RtpStream* rtpStream, uint8_t score)
	{
		MS_TRACE();

		// Notify the listener.
		this->listener->OnProducerRtpStreamScore(this, rtpStream, score);

		// Emit the score event.
		EmitScore();
	}

	inline void Producer::OnRtpStreamSendRtcpPacket(
	  RTC::RtpStreamRecv* /*rtpStream*/, RTC::RTCP::Packet* packet)
	{
		// Notify the listener.
		this->listener->OnProducerSendRtcpPacket(this, packet);
	}

	inline void Producer::OnRtpStreamNeedWorstRemoteFractionLost(
	  RTC::RtpStreamRecv* rtpStream, uint8_t& worstRemoteFractionLost)
	{
		auto mappedSsrc = this->mapRtpStreamMappedSsrc.at(rtpStream);

		// Notify the listener.
		this->listener->OnProducerNeedWorstRemoteFractionLost(this, mappedSsrc, worstRemoteFractionLost);
	}

	inline void Producer::OnKeyFrameNeeded(
	  RTC::KeyFrameRequestManager* /*keyFrameRequestManager*/, uint32_t ssrc)
	{
		MS_TRACE();

		auto it = this->mapSsrcRtpStream.find(ssrc);

		if (it == this->mapSsrcRtpStream.end())
		{
			MS_WARN_2TAGS(rtcp, rtx, "no associated RtpStream found [ssrc:%" PRIu32 "]", ssrc);

			return;
		}

		auto* rtpStream = it->second;

		rtpStream->RequestKeyFrame();
	}
} // namespace RTC
