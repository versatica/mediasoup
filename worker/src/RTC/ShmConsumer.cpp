#define MS_CLASS "RTC::ShmConsumer"
// #define MS_LOG_DEV

#include "RTC/ShmConsumer.hpp"
#include "DepLibUV.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "Channel/Notifier.hpp"
#include "RTC/Codecs/Codecs.hpp"

namespace RTC
{
	/* Instance methods. */

	ShmConsumer::ShmConsumer(const std::string& id, RTC::Consumer::Listener* listener, json& data, DepLibSfuShm::SfuShmMapItem *shmCtx)
	  : RTC::Consumer::Consumer(id, listener, data, RTC::RtpParameters::Type::SHM)
	{
		MS_TRACE();

		// Ensure there is a single encoding.
		if (this->consumableRtpEncodings.size() != 1)
			MS_THROW_TYPE_ERROR("invalid consumableRtpEncodings with size != 1");

		auto& encoding   = this->rtpParameters.encodings[0];
		auto* mediaCodec = this->rtpParameters.GetCodecForEncoding(encoding);
		if (!RTC::Codecs::IsValidTypeForCodec(this->type, mediaCodec->mimeType))
		{
			MS_THROW_TYPE_ERROR("%s codec not supported for svc", mediaCodec->mimeType.ToString().c_str());
		}

		this->keyFrameSupported = RTC::Codecs::CanBeKeyFrame(mediaCodec->mimeType);

		// Now read shm name - same as in ShmTransport ctor
		// TODO: get rid of shm name in shm consumer's data!!!
		this->shmCtx = shmCtx;

		CreateRtpStream();
	}

	ShmConsumer::~ShmConsumer()
	{
		MS_TRACE();

		delete this->rtpStream;
	}

	void ShmConsumer::FillJson(json& jsonObject) const
	{
		MS_TRACE();

		// Call the parent method.
		RTC::Consumer::FillJson(jsonObject);

		// Add rtpStream.
	  this->rtpStream->FillJson(jsonObject["rtpStream"]);

		// TODO: add smth about shm writes
	}

	void ShmConsumer::FillJsonStats(json& jsonArray) const
	{
		MS_TRACE();
		
		uint64_t nowMs = DepLibUV::GetTimeMs();

		// Add stats of our send stream.
	  jsonArray.emplace_back(json::value_t::object);
		//this->rtpStream->FillJsonStats(jsonArray[0]);
		jsonArray[0]["type"]        = "shm-writer-stats";

		RTC::RtpDataCounter* ptr = const_cast<RTC::RtpDataCounter*>(&this->shmWriterCounter);
		jsonArray[0]["packetCount"] = ptr->GetPacketCount();
		jsonArray[0]["byteCount"]   = ptr->GetBytes();
		jsonArray[0]["bitrate"]     = ptr->GetBitrate(nowMs);

		// Add stats of our recv stream.
		if (this->producerRtpStream)
		{
			jsonArray.emplace_back(json::value_t::object);
			this->producerRtpStream->FillJsonStats(jsonArray[1]);
		}

		// Shm writing stats
		

	}

	void ShmConsumer::FillJsonScore(json& jsonObject) const
	{
		MS_TRACE();

		// NOTE: Hardcoded values
		jsonObject["score"]         = 10;
		jsonObject["producerScore"] = 10;
	}

