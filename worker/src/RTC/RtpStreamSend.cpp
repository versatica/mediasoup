#define MS_CLASS "RTC::RtpStreamSend"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/RtpStreamSend.hpp"
#include "Logger.hpp"
#include "Utils.hpp"
#include "RTC/RtpDictionaries.hpp"
#include "RTC/SeqManager.hpp"

namespace RTC
{
	/* Static. */

	static constexpr size_t MaxBufferEntries{ 500u };
	static constexpr uint64_t CearBufferInterval{ 2000u }; // In ms.
	// 17: 16 bit mask + the initial sequence number.
	static constexpr size_t MaxRequestedPackets{ 17u };
	thread_local static std::vector<RTC::RtpStreamSend::RetransmissionItem*> RetransmissionContainer(
	  MaxRequestedPackets + 1);
	static constexpr uint32_t DefaultRtt{ 100u };

	/* Class Static. */

	const uint32_t RtpStreamSend::MaxRetransmissionDelayForVideoMs{ 2000u };
	const uint32_t RtpStreamSend::MaxRetransmissionDelayForAudioMs{ 1000u };

	/* Instance methods. */

	RtpStreamSend::RtpStreamSend(
	  RTC::RtpStreamSend::Listener* listener, RTC::RtpStream::Params& params, std::string& mid)
	  : RTC::RtpStream::RtpStream(listener, params, 10), retransmissionBuffer(MaxBufferEntries),
	    mid(mid)
	{
		MS_TRACE();

		if (this->params.useNack)
		{
			this->clearBufferPeriodicTimer = new Timer(this);
		}

		switch (params.mimeType.type)
		{
			case RTC::RtpCodecMimeType::Type::VIDEO:
			{
				this->maxRetransmissionDelayMs = RtpStreamSend::MaxRetransmissionDelayForVideoMs;

				if (this->params.useNack)
				{
					this->clearBufferPeriodicTimer->Start(RtpStreamSend::MaxRetransmissionDelayForVideoMs);
				}

				break;
			}

			case RTC::RtpCodecMimeType::Type::AUDIO:
			{
				this->maxRetransmissionDelayMs = RtpStreamSend::MaxRetransmissionDelayForAudioMs;

				if (this->params.useNack)
				{
					this->clearBufferPeriodicTimer->Start(RtpStreamSend::MaxRetransmissionDelayForAudioMs);
				}

				break;
			}

			case RTC::RtpCodecMimeType::Type::UNSET:
			{
				MS_ABORT("codec mimeType not set");
			}
		}
	}

	RtpStreamSend::~RtpStreamSend()
	{
		MS_TRACE();

		// Clear retransmission buffer.
		this->retransmissionBuffer.Clear();

		delete this->clearBufferPeriodicTimer;
	}

	void RtpStreamSend::FillJsonStats(json& jsonObject)
	{
		MS_TRACE();

		const uint64_t nowMs = DepLibUV::GetTimeMs();

		RTC::RtpStream::FillJsonStats(jsonObject);

		jsonObject["type"]        = "outbound-rtp";
		jsonObject["packetCount"] = this->transmissionCounter.GetPacketCount();
		jsonObject["byteCount"]   = this->transmissionCounter.GetBytes();
		jsonObject["bitrate"]     = this->transmissionCounter.GetBitrate(nowMs);
	}

	void RtpStreamSend::SetRtx(uint8_t payloadType, uint32_t ssrc)
	{
		MS_TRACE();

		RTC::RtpStream::SetRtx(payloadType, ssrc);

		this->rtxSeq = Utils::Crypto::GetRandomUInt(0u, 0xFFFF);
	}

	bool RtpStreamSend::ReceivePacket(RTC::RtpPacket* packet, std::shared_ptr<RTC::RtpPacket>& sharedPacket)
	{
		MS_TRACE();

		// Call the parent method.
		if (!RtpStream::ReceiveStreamPacket(packet))
		{
			return false;
		}

		// If NACK is enabled, store the packet into the buffer.
		if (this->params.useNack)
		{
			StorePacket(packet, sharedPacket);
		}

		// Increase transmission counter.
		this->transmissionCounter.Update(packet);

		return true;
	}

