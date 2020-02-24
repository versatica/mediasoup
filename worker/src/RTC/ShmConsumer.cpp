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

	ShmConsumer::ShmConsumer(const std::string& id, RTC::Consumer::Listener* listener, json& data, DepLibSfuShm::SfuShmCtx *shmCtx)
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

		//MS_DEBUG_TAG(rtp, "ShmConsumer created from data: %s media codec: %s", data.dump().c_str(), mediaCodec->mimeType.ToString().c_str());

		this->keyFrameSupported = RTC::Codecs::CanBeKeyFrame(mediaCodec->mimeType);

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
			MS_DEBUG_TAG(rtp, "need to sync but this is not keyframe, ignore packet");
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
		// If we have not written any packets yet, need to configure shm writer
		if (shmWriterCounter.GetPacketCount() == 0)
		{
			auto kind = (this->GetKind() == RTC::Media::Kind::AUDIO) ? DepLibSfuShm::ShmChunkType::AUDIO : DepLibSfuShm::ShmChunkType::VIDEO;
			if (DepLibSfuShm::ShmWriterStatus::SHM_WRT_READY != shmCtx->SetSsrcInShmConf(packet->GetSsrc(), kind))
			{
				return false;
			}
		}

		//TODO: packet->ReadVideoOrientation() will return some data which we could pass to shm

		uint8_t const* pdata = packet->GetData();
		uint8_t const* cdata = packet->GetPayload();
		uint8_t* data        = const_cast<uint8_t*>(cdata);
		size_t len           = packet->GetPayloadLength();
		uint64_t ts          = static_cast<uint64_t>(packet->GetTimestamp());
		uint64_t seq         = static_cast<uint64_t>(packet->GetSequenceNumber());
		uint32_t ssrc        = packet->GetSsrc();

		std::memset(&chunk, 0, sizeof(chunk));

		switch (this->GetKind())
		{
			case RTC::Media::Kind::AUDIO:
			{
				ts = shmCtx->AdjustPktTs(ts, DepLibSfuShm::ShmChunkType::AUDIO);
				seq = shmCtx->AdjustPktSeq(seq, DepLibSfuShm::ShmChunkType::AUDIO);
				/* Just write out the whole opus packet without parsing into separate opus frames.
					+----------+--------------+
					|RTP Header| Opus Payload |
					+----------+--------------+	*/
				size_t offset{ 1 };

				this->chunk.data = data + offset;
				this->chunk.len = len - offset;
				this->chunk.rtp_time = ts;
				this->chunk.first_rtp_seq = this->chunk.last_rtp_seq = seq;
				this->chunk.ssrc = packet->GetSsrc();
				this->chunk.begin = this->chunk.end = 1;

				if(0 != shmCtx->WriteChunk(&chunk, DepLibSfuShm::ShmChunkType::AUDIO, packet->GetSsrc()))
				{
					//MS_WARN_TAG(rtp, "FAIL writing audio pkt to shm: len %zu ts %" PRIu64 " seq %" PRIu64, this->chunk.len, this->chunk.rtp_time, this->chunk.first_rtp_seq);
					return false;
				}
				
				shmCtx->UpdatePktStat(seq, ts, DepLibSfuShm::ShmChunkType::AUDIO);
				
				break;
			} // audio

			case RTC::Media::Kind::VIDEO: //video
			{
				ts = shmCtx->AdjustPktTs(ts, DepLibSfuShm::ShmChunkType::VIDEO);
				seq = shmCtx->AdjustPktSeq(seq, DepLibSfuShm::ShmChunkType::VIDEO);

				uint8_t nal = *cdata & 0x1F;
				uint8_t marker = pdata[1] & 0x80; // Marker bit indicates the last or the only NALU in this packet is the end of the picture data

				// If the first video pkt, or pkt's timestamp is different from the previous video pkt written out
 				int begin_picture = (shmCtx->IsSeqUnset(DepLibSfuShm::ShmChunkType::VIDEO)) || (ts > shmCtx->LastTs(DepLibSfuShm::ShmChunkType::VIDEO));


					// Single NAL unit packet
					if (nal >= 1 && nal <= 23)  {
						size_t offset{ 1 };
						if (len < 1) {
							MS_WARN_TAG(rtp, "NALU data len < 1: %lu", len);
							break;
						}
						
						//uint8_t subnal   = *(data + offset) & 0x1F;
						uint16_t chunksize = len - offset;

						//TODO: Add Annex B 0x00000001 to the begininng of this packet will overwrite pkt header data b/c we need 3 more bytes, see whether it is okay to do i.e. that data will not be used for anything else?
						if (begin_picture)
						{
							data[offset - 1] = 0x01;
							data[offset - 2] = 0x00;
							data[offset - 3] = 0x00;
							data[offset - 4] = 0x00;
							offset -= 4;
							chunksize += 4;
						}
						else {
							data[offset - 1] = 0x01;
							data[offset - 2] = 0x00;
							data[offset - 3] = 0x00;
							offset -= 3;
							chunksize += 3;
						}
					
						this->chunk.data          = const_cast<uint8_t*>(cdata + offset);
						this->chunk.len           = chunksize;
						this->chunk.rtp_time      = ts;
						this->chunk.first_rtp_seq = this->chunk.last_rtp_seq = seq;
						this->chunk.ssrc          = ssrc;
						this->chunk.begin         = begin_picture;
						this->chunk.end           = (marker != 0);

						MS_DEBUG_TAG(rtp, "video single NALU=%d%s len %zu ts %" PRIu64 " seq %" PRIu64 " begin_picture=%d end_picture=%d lastTs=%" PRIu64 " ts > lastTs is %s",
							nal, 
							(UINT64_UNSET == this->chunk.first_rtp_seq) ? " (seq UINT64_UNSET)" : "",
							this->chunk.len, this->chunk.rtp_time, this->chunk.first_rtp_seq, begin_picture, marker, shmCtx->LastTs(DepLibSfuShm::ShmChunkType::VIDEO), (ts > shmCtx->LastTs(DepLibSfuShm::ShmChunkType::VIDEO)) ? "true" : "false");

						if (0 != shmCtx->WriteChunk(&chunk, DepLibSfuShm::ShmChunkType::VIDEO, packet->GetSsrc()))
						{
							MS_WARN_TAG(rtp, "FAIL writing video NALU: len %" PRIu32 " ts %" PRIu64 " seq %" PRIu64, this->chunk.len, this->chunk.rtp_time, this->chunk.first_rtp_seq);
							return false;
						}
				}
				else {
					switch (nal)
					{

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
						*/
						case 24:
						{
							size_t offset{ 1 }; // Skip over STAP-A NAL header
							size_t leftLen = len - 1 - sizeof(uint16_t) - 1; // skip over STAP-A NAL header, NALU size and NALU header
							
							// Iterate NAL units.
							while (leftLen > 1)
							{
								uint16_t chunksize;
								uint16_t naluSize  = Utils::Byte::Get2Bytes(data, offset); 

								// Check if there is room for the indicated NAL unit size.
								if (leftLen < naluSize || leftLen < 4) {
									MS_WARN_TAG(rtp, "payload left to read from STAP-A is shorter than NALU size or just too short: %" PRIu32"<%" PRIu16, leftLen, naluSize);
									break; // okay to have up to 4 bytes of padding
								}
								else {
									offset += sizeof(naluSize) + 1; // skip over NALU size and NALU header
									chunksize = naluSize - 1;

									if (begin_picture) {
										data[offset - 1] = 0x01;
										data[offset - 2] = 0x00;
										data[offset - 3] = 0x00;
										data[offset - 4] = 0x00;
										begin_picture = 0;
										this->chunk.begin = 1;
										offset -= 4;
										chunksize += 4;
									}
									else {
										data[offset - 1] = 0x01;
										data[offset - 2] = 0x00;
										data[offset - 3] = 0x00;
										this->chunk.begin = 0;
										offset -= 3;
										chunksize += 3;
									}

									this->chunk.data          = data + offset;
									this->chunk.len           = chunksize;
									this->chunk.rtp_time      = ts; // all NALUs share the same timestamp, https://tools.ietf.org/html/rfc6184#section-5.7.1
									this->chunk.first_rtp_seq = this->chunk.last_rtp_seq = seq;
									this->chunk.ssrc          = ssrc;
									this->chunk.end           = (offset + chunksize >= len - 3) /* 3 is max length of RTP padding observed so far */ && (marker != 0); // if last unit in payload and marker indicates end of picture data. 

									MS_DEBUG_TAG(rtp, "video STAP-A: payloadlen=%" PRIu32 " nalulen=%" PRIu16 " chunklen=%" PRIu32 " ts=%" PRIu64 " seq=%" PRIu64 " lastTs=%" PRIu64,
										len, naluSize, this->chunk.len, this->chunk.rtp_time, this->chunk.first_rtp_seq, shmCtx->LastTs(DepLibSfuShm::ShmChunkType::VIDEO));
									
									if (0 != shmCtx->WriteChunk(&chunk, DepLibSfuShm::ShmChunkType::VIDEO, ssrc))
									{
										MS_WARN_TAG(rtp, "FAIL writing STAP-A pkt to shm: len %zu ts %" PRIu64 " seq %" PRIu64, this->chunk.len, this->chunk.rtp_time, this->chunk.first_rtp_seq);
										return false;
									}
								}

								offset += chunksize; // start reading at the next NALU size
								leftLen -= naluSize - 1;
							}
							break;
						}

						/*
						Fragmentation unit. Single NAL unit is spreaded accross several RTP packets.
						FU-A
									0                   1                   2                   3
									0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
									+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
									| FU indicator  |   FU header   |                               |
									+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+                               |
									|                                                               |
									|                         FU payload                            |
									|                                                               |
									|                               +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
									|                               :...OPTIONAL RTP padding        |
									+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ */
						case 28:
						{
							if (len < 3)
							{
								MS_DEBUG_TAG(rtp, "FU-A payload too short");
								return false;
							}
							// Parse FU header octet
							uint8_t startBit = *(data + 1) & 0x80; // 1st bit indicates the starting fragment
							uint8_t endBit   = *(data + 1) & 0x40; // 2nd bit indicates the ending fragment
							uint8_t subnal   = *(data + 1) & 0x1F; // Last 5 bits in FU header, subtype of FU unit, if (subnal == 7 (SPS) && startBit == 128) then we have a key frame

							size_t offset{ 2 };
							size_t chunksize = len - 2;
							
							if (shmCtx->IsTsUnset(DepLibSfuShm::ShmChunkType::VIDEO) || ts == shmCtx->LastTs(DepLibSfuShm::ShmChunkType::VIDEO))
								begin_picture = 0;

							if (startBit == 128)
							{
								if (begin_picture)
								{
									data[offset - 1] = 0x01;
									data[offset - 2] = 0x00;
									data[offset - 3] = 0x00;
									data[offset - 4] = 0x00;
									this->chunk.begin = 1;
									offset -= 4;
									chunksize += 4;
								}
								else
								{
									data[offset - 1] = 0x01;
									data[offset - 2] = 0x00;
									data[offset - 3] = 0x00;
									this->chunk.begin  = 0;
									offset -= 3;
									chunksize += 3;
								}
							}

							this->chunk.data          = data + offset;
							this->chunk.len           = chunksize;
							this->chunk.rtp_time      = ts;
							this->chunk.first_rtp_seq = this->chunk.last_rtp_seq = seq;
							this->chunk.ssrc          = ssrc;
							this->chunk.end           = (endBit && marker) ? 1 : 0;
							
							MS_DEBUG_TAG(rtp, "video FU-A%s: len=%" PRIu32 " ts=%" PRIu64 " prev_ts=%" PRIu64 " seq=%" PRIu64 " subnal=%" PRIu8 " startBit=%" PRIu8 " end_marker=%" PRIu8 "%s%s",
								(UINT64_UNSET == this->chunk.first_rtp_seq) ? " (seq UINT64_UNSET)" : "",
								this->chunk.len, this->chunk.rtp_time, shmCtx->LastTs(DepLibSfuShm::ShmChunkType::VIDEO), this->chunk.first_rtp_seq, subnal, startBit, marker, this->chunk.begin ? " begin" : "", this->chunk.end ? " end": "");
							if (0 != shmCtx->WriteChunk(&chunk, DepLibSfuShm::ShmChunkType::VIDEO, ssrc))
							{
								MS_WARN_TAG(rtp, "FAIL writing FU-A pkt to shm: len=%zu ts=%" PRIu64 " seq=%" PRIu64 "%s%s", this->chunk.len, this->chunk.rtp_time, this->chunk.first_rtp_seq, this->chunk.begin ? " begin" : "", this->chunk.end ? " end": "");
								return false;
							}
							break;
						}
						case 25: // STAB-B
						case 26: // MTAP-16
						case 27: // MTAP-24
						case 29: // FU-B
						{
							MS_WARN_TAG(rtp, "Unsupported NAL unit type %u in video packet", nal);
							return false;
						}
						default: // ignore the rest
						{
							MS_DEBUG_TAG(rtp, "Unknown NAL unit type %u in video packet", nal);
							return false;
						}
					} // case nal
				} // if
				shmCtx->UpdatePktStat(seq, ts, DepLibSfuShm::ShmChunkType::VIDEO); // TODO: ensure that previous seqId and ts updated only for successfully written packets?
				break;
			} // case video

			default:
				break;
		} // kind

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

		this->rtpStream->Pause();
	}

	void ShmConsumer::UserOnPaused()
	{
		MS_TRACE();

		this->rtpStream->Pause();
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
