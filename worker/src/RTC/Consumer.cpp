#define MS_CLASS "RTC::Consumer"
// #define MS_LOG_DEV

#include "RTC/Consumer.hpp"
#include "Logger.hpp"
#include "MediaSoupError.hpp"
#include "Utils.hpp"
#include "RTC/Codecs/Codecs.hpp"
#include "RTC/RTCP/FeedbackRtpNack.hpp"
#include "RTC/RTCP/SenderReport.hpp"
#include <vector>

namespace RTC
{
	/* Static. */

	static uint8_t RtxPacketBuffer[RtpBufferSize];

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

		delete this->rtpStream;
		delete this->rtpMonitor;

		for (auto& listener : this->listeners)
		{
			listener->OnConsumerClosed(this);
		}

		this->notifier->Emit(this->consumerId, "close");
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
		static const Json::StaticString JsonStringPreferredProfile{ "preferredProfile" };
		static const Json::StaticString JsonStringEffectiveProfile{ "effectiveProfile" };
		static const Json::StaticString JsonStringLossPercentage{ "lossPercentage" };

		Json::Value json(Json::objectValue);

		json[JsonStringConsumerId] = Json::UInt{ this->consumerId };

		json[JsonStringKind] = RTC::Media::GetJsonString(this->kind);

		json[JsonStringSourceProducerId] = Json::UInt{ this->sourceProducerId };

		if (this->transport != nullptr)
			json[JsonStringRtpParameters] = this->rtpParameters.ToJson();

		if (this->rtpStream != nullptr)
		{
			json[JsonStringRtpStream]      = this->rtpStream->ToJson();
			json[JsonStringLossPercentage] = GetLossPercentage();
		}

		json[JsonStringPaused] = this->paused;

		json[JsonStringSourcePaused] = this->sourcePaused;

		json[JsonStringPreferredProfile] =
		  RTC::RtpEncodingParameters::profile2String[GetPreferredProfile()];

		json[JsonStringEffectiveProfile] =
		  RTC::RtpEncodingParameters::profile2String[this->effectiveProfile];