	void RtpStreamSend::ReceiveNack(RTC::RTCP::FeedbackRtpNackPacket* nackPacket)
	{
		MS_TRACE();

		this->nackCount++;

		for (auto it = nackPacket->Begin(); it != nackPacket->End(); ++it)
		{
			RTC::RTCP::FeedbackRtpNackItem* item = *it;

			this->nackPacketCount += item->CountRequestedPackets();

			FillRetransmissionContainer(item->GetPacketId(), item->GetLostPacketBitmask());

			for (auto* item : RetransmissionContainer)
			{
				if (!item)
				{
					break;
				}

				// Note that this is an already RTX encoded packet if RTX is used
				// (FillRetransmissionContainer() did it).
				auto packet = item->packet;

				// Retransmit the packet.
				static_cast<RTC::RtpStreamSend::Listener*>(this->listener)
				  ->OnRtpStreamRetransmitRtpPacket(this, packet.get());

				// Mark the packet as retransmitted.
				RTC::RtpStream::PacketRetransmitted(packet.get());

				// Mark the packet as repaired (only if this is the first retransmission).
				if (item->sentTimes == 1)
				{
					RTC::RtpStream::PacketRepaired(packet.get());
				}

				if (HasRtx())
				{
					// Restore the packet.
					packet->RtxDecode(RtpStream::GetPayloadType(), item->ssrc);
				}
			}
		}
	}

	void RtpStreamSend::ReceiveKeyFrameRequest(RTC::RTCP::FeedbackPs::MessageType messageType)
	{
		MS_TRACE();

		switch (messageType)
		{
			case RTC::RTCP::FeedbackPs::MessageType::PLI:
			{
				this->pliCount++;

				break;
			}

			case RTC::RTCP::FeedbackPs::MessageType::FIR:
			{
				this->firCount++;

				break;
			}

			default:;
		}
	}

	void RtpStreamSend::ReceiveRtcpReceiverReport(RTC::RTCP::ReceiverReport* report)
	{
		MS_TRACE();

		/* Calculate RTT. */

		// Get the NTP representation of the current timestamp.
		const uint64_t nowMs = DepLibUV::GetTimeMs();
		auto ntp             = Utils::Time::TimeMs2Ntp(nowMs);

		// Get the compact NTP representation of the current timestamp.
		uint32_t compactNtp = (ntp.seconds & 0x0000FFFF) << 16;

		compactNtp |= (ntp.fractions & 0xFFFF0000) >> 16;

		const uint32_t lastSr = report->GetLastSenderReport();
		const uint32_t dlsr   = report->GetDelaySinceLastSenderReport();

		// RTT in 1/2^16 second fractions.
		uint32_t rtt{ 0 };

		// If no Sender Report was received by the remote endpoint yet, ignore lastSr
		// and dlsr values in the Receiver Report.
		if (lastSr && dlsr && (compactNtp > dlsr + lastSr))
		{
			rtt = compactNtp - dlsr - lastSr;
		}

		// RTT in milliseconds.
		this->rtt = static_cast<float>(rtt >> 16) * 1000;
		this->rtt += (static_cast<float>(rtt & 0x0000FFFF) / 65536) * 1000;

		if (this->rtt > 0.0f)
		{
			this->hasRtt = true;
		}

		this->packetsLost  = report->GetTotalLost();
		this->fractionLost = report->GetFractionLost();

		// Update the score with the received RR.
		UpdateScore(report);
	}

	void RtpStreamSend::ReceiveRtcpXrReceiverReferenceTime(RTC::RTCP::ReceiverReferenceTime* report)
	{
		MS_TRACE();

		this->lastRrReceivedMs = DepLibUV::GetTimeMs();
		this->lastRrTimestamp  = report->GetNtpSec() << 16;
		this->lastRrTimestamp += report->GetNtpFrac() >> 16;
	}

	RTC::RTCP::SenderReport* RtpStreamSend::GetRtcpSenderReport(uint64_t nowMs)
	{
		MS_TRACE();

		if (this->transmissionCounter.GetPacketCount() == 0u)
		{
			return nullptr;
		}

		auto ntp     = Utils::Time::TimeMs2Ntp(nowMs);
		auto* report = new RTC::RTCP::SenderReport();

		// Calculate TS difference between now and maxPacketMs.
		auto diffMs = nowMs - this->maxPacketMs;
		auto diffTs = diffMs * GetClockRate() / 1000;

		report->SetSsrc(GetSsrc());
		report->SetPacketCount(this->transmissionCounter.GetPacketCount());
		report->SetOctetCount(this->transmissionCounter.GetBytes());
		report->SetNtpSec(ntp.seconds);
		report->SetNtpFrac(ntp.fractions);
		report->SetRtpTs(this->maxPacketTs + diffTs);

		// Update info about last Sender Report.
		this->lastSenderReportNtpMs = nowMs;
		this->lastSenderReportTs    = this->maxPacketTs + diffTs;

		return report;
	}

