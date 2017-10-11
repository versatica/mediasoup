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
	static constexpr uint16_t RtpPacketsBeforeProbation{ 2000 };

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

		Json::Value json(Json::arrayValue);

		if (this->rtpStream != nullptr)
		{
			auto jsonRtpStream = this->rtpStream->GetStats();

			if (this->transport != nullptr)
			{
				jsonRtpStream[JsonStringTransportId] = this->transport->transportId;
			}

			json.append(jsonRtpStream);
		}

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
			this->rtpStream->ClearRetransmissionBuffer();
			this->rtpPacketsBeforeProbation = RtpPacketsBeforeProbation;

			if (this->isProbing)
			{
				this->isProbing      = false;
				this->probingProfile = RTC::RtpEncodingParameters::Profile::NONE;
			}
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
			this->rtpStream->ClearRetransmissionBuffer();
			this->rtpPacketsBeforeProbation = RtpPacketsBeforeProbation;

			if (this->isProbing)
			{
				this->isProbing      = false;
				this->probingProfile = RTC::RtpEncodingParameters::Profile::NONE;
			}
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

	void Consumer::SourceRtpParametersUpdated()
	{
		MS_TRACE();

		if (!IsEnabled())
			return;

		this->syncRequired = true;
	}

	void Consumer::AddProfile(
	  const RTC::RtpEncodingParameters::Profile profile, const RTC::RtpStream* rtpStream)
	{
		MS_ASSERT(profile != RTC::RtpEncodingParameters::Profile::NONE, "invalid profile");

		MS_ASSERT(this->profiles.find(profile) == this->profiles.end(), "profile already exists");

		MS_ASSERT(
		  !(profile == RTC::RtpEncodingParameters::Profile::DEFAULT && !this->profiles.empty()),
		  "default profile cannot coexist with others");

		// Insert the new profile.
		this->mapProfileRtpStream[profile] = rtpStream;
		this->profiles.insert(profile);

		MS_DEBUG_TAG(
		  rtp, "profile added [profile:%s]", RTC::RtpEncodingParameters::profile2String[profile].c_str());

		if (profile > this->targetProfile)
			RecalculateTargetProfile();
	}

	void Consumer::RemoveProfile(const RTC::RtpEncodingParameters::Profile profile)
	{
		MS_ASSERT(profile != RTC::RtpEncodingParameters::Profile::NONE, "invalid profile");

		MS_ASSERT(this->profiles.find(profile) != this->profiles.end(), "profile not found");

		MS_ASSERT(
		  !(profile == RTC::RtpEncodingParameters::Profile::DEFAULT && this->profiles.size() != 1),
		  "default profile cannot coexist with others");

		// Remove the profile.
		this->mapProfileRtpStream.erase(profile);
		this->profiles.erase(profile);

		MS_DEBUG_TAG(
		  rtp,
		  "profile removed [profile:%s]",
		  RTC::RtpEncodingParameters::profile2String[profile].c_str());

		if (this->profiles.empty())
		{
			SetEffectiveProfile(RtpEncodingParameters::Profile::NONE);
			RecalculateTargetProfile();
		}
		// If there is an ongoing probation for this profile, disable it.
		else if (this->isProbing && this->probingProfile == profile)
		{
			// Disable probation flag.
			this->isProbing      = false;
			this->probingProfile = RtpEncodingParameters::Profile::NONE;

			// Reset the health check timer so this probation doesn't affect the effective profile.
			this->rtpStream->ResetStatusCheckTimer();

			return;
		}
		// If it is the effective profile, try to downgrade, or upgrade it to the next higher profile if
		// the removed profile was lower than other existing ones.
		else if (this->effectiveProfile == profile)
		{
			SetEffectiveProfile(RtpEncodingParameters::Profile::NONE);

			auto it               = this->profiles.begin();
			auto newTargetProfile = *it;

			while (++it != this->profiles.end())
			{
				auto candidateTargetProfile = *it;

				if (candidateTargetProfile < profile)
					newTargetProfile = candidateTargetProfile;
				else
					break;
			}

			this->targetProfile = newTargetProfile;

			RequestKeyFrame();

			return;
		}
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

		RecalculateTargetProfile(true /*forcePreferred*/);
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

		RecalculateTargetProfile(true /*forcePreferred*/);
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
		if (this->isProbing)
		{
			this->isProbing      = false;
			this->probingProfile = RTC::RtpEncodingParameters::Profile::NONE;
		}
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

			this->syncRequired = false;

			if (this->encodingContext)
				this->encodingContext->SyncRequired();
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

		// Rewrite payload if needed.
		if (this->encodingContext)
			packet->EncodePayload(this->encodingContext.get());

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
			if (this->isProbing)
				RetransmitRtpPacket(packet);
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
				break;
		}

		RequestKeyFrame();
	}

	void Consumer::ReceiveRtcpReceiverReport(RTC::RTCP::ReceiverReport* report)
	{
		MS_TRACE();

		if (!IsEnabled())
			return;

		this->rtpStream->ReceiveRtcpReceiverReport(report);
	}

	float Consumer::GetLossPercentage() const
	{
		float lossPercentage = 0;

		if (this->effectiveProfile != RTC::RtpEncodingParameters::Profile::NONE)
		{
			auto it = this->mapProfileRtpStream.find(this->effectiveProfile);

			MS_ASSERT(
			  it != this->mapProfileRtpStream.end(), "no RtpStream associated with current profile");

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

	void Consumer::OnRtpStreamHealthy(RtpStream* rtpStream)
	{
		MS_TRACE();

		if (!this->isProbing)
			return;

		MS_DEBUG_TAG(
		  rtp,
		  "successful probation [ssrc:%" PRIu32 ", profile:%s]",
		  rtpStream->GetSsrc(),
		  RTC::RtpEncodingParameters::profile2String[this->probingProfile].c_str());

		// Promote probing profile.
		this->targetProfile = this->probingProfile;

		// Disable probation flag.
		this->isProbing      = false;
		this->probingProfile = RtpEncodingParameters::Profile::NONE;

		if (IsEnabled() && !IsPaused())
			RequestKeyFrame();

		MS_DEBUG_TAG(
		  rtp,
		  "target profile set [ssrc:%" PRIu32 ", profile:%s]",
		  rtpStream->GetSsrc(),
		  RTC::RtpEncodingParameters::profile2String[this->targetProfile].c_str());
	}

	void Consumer::OnRtpStreamUnhealthy(RtpStream* rtpStream)
	{
		MS_TRACE();

		// Probation failed.
		if (this->isProbing)
		{
			MS_DEBUG_TAG(
			  rtp,
			  "unsuccessful probation [ssrc:%" PRIu32 ", profile:%s]",
			  rtpStream->GetSsrc(),
			  RTC::RtpEncodingParameters::profile2String[this->probingProfile].c_str());

			// Disable probation flag.
			this->isProbing      = false;
			this->probingProfile = RtpEncodingParameters::Profile::NONE;

			return;
		}

		MS_DEBUG_TAG(
		  rtp,
		  "effective profile unhealthy [ssrc:%" PRIu32 ", profile:%s]",
		  rtpStream->GetSsrc(),
		  RTC::RtpEncodingParameters::profile2String[this->effectiveProfile].c_str());

		// No simulcast/SVC.
		if (this->effectiveProfile == RtpEncodingParameters::Profile::DEFAULT)
			return;

		auto it = this->profiles.find(this->effectiveProfile);

		// This is already the lowest profile.
		if (it == this->profiles.begin())
			return;

		// Downgrade the target profile.
		this->targetProfile = *(std::prev(it));

		if (IsEnabled() && !IsPaused())
			RequestKeyFrame();

		// We want to be notified about this new profile's health.
		this->rtpStream->ResetStatusCheckTimer();

		MS_DEBUG_TAG(
		  rtp,
		  "target profile set [ssrc:%" PRIu32 ", profile:%s]",
		  rtpStream->GetSsrc(),
		  RTC::RtpEncodingParameters::profile2String[this->targetProfile].c_str());
	}

	void Consumer::MayRunProbation()
	{
		// No simulcast or SVC.
		if (this->profiles.empty())
			return;

		// There is an ongoing profile upgrade. Do not interfere.
		if (this->effectiveProfile != this->targetProfile)
			return;

		// Ongoing probation.
		if (this->isProbing)
			return;

		// Current health status is not good.
		if (!this->rtpStream->IsHealthy())
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
			this->rtpStream = new RTC::RtpStreamSend(this, params, 1500);
		else
			this->rtpStream = new RTC::RtpStreamSend(this, params, 0);

		if (encoding.hasRtx && encoding.rtx.ssrc != 0u)
		{
			auto& codec = this->rtpParameters.GetRtxCodecForEncoding(encoding);

			this->rtpStream->SetRtx(codec.payloadType, encoding.rtx.ssrc);
		}

		this->encodingContext.reset(RTC::Codecs::GetEncodingContext(codec.mimeType));
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

	void Consumer::RecalculateTargetProfile(bool forcePreferred)
	{
		MS_TRACE();

		RTC::RtpEncodingParameters::Profile newTargetProfile;

		// If there are no profiles, select none or default, depending on whether this
		// is single stream or simulcast/SVC.
		if (this->profiles.empty())
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
		// If there is no preferred profile, try the next higher profile.
		else if (GetPreferredProfile() == RTC::RtpEncodingParameters::Profile::DEFAULT)
		{
			newTargetProfile = this->effectiveProfile;
			auto it          = this->profiles.upper_bound(this->effectiveProfile);

			if (it != this->profiles.end())
				newTargetProfile = *it;
		}
		// If the preferred profile is forced, try with the closest profile to it.
		else if (forcePreferred)
		{
			auto it          = this->profiles.crbegin();
			newTargetProfile = *it;

			for (; it != this->profiles.crend(); ++it)
			{
				auto profile = *it;

				if (profile <= GetPreferredProfile())
				{
					newTargetProfile = profile;
					break;
				}
			}
		}
		// Try with the next higher profile which is lower or equal to the preferred.
		else
		{
			newTargetProfile = this->effectiveProfile;
			auto it          = this->profiles.upper_bound(this->effectiveProfile);

			if (it != this->profiles.end() && *it <= GetPreferredProfile())
				newTargetProfile = *it;
		}

		// Not enabled. Make this the target profile.
		if (!IsEnabled())
		{
			this->targetProfile = newTargetProfile;
		}
		// Ongoing probation.
		else if (this->isProbing)
		{
			// New profile higher or equal than the one being probed. Do not upgrade.
			if (newTargetProfile >= this->probingProfile)
				return;

			// New profile lower than the one begin probed. Stop probation.
			this->isProbing      = false;
			this->probingProfile = RTC::RtpEncodingParameters::Profile::NONE;
			this->targetProfile  = newTargetProfile;
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
			// Probe it before promotion.
			// TODO: If we are receiving REMB, consider such value and avoid probation.
			else
			{
				this->isProbing      = true;
				this->probingProfile = newTargetProfile;
				this->rtpStream->ResetStatusCheckTimer();

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

		// Notify.
		eventData[JsonStringProfile] = RTC::RtpEncodingParameters::profile2String[this->effectiveProfile];
		this->notifier->Emit(this->consumerId, "effectiveprofilechange", eventData);
	}
} // namespace RTC
