#define MS_CLASS "RTC::SimulcastConsumer"
// #define MS_LOG_DEV

#include "RTC/SimulcastConsumer.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "Channel/Notifier.hpp"
#include "RTC/Codecs/Codecs.hpp"

namespace RTC
{
	/* Static. */

	static constexpr uint16_t PacketsBeforeProbation{ 2000 };
	// Must be a power of 2.
	static constexpr uint16_t ProbationPacketNumber{ 256 };

	/* Instance methods. */

	SimulcastConsumer::SimulcastConsumer(const std::string& id, Listener* listener, json& data)
	  : RTC::Consumer::Consumer(id, listener, data, RTC::RtpParameters::Type::SIMULCAST),
	    packetsBeforeProbation(PacketsBeforeProbation)
	{
		MS_TRACE();

		// Ensure there are N > 1 encodings.
		if (this->consumableRtpEncodings.size() <= 1)
			MS_THROW_TYPE_ERROR("invalid consumableRtpEncodings with size <= 1");

		// Set the RTCP report generation interval.
		if (this->kind == RTC::Media::Kind::AUDIO)
			this->maxRtcpInterval = RTC::RTCP::MaxAudioIntervalMs;
		else
			this->maxRtcpInterval = RTC::RTCP::MaxVideoIntervalMs;

		// Initially set preferreSpatialLayer to the maximum value.
		this->preferredSpatialLayer = static_cast<int16_t>(this->consumableRtpEncodings.size()) - 1;

		// Create RtpStreamSend instance for sending a single stream to the remote.
		CreateRtpStream();

		// May need a key frame.
		RequestKeyFrame();
	}

	SimulcastConsumer::~SimulcastConsumer()
	{
		MS_TRACE();

		delete this->rtpStream;
	}

	void SimulcastConsumer::FillJson(json& jsonObject) const
	{
		MS_TRACE();

		// Call the parent method.
		RTC::Consumer::FillJson(jsonObject);

		// Add rtpStream.
		this->rtpStream->FillJson(jsonObject["rtpStream"]);

		// TODO: Add layers, etc.
	}

	void SimulcastConsumer::FillJsonStats(json& jsonArray) const
	{
		MS_TRACE();

		// Add stats of our send stream.
		jsonArray.emplace_back(json::value_t::object);
		this->rtpStream->FillJsonStats(jsonArray[0]);

		// Add stats of our recv stream.
		if (this->producerRtpStream)
		{
			jsonArray.emplace_back(json::value_t::object);
			this->producerRtpStream->FillJsonStats(jsonArray[1]);
		}
	}

	void SimulcastConsumer::HandleRequest(Channel::Request* request)
	{
		MS_TRACE();

		switch (request->methodId)
		{
			case Channel::Request::MethodId::CONSUMER_REQUEST_KEY_FRAME:
			{
				RequestKeyFrame();

				request->Accept();

				break;
			}

			case Channel::Request::MethodId::CONSUMER_SET_PREFERRED_LAYERS:
			{
				// TODO

				request->Accept();

				break;
			}

			default:
			{
				// Pass it to the parent class.
				RTC::Consumer::HandleRequest(request);
			}
		}
	}

	void SimulcastConsumer::TransportConnected()
	{
		MS_TRACE();

		RequestKeyFrame();
	}

	void SimulcastConsumer::ProducerNewRtpStream(RTC::RtpStream* rtpStream, uint32_t mappedSsrc)
	{
		MS_TRACE();

		auto it = this->mapMappedSsrcSpatialLayer.find(mappedSsrc);

		MS_ASSERT(it != this->mapMappedSsrcSpatialLayer.end(), "unknown mappedSsrc");

		int16_t spatialLayer = it->second;

		this->producerRtpStreams[spatialLayer] = rtpStream;

		// TODO: Recalculate layers.

		// TODO: Remove.
		this->producerRtpStream = rtpStream;

		// Emit the score event.
		EmitScore();
	}