	RTC::RTCP::DelaySinceLastRr::SsrcInfo* RtpStreamSend::GetRtcpXrDelaySinceLastRr(uint64_t nowMs)
	{
		MS_TRACE();

		if (this->lastRrReceivedMs == 0u)
		{
			return nullptr;
		}

		// Get delay in milliseconds.
		auto delayMs = static_cast<uint32_t>(nowMs - this->lastRrReceivedMs);
		// Express delay in units of 1/65536 seconds.
		uint32_t dlrr = (delayMs / 1000) << 16;

		dlrr |= uint32_t{ (delayMs % 1000) * 65536 / 1000 };

		auto* ssrcInfo = new RTC::RTCP::DelaySinceLastRr::SsrcInfo();

		ssrcInfo->SetSsrc(GetSsrc());
		ssrcInfo->SetDelaySinceLastReceiverReport(dlrr);
		ssrcInfo->SetLastReceiverReport(this->lastRrTimestamp);

		return ssrcInfo;
	}

	RTC::RTCP::SdesChunk* RtpStreamSend::GetRtcpSdesChunk()
	{
		MS_TRACE();

		const auto& cname = GetCname();
		auto* sdesChunk   = new RTC::RTCP::SdesChunk(GetSsrc());
		auto* sdesItem =
		  new RTC::RTCP::SdesItem(RTC::RTCP::SdesItem::Type::CNAME, cname.size(), cname.c_str());

		sdesChunk->AddItem(sdesItem);

		return sdesChunk;
	}

	void RtpStreamSend::Pause()
	{
		MS_TRACE();

		// Clear retransmission buffer.
		this->retransmissionBuffer.Clear();
	}

	void RtpStreamSend::Resume()
	{
		MS_TRACE();
	}

	uint32_t RtpStreamSend::GetBitrate(
	  uint64_t /*nowMs*/, uint8_t /*spatialLayer*/, uint8_t /*temporalLayer*/)
	{
		MS_TRACE();

		MS_ABORT("invalid method call");
	}

	uint32_t RtpStreamSend::GetSpatialLayerBitrate(uint64_t /*nowMs*/, uint8_t /*spatialLayer*/)
	{
		MS_TRACE();

		MS_ABORT("invalid method call");
	}

	uint32_t RtpStreamSend::GetLayerBitrate(
	  uint64_t /*nowMs*/, uint8_t /*spatialLayer*/, uint8_t /*temporalLayer*/)
	{
		MS_TRACE();

		MS_ABORT("invalid method call");
	}