		return json;
	}

	Json::Value Consumer::GetStats() const
	{
		MS_TRACE();

		static const Json::StaticString JsonStringTransportId{ "transportId" };
		static const Json::StaticString JsonStringInboundRtpId{ "inboundRtpId" };

		Json::Value json(Json::arrayValue);

		if (this->rtpStream == nullptr)
			return json;

		auto jsonRtpStream = this->rtpStream->GetStats();

		if (this->transport != nullptr)
		{
			jsonRtpStream[JsonStringTransportId] = this->transport->transportId;
		}

		if (this->effectiveProfile == RTC::RtpEncodingParameters::Profile::NONE)
		{
			json.append(jsonRtpStream);

			return json;
		}

		auto it = this->mapProfileRtpStream.find(this->effectiveProfile);

		MS_ASSERT(it != this->mapProfileRtpStream.end(), "effective profile does not map to a rtp stream");

		auto inboundRtpStream     = const_cast<RTC::RtpStream*>(it->second);
		auto jsonInboundRtpStream = inboundRtpStream->GetStats();

		jsonRtpStream[JsonStringInboundRtpId] = inboundRtpStream->GetId();
		json.append(jsonInboundRtpStream);
		json.append(jsonRtpStream);

		return json;
	}

	/**
	 * A Transport has been assigned, and hence sending RTP parameters.
	 */
	void Consumer::Enable(RTC::Transport* transport, RTC::RtpParameters& rtpParameters)
	{
		MS_TRACE();

		// Must have a single encoding.
		if (rtpParameters.encodings.empty())
			MS_THROW_ERROR("invalid empty rtpParameters.encodings");
		else if (rtpParameters.encodings[0].ssrc == 0)
			MS_THROW_ERROR("missing rtpParameters.encodings[0].ssrc");

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
			this->rtpMonitor->Reset();
			this->rtpStream->ClearRetransmissionBuffer();
			this->rtpPacketsBeforeProbation = RtpPacketsBeforeProbation;

			if (IsProbing())
				StopProbation();
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
			// We need to sync and wait for a key frame. Otherwise the receiver will
			// request lot of NACKs due to unknown RTP packets.
			this->syncRequired = true;

			RequestKeyFrame();
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
			this->rtpMonitor->Reset();
			this->rtpStream->ClearRetransmissionBuffer();
			this->rtpPacketsBeforeProbation = RtpPacketsBeforeProbation;

			if (IsProbing())
				StopProbation();
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
			// We need to sync. However we don't need to request a key frame since the source
			// (Producer) already requested it.
			this->syncRequired = true;
		}
	}

	void Consumer::AddProfile(
	  const RTC::RtpEncodingParameters::Profile profile, const RTC::RtpStream* rtpStream)
	{
		MS_ASSERT(profile != RTC::RtpEncodingParameters::Profile::NONE, "invalid profile");

		MS_ASSERT(
		  this->mapProfileRtpStream.find(profile) == this->mapProfileRtpStream.end(),
		  "profile already exists");

		MS_ASSERT(
		  !(profile == RTC::RtpEncodingParameters::Profile::DEFAULT && !this->mapProfileRtpStream.empty()),
		  "default profile cannot coexist with others");

		// Insert the new profile.
		this->mapProfileRtpStream[profile] = rtpStream;

		MS_DEBUG_TAG(
		  rtp, "profile added [profile:%s]", RTC::RtpEncodingParameters::profile2String[profile].c_str());

		RecalculateTargetProfile();
	}

	void Consumer::RemoveProfile(const RTC::RtpEncodingParameters::Profile profile)
	{
		MS_ASSERT(profile != RTC::RtpEncodingParameters::Profile::NONE, "invalid profile");

		MS_ASSERT(
		  this->mapProfileRtpStream.find(profile) != this->mapProfileRtpStream.end(),
		  "profile not found");

		MS_ASSERT(
		  !(profile == RTC::RtpEncodingParameters::Profile::DEFAULT &&
		    this->mapProfileRtpStream.size() != 1),
		  "default profile cannot coexist with others");

		// Remove the profile.
		this->mapProfileRtpStream.erase(profile);

		MS_DEBUG_TAG(
		  rtp,
		  "profile removed [profile:%s]",
		  RTC::RtpEncodingParameters::profile2String[profile].c_str());

		// There is an ongoing probation for this profile, disable it.
		if (IsProbing() && this->probingProfile == profile)
		{
			// Stop probation.
			StopProbation();

			// Reset the RTP monitor so this probation doesn't affect the current profile.
			this->rtpMonitor->Reset();
		}
		else if (this->effectiveProfile == profile)
		{
			SetEffectiveProfile(RtpEncodingParameters::Profile::NONE);
		}
		else if (this->targetProfile == profile)
		{
			this->targetProfile = this->effectiveProfile;
		}

		RecalculateTargetProfile();
	}

	void Consumer::SetPreferredProfile(const RTC::RtpEncodingParameters::Profile profile)
	{
		MS_TRACE();

		if (this->preferredProfile == profile)
			return;

		this->preferredProfile = profile;

		MS_DEBUG_TAG(
		  rtp,
		  "preferred profile set [profile:%s]",
		  RTC::RtpEncodingParameters::profile2String[profile].c_str());

		RecalculateTargetProfile(true /*force*/);
	}

	void Consumer::SetSourcePreferredProfile(const RTC::RtpEncodingParameters::Profile profile)
	{
		MS_TRACE();

		if (this->sourcePreferredProfile == profile)
			return;

		this->sourcePreferredProfile = profile;

		MS_DEBUG_TAG(
		  rtp,
		  "source preferred profile set [profile:%s]",
		  RTC::RtpEncodingParameters::profile2String[profile].c_str());

		RecalculateTargetProfile();
	}

	void Consumer::SetEncodingPreferences(const RTC::Codecs::EncodingContext::Preferences preferences)
	{
		MS_TRACE();

		if (!this->encodingContext)
			return;

		MS_DEBUG_TAG(
		  rtp,
		  "setting encoding preferences [qualityLayer:%" PRIu8 ", spatialLayer:%" PRIu8
		  ", temporalLayer:%" PRIu8 "]",
		  preferences.qualityLayer,
		  preferences.spatialLayer,
		  preferences.temporalLayer);

		this->encodingContext->SetPreferences(preferences);
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

		// Reset last RTCP sent time counter.
		this->lastRtcpSentTime = 0;

		// Reset probation.
		if (IsProbing())
			StopProbation();
	}

	void Consumer::SendRtpPacket(RTC::RtpPacket* packet, RTC::RtpEncodingParameters::Profile profile)
	{
		MS_TRACE();

		if (!IsEnabled())
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

		// Check whether this is the key frame we are waiting for in order to update the effective
		// profile.
		if (this->effectiveProfile != this->targetProfile && profile == this->targetProfile)
		{
			bool isKeyFrame    = false;
			bool canBeKeyFrame = Codecs::CanBeKeyFrame(this->rtpStream->GetMimeType());

			if (canBeKeyFrame && packet->IsKeyFrame())
			{
				isKeyFrame = true;

				MS_DEBUG_TAG(
				  rtp,
				  "key frame received for target profile [profile:%s]",
				  RTC::RtpEncodingParameters::profile2String[profile].c_str());
			}

			if (isKeyFrame || !canBeKeyFrame)
			{
				SetEffectiveProfile(this->targetProfile);

				// Resynchronize the stream.
				this->syncRequired = true;

				// Clear RTP retransmission buffer to avoid congesting the receiver by
				// sending useless retransmissions (now that we are sending a newer key
				// frame).
				this->rtpStream->ClearRetransmissionBuffer();

				// Stop probation if probing profile is the new effective profile.
				if (IsProbing() && this->probingProfile == this->effectiveProfile)
					StopProbation();
			}
		}

		// If the packet belongs to different profile than the one being sent, drop it.
		// NOTE: This is specific to simulcast with no temporal layers.
		if (profile != this->effectiveProfile)
			return;

		// Whether this is the first packet after re-sync.
		bool isSyncPacket = false;

		// Check whether sequence number and timestamp sync is required.
		if (this->syncRequired)
		{
			isSyncPacket = true;

			this->rtpSeqManager.Sync(packet->GetSequenceNumber());
			this->rtpTimestampManager.Sync(packet->GetTimestamp());

			// Calculate RTP timestamp diff between now and last sent RTP packet.
			if (this->rtpStream->GetMaxPacketMs() != 0u)
			{
				auto now    = DepLibUV::GetTime();
				auto diffMs = now - this->rtpStream->GetMaxPacketMs();
				auto diffTs = diffMs * this->rtpStream->GetClockRate() / 1000;

				this->rtpTimestampManager.Offset(diffTs);
			}

			this->syncRequired = false;

			if (this->encodingContext)
				this->encodingContext->SyncRequired();
		}

		// Rewrite payload if needed. Drop packet if necessary.
		if (this->encodingContext && !packet->EncodePayload(this->encodingContext.get()))
		{
			this->rtpSeqManager.Drop(packet->GetSequenceNumber());
			this->rtpTimestampManager.Drop(packet->GetTimestamp());

			return;
		}

		// Update RTP seq number and timestamp.
		uint16_t rtpSeq;
		uint32_t rtpTimestamp;

		this->rtpSeqManager.Input(packet->GetSequenceNumber(), rtpSeq);
		this->rtpTimestampManager.Input(packet->GetTimestamp(), rtpTimestamp);

		// Save the received SSRC.
		auto origSsrc = packet->GetSsrc();

		// Save the received sequence number.
		auto origSeq = packet->GetSequenceNumber();

		// Save the received timestamp.
		auto origTimestamp = packet->GetTimestamp();

		// Rewrite packet SSRC.
		packet->SetSsrc(this->rtpParameters.encodings[0].ssrc);

		// Rewrite packet sequence number.
		packet->SetSequenceNumber(rtpSeq);

		// Rewrite packet timestamp.
		packet->SetTimestamp(rtpTimestamp);

		if (isSyncPacket)
		{
			MS_DEBUG_TAG(
			  rtp,
			  "sending sync packet [ssrc:%" PRIu32 ", seq:%" PRIu16 ", ts:%" PRIu32
			  ", profile:%s] from original [seq:%" PRIu16 ", ts:%" PRIu32 "]",
			  packet->GetSsrc(),
			  packet->GetSequenceNumber(),
			  packet->GetTimestamp(),
			  RTC::RtpEncodingParameters::profile2String[profile].c_str(),
			  origSeq,
			  origTimestamp);
		}

		// Process the packet.
		if (this->rtpStream->ReceivePacket(packet))
		{
			// Send the packet.
			this->transport->SendRtpPacket(packet);

			// Retransmit the RTP packet if probing.
			if (IsProbing())
				SendProbation(packet);
		}
		else
		{
			MS_WARN_TAG(
			  rtp,
			  "failed to send packet [ssrc:%" PRIu32 ", seq:%" PRIu16 ", ts:%" PRIu32
			  ", profile:%s] from original [seq:%" PRIu16 ", ts:%" PRIu32 "]",
			  packet->GetSsrc(),
			  packet->GetSequenceNumber(),
			  packet->GetTimestamp(),
			  RTC::RtpEncodingParameters::profile2String[profile].c_str(),
			  origSeq,
			  origTimestamp);
		}

		// Restore packet SSRC.
		packet->SetSsrc(origSsrc);

		// Restore the original sequence number.
		packet->SetSequenceNumber(origSeq);

		// Restore the original timestamp.
		packet->SetTimestamp(origTimestamp);

		// Restore the original payload if needed.
		if (this->encodingContext)
			packet->RestorePayload();

		// Run probation if needed.
		if (this->kind == RTC::Media::Kind::VIDEO && --this->rtpPacketsBeforeProbation == 0)
		{
			this->rtpPacketsBeforeProbation = RtpPacketsBeforeProbation;
			MayRunProbation();
		}
	}

	void Consumer::GetRtcp(RTC::RTCP::CompoundPacket* packet, uint64_t now)
	{
		MS_TRACE();

		if (!IsEnabled() || IsPaused() || this->effectiveProfile == RTC::RtpEncodingParameters::Profile::NONE)
			return;

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

		this->rtpStream->nackCount++;

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

				this->rtpMonitor->RtpPacketRepaired(packet);

				// Packet repaired after applying RTX.
				this->rtpStream->packetsRepaired++;
			}
		}
	}

	void Consumer::ReceiveKeyFrameRequest(RTCP::FeedbackPs::MessageType messageType)
	{
		MS_TRACE();

		if (!IsEnabled())
			return;

		switch (messageType)
		{
			case RTCP::FeedbackPs::MessageType::PLI:
				this->rtpStream->pliCount++;
				break;

			case RTCP::FeedbackPs::MessageType::FIR:
				this->rtpStream->firCount++;
				break;

			default:
				MS_ASSERT(false, "invalid messageType");
		}

		RequestKeyFrame();
	}

	void Consumer::ReceiveRtcpReceiverReport(RTC::RTCP::ReceiverReport* report)
	{
		MS_TRACE();

		if (!IsEnabled())
			return;

		if (IsPaused())
			return;

		// Ignore reports that do not refer to the main RTP stream. Ie: RTX stream.
		if (report->GetSsrc() != this->rtpStream->GetSsrc())
			return;

		this->rtpStream->ReceiveRtcpReceiverReport(report);

		if (this->kind == RTC::Media::Kind::VIDEO)
			this->rtpMonitor->ReceiveRtcpReceiverReport(report);
	}

	float Consumer::GetLossPercentage() const
	{
		float lossPercentage = 0;

		if (this->effectiveProfile != RTC::RtpEncodingParameters::Profile::NONE)
		{
			auto it = this->mapProfileRtpStream.find(this->effectiveProfile);

			MS_ASSERT(it != this->mapProfileRtpStream.end(), "no RtpStream associated with current profile");

			auto rtpStream = it->second;

			if (rtpStream->GetLossPercentage() >= this->rtpStream->GetLossPercentage())
				lossPercentage = 0;
			else
				lossPercentage = (this->rtpStream->GetLossPercentage() - rtpStream->GetLossPercentage());
		}

		return lossPercentage;
	}

	void Consumer::RequestKeyFrame()
	{
		MS_TRACE();

		if (!IsEnabled())
			return;

		if (this->kind != RTC::Media::Kind::VIDEO || IsPaused())
			return;

		for (auto& listener : this->listeners)
		{
			listener->OnConsumerKeyFrameRequired(this);
		}
	}

	void Consumer::OnRtpMonitorScore(uint8_t /*score*/)
	{
		MS_TRACE();

		RecalculateTargetProfile();
	}

	void Consumer::MayRunProbation()
	{
		// No simulcast or SVC.
		if (this->mapProfileRtpStream.empty())
			return;

		// There is an ongoing profile upgrade. Do not interfere.
		if (this->effectiveProfile != this->targetProfile)
			return;

		// Ongoing probation.
		if (IsProbing())
			return;

		// Current health status is not good.
		if (!this->rtpMonitor->IsHealthy())
			return;

		RecalculateTargetProfile();
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
		params.mimeType    = codec.mimeType;
		params.clockRate   = codec.clockRate;
		params.useNack     = useNack;
		params.usePli      = usePli;

		// Create a RtpStreamSend for sending a single media stream.
		if (useNack)
			this->rtpStream = new RTC::RtpStreamSend(params, 1500);
		else
			this->rtpStream = new RTC::RtpStreamSend(params, 0);

		if (encoding.hasRtx && encoding.rtx.ssrc != 0u)
		{
			auto& codec = this->rtpParameters.GetRtxCodecForEncoding(encoding);

			this->rtpStream->SetRtx(codec.payloadType, encoding.rtx.ssrc);
		}

		this->encodingContext.reset(RTC::Codecs::GetEncodingContext(codec.mimeType));

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
		this->transport->SendRtpPacket(rtxPacket);

		// Delete the RTX RtpPacket if it was created.
		if (rtxPacket != packet)
			delete rtxPacket;
	}

	void Consumer::RecalculateTargetProfile(bool force)
	{
		MS_TRACE();

		RTC::RtpEncodingParameters::Profile newTargetProfile;
		auto probatedProfile = RTC::RtpEncodingParameters::Profile::NONE;

		// Probation finished.
		if (!IsProbing() && this->probingProfile != RTC::RtpEncodingParameters::Profile::NONE)
		{
			probatedProfile      = this->probingProfile;
			this->probingProfile = RTC::RtpEncodingParameters::Profile::NONE;
		}

		// There are no profiles, select none or default, depending on whether this
		// is single stream or simulcast/SVC.
		if (this->mapProfileRtpStream.empty())
		{
			MS_ASSERT(
			  this->effectiveProfile == RtpEncodingParameters::Profile::NONE ||
			    this->effectiveProfile == RtpEncodingParameters::Profile::DEFAULT,
			  "no profiles, but effective profile is not none nor default");

			if (this->effectiveProfile == RtpEncodingParameters::Profile::NONE)
				newTargetProfile = RtpEncodingParameters::Profile::NONE;
			else
				newTargetProfile = RtpEncodingParameters::Profile::DEFAULT;
		}
		// RTP state is unhealty.
		else if (IsEnabled() && !this->rtpMonitor->IsHealthy())
		{
			// Ongoing probation, abort.
			if (IsProbing())
			{
				StopProbation();

				return;
			}

			// Probated profile did not succeed.
			if (probatedProfile != RtpEncodingParameters::Profile::NONE)
				return;

			auto it = this->mapProfileRtpStream.find(this->effectiveProfile);

			// This is already the lowest profile.
			if (it == this->mapProfileRtpStream.begin())
				return;

			// Downgrade the target profile.
			newTargetProfile = (std::prev(it))->first;
		}
		// If there is no preferred profile, get the highest one.
		else if (GetPreferredProfile() == RTC::RtpEncodingParameters::Profile::DEFAULT)
		{
			auto it = this->mapProfileRtpStream.crbegin();

			newTargetProfile = it->first;
		}
		// Try with the closest profile to the preferred one.
		else
		{
			auto it = this->mapProfileRtpStream.lower_bound(this->preferredProfile);

			// Preferred profile is actually present.
			if (it->first == this->preferredProfile)
			{
				newTargetProfile = it->first;
			}
			// The lowest profile is already higher than the preferred, use it.
			else if (it == this->mapProfileRtpStream.begin())
			{
				newTargetProfile = it->first;
			}
			// There is a lower profile available, prefer it over any higher one.
			else
			{
				newTargetProfile = (--it)->first;
			}
		}

		// Not enabled. Make this the target profile.
		if (!IsEnabled())
		{
			this->targetProfile = newTargetProfile;
		}
		// Ongoing probation.
		else if (IsProbing())
		{
			// New profile higher or equal than the one being probed. Do not upgrade.
			if (newTargetProfile >= this->probingProfile)
				return;

			// New profile lower than the one begin probed. Stop probation.
			StopProbation();
			this->targetProfile = newTargetProfile;
		}
		// The profile has just been successfully probated.
		else if (probatedProfile == newTargetProfile)
		{
			this->targetProfile = newTargetProfile;
		}
		// New profile is higher than current target.
		else if (newTargetProfile > this->targetProfile)
		{
			// No specific profile is being sent.
			if (
			  this->targetProfile == RTC::RtpEncodingParameters::Profile::NONE ||
			  this->targetProfile == RTC::RtpEncodingParameters::Profile::DEFAULT)
			{
				this->targetProfile = newTargetProfile;
			}
			else if (force)
			{
				this->targetProfile = newTargetProfile;
			}
			// Probe it before promotion.
			// TODO: If we are receiving REMB, consider such value and avoid probation.
			else
			{
				StartProbation(newTargetProfile);

				MS_DEBUG_TAG(
				  rtp,
				  "probing profile [%s]",
				  RTC::RtpEncodingParameters::profile2String[this->probingProfile].c_str());

				return;
			}
		}
		// New profile is lower than the target profile.
		else
		{
			this->targetProfile = newTargetProfile;
		}

		if (this->targetProfile == this->effectiveProfile)
			return;

		if (IsEnabled() && !IsPaused())
			RequestKeyFrame();

		MS_DEBUG_TAG(
		  rtp,
		  "target profile set [profile:%s]",
		  RTC::RtpEncodingParameters::profile2String[this->targetProfile].c_str());
	}

	void Consumer::SetEffectiveProfile(RTC::RtpEncodingParameters::Profile profile)
	{
		MS_TRACE();

		static const Json::StaticString JsonStringProfile{ "profile" };

		Json::Value eventData(Json::objectValue);

		this->effectiveProfile = profile;

		MS_DEBUG_TAG(
		  rtp,
		  "effective profile set [profile:%s]",
		  RTC::RtpEncodingParameters::profile2String[this->effectiveProfile].c_str());

		if (!IsEnabled())
			return;

		this->rtpMonitor->Reset();

		// Notify.
		eventData[JsonStringProfile] = RTC::RtpEncodingParameters::profile2String[this->effectiveProfile];

		this->notifier->Emit(this->consumerId, "effectiveprofilechange", eventData);
	}

	void Consumer::SendProbation(RTC::RtpPacket* packet)
	{
		if (!IsProbing())
			return;

		RetransmitRtpPacket(packet);

		switch (this->effectiveProfile)
		{
			case RTC::RtpEncodingParameters::Profile::HIGH:
				this->probationPackets--;
				break;

			case RTC::RtpEncodingParameters::Profile::MEDIUM:
				this->probationPackets -= 2;
				break;

			case RTC::RtpEncodingParameters::Profile::LOW:
				this->probationPackets -= 4;
				break;

			default:
				MS_ABORT("invalid profile");
		}

		if (this->probationPackets == 0)
			RecalculateTargetProfile();
	}
} // namespace RTC