	void SimulcastConsumer::ProducerRtpStreamScore(RTC::RtpStream* rtpStream, uint8_t score)
	{
		MS_TRACE();

		// Emit the score event.
		EmitScore();

		// TODO: Recalculate layers.
	}

	void SimulcastConsumer::SendRtpPacket(RTC::RtpPacket* packet)
	{
		MS_TRACE();

		if (!IsActive())
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

		bool isKeyFrame = false;

		// Just check if the packet contains a key frame when we need to sync.
		if (this->syncRequired && packet->IsKeyFrame())
			isKeyFrame = true;

		// If we are waiting for a key frame and this is not one, ignore the packet.
		if (this->syncRequired && this->keyFrameSupported && !isKeyFrame)
			return;

		// Whether this is the first packet after re-sync.
		bool isSyncPacket = this->syncRequired;

		// Sync sequence number and timestamp if required.
		if (isSyncPacket)
		{
			if (isKeyFrame)
				MS_DEBUG_TAG(rtp, "awaited key frame received");

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

			if (this->encodingContext)
				this->encodingContext->SyncRequired();

			this->syncRequired = false;
		}

		// Rewrite payload if needed. Drop packet if necessary.
		if (this->encodingContext && !packet->EncodePayload(this->encodingContext.get()))
		{
			this->rtpSeqManager.Drop(packet->GetSequenceNumber());
			this->rtpTimestampManager.Drop(packet->GetTimestamp());

			return;
		}

		// Update RTP seq number and timestamp.
		uint16_t seq;
		uint32_t timestamp;

		this->rtpSeqManager.Input(packet->GetSequenceNumber(), seq);
		this->rtpTimestampManager.Input(packet->GetTimestamp(), timestamp);

		// Save original packet fields.
		auto origSsrc      = packet->GetSsrc();
		auto origSeq       = packet->GetSequenceNumber();
		auto origTimestamp = packet->GetTimestamp();

		// Rewrite packet.
		packet->SetSsrc(this->rtpParameters.encodings[0].ssrc);
		packet->SetSequenceNumber(seq);
		packet->SetTimestamp(timestamp);

		if (isSyncPacket)
		{
			MS_DEBUG_TAG(
			  rtp,
			  "sending sync packet [ssrc:%" PRIu32 ", seq:%" PRIu16 ", ts:%" PRIu32
			  "] from original [seq:%" PRIu16 ", ts:%" PRIu32 "]",
			  packet->GetSsrc(),
			  packet->GetSequenceNumber(),
			  packet->GetTimestamp(),
			  origSeq,
			  origTimestamp);
		}

		// Process the packet.
		if (this->rtpStream->ReceivePacket(packet))
		{
			// Send the packet.
			this->listener->OnConsumerSendRtpPacket(this, packet);

			// Retransmit the RTP packet if probing.
			if (IsProbing())
				SendProbationPacket(packet);
		}
		else
		{
			MS_WARN_TAG(
			  rtp,
			  "failed to send packet [ssrc:%" PRIu32 ", seq:%" PRIu16 ", ts:%" PRIu32
			  "] from original [seq:%" PRIu16 ", ts:%" PRIu32 "]",
			  packet->GetSsrc(),
			  packet->GetSequenceNumber(),
			  packet->GetTimestamp(),
			  origSeq,
			  origTimestamp);
		}

		// Restore packet fields.
		packet->SetSsrc(origSsrc);
		packet->SetSequenceNumber(origSeq);
		packet->SetTimestamp(origTimestamp);

		// Restore the original payload if needed.
		if (this->encodingContext)
			packet->RestorePayload();
	}

	void SimulcastConsumer::GetRtcp(RTC::RTCP::CompoundPacket* packet, uint64_t now)
	{
		MS_TRACE();

		if (static_cast<float>((now - this->lastRtcpSentTime) * 1.15) < this->maxRtcpInterval)
			return;

		auto* report = this->rtpStream->GetRtcpSenderReport(now);

		if (!report)
			return;

		packet->AddSenderReport(report);

		// Build SDES chunk for this sender.
		auto* sdesChunk = this->rtpStream->GetRtcpSdesChunk();

		packet->AddSdesChunk(sdesChunk);

		this->lastRtcpSentTime = now;
	}