	void RtpStreamSend::StorePacket(RTC::RtpPacket* packet, std::shared_ptr<RTC::RtpPacket>& sharedPacket)
	{
		MS_TRACE();

		MS_DUMP(
		  "packet [seq:%" PRIu16 ", timestamp:%" PRIu32 ", now:%" PRIu64 "]",
		  packet->GetSequenceNumber(),
		  packet->GetTimestamp(),
		  DepLibUV::GetTimeMs());

		MS_ASSERT(
		  packet->GetSsrc() == this->params.ssrc, "RTP packet SSRC does not match the encodings SSRC");

		if (packet->GetSize() > RTC::MtuSize)
		{
			MS_WARN_TAG(
			  rtp,
			  "packet too big [ssrc:%" PRIu32 ", seq:%" PRIu16 ", size:%zu]",
			  packet->GetSsrc(),
			  packet->GetSequenceNumber(),
			  packet->GetSize());

			return;
		}

		// Check if RTP packet is too old to be stored.
		if (this->retransmissionBuffer.GetSize() > 0)
		{
			auto* newestItem = this->retransmissionBuffer.GetNewest();

			// Processing RTP packet is older than first one.
			if (RTC::SeqManager<uint32_t>::IsSeqLowerThan(packet->GetTimestamp(), newestItem->timestamp))
			{
				// Packet has lower sequence number than newest stored packet but its
				// timestamp is higher. This should not happen, so drop the packet.
				if (packet->GetTimestamp() > newestItem->timestamp)
				{
					MS_WARN_TAG(
					  rtp,
					  "packet has lower seq but higher timestamp than newest stored packet, dropping [ssrc:%" PRIu32
					  ", seq:%" PRIu16 ", timestamp:%" PRIu32 "]",
					  packet->GetSsrc(),
					  packet->GetSequenceNumber(),
					  packet->GetTimestamp());

					return;
				}

				const uint32_t diffTs{ newestItem->timestamp - packet->GetTimestamp() };

				// RTP packet is older than the max retransmission delay.
				if (static_cast<uint32_t>(diffTs * 1000 / this->params.clockRate) >= this->maxRetransmissionDelayMs)
				{
					return;
				}
			}
		}

		auto seq   = packet->GetSequenceNumber();
		auto* item = this->retransmissionBuffer.Get(seq);

		// The buffer item is already used. Check whether we should replace its
		// storage with the new packet or just ignore it (if duplicated packet).
		if (item)
		{
			if (packet->GetTimestamp() == item->timestamp)
			{
				return;
			}

			// Reset the stored item (decrease RTP packet shared pointer counter).
			item->Reset();
		}
		// Allocate new buffer item.
		else
		{
			// Allocate a new item.
			item = new RetransmissionItem();

			this->retransmissionBuffer.Insert(seq, item);
		}

		// Only clone once and only if necessary.
		if (!sharedPacket.get())
		{
			sharedPacket.reset(packet->Clone());
		}

		// Store original packet and some extra info into the item.
		item->packet         = sharedPacket;
		item->ssrc           = packet->GetSsrc();
		item->sequenceNumber = packet->GetSequenceNumber();
		item->timestamp      = packet->GetTimestamp();
		item->receivedAtMs   = DepLibUV::GetTimeMs();
	}

	void RtpStreamSend::ClearOldStoredPackets()
	{
		MS_TRACE();

		const uint64_t nowMs = DepLibUV::GetTimeMs();

		// Go through all buffer items starting with the first and free all items
		// that contain too old packets.
		for (size_t i{ 0u }; this->retransmissionBuffer.GetSize() > 0; ++i)
		{
			auto* item = this->retransmissionBuffer.GetOldest();

			// If current oldest stored packet is still valid for retransmission,
			// exit the loop since we know that stored packets after it are newer.
			//
			// TODO: This is not ok because here we are checking the received time
			// of the oldest packet in the buffer. However it may have been received
			// later than other newer packets.
			//
			// SOLUTION: We should read receivedAtMs of the newest stored packet, and
			// then use receivedAtMs and timestamp of the newest stored packet and
			// iterate the buffer from older to newer, remove those older than the
			// max retransmission delay and break as soon as current oldest packet
			// is still valid for retransmission.
			if (nowMs - item->receivedAtMs < this->maxRetransmissionDelayMs)
			{
				break;
			}

			this->retransmissionBuffer.RemoveOldest();
		}
	}