	void ShmConsumer::HandleRequest(Channel::Request* request)
	{
		MS_TRACE();

		switch (request->methodId)
		{
			case Channel::Request::MethodId::CONSUMER_REQUEST_KEY_FRAME:
			{
				if (IsActive())
					RequestKeyFrame();

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

	void ShmConsumer::ProducerRtpStream(RTC::RtpStream* rtpStream, uint32_t /*mappedSsrc*/)
	{
		MS_TRACE();

		this->producerRtpStream = rtpStream;
	}

	void ShmConsumer::ProducerNewRtpStream(RTC::RtpStream* rtpStream, uint32_t /*mappedSsrc*/)
	{
		MS_TRACE();

		this->producerRtpStream = rtpStream;
	}

	void ShmConsumer::ProducerRtpStreamScore(
	  RTC::RtpStream* /*rtpStream*/, uint8_t /*score*/, uint8_t /*previousScore*/)
	{
		MS_TRACE();

		// Emit the score event.
	}

	void ShmConsumer::ProducerRtcpSenderReport(RTC::RtpStream* /*rtpStream*/, bool /*first*/)
	{
		MS_TRACE();

		// Do nothing.
	}

	void ShmConsumer::SendRtpPacket(RTC::RtpPacket* packet)
	{
		MS_TRACE();

		if (!IsActive()) {
/*			MS_DEBUG_TAG(rtp, "-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- SHMCONSUMER INACTIVE: parent.IsActive %ul isPaused %ul isProducerPaused %ul producerRtpStream %ul",
				RTC::Consumer::IsActive(),
				IsPaused(),
				IsProducerPaused(), 
				(this->producerRtpStream != nullptr)); */
			return;
		}

		auto payloadType = packet->GetPayloadType();

		// NOTE: This may happen if this Consumer supports just some codecs of those
		// in the corresponding Producer.
		if (this->supportedCodecPayloadTypes.find(payloadType) == this->supportedCodecPayloadTypes.end())
		{
			MS_DEBUG_TAG(rtp, "payload type not supported [payloadType:%" PRIu8 "]", payloadType);

			return;
		}

		// If we need to sync, support key frames and this is not a key frame, ignore
		// the packet.
		if (this->syncRequired && this->keyFrameSupported && !packet->IsKeyFrame()) {
			MS_DEBUG_TAG(rtp, "-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- need to sync but this is not keyframe, ignore packet");
			return;
		}

		// Whether this is the first packet after re-sync.
		bool isSyncPacket = this->syncRequired;

		// Sync sequence number and timestamp if required.
		if (isSyncPacket)
		{
			if (packet->IsKeyFrame())
				MS_DEBUG_TAG(rtp, "sync key frame received");

			this->rtpSeqManager.Sync(packet->GetSequenceNumber() - 1);

			this->syncRequired = false;
		}

		// Update RTP seq number and timestamp.
		uint16_t seq;

		this->rtpSeqManager.Input(packet->GetSequenceNumber(), seq);

		// Save original packet fields.
		auto origSsrc = packet->GetSsrc();
		auto origSeq  = packet->GetSequenceNumber();

		// Rewrite packet.
		packet->SetSsrc(this->rtpParameters.encodings[0].ssrc);
		packet->SetSequenceNumber(seq);

		if (isSyncPacket)
		{
			MS_DEBUG_TAG(
			  rtp,
			  "sending sync packet [ssrc:%" PRIu32 ", seq:%" PRIu16 ", ts:%" PRIu32
			  "] from original [seq:%" PRIu16 "]",
			  packet->GetSsrc(),
			  packet->GetSequenceNumber(),
			  packet->GetTimestamp(),
			  origSeq);
		}

		// Process the packet.
		//if (this->rtpStream->ReceivePacket(packet))
		if (this->WritePacketToShm(packet))
		{

			// Increase transmission counter.
			this->shmWriterCounter.Update(packet);

			// Send the packet.
			this->listener->OnConsumerSendRtpPacket(this, packet);
		}
		else
		{
			MS_WARN_TAG(
			  rtp,
			  "failed to send packet [ssrc:%" PRIu32 ", seq:%" PRIu16 ", ts:%" PRIu32
			  "] from original [seq:%" PRIu16 "]",
			  packet->GetSsrc(),
			  packet->GetSequenceNumber(),
			  packet->GetTimestamp(),
			  origSeq);
		}

		// Restore packet fields.
		packet->SetSsrc(origSsrc);
		packet->SetSequenceNumber(origSeq);
	}

	bool ShmConsumer::WritePacketToShm(RTC::RtpPacket* packet)
	{
		MS_TRACE();
		// 01/02/2020: Moved everything from ShmTranport, because only consumer knows its content type - audio or video.
		// Transport isn't the right place for it because there can be > 1 consumer on a single transport entity

		// If we have not written any packets yet, need to configure shm writer
		if (shmWriterCounter.GetPacketCount() == 0) {
			DepLibSfuShm::ShmWriterStatus stat = DepLibSfuShm::ConfigureShmWriterCtx( 
				this->shmCtx,
				(this->GetKind() == RTC::Media::Kind::AUDIO ? DepLibSfuShm::ShmChunkType::AUDIO : DepLibSfuShm::ShmChunkType::VIDEO), 
				packet->GetSsrc());
			
			MS_DEBUG_TAG(rtp, "-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- SUCCESS CONFIGURING SHM %s this->shmCtx->Status(): %u", this->GetKind() == RTC::Media::Kind::AUDIO ? "audio" : "video", this->shmCtx->Status() );
		}

		if ( this->shmCtx->Status() != DepLibSfuShm::ShmWriterStatus::SHM_WRT_READY )
		{
			MS_DEBUG_TAG(rtp, "-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-Skip writing %s RTP packet to SHM because this->shmCtx->Status(): %u", this->GetKind() == RTC::Media::Kind::AUDIO ? "audio" : "video", this->shmCtx->Status() );
			return false;
		}
		//TODO: packet->ReadVideoOrientation() will return some data which we could pass to shm...

		uint8_t const* cdata = packet->GetData();
		uint8_t* data = const_cast<uint8_t*>(cdata);
		size_t len          = packet->GetSize();

		switch (this->GetKind())
		{
			case RTC::Media::Kind::AUDIO:
			{
				/* Just write out the whole opus packet without parsing into separate opus frames.
					+----------+--------------+
					|RTP Header| Opus Payload |
					+----------+--------------+	*/
				this->chunk.data = data;
				this->chunk.len = len;
				this->chunk.rtp_time = packet->GetTimestamp(); // TODO: recalculate into actual one including cycles info from RTPStream
				this->chunk.first_rtp_seq = this->chunk.last_rtp_seq = packet->GetSequenceNumber();
				this->chunk.begin = this->chunk.end = 1;
				DepLibSfuShm::WriteChunk(shmCtx, &chunk, DepLibSfuShm::ShmChunkType::AUDIO, packet->GetSsrc());

				break;
			} // audio

			case RTC::Media::Kind::VIDEO: //video
			{
				uint8_t nal = *data & 0x1F;

				switch (nal)
				{
					// Single NAL unit packet.
					case 7:
					{
						size_t offset{ 1 };

						len -= 1;
						if (len < 3) {
							MS_WARN_TAG(rtp, "NALU data len <3: %lu", len);
							break;
						}

						auto naluSize  = Utils::Byte::Get2Bytes(data, offset);

						//TODO: Do I worry about padding?
						/*
						  The RTP timestamp is set to the sampling timestamp of the content.
      				A 90 kHz clock rate MUST be used.
						*/

						this->chunk.data = data+offset;
						this->chunk.len = naluSize;
						this->chunk.rtp_time = packet->GetTimestamp(); // TODO: recalculate into actual one including cycles info from RTPStream
						this->chunk.first_rtp_seq = this->chunk.last_rtp_seq = packet->GetSequenceNumber();
						this->chunk.begin = this->chunk.end = 1;

						DepLibSfuShm::WriteChunk(shmCtx, &chunk, DepLibSfuShm::ShmChunkType::VIDEO, packet->GetSsrc());
						break;
					}

					// Aggregation packet. Contains several NAL units in a single RTP packet
					// STAP-A.
					/*
						0                   1                   2                   3
						0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
						+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
						|                          RTP Header                           |
						+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
						|STAP-A NAL HDR |         NALU 1 Size           | NALU 1 HDR    |
						+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
						|                         NALU 1 Data                           |
						:                                                               :
						+               +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
						|               | NALU 2 Size                   | NALU 2 HDR    |
						+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
						|                         NALU 2 Data                           |
						:                                                               :
						|                               +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
						|                               :...OPTIONAL RTP padding        |
						+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
						Figure 7.  An example of an RTP packet including an STAP-A
						containing two single-time aggregation units
					*/
					case 24:
					{
						size_t offset{ 1 };
						len -= 1;
						// Iterate NAL units.
						while (len >= 3)
						{
							auto naluSize  = Utils::Byte::Get2Bytes(data, offset);

							// Check if there is room for the indicated NAL unit size.
							if (len < (naluSize + sizeof(naluSize))) {
								break;
							}
							else {
								/* The RTP timestamp MUST be set to the earliest of the NALU-times of
      					all the NAL units to be aggregated. */
								this->chunk.data = data+offset;
								this->chunk.len = len;
								this->chunk.rtp_time = packet->GetTimestamp(); // TODO: recalculate into actual one including cycles info from RTPStream
								this->chunk.first_rtp_seq = this->chunk.last_rtp_seq = packet->GetSequenceNumber(); // TODO: does NALU have its own seqId?
								this->chunk.begin = this->chunk.end = 1;

								DepLibSfuShm::WriteChunk(shmCtx, &chunk, DepLibSfuShm::ShmChunkType::VIDEO, packet->GetSsrc());
							}

							offset += naluSize + sizeof(naluSize);
							len -= naluSize + sizeof(naluSize);
						}
						break;
					}

					// Fragmentation unit. Single NAL unit is spreaded accross several RTP packets.
					// FU-A
					case 29:
					{
						size_t offset{ 1 };
						auto naluSize  = Utils::Byte::Get2Bytes(data, offset);

						uint8_t subnal   = *(data + 1) & 0x1F; // Last 5 bits in FU header, subtype of FU unit, we don't use it
						uint8_t startBit = *(data + 1) & 0x80; // 1st bit indicates start of fragmented NALU
						uint8_t endBit = *(data + 1) & 0x40; ; // 2nd bit indicates end

						this->chunk.data = data+offset;
						this->chunk.len = naluSize;
						this->chunk.rtp_time = packet->GetTimestamp(); // TODO: recalculate into actual one including cycles info from RTPStream
						this->chunk.first_rtp_seq = this->chunk.last_rtp_seq = packet->GetSequenceNumber();
						this->chunk.begin = (startBit == 128)? 1 : 0;
						this->chunk.end = (endBit) ? 1 : 0;

						DepLibSfuShm::WriteChunk(shmCtx, &chunk, DepLibSfuShm::ShmChunkType::VIDEO, packet->GetSsrc());
						break;
					}
					case 25: // STAB-B
					case 26: // MTAP-16
					case 27: // MTAP-24
					case 28: // FU-B
					{
						MS_WARN_TAG(rtp, "Unsupported NAL unit type %d", nal);
						break;
					}
					default: // ignore the rest
						break;
				}
				break;
			} // video

			default:
			  // RTC::Media::Kind::ALL
			  break;
		}
	
		return true;
	}

	void ShmConsumer::GetRtcp(
	  RTC::RTCP::CompoundPacket* packet, RTC::RtpStreamSend* rtpStream, uint64_t now)
	{
		MS_TRACE();
	}

	void ShmConsumer::NeedWorstRemoteFractionLost(
	  uint32_t /*mappedSsrc*/, uint8_t& worstRemoteFractionLost)
	{
		MS_TRACE();
	}

	void ShmConsumer::ReceiveNack(RTC::RTCP::FeedbackRtpNackPacket* nackPacket)
	{
		MS_TRACE();

		if (!IsActive())
			return;
	}

	void ShmConsumer::ReceiveKeyFrameRequest(
	  RTC::RTCP::FeedbackPs::MessageType messageType, uint32_t /*ssrc*/)
	{
		MS_TRACE();

		if (IsActive())
			RequestKeyFrame();
	}

	void ShmConsumer::ReceiveRtcpReceiverReport(RTC::RTCP::ReceiverReport* report)
	{
		MS_TRACE();
	}

	uint32_t ShmConsumer::GetTransmissionRate(uint64_t now)
	{
		MS_TRACE();

		return 0u;
	}

	void ShmConsumer::UserOnTransportConnected()
	{
		MS_TRACE();

		this->syncRequired = true;

		if (IsActive())
			RequestKeyFrame();
	}

	void ShmConsumer::UserOnTransportDisconnected()
	{
		MS_TRACE();

//		this->rtpStream->Pause();
	}

	void ShmConsumer::UserOnPaused()
	{
		MS_TRACE();

//		this->rtpStream->Pause();
	}

	void ShmConsumer::UserOnResumed()
	{
		MS_TRACE();

		this->syncRequired = true;

		if (IsActive())
			RequestKeyFrame();
	}

	void ShmConsumer::CreateRtpStream()
	{
		//TBD: simply copied from SimpleConsumer.cpp
		auto& encoding   = this->rtpParameters.encodings[0];
		auto* mediaCodec = this->rtpParameters.GetCodecForEncoding(encoding);

		MS_DEBUG_TAG(
		  rtp, "[ssrc:%" PRIu32 ", payloadType:%" PRIu8 "]", encoding.ssrc, mediaCodec->payloadType);

		// Set stream params.
		RTC::RtpStream::Params params;

		params.ssrc        = encoding.ssrc;
		params.payloadType = mediaCodec->payloadType;
		params.mimeType    = mediaCodec->mimeType;
		params.clockRate   = mediaCodec->clockRate;
		params.cname       = this->rtpParameters.rtcp.cname;

		// Check in band FEC in codec parameters.
		if (mediaCodec->parameters.HasInteger("useinbandfec") && mediaCodec->parameters.GetInteger("useinbandfec") == 1)
		{
			MS_DEBUG_TAG(rtp, "in band FEC enabled");

			params.useInBandFec = true;
		}

		// Check DTX in codec parameters.
		if (mediaCodec->parameters.HasInteger("usedtx") && mediaCodec->parameters.GetInteger("usedtx") == 1)
		{
			MS_DEBUG_TAG(rtp, "DTX enabled");

			params.useDtx = true;
		}

		// Check DTX in the encoding.
		if (encoding.dtx)
		{
			MS_DEBUG_TAG(rtp, "DTX enabled");

			params.useDtx = true;
		}

	for (auto& fb : mediaCodec->rtcpFeedback)
	{
		if (!params.useNack && fb.type == "nack" && fb.parameter == "")
		{
			MS_DEBUG_2TAGS(rtp, rtcp, "NACK supported");

			params.useNack = true;
		}
		else if (!params.usePli && fb.type == "nack" && fb.parameter == "pli")
		{
			MS_DEBUG_2TAGS(rtp, rtcp, "PLI supported");

			params.usePli = true;
		}
		else if (!params.useFir && fb.type == "ccm" && fb.parameter == "fir")
		{
			MS_DEBUG_2TAGS(rtp, rtcp, "FIR supported");

			params.useFir = true;
		}
	}

	// Create a RtpStreamSend for sending a single media stream.
	size_t bufferSize = params.useNack ? 600u : 0u;

	this->rtpStream = new RTC::RtpStreamSend(this, params, bufferSize);
	this->rtpStreams.push_back(this->rtpStream);

		// If the Consumer is paused, tell the RtpStreamSend.
		if (IsPaused() || IsProducerPaused())
			rtpStream->Pause();

		auto* rtxCodec = this->rtpParameters.GetRtxCodecForEncoding(encoding);

		if (rtxCodec && encoding.hasRtx)
			rtpStream->SetRtx(rtxCodec->payloadType, encoding.rtx.ssrc);

		this->rtpStreams.push_back(rtpStream);
	}


	void ShmConsumer::RequestKeyFrame()
	{
		MS_TRACE();

		if (this->kind != RTC::Media::Kind::VIDEO)
			return;

		auto mappedSsrc = this->consumableRtpEncodings[0].ssrc;

		this->listener->OnConsumerKeyFrameRequested(this, mappedSsrc);
	}

	void ShmConsumer::OnRtpStreamRetransmitRtpPacket(RTC::RtpStreamSend* rtpStream, RTC::RtpPacket* packet)
	{
		MS_TRACE();

		//this->listener->OnConsumerRetransmitRtpPacket(this, packet);

		// May emit 'trace' event.
		//EmitTraceEventRtpAndKeyFrameTypes(packet, this->rtpStream->HasRtx());
	}

	inline void ShmConsumer::OnRtpStreamScore(
	  RTC::RtpStream* /*rtpStream*/, uint8_t /*score*/, uint8_t /*previousScore*/)
	{
		MS_TRACE();
	}

	uint8_t ShmConsumer::GetBitratePriority() const
	{
		MS_TRACE();

		// PipeConsumer does not play the BWE game.
		return 0u;
	}

	uint32_t ShmConsumer::IncreaseLayer(uint32_t /*bitrate*/, bool /*considerLoss*/)
	{
		MS_TRACE();

		return 0u;
	}

	void ShmConsumer::ApplyLayers()
	{
		MS_TRACE();
	}

	uint32_t ShmConsumer::GetDesiredBitrate() const
	{
		MS_TRACE();

		return 0u;
	}


	float ShmConsumer::GetRtt() const
	{
		MS_TRACE();

		return this->rtpStream->GetRtt();
	}
} // namespace RTC