	void SimulcastConsumer::ReceiveNack(RTC::RTCP::FeedbackRtpNackPacket* nackPacket)
	{
		MS_TRACE();

		if (!IsActive())
			return;

		this->rtpStream->ReceiveNack(nackPacket);
	}

	void SimulcastConsumer::ReceiveKeyFrameRequest(RTC::RTCP::FeedbackPs::MessageType messageType)
	{
		MS_TRACE();

		if (!IsActive())
			return;

		this->rtpStream->ReceiveKeyFrameRequest(messageType);

		RequestKeyFrame();
	}

	void SimulcastConsumer::ReceiveRtcpReceiverReport(RTC::RTCP::ReceiverReport* report)
	{
		MS_TRACE();

		this->rtpStream->ReceiveRtcpReceiverReport(report);
	}

	uint32_t SimulcastConsumer::GetTransmissionRate(uint64_t now)
	{
		MS_TRACE();

		if (!IsActive())
			return 0u;

		return this->rtpStream->GetRate(now);
	}

	float SimulcastConsumer::GetLossPercentage() const
	{
		if (!IsActive() || !this->producerRtpStream)
			return 0;

		if (this->producerRtpStream->GetLossPercentage() >= this->rtpStream->GetLossPercentage())
		{
			return 0;
		}
		else
		{
			return this->rtpStream->GetLossPercentage() - this->producerRtpStream->GetLossPercentage();
		}
	}

	json SimulcastConsumer::GetScore() const
	{
		MS_TRACE();

		json data = json::object();

		if (this->producerRtpStream)
			data["producer"] = this->producerRtpStream->GetScore();
		else
			data["producer"] = 0;

		data["consumer"] = this->rtpStream->GetScore();

		return data;
	}

	void SimulcastConsumer::Paused(bool /*wasProducer*/)
	{
		MS_TRACE();

		this->rtpStream->Pause();

		this->packetsBeforeProbation = PacketsBeforeProbation;

		if (IsProbing())
			StopProbation();
	}

	void SimulcastConsumer::Resumed(bool wasProducer)
	{
		MS_TRACE();

		this->rtpStream->Resume();

		// We need to sync and wait for a key frame (if supported). Otherwise the
		// receiver will request lot of NACKs due to unknown RTP packets.
		this->syncRequired = true;

		// If we have been resumed due to the Producer becoming resumed, we don't
		// need to request a key frame since the Producer already requested it.
		if (!wasProducer)
			RequestKeyFrame();
	}

	void SimulcastConsumer::CreateRtpStream()
	{
		MS_TRACE();

		auto& encoding   = this->rtpParameters.encodings[0];
		auto* mediaCodec = this->rtpParameters.GetCodecForEncoding(encoding);

		// Set stream params.
		RTC::RtpStream::Params params;

		params.ssrc        = encoding.ssrc;
		params.payloadType = mediaCodec->payloadType;
		params.mimeType    = mediaCodec->mimeType;
		params.clockRate   = mediaCodec->clockRate;
		params.cname       = this->rtpParameters.rtcp.cname;

		for (auto& fb : mediaCodec->rtcpFeedback)
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
		}

		// Create a RtpStreamSend for sending a single media stream.
		if (params.useNack)
			this->rtpStream = new RTC::RtpStreamSend(this, params, 1500);
		else
			this->rtpStream = new RTC::RtpStreamSend(this, params, 0);

		// If the Consumer is paused, tell the RtpStreamSend.
		if (IsPaused() || IsProducerPaused())
			this->rtpStream->Pause();

		auto* rtxCodec = this->rtpParameters.GetRtxCodecForEncoding(encoding);

		if (rtxCodec && encoding.hasRtx)
			this->rtpStream->SetRtx(rtxCodec->payloadType, encoding.rtx.ssrc);