	// This method looks for the requested RTP packets and inserts them into the
	// RetransmissionContainer vector (and sets to null the next position).
	//
	// If RTX is used the stored packet will be RTX encoded now (if not already
	// encoded in a previous resend).
	void RtpStreamSend::FillRetransmissionContainer(uint16_t seq, uint16_t bitmask)
	{
		MS_TRACE();

		// Ensure the container's first element is 0.
		RetransmissionContainer[0] = nullptr;

		// If NACK is not supported, exit.
		if (!this->params.useNack)
		{
			MS_WARN_TAG(rtx, "NACK not supported");

			return;
		}

		// Look for each requested packet.
		const uint64_t nowMs = DepLibUV::GetTimeMs();
		const uint16_t rtt   = (this->rtt != 0u ? this->rtt : DefaultRtt);
		uint16_t currentSeq  = seq;
		bool requested{ true };
		size_t containerIdx{ 0 };

		// Variables for debugging.
		const uint16_t origBitmask = bitmask;
		uint16_t sentBitmask{ 0b0000000000000000 };
		bool isFirstPacket{ true };
		bool firstPacketSent{ false };
		uint8_t bitmaskCounter{ 0 };

		while (requested || bitmask != 0)
		{
			bool sent = false;

			if (requested)
			{
				auto* item = this->retransmissionBuffer.Get(currentSeq);
				std::shared_ptr<RTC::RtpPacket> packet{ nullptr };

				// Calculate the elapsed time between the max timestamp seen and the
				// requested packet's timestamp (in ms).
				if (item)
				{
					packet = item->packet;
					// Put correct info into the packet.
					packet->SetSsrc(item->ssrc);
					packet->SetSequenceNumber(item->sequenceNumber);
					packet->SetTimestamp(item->timestamp);

					// Update MID RTP extension value.
					if (!this->mid.empty())
					{
						packet->UpdateMid(mid);
					}
				}

				// Packet not found.
				if (!item)
				{
					// Do nothing.
				}
				// Don't resent the packet if it was resent in the last RTT ms.
				// clang-format off
				else if (
					item->resentAtMs != 0u &&
					nowMs - item->resentAtMs <= static_cast<uint64_t>(rtt)
				)
				// clang-format on
				{
					MS_DEBUG_TAG(
					  rtx,
					  "ignoring retransmission for a packet already resent in the last RTT ms "
					  "[seq:%" PRIu16 ", rtt:%" PRIu32 "]",
					  packet->GetSequenceNumber(),
					  rtt);
				}
				// Stored packet is valid for retransmission. Resend it.
				else
				{
					// If we use RTX and the packet has not yet been resent, encode it now.
					if (HasRtx())
					{
						// Increment RTX seq.
						++this->rtxSeq;

						packet->RtxEncode(this->params.rtxPayloadType, this->params.rtxSsrc, this->rtxSeq);
					}

					// Save when this packet was resent.
					item->resentAtMs = nowMs;

					// Increase the number of times this packet was sent.
					item->sentTimes++;

					// Store the item in the container and then increment its index.
					RetransmissionContainer[containerIdx++] = item;

					sent = true;

					if (isFirstPacket)
					{
						firstPacketSent = true;
					}
				}
			}

			requested = (bitmask & 1) != 0;
			bitmask >>= 1;
			++currentSeq;

			if (!isFirstPacket)
			{
				sentBitmask |= (sent ? 1 : 0) << bitmaskCounter;
				++bitmaskCounter;
			}
			else
			{
				isFirstPacket = false;
			}
		}

		// If not all the requested packets was sent, log it.
		if (!firstPacketSent || origBitmask != sentBitmask)
		{
			MS_WARN_DEV(
			  "could not resend all packets [seq:%" PRIu16
			  ", first:%s, "
			  "bitmask:" MS_UINT16_TO_BINARY_PATTERN ", sent bitmask:" MS_UINT16_TO_BINARY_PATTERN "]",
			  seq,
			  firstPacketSent ? "yes" : "no",
			  MS_UINT16_TO_BINARY(origBitmask),
			  MS_UINT16_TO_BINARY(sentBitmask));
		}
		else
		{
			MS_DEBUG_DEV(
			  "all packets resent [seq:%" PRIu16 ", bitmask:" MS_UINT16_TO_BINARY_PATTERN "]",
			  seq,
			  MS_UINT16_TO_BINARY(origBitmask));
		}

		// Set the next container element to null.
		RetransmissionContainer[containerIdx] = nullptr;
	}

