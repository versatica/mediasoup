#define MS_CLASS "RTC::ShmConsumer"
// #define MS_LOG_DEV

#include "RTC/ShmConsumer.hpp"
#include "DepLibUV.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "Channel/ChannelNotifier.hpp"
#include "RTC/Codecs/Tools.hpp"
#include "RTC/RTCP/FeedbackRtpNack.hpp"
#include "RTC/RtpStreamRecv.hpp"
#include "Lively.hpp"
#include "LivelyAppDataToJson.hpp"


namespace RTC
{
	/* Static. */
	static constexpr uint64_t ShmIdleCheckInterval{ 20000u }; // In ms.


	void RtpLostPktRateCounter::Update(RTC::RtpPacket* packet)
	{
		// Each packet if in order will increate number of total received packets
		// If there is gap in seqIds also increase number of lost packets
		// If packet's seqId is older then do not update any counters
		size_t lost    = 0;
		size_t total   = 0;
		uint64_t nowMs = DepLibUV::GetTimeMs();
		uint64_t seq   = static_cast<uint64_t>(packet->GetSequenceNumber());

		// The very first packet, no loss yet, increment total
		if (0u == this->firstSeqId)
		{
			total = 1;
			this->firstSeqId = this->lastSeqId = seq;			
		}
		else if (this->lastSeqId >= seq)
		{
			return; // probably retransmission of earlier lost pkt, ignore
		}
		else
		{
			// Packet in order, check for gaps in seq ids and update the counters
			lost = seq - this->lastSeqId - 1;
			total = seq - this->lastSeqId;
			this->lastSeqId = seq;
		}
		
		this->totalPackets.Update(total, nowMs);
		this->lostPackets.Update(lost, nowMs);
	}


	ShmConsumer::ShmConsumer(const std::string& id, const std::string& producerId, RTC::Consumer::Listener* listener, json& data, DepLibSfuShm::ShmCtx *shmCtx)
	  : RTC::Consumer::Consumer(id, producerId, listener, data, RTC::RtpParameters::Type::SHM)
	{
		MS_TRACE();

		// Ensure there is a single encoding.
		if (this->consumableRtpEncodings.size() != 1)
			MS_THROW_TYPE_ERROR("invalid consumableRtpEncodings with size != 1");

		auto& encoding   = this->rtpParameters.encodings[0];
		auto* mediaCodec = this->rtpParameters.GetCodecForEncoding(encoding);
		if (!RTC::Codecs::Tools::IsValidTypeForCodec(this->type, mediaCodec->mimeType))
		{
			MS_THROW_TYPE_ERROR("%s codec not supported for shm", mediaCodec->mimeType.ToString().c_str());
		}

		MS_DEBUG_TAG(xcode, "ShmConsumer ctor() data [%s] media codec [%s]", data.dump().c_str(), mediaCodec->mimeType.ToString().c_str());
		
		// Optional appData
		auto jsonAppDataIt = data.find("appData");
		Lively::AppData lively;
		if (jsonAppDataIt != data.end() && jsonAppDataIt->is_object())
		{
			try {
				lively = jsonAppDataIt->get<Lively::AppData>();
				this->appData = lively.ToStr();
			}
			catch (const std::exception& e) {
				MS_WARN_TAG(xcode, "%s\t%s", e.what(), (*jsonAppDataIt).dump().c_str());
			}
		}

		this->keyFrameSupported = RTC::Codecs::Tools::CanBeKeyFrame(mediaCodec->mimeType);

		this->shmCtx = shmCtx;
		if (this->GetKind() == RTC::Media::Kind::VIDEO)
			this->shmCtx->SetListener(this);

		this->testNackEachMs = shmCtx->TestNackMs();
		uint64_t nowTs = DepLibUV::GetTimeMs();
		this->lastNACKTestTs = nowTs;

		CreateRtpStream();

		this->shmIdleCheckTimer = new Timer(this);
		this->shmIdleCheckTimer->Start(ShmIdleCheckInterval);

		this->shmCtx->ResetShmMediaStatsAndQueue((this->GetKind() == RTC::Media::Kind::AUDIO) ? DepLibSfuShm::Media::AUDIO : DepLibSfuShm::Media::VIDEO);
	}

	ShmConsumer::~ShmConsumer()
	{
		MS_TRACE();

		delete this->rtpStream;
		delete this->shmIdleCheckTimer;
	}