		this->keyFrameSupported = Codecs::CanBeKeyFrame(mediaCodec->mimeType);

		this->encodingContext.reset(RTC::Codecs::GetEncodingContext(mediaCodec->mimeType));
	}

	void SimulcastConsumer::RequestKeyFrame()
	{
		MS_TRACE();

		if (!IsActive() || !this->producerRtpStream || this->kind != RTC::Media::Kind::VIDEO)
			return;

		auto mappedSsrc = this->consumableRtpEncodings[0].ssrc;

		this->listener->OnConsumerKeyFrameRequested(this, mappedSsrc);
	}

	inline void SimulcastConsumer::EmitScore() const
	{
		MS_TRACE();

		json data = json::object();

		if (this->producerRtpStream)
			data["producer"] = this->producerRtpStream->GetScore();
		else
			data["producer"] = 0;

		data["consumer"] = this->rtpStream->GetScore();

		Channel::Notifier::Emit(this->id, "score", data);
	}

	void SimulcastConsumer::RecalculateTargetSpatialLayer(bool force)
	{
		MS_TRACE();

		// RTC::RtpEncodingParameters::Profile newTargetProfile;
		// auto probatedProfile = RTC::RtpEncodingParameters::Profile::NONE;

		// // Probation finished.
		// if (this->probationSpatialLayer != -1 && this->probationPackets == 0)
		// {
		// 	probatedProfile      = this->probingProfile;
		// 	this->probationSpatialLayer = -1;
		// }

		// // There are no profiles, select none or default, depending on whether this
		// // is single stream or simulcast/SVC.
		// if (this->mapProfileRtpStream.empty())
		// {
		// 	MS_ASSERT(
		// 	  this->effectiveProfile == RtpEncodingParameters::Profile::NONE ||
		// 	    this->effectiveProfile == RtpEncodingParameters::Profile::DEFAULT,
		// 	  "no profiles, but effective profile is not none nor default");

		// 	if (this->effectiveProfile == RtpEncodingParameters::Profile::NONE)
		// 		newTargetProfile = RtpEncodingParameters::Profile::NONE;
		// 	else
		// 		newTargetProfile = RtpEncodingParameters::Profile::DEFAULT;
		// }
		// // RTP state is unhealty.
		// else if (IsEnabled() && !this->rtpMonitor->IsHealthy())
		// {
		// 	// Ongoing probation, abort.
		// 	if (IsProbing())
		// 	{
		// 		StopProbation();

		// 		return;
		// 	}

		// 	// Probated profile did not succeed.
		// 	if (probatedProfile != RtpEncodingParameters::Profile::NONE)
		// 		return;

		// 	auto it = this->mapProfileRtpStream.find(this->effectiveProfile);

		// 	// This is already the lowest profile.
		// 	if (it == this->mapProfileRtpStream.begin())
		// 		return;

		// 	// Downgrade the target profile.
		// 	newTargetProfile = (std::prev(it))->first;
		// }
		// // If there is no preferred profile, get the highest one.
		// else if (GetPreferredProfile() == RTC::RtpEncodingParameters::Profile::DEFAULT)
		// {
		// 	auto it = this->mapProfileRtpStream.crbegin();

		// 	newTargetProfile = it->first;
		// }
		// // Try with the closest profile to the preferred one.
		// else
		// {
		// 	auto it = this->mapProfileRtpStream.lower_bound(this->preferredProfile);

		// 	// Preferred profile is actually present.
		// 	if (it->first == this->preferredProfile)
		// 	{
		// 		newTargetProfile = it->first;
		// 	}
		// 	// The lowest profile is already higher than the preferred, use it.
		// 	else if (it == this->mapProfileRtpStream.begin())
		// 	{
		// 		newTargetProfile = it->first;
		// 	}
		// 	// There is a lower profile available, prefer it over any higher one.
		// 	else
		// 	{
		// 		newTargetProfile = (--it)->first;
		// 	}
		// }

		// // Not enabled. Make this the target profile.
		// if (!IsEnabled())
		// {
		// 	this->targetProfile = newTargetProfile;
		// }
		// // Ongoing probation.
		// else if (IsProbing())
		// {
		// 	// New profile higher or equal than the one being probed. Do not upgrade.
		// 	if (newTargetProfile >= this->probingProfile)
		// 		return;

		// 	// New profile lower than the one begin probed. Stop probation.
		// 	StopProbation();
		// 	this->targetProfile = newTargetProfile;
		// }
		// // The profile has just been successfully probated.
		// else if (probatedProfile == newTargetProfile)
		// {
		// 	this->targetProfile = newTargetProfile;
		// }
		// // New profile is higher than current target.
		// else if (newTargetProfile > this->targetProfile)
		// {
		// 	// No specific profile is being sent.
		// 	if (
		// 	  this->targetProfile == RTC::RtpEncodingParameters::Profile::NONE ||
		// 	  this->targetProfile == RTC::RtpEncodingParameters::Profile::DEFAULT)
		// 	{
		// 		this->targetProfile = newTargetProfile;
		// 	}
		// 	else if (force)
		// 	{
		// 		this->targetProfile = newTargetProfile;
		// 	}
		// 	// Probe it before promotion.
		// 	// TODO: If we are receiving REMB, consider such value and avoid probation.
		// 	else
		// 	{
		// 		StartProbation(newTargetProfile);

		// 		MS_DEBUG_TAG(
		// 		  rtp,
		// 		  "probing profile [%s]",
		// 		  RTC::RtpEncodingParameters::profile2String[this->probingProfile].c_str());

		// 		return;
		// 	}
		// }
		// // New profile is lower than the target profile.
		// else
		// {
		// 	this->targetProfile = newTargetProfile;
		// }

		// if (this->targetProfile == this->effectiveProfile)
		// 	return;

		// if (IsEnabled() && !IsPaused())
		// 	RequestKeyFrame();

		// MS_DEBUG_TAG(
		//   rtp,
		//   "target profile set [profile:%s]",
		//   RTC::RtpEncodingParameters::profile2String[this->targetProfile].c_str());
	}

	inline bool SimulcastConsumer::IsProbing() const
	{
		return this->probationPackets != 0;
	}

	void SimulcastConsumer::StartProbation(int16_t spatialLayer)
	{
		MS_TRACE();

		this->probationPackets      = ProbationPacketNumber;
		this->probationSpatialLayer = spatialLayer;
	}

	void SimulcastConsumer::StopProbation()
	{
		MS_TRACE();

		this->probationPackets      = 0;
		this->probationSpatialLayer = -1;
	}

	void SimulcastConsumer::SendProbationPacket(RTC::RtpPacket* packet)
	{
		MS_TRACE();

		if (!IsProbing())
			return;

		this->rtpStream->RetransmitPacket(packet);

		switch (this->currentSpatialLayer)
		{
			case -1:
				MS_ABORT("cannot send probation packet without any current spatial layer");
				break;

			case 0:
				this->probationPackets -= 4;
				break;

			case 1:
				this->probationPackets -= 2;
				break;

			// 2 and bigger.
			default:
				this->probationPackets--;
				break;
		}

		if (this->probationPackets == 0)
			RecalculateTargetSpatialLayer();
	}

	inline void SimulcastConsumer::OnRtpStreamSendRtcpPacket(
	  RTC::RtpStream* /*rtpStream*/, RTC::RTCP::Packet* /*packet*/)
	{
		MS_TRACE();

		// Do nothing.
	}

	inline void SimulcastConsumer::OnRtpStreamRetransmitRtpPacket(
	  RTC::RtpStream* /*rtpStream*/, RTC::RtpPacket* packet)
	{
		MS_TRACE();

		this->listener->OnConsumerSendRtpPacket(this, packet);
	}

	inline void SimulcastConsumer::OnRtpStreamScore(RTC::RtpStream* /*rtpStream*/, uint8_t /*score*/)
	{
		MS_TRACE();

		// Emit the score event.
		EmitScore();
	}
} // namespace RTC