	void RtpStreamSend::UpdateScore(RTC::RTCP::ReceiverReport* report)
	{
		MS_TRACE();

		// Calculate number of packets sent in this interval.
		auto totalSent = this->transmissionCounter.GetPacketCount();
		auto sent      = totalSent - this->sentPriorScore;

		this->sentPriorScore = totalSent;

		// Calculate number of packets lost in this interval.
		const uint32_t totalLost = report->GetTotalLost() > 0 ? report->GetTotalLost() : 0;
		uint32_t lost;

		if (totalLost < this->lostPriorScore)
		{
			lost = 0;
		}
		else
		{
			lost = totalLost - this->lostPriorScore;
		}

		this->lostPriorScore = totalLost;

		// Calculate number of packets repaired in this interval.
		auto totalRepaired = this->packetsRepaired;
		uint32_t repaired  = totalRepaired - this->repairedPriorScore;

		this->repairedPriorScore = totalRepaired;

		// Calculate number of packets retransmitted in this interval.
		auto totatRetransmitted      = this->packetsRetransmitted;
		const uint32_t retransmitted = totatRetransmitted - this->retransmittedPriorScore;

		this->retransmittedPriorScore = totatRetransmitted;

		// We didn't send any packet.
		if (sent == 0)
		{
			RTC::RtpStream::UpdateScore(10);

			return;
		}

		if (lost > sent)
		{
			lost = sent;
		}

		if (repaired > lost)
		{
			repaired = lost;
		}

#if MS_LOG_DEV_LEVEL == 3
		MS_DEBUG_TAG(
		  score,
		  "[totalSent:%zu, totalLost:%" PRIi32 ", totalRepaired:%zu",
		  totalSent,
		  totalLost,
		  totalRepaired);

		MS_DEBUG_TAG(
		  score,
		  "fixed values [sent:%zu, lost:%" PRIu32 ", repaired:%" PRIu32 ", retransmitted:%" PRIu32,
		  sent,
		  lost,
		  repaired,
		  retransmitted);
#endif

		auto repairedRatio  = static_cast<float>(repaired) / static_cast<float>(sent);
		auto repairedWeight = std::pow(1 / (repairedRatio + 1), 4);

		MS_ASSERT(retransmitted >= repaired, "repaired packets cannot be more than retransmitted ones");

		if (retransmitted > 0)
		{
			repairedWeight *= static_cast<float>(repaired) / retransmitted;
		}

		lost -= repaired * repairedWeight;

		auto deliveredRatio = static_cast<float>(sent - lost) / static_cast<float>(sent);
		auto score          = static_cast<uint8_t>(std::round(std::pow(deliveredRatio, 4) * 10));

#if MS_LOG_DEV_LEVEL == 3
		MS_DEBUG_TAG(
		  score,
		  "[deliveredRatio:%f, repairedRatio:%f, repairedWeight:%f, new lost:%" PRIu32 ", score:%" PRIu8
		  "]",
		  deliveredRatio,
		  repairedRatio,
		  repairedWeight,
		  lost,
		  score);
#endif

		RtpStream::UpdateScore(score);
	}

	inline void RtpStreamSend::OnTimer(Timer* timer)
	{
		MS_TRACE();

		if (timer == this->clearBufferPeriodicTimer)
		{
			ClearOldStoredPackets();
		}
	}

	RtpStreamSend::RetransmissionBuffer::RetransmissionBuffer(size_t maxEntries)
	  : maxEntries(maxEntries)
	{
		MS_TRACE();
	}

	RtpStreamSend::RetransmissionBuffer::~RetransmissionBuffer()
	{
		MS_TRACE();

		Clear();
	}

	size_t RtpStreamSend::RetransmissionBuffer::GetSize() const
	{
		MS_TRACE();

		return this->buffer.size();
	}

	RtpStreamSend::RetransmissionItem* RtpStreamSend::RetransmissionBuffer::GetOldest() const
	{
		MS_TRACE();

		return this->Get(this->startSeq);
	}

	RtpStreamSend::RetransmissionItem* RtpStreamSend::RetransmissionBuffer::GetNewest() const
	{
		MS_TRACE();

		return this->Get(this->startSeq + this->buffer.size() - 1);
	}

	RtpStreamSend::RetransmissionItem* RtpStreamSend::RetransmissionBuffer::Get(uint16_t seq) const
	{
		MS_TRACE();

		if (this->buffer.empty())
		{
			return nullptr;
		}

		if (RTC::SeqManager<uint16_t>::IsSeqLowerThan(seq, this->startSeq))
		{
			return nullptr;
		}

		auto idx{ static_cast<uint16_t>(seq - this->startSeq) };

		if (idx > static_cast<uint16_t>(this->buffer.size() - 1))
		{
			return nullptr;
		}

		auto* item = this->buffer.at(idx);

		if (item)
		{
			MS_ASSERT(item->packet, "obtained item does not contain original packet");
		}

		return item;
	}

	void RtpStreamSend::RetransmissionBuffer::RemoveOldest()
	{
		MS_TRACE();

		if (this->buffer.empty())
		{
			return;
		}

		auto* item = this->buffer.at(0);

		// Reset the stored item (decrease RTP packet shared pointer counter).
		item->Reset();

		delete item;

		this->buffer.pop_front();
		this->startSeq++;

		// Remove all nullptr elements from the beginning of the buffer.
		// NOTE: Calling front on an empty container is undefined.
		while (!this->buffer.empty() && this->buffer.front() == nullptr)
		{
			this->buffer.pop_front();
			this->startSeq++;
		}

		// If we emptied the full buffer, reset startSeq.
		if (this->buffer.empty())
		{
			this->startSeq = 0u;
		}
	}