	inline void ShmConsumer::OnTimer(Timer* timer)
	{
		MS_TRACE();

		if (timer == this->shmIdleCheckTimer)
		{
			this->idle = true;
			this->OnIdleShmConsumer();
		}
	}


	void ShmConsumer::FillJson(json& jsonObject) const
	{
		MS_TRACE();

		// Call the parent method.
		RTC::Consumer::FillJson(jsonObject);

		// Add rtpStream.
	  this->rtpStream->FillJson(jsonObject["rtpStream"]);
	}


	void ShmConsumer::FillJsonStats(json& jsonArray) const
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

		// Shm writing stats
	  jsonArray.emplace_back(json::value_t::object);
		this->FillShmWriterStats(jsonArray[2]);
	}


	void ShmConsumer::FillShmWriterStats(json& jsonObject) const
	{
		MS_TRACE();
		
		uint64_t nowMs = DepLibUV::GetTimeMs();
		RTC::RtpLostPktRateCounter* loss = const_cast<RTC::RtpLostPktRateCounter*>(&this->lostPktRateCounter);
		uint64_t totalRate = loss->GetTotalRate(nowMs);
		uint64_t lossRate = loss->GetLossRate(nowMs);

		RTC::RtpDataCounter* ptr       = const_cast<RTC::RtpDataCounter*>(&this->shmWriterCounter);
		RTC::RtpStreamRecv* recvStream = dynamic_cast<RTC::RtpStreamRecv*>(this->producerRtpStream);
		jsonObject["type"]             = "shm-writer-stats";
		jsonObject["packetCount"]      = ptr->GetPacketCount();
		jsonObject["byteCount"]        = ptr->GetBytes();
		jsonObject["bitrate"]          = ptr->GetBitrate(nowMs);
		jsonObject["packetLossRate"]   = totalRate != 0 ? (float)lossRate / (float)totalRate : 0.0f;
		jsonObject["recvJitter"]       = recvStream != nullptr ? recvStream->GetJitter() : 0;
	}
 
 
	void ShmConsumer::FillJsonScore(json& jsonObject) const
	{
		MS_TRACE();

		// NOTE: Hardcoded values
		jsonObject["score"]         = 10;
		jsonObject["producerScore"] = 10;
	}

	
	inline void ShmConsumer::OnIdleShmConsumer()
	{
		MS_TRACE();

		MS_WARN_TAG_LIVELYAPP(xcode, this->appData, "shm[%s] idle shm consumer", this->shmCtx->StreamName().c_str());

		Channel::ChannelNotifier::Emit(this->id, "idleshmconsumer");
	}

	void ShmConsumer::HandleRequest(Channel::ChannelRequest* request)
	{
		MS_TRACE();

		switch (request->methodId)
		{
			case Channel::ChannelRequest::MethodId::CONSUMER_REQUEST_KEY_FRAME:
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
		MS_DEBUG_TAG(rtp, "ShmConsumer's producerRtpStream is set up");
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

		if (!this->producerRtpStream || !this->producerRtpStream->GetSenderReportNtpMs() || !this->producerRtpStream->GetSenderReportTs())
		{
			MS_DEBUG_2TAGS_LIVELYAPP(rtcp, xcode, this->appData, "shm[%s] Producer stream failed to read SR RTCP msg", this->shmCtx->StreamName().c_str());
			return;
		}

		uint64_t lastSenderReportNtpMs = this->producerRtpStream->GetSenderReportNtpMs(); // NTP timestamp in last Sender Report (in ms)
		uint32_t lastSenderReportTs    = this->producerRtpStream->GetSenderReportTs();    // RTP timestamp in last Sender Report

		shmCtx->WriteRtcpSenderReportTs(lastSenderReportNtpMs, lastSenderReportTs, (this->GetKind() == RTC::Media::Kind::AUDIO) ? DepLibSfuShm::Media::AUDIO : DepLibSfuShm::Media::VIDEO);
	}


	void ShmConsumer::SendRtpPacket(RTC::RtpPacket* packet)
	{
		MS_TRACE();

		// Restart the shmIdleCheckTimer each time RTP packet arrived
		this->idle = false;
		if (this->shmIdleCheckTimer)
			this->shmIdleCheckTimer->Restart();

		if (!IsActive()) {
			MS_DEBUG_TAG(rtp, "consumer is inactive, ignore pkt");
			return;
		}

		auto payloadType = packet->GetPayloadType();

		// NOTE: This may happen if this Consumer supports just some codecs of those
		// in the corresponding Producer.
		if (this->supportedCodecPayloadTypes.find(payloadType) == this->supportedCodecPayloadTypes.end())
		{
			MS_WARN_TAG(rtp, "payload type not supported [payloadType:%" PRIu8 "]", payloadType);

			return;
		}

		// If we need to sync, support key frames but this pkt is not a key frame,
		// do not write it into shm,
		// might use it to check video orientation and config shm ssrc
		bool ignorePkt = false;
		bool isSyncPacket = this->syncRequired;

		if (this->syncRequired)
		{
			if(this->keyFrameSupported && !packet->IsKeyFrame())
			{
				ignorePkt = true;
			}
			else
			{
			// Whether this is the first packet after re-sync.
			// Sync sequence number and timestamp if required.
				if (packet->IsKeyFrame())
					MS_DEBUG_TAG(rtp, "sync key frame received");

				this->rtpSeqManager.Sync(packet->GetSequenceNumber() - 1);

				this->syncRequired = false;
			}
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

		// Check for video orientation changes and discover ssrc.
		if (VideoOrientationChanged(packet))
		{
				MS_DEBUG_2TAGS_LIVELYAPP(rtp, xcode, this->appData, "shm[%s] video orientation changed to %d in packet[ssrc:%" PRIu32 ", seq:%" PRIu16 ", ts:%" PRIu32 "]",
					this->shmCtx->StreamName().c_str(),
					this->rotation,
					packet->GetSsrc(),
					packet->GetSequenceNumber(),
					packet->GetTimestamp());
			
			shmCtx->WriteVideoOrientation(this->rotation);
		}

		// Update stats: received and missed packet counters
		lostPktRateCounter.Update(packet);

		// Done with this pkt
		if (ignorePkt)
		{
			MS_DEBUG_TAG(rtp, "need to sync but this is not keyframe, ignore packet [ssrc:%" PRIu32 ", seq:%" PRIu16 ", ts:%" PRIu32
				"] from original [seq:%" PRIu16 "]",
				packet->GetSsrc(),
				packet->GetSequenceNumber(),
				packet->GetTimestamp(),
				origSeq);

			if (this->GetKind() == Media::Kind::AUDIO)
				DepLibSfuShm::ShmCtx::bin_log_ctx.record.a_num_discarded_rtp_pkts += 1;
			else
				DepLibSfuShm::ShmCtx::bin_log_ctx.record.v_num_discarded_rtp_pkts += 1;

			return; 
		}

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

		// Process the packet. In case of shm writer this logic is still needed for NACKs
		if (this->rtpStream->ReceivePacket(packet))
		{
			// Send the packet.
			this->listener->OnConsumerSendRtpPacket(this, packet);

			// May emit 'trace' event.
			EmitTraceEventRtpAndKeyFrameTypes(packet);
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

		if (this->testNackEachMs != 0)
		{
			if (this->TestNACK(packet))
			{
				MS_DEBUG_TAG(rtp, "Pretend NACK ssrc:%" PRIu32 ", seq:%" PRIu16 " ts: %" PRIu32 " and wait for retransmission",
				packet->GetSsrc(), packet->GetSequenceNumber(), packet->GetTimestamp());
				return;
			}
		}

		// End of NACK test simulation
		if (shmCtx->CanWrite((this->GetKind() == RTC::Media::Kind::AUDIO) ? DepLibSfuShm::Media::AUDIO : DepLibSfuShm::Media::VIDEO))
		{
			this->WritePacketToShm(packet);

			// Increase transmission counter.
			this->shmWriterCounter.Update(packet);
		}
		else {
			MS_DEBUG_DEV("shm[%s] writer not ready, skip pkt [ssrc:%" PRIu32 ", seq:%" PRIu16 ", ts:%" PRIu32 "]",
				this->shmCtx->StreamName().c_str(),
				packet->GetSsrc(),
				packet->GetSequenceNumber(),
				packet->GetTimestamp());
		
			if (this->GetKind() == Media::Kind::AUDIO)
				shmCtx->bin_log_ctx.record.a_num_discarded_rtp_pkts += 1;
			else
				shmCtx->bin_log_ctx.record.v_num_discarded_rtp_pkts += 1;
		}

		// Restore packet fields.
		packet->SetSsrc(origSsrc);
		packet->SetSequenceNumber(origSeq);
	}


	bool ShmConsumer::TestNACK(RTC::RtpPacket* packet)
	{
		if (this->GetKind() != RTC::Media::Kind::VIDEO)
			return false; // not video

		if (this->testNackEachMs == 0)
			return false; // test disabled

		uint64_t nowTs = DepLibUV::GetTimeMs();
		if (nowTs - this->lastNACKTestTs < testNackEachMs)
			return false; // too soon

		this->lastNACKTestTs = nowTs;

		RtpStream::Params params;
		params.ssrc      = packet->GetSsrc();
		params.clockRate = 90000;
		params.useNack   = true;

		// Create a NACK item that request for all the packets.
		RTCP::FeedbackRtpNackPacket nackPacket(0, params.ssrc);
		auto* nackItem = new RTCP::FeedbackRtpNackItem(packet->GetSequenceNumber(), 0b0000000000000000);
		nackPacket.AddItem(nackItem);

		auto it = nackPacket.Begin();
		if (it != nackPacket.End())
		{
			RTC::RTCP::FeedbackRtpNackItem* item = *it;	
			MS_DEBUG_TAG(rtp,"NACK packet NOT EMPTY");
			item->Dump();		
		}
		else {
			MS_DEBUG_TAG(rtp,"NACK packet EMPTY");
		}

		this->ReceiveNack(&nackPacket);
		return true; 
	}
// End of NACK test simulation


	bool ShmConsumer::VideoOrientationChanged(RTC::RtpPacket* packet)
	{
		if (this->GetKind() != RTC::Media::Kind::VIDEO)
			return false;
		
		bool c;
		bool f;
		uint16_t r;

		// If it's the first read or value changed then true
		if (packet->ReadVideoOrientation(c, f, r))
		{
			if (!this->rotationDetected)
			{
				this->rotationDetected = true;
				this->rotation = r;
				return true;
			}
			
			if (r != this->rotation)
			{
				this->rotation = r;
				return true;
			}
		}
		return false;
	}


	void ShmConsumer::WritePacketToShm(RTC::RtpPacket* packet)
	{
		MS_TRACE();

		uint8_t const* pktdata = packet->GetData();
		uint8_t const* cdata   = packet->GetPayload();
		uint8_t* data          = const_cast<uint8_t*>(cdata);
		size_t len             = packet->GetPayloadLength();
		uint64_t ts            = static_cast<uint64_t>(packet->GetTimestamp());
		uint64_t seq           = static_cast<uint64_t>(packet->GetSequenceNumber());
		bool keyframe          = packet->IsKeyFrame();

		if (this->GetKind() == Media::Kind::AUDIO)
		{
			ts = shmCtx->AdjustAudioPktTs(ts);
			seq = shmCtx->AdjustAudioPktSeq(seq);
			
			// Audio NALUs are always simple
			shmCtx->WriteAudioRtpDataToShm(data, len, seq, ts);

			// Update last ts and seq even if we failed to write pkt
			shmCtx->UpdateRtpStats(seq, ts, DepLibSfuShm::Media::AUDIO);
		} // end of audio
		else
		{ // video
			ts = shmCtx->AdjustVideoPktTs(ts);
			seq = shmCtx->AdjustVideoPktSeq(seq);

			uint8_t nal = *(data) & 0x1F;
			uint8_t marker = *(pktdata + 1) & 0x80; // Marker bit indicates the last or the only NALU in this packet is the end of the picture data
			
			// Picture begins when timestamp is incremented. Different from the first fragment of pic data. For example:
			// RTP STAP-A pkt contains SPS and PPS NALUs, followed by RTP pkts each containing a fragment of the actual frame. ALl pkts have the same Ts.
			// In this case, SPS NALU begins the picture (full AnexB header 0x00000001 is added in front), the rest of NALUs have shorter 0x000001 AnnexB headers,
			// including the first fragmented piece of frame content.

			// Only if RTP packet arrived in order, we can tell if this is the beginning of a picture
			bool tsIncremented = (shmCtx->IsLastVideoSeqNotSet() || (ts > shmCtx->LastVideoTs()));
			bool seqIncremented = (shmCtx->IsLastVideoSeqNotSet() || (seq > shmCtx->LastVideoSeq()));

			// Single NALU packet
			if (nal >= 1 && nal <= 23)
			{
				if (len >= 1)
				{
					MS_DEBUG_DEV("shm[%s] single nal=%d len=%zu ts %" PRIu64 " seq %" PRIu64 " keyframe=%d newpic=%d marker=%d lastTs=%" PRIu64,
						this->shmCtx->StreamName().c_str(), nal, len, ts, seq, keyframe, tsIncremented, marker, shmCtx->LastVideoTs());

					shmCtx->WriteVideoRtpDataToShm( data, len, seq, ts, nal,
																					false, // not a fragment
																					true,  // is first fragment? n/a for complete NALU
																					true,  // is last fragment? n/a
																					(tsIncremented && seqIncremented), // picture beginning
																					(marker != 0), // picture ending
																					keyframe);
				}
				else{
					MS_WARN_TAG_LIVELYAPP(xcode, this->appData, "shm[%s] NALU data len < 1: %lu", this->shmCtx->StreamName().c_str(), len);
				}
			}
			else
			{	// Aggregated or fragmented NALUs
				switch (nal)
				{
					// Aggregation packet. Contains several NAL units in a single RTP packet. Cannot contain fragments of NALUs.
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
						bool beginpicture = (tsIncremented && seqIncremented);

						// Iterate NAL units.
						while (offset < len - 3) // 2 bytes of NALU size and min 1 byte of NALU data
						{
							uint16_t naluSize = Utils::Byte::Get2Bytes(data, offset);

							if ( offset + naluSize > len) {
								MS_WARN_TAG_LIVELYAPP(xcode, this->appData, "shm[%s] payload left to read from STAP-A is too short: %zu > %zu",
									this->shmCtx->StreamName().c_str(), offset + naluSize, len);
								break;
							}

							offset += 2; // skip over NALU size
							uint8_t subnal = *(data + offset) & 0x1F; // actual NAL type
							uint16_t chunksize = naluSize;

							MS_DEBUG_DEV("shm[%s] STAP-A: nal=%" PRIu8 " seq=%" PRIu64 " payloadlen=%" PRIu64 " nalulen=%" PRIu16 " chunklen=%" PRIu32 " ts=%" PRIu64 " lastTs=%" PRIu64 " keyframe=%d beginpicture=%d endpicture=%d",
														this->shmCtx->StreamName().c_str(), subnal, seq, len, naluSize, chunksize, ts,
														shmCtx->LastVideoTs(), keyframe, beginpicture, (marker != 0));

							shmCtx->WriteVideoRtpDataToShm(data + offset, chunksize, seq, ts, subnal,
																							false, // non-fragmented
																							true, // is first fragment? n/a
																							true, // is end fragment? n/a
																							beginpicture,
																							(marker != 0),
																							keyframe);
							offset += chunksize;
							beginpicture = 0; // all NALUs in aggregated pkt have same rtp ts, first one may be pic beginning, the rest certainly will not be
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
							MS_WARN_TAG_LIVELYAPP(xcode, this->appData, "shm[%s] FU-A payload too short", this->shmCtx->StreamName().c_str());
							break;
						}
						// Parse FU header octet
						uint8_t startBit = *(data + 1) & 0x80; // 1st bit indicates the starting fragment
						uint8_t endBit   = *(data + 1) & 0x40; // 2nd bit indicates the ending fragment
						uint8_t fuInd    = *(data) & 0xE0;     // 3 first bytes of FU indicator, use to compose NAL header for the beginning fragment 
						bool beginpicture, endpicture, startfragment, endfragment;
						
						// Last 5 bits in FU header subtype of FU unit, use to compose NAL header for the beginning fragment.
						// Sometimes packet->IsKeyFrame() may be 0 even if actually we have a key frame, so if (subnal == 5 (i.e. IDR) and startBit == 128) then this is a key frame
						uint8_t subnal   = *(data + 1) & 0x1F;
						if (subnal == 5)
							keyframe = true;

						// Skip over FU indicator byte
						size_t chunksize = len - 1;
						data += 1;
						
						beginpicture = (startBit == 128) && tsIncremented && seqIncremented; // if right after SPS and PPS then this is the first fragment but NOT the beginning of the picture i.e. 3 bytes of AnnexB
						startfragment = (startBit == 128);
						endfragment = (endBit == 64);
						endpicture = (endBit && marker) ? 1 : 0; // If endBit is true, marker is also true. TBD, if this is ever incorrect, then need to differentiate btw last fragment and ending frame data piece

						if (startBit == 128)
						{
							// Put NAL header in place of FU header
							data[0] = fuInd + subnal;
						}
						else {
							// If not the beginning fragment, skip over FU header byte
							chunksize -= 1;
							data += 1;
						}

						MS_DEBUG_DEV("shm[%s] FU-A nal=%" PRIu8 " seq=%" PRIu64 " len=%" PRIu64 " ts=%" PRIu64 " prev_ts=%" PRIu64 " keyframe=%d startBit=%" PRIu8 " endBit=%" PRIu8 " marker=%" PRIu8 " beginpicture=%d endpicture=%d",
							this->shmCtx->StreamName().c_str(), subnal, seq, chunksize, ts, shmCtx->LastVideoTs(), keyframe, startBit, endBit, marker, beginpicture, endpicture);
						shmCtx->WriteVideoRtpDataToShm(data, chunksize, seq, ts, subnal, 
																						true, 
																						startfragment, 
																						endfragment, 
																						beginpicture, 
																						endpicture,
																						keyframe);
						break;
					}
					case 25: // STAB-B
					case 26: // MTAP-16
					case 27: // MTAP-24
					case 29: // FU-B
					{
						MS_WARN_TAG_LIVELYAPP(xcode, this->appData, "shm[%s] Unsupported NALU type %u in video packet",
							this->shmCtx->StreamName().c_str(), nal);
						break;
					}
					default: // ignore the rest
					{
						MS_WARN_TAG_LIVELYAPP(xcode, this->appData, "shm[%s] unknown NALU type %u in video packet",
							this->shmCtx->StreamName().c_str(), nal);
						break;
					}
				}
			} // Aggregated or fragmented NALUs

			shmCtx->UpdateRtpStats(seq, ts, DepLibSfuShm::Media::VIDEO); // Update last ts and seq even if we failed to write pkt
		} // end of video
	}


	void ShmConsumer::GetRtcp(
	  RTC::RTCP::CompoundPacket* packet, RTC::RtpStreamSend* rtpStream, uint64_t nowMs)
	{
		MS_TRACE();

		MS_ASSERT(rtpStream == this->rtpStream, "RTP stream does not match");

		if (static_cast<float>((nowMs - this->lastRtcpSentTime) * 1.15) < this->maxRtcpInterval)
			return;

		auto* report = this->rtpStream->GetRtcpSenderReport(nowMs);

		if (!report)
			return;

		packet->AddSenderReport(report);

		// Build SDES chunk for this sender.
		auto* sdesChunk = this->rtpStream->GetRtcpSdesChunk();

		packet->AddSdesChunk(sdesChunk);

		this->lastRtcpSentTime = nowMs;
	}

	void ShmConsumer::NeedWorstRemoteFractionLost(
	  uint32_t /*mappedSsrc*/, uint8_t& worstRemoteFractionLost)
	{
		MS_TRACE();

		if (!IsActive())
			return;

		auto fractionLost = this->rtpStream->GetFractionLost();

		// If our fraction lost is worse than the given one, update it.
		if (fractionLost > worstRemoteFractionLost)
			worstRemoteFractionLost = fractionLost;
	}


	void ShmConsumer::ReceiveNack(RTC::RTCP::FeedbackRtpNackPacket* nackPacket)
	{
		MS_TRACE();

		if (!IsActive())
			return;

		// May emit 'trace' event.
		EmitTraceEventNackType();

		this->rtpStream->ReceiveNack(nackPacket);
	}


	void ShmConsumer::ReceiveKeyFrameRequest(
	  RTC::RTCP::FeedbackPs::MessageType messageType, uint32_t ssrc)
	{
		MS_TRACE();

		switch (messageType)
		{
			case RTC::RTCP::FeedbackPs::MessageType::PLI:
			{
				EmitTraceEventPliType(ssrc);

				break;
			}

			case RTC::RTCP::FeedbackPs::MessageType::FIR:
			{
				EmitTraceEventFirType(ssrc);

				break;
			}

			default:;
		}

		this->rtpStream->ReceiveKeyFrameRequest(messageType);

		if (IsActive())
			RequestKeyFrame();
	}


	void ShmConsumer::ReceiveRtcpReceiverReport(RTC::RTCP::ReceiverReport* report)
	{
		MS_TRACE();

		this->rtpStream->ReceiveRtcpReceiverReport(report);
	}


	uint32_t ShmConsumer::GetTransmissionRate(uint64_t nowMs)
	{
		MS_TRACE();

		//return 0u;
		if (!IsActive())
			return 0u;

		return this->rtpStream->GetBitrate(nowMs);
	}


	void ShmConsumer::UserOnTransportConnected()
	{
		MS_TRACE();

		this->syncRequired = true;

		if (this->shmIdleCheckTimer)
			this->shmIdleCheckTimer->Restart();

		if (IsActive())
			RequestKeyFrame();
	}


	void ShmConsumer::UserOnTransportDisconnected()
	{
		MS_TRACE();

		if (this->shmIdleCheckTimer)
			this->shmIdleCheckTimer->Stop();

		this->rtpStream->Pause();
	}

	void ShmConsumer::UserOnPaused()
	{
		MS_TRACE();

		if (this->shmIdleCheckTimer)
			this->shmIdleCheckTimer->Stop();

		this->rtpStream->Pause();
	}

	void ShmConsumer::UserOnResumed()
	{
		MS_TRACE();

		if (this->shmIdleCheckTimer)
			this->shmIdleCheckTimer->Restart();
		
		this->syncRequired = true;

		if (IsActive())
			RequestKeyFrame();
	}

	void ShmConsumer::CreateRtpStream()
	{
		MS_TRACE();

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

		// Always enable these for ShmConsumer
		params.useNack = true;
		params.usePli = true;
		params.useFir = true;

		// Create a RtpStreamSend for sending a single media stream.
		size_t bufferSize = params.useNack ? 600u : 0u;

		this->rtpStream = new RTC::RtpStreamSend(this, params, bufferSize);
		this->rtpStreams.push_back(this->rtpStream);

		// If the Consumer is paused, tell the RtpStreamSend.
		if (IsPaused() || IsProducerPaused())
			rtpStream->Pause();

		auto* rtxCodec = this->rtpParameters.GetRtxCodecForEncoding(encoding);

		if (rtxCodec && encoding.hasRtx)
			this->rtpStream->SetRtx(rtxCodec->payloadType, encoding.rtx.ssrc);
	}


	void ShmConsumer::RequestKeyFrame()
	{
		MS_TRACE();

		if (this->kind != RTC::Media::Kind::VIDEO)
			return;

		auto mappedSsrc = this->consumableRtpEncodings[0].ssrc;

		this->listener->OnConsumerKeyFrameRequested(this, mappedSsrc);
	}

	void ShmConsumer::OnNeedToSync()
	{
		MS_TRACE();

		if (this->kind != RTC::Media::Kind::VIDEO)
			return;

		MS_DEBUG_2TAGS_LIVELYAPP(rtp, xcode, this->appData, "shm[%s] writer needs kf: consumer.IsActive()=%d consumer.syncRequired=%d", 
			this->shmCtx->StreamName().c_str(), IsActive(), this->syncRequired);

		if (this->syncRequired)
			return; // we have already asked for a key frame and waiting, no need to re-request

		this->syncRequired = true;

		if (IsActive())
			RequestKeyFrame();
	}

	void ShmConsumer::OnRtpStreamRetransmitRtpPacket(RTC::RtpStreamSend* rtpStream, RTC::RtpPacket* packet)
	{
		MS_TRACE();

		this->listener->OnConsumerRetransmitRtpPacket(this, packet);

		// May emit 'trace' event.
		EmitTraceEventRtpAndKeyFrameTypes(packet, this->rtpStream->HasRtx());
	}

	inline void ShmConsumer::OnRtpStreamScore(
	  RTC::RtpStream* /*rtpStream*/, uint8_t /*score*/, uint8_t /*previousScore*/)
	{
		MS_TRACE();
	}

	uint8_t ShmConsumer::GetBitratePriority() const
	{
		MS_TRACE();

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
