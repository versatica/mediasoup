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

	static std::vector<RTC::RtpPacket*> RtpRetransmissionContainer(18);

	/* Instance methods. */

	Consumer::Consumer(
	  Channel::Notifier* notifier, uint32_t consumerId, RTC::Media::Kind kind, uint32_t sourceProducerId)
	  : consumerId(consumerId), kind(kind), sourceProducerId(sourceProducerId), notifier(notifier)
	{
		MS_TRACE();

		// Initialize sequence number.
		this->seqNum = static_cast<uint16_t>(Utils::Crypto::GetRandomUInt(0x00FF, 0xFFFF));

		// Initialize RTP timestamp.
		this->rtpTimestamp = Utils::Crypto::GetRandomUInt(0x00FF, 0xFFFF);

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

		Json::Value json(Json::objectValue);

		json[JsonStringConsumerId] = Json::UInt{ this->consumerId };

		json[JsonStringKind] = RTC::Media::GetJsonString(this->kind);

		json[JsonStringSourceProducerId] = Json::UInt{ this->sourceProducerId };

		if (this->transport != nullptr)
			json[JsonStringRtpParameters] = this->rtpParameters.ToJson();

		if (this->rtpStream != nullptr)
			json[JsonStringRtpStream] = this->rtpStream->ToJson();

		json[JsonStringPaused] = this->paused;

		json[JsonStringSourcePaused] = this->sourcePaused;

		json[JsonStringPreferredProfile] =
		  RTC::RtpEncodingParameters::profile2String[this->preferredProfile];

		json[JsonStringEffectiveProfile] =
		  RTC::RtpEncodingParameters::profile2String[this->effectiveProfile];

		return json;
	}

	void Consumer::HandleRequest(Channel::Request* request)
	{
		MS_TRACE();

		switch (request->methodId)
		{
			case Channel::Request::MethodId::CONSUMER_DUMP:
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
			this->rtpStream->ClearRetransmissionBuffer();
	}

	void Consumer::Resume()
	{
		MS_TRACE();

		if (!this->paused)
			return;

		this->paused = false;

		MS_DEBUG_DEV("Consumer resumed [consumerId:%" PRIu32 "]", this->consumerId);

		if (IsEnabled() && !this->sourcePaused)
			RequestKeyFrame();
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
			this->rtpStream->ClearRetransmissionBuffer();
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
			RequestKeyFrame();
	}

	void Consumer::SourceRtpParametersUpdated()
	{
		MS_TRACE();

		if (!IsEnabled())
			return;

		this->syncRequired = true;
	}

	void Consumer::AddProfile(const RTC::RtpEncodingParameters::Profile profile)
	{
		MS_ASSERT(
		  profile != RTC::RtpEncodingParameters::Profile::NONE &&
		    profile != RTC::RtpEncodingParameters::Profile::DEFAULT,
		  "invalid profile");

		MS_ASSERT(this->profiles.find(profile) == this->profiles.end(), "profile already exists");

		// Insert profile.
		this->profiles.insert(profile);

		MS_DEBUG_TAG(
		  rtp, "profile added [profile:%s]", RTC::RtpEncodingParameters::profile2String[profile].c_str());

		RecalculateTargetProfile();
	}

	void Consumer::RemoveProfile(const RTC::RtpEncodingParameters::Profile profile)
	{
		MS_ASSERT(
		  profile != RTC::RtpEncodingParameters::Profile::NONE &&
		    profile != RTC::RtpEncodingParameters::Profile::DEFAULT,
		  "invalid profile");

		MS_ASSERT(this->profiles.find(profile) != this->profiles.end(), "profile not found");

		// Remove profile.
		this->profiles.erase(profile);

		// If it was our effective profile, set it to none.
		if (this->effectiveProfile == profile)
			SetEffectiveProfile(RtpEncodingParameters::Profile::NONE);

		MS_DEBUG_TAG(
		  rtp,
		  "profile removed [profile:%s]",
		  RTC::RtpEncodingParameters::profile2String[profile].c_str());

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

		RecalculateTargetProfile();
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

		// Reset RTCP and RTP counter stuff.
		this->lastRtcpSentTime = 0;
		this->transmittedCounter.Reset();
		this->retransmittedCounter.Reset();
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

		// Whether we want to re-transmit the packet.
		bool retransmissionNeeded = false;

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
				  "key frame received [profile:%s]",
				  RTC::RtpEncodingParameters::profile2String[profile].c_str());
			}

			if (isKeyFrame || !canBeKeyFrame)
			{
				SetEffectiveProfile(this->targetProfile);

				// Send this packet twice.
				retransmissionNeeded = true;

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

			this->seqNumPreviousBase = this->seqNum;
			this->seqNumBase         = packet->GetSequenceNumber();

			this->rtpTimestampPreviousBase = this->rtpTimestamp;
			this->rtpTimestampBase         = packet->GetTimestamp();

			this->syncRequired = false;

			MS_DEBUG_TAG(rtp, "re-syncing Consumer stream [seqNum:%" PRIu16 "]", this->seqNum);
		}
		else
		{
			MS_ASSERT(this->lastReceivedSsrc == packet->GetSsrc(), "new SSRC requires re-syncing");
		}

		this->rtpTimestamp =
		  (packet->GetTimestamp() - this->rtpTimestampBase) + this->rtpTimestampPreviousBase + 1;

		this->seqNum = (packet->GetSequenceNumber() - this->seqNumBase) + this->seqNumPreviousBase + 1;

		// Save the received SSRC.
		this->lastReceivedSsrc = packet->GetSsrc();

		// Save the received sequence number.
		auto seqNum = packet->GetSequenceNumber();

		// Save the received timestamp.
		auto rtpTimestamp = packet->GetTimestamp();

		// Rewrite packet SSRC.
		packet->SetSsrc(this->rtpParameters.encodings[0].ssrc);

		// Rewrite packet sequence number.
		packet->SetSequenceNumber(this->seqNum);

		// Rewrite packet timestamp.
		packet->SetTimestamp(this->rtpTimestamp);

		if (isSyncPacket)
		{
			MS_DEBUG_TAG(
			  rtp,
			  "sending sync packet [ssrc:%" PRIu32 ", seq:%" PRIu16 ", ts:%" PRIu32
			  "], from original [ssrc:%" PRIu32 ", seq:%" PRIu16 ", ts:%" PRIu32 ", profile:%s]",
			  packet->GetSsrc(),
			  packet->GetSequenceNumber(),
			  packet->GetTimestamp(),
			  this->lastReceivedSsrc,
			  seqNum,
			  rtpTimestamp,
			  RTC::RtpEncodingParameters::profile2String[profile].c_str());
		}

		// Process the packet.
		if (this->rtpStream->ReceivePacket(packet))
		{
			// Send the packet.
			this->transport->SendRtpPacket(packet);

			// Update transmitted RTP data counter.
			this->transmittedCounter.Update(packet);

			if (retransmissionNeeded)
			{
				// Send the packet.
				this->transport->SendRtpPacket(packet);

				// Update transmitted RTP data counter.
				this->transmittedCounter.Update(packet);
			}
		}
		else
		{
			MS_DEBUG_TAG(
			  rtp,
			  "failed to send packet [ssrc:%" PRIu32 ", seq:%" PRIu16 ", ts:%" PRIu32
			  "], from original [ssrc:%" PRIu32 ", seq:%" PRIu16 ", ts:%" PRIu32 ", profile:%s]",
			  packet->GetSsrc(),
			  packet->GetSequenceNumber(),
			  packet->GetTimestamp(),
			  this->lastReceivedSsrc,
			  seqNum,
			  rtpTimestamp,
			  RTC::RtpEncodingParameters::profile2String[profile].c_str());
		}

		// Restore packet SSRC.
		packet->SetSsrc(this->lastReceivedSsrc);

		// Restore the original sequence number.
		packet->SetSequenceNumber(seqNum);

		// Restore the original timestamp.
		packet->SetTimestamp(rtpTimestamp);
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

	void Consumer::ReceiveRtcpReceiverReport(RTC::RTCP::ReceiverReport* report)
	{
		MS_TRACE();

		if (!IsEnabled())
			return;

		this->rtpStream->ReceiveRtcpReceiverReport(report);
	}

	void Consumer::RequestKeyFrame()
	{
		MS_TRACE();

		if (!IsEnabled())
			return;

		if (this->kind == RTC::Media::Kind::AUDIO || IsPaused())
			return;

		for (auto& listener : this->listeners)
		{
			listener->OnConsumerKeyFrameRequired(this);
		}
	}

	void Consumer::OnRtpStreamHealthReport(RtpStream* stream, bool healthy)
	{
		MS_TRACE();

		if (!IsEnabled())
			return;
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
		params.mime        = codec.mime;
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
	}

	void Consumer::RetransmitRtpPacket(RTC::RtpPacket* packet)
	{
		MS_TRACE();

		RTC::RtpPacket* rtxPacket{ nullptr };

		if (this->rtpStream->HasRtx())
		{
			static uint8_t rtxBuffer[MtuSize];

			rtxPacket = packet->Clone(rtxBuffer);
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

	void Consumer::RecalculateTargetProfile()
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
		// If there is no preferred profile, take the best one available.
		else if (this->preferredProfile == RTC::RtpEncodingParameters::Profile::DEFAULT)
		{
			auto it          = this->profiles.crbegin();
			newTargetProfile = *it;
		}
		// Otherwise take the highest available profile equal or lower than the preferred.
		else
		{
			std::set<RtpEncodingParameters::Profile>::reverse_iterator it;

			newTargetProfile = RtpEncodingParameters::Profile::NONE;

			for (it = this->profiles.rbegin(); it != this->profiles.rend(); ++it)
			{
				auto profile = *it;

				if (profile <= this->preferredProfile)
				{
					newTargetProfile = *it;
					break;
				}
			}
		}

		this->targetProfile = newTargetProfile;

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