	void RtpStreamSend::RetransmissionBuffer::Clear()
	{
		MS_TRACE();

		for (auto* item : this->buffer)
		{
			if (!item)
			{
				continue;
			}

			// Reset the stored item (decrease RTP packet shared pointer counter).
			item->Reset();

			delete item;
		}

		this->buffer.clear();
		this->startSeq = 0u;
	}

	void RtpStreamSend::RetransmissionBuffer::Insert(uint16_t seq, RetransmissionItem* item)
	{
		MS_TRACE();

		MS_DUMP("--- [seq:%" PRIu16 "]", seq);

		if (this->buffer.empty())
		{
			MS_DUMP("--- 1 [seq:%" PRIu16 "] buffer empty", seq);

			this->startSeq = seq;
			this->buffer.push_back(item);
		}
		// Packet sequence number is higher than startSeq.
		else if (RTC::SeqManager<uint16_t>::IsSeqHigherThan(seq, this->startSeq))
		{
			MS_DUMP(
			  "--- 2 [seq:%" PRIu16 "] IsSeqHigherThan(%" PRIu16 "%" PRIu16 ")", seq, seq, this->startSeq);

			auto idx{ static_cast<uint16_t>(seq - this->startSeq) };

			// Packet arrived out of order, so we already have a slot allocated for it.
			if (idx <= static_cast<uint16_t>(this->buffer.size() - 1))
			{
				MS_DUMP("--- 2.a [seq:%" PRIu16 "] packet out of order", seq);

				MS_ASSERT(this->buffer.at(idx) == nullptr, "must insert into empty slot");

				MS_DUMP("--- 2.a [seq:%" PRIu16 "] buffer[idx] = item", seq);

				this->buffer[idx] = item;
			}
			else
			{
				MS_DUMP("--- 2.b [seq:%" PRIu16 "] packet in order", seq);

				// Calculate how many elements would it be necessary to add when pushing new item
				// to the back of the deque.
				auto addToBack = static_cast<uint16_t>(seq - (this->startSeq + this->buffer.size() - 1));

				MS_DUMP("--- 2.b [seq:%" PRIu16 "] addToBack:%" PRIu16, seq, addToBack);

				// Packets can arrive out of order, add blank slots.
				for (uint16_t i{ 1 }; i < addToBack; ++i)
				{
					MS_DUMP("--- 2.b [seq:%" PRIu16 "] buffer.push_back(nullptr)", seq);

					this->buffer.push_back(nullptr);
				}

				MS_DUMP("--- 2.b [seq:%" PRIu16 "] buffer.push_back(item)", seq);

				this->buffer.push_back(item);
			}
		}
		// Packet sequence number is the same or lower than startSeq.
		else
		{
			MS_DUMP(
			  "--- 3 [seq:%" PRIu16 "] ! IsSeqHigherThan(%" PRIu16 ", %" PRIu16 ")", seq, seq, this->startSeq);

			// Calculate how many elements would it be necessary to add when pushing new item
			// to the front of the deque.
			auto addToFront = static_cast<uint16_t>(this->startSeq - seq);

			MS_DUMP("--- 3 [seq:%" PRIu16 "] addToFront:%" PRIu16, seq, addToFront);

			// Packets can arrive out of order, add blank slots.
			for (uint16_t i{ 1 }; i < addToFront; ++i)
			{
				MS_DUMP("--- 3 [seq:%" PRIu16 "] buffer.push_front(nullptr)", seq);

				this->buffer.push_front(nullptr);
			}

			MS_DUMP("--- 3 [seq:%" PRIu16 "] buffer.push_front(item)", seq);

			this->buffer.push_front(item);
			this->startSeq = seq;
		}

		MS_DUMP("--- 4 [seq:%" PRIu16 "] buffer.size:%zu", seq, this->buffer.size());

		MS_ASSERT(
		  this->buffer.size() <= this->maxEntries,
		  "RetransmissionBuffer contains more than %zu entries",
		  this->maxEntries);
	}

	void RtpStreamSend::RetransmissionItem::Reset()
	{
		MS_TRACE();

		this->packet.reset();
		this->ssrc           = 0u;
		this->sequenceNumber = 0u;
		this->timestamp      = 0u;
		this->receivedAtMs   = 0u;
		this->resentAtMs     = 0u;
		this->sentTimes      = 0u;
	}
} // namespace RTC
