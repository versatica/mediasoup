#define MS_CLASS "RTC::RtpStreamSend"
// #define MS_LOG_DEV

#include "RTC/RtpStreamSend.hpp"
#include "DepLibUV.hpp"
#include "Logger.hpp"
#include "Utils.hpp"
#include "RTC/SeqManager.hpp"

namespace RTC
{
	/* Static. */

	// 17: 16 bit mask + the initial sequence number.
	static constexpr size_t MaxRequestedPackets{ 17 };
	static std::vector<RTC::RtpStreamSend::StorageItem*> RetransmissionContainer(MaxRequestedPackets + 1);
	// Don't retransmit packets older than this (ms).
	static constexpr uint32_t MaxRetransmissionDelay{ 2000 };
	static constexpr uint32_t DefaultRtt{ 100 };

	/* Instance methods. */

	RtpStreamSend::RtpStreamSend(
	  RTC::RtpStreamSend::Listener* listener, RTC::RtpStream::Params& params, size_t bufferSize)
	  : RTC::RtpStream::RtpStream(listener, params, 10), buffer(bufferSize > 0 ? 65536 : 0, nullptr),
	    storage(bufferSize)
	{
		MS_TRACE();
	}

	RtpStreamSend::~RtpStreamSend()
	{
		MS_TRACE();

		// Clear the RTP buffer.
		ClearBuffer();
	}

	void RtpStreamSend::FillJsonStats(json& jsonObject)
	{
		MS_TRACE();

		uint64_t now = DepLibUV::GetTime();

		RTC::RtpStream::FillJsonStats(jsonObject);

		jsonObject["timestamp"]     = now;
		jsonObject["type"]          = "outbound-rtp";
		jsonObject["roundTripTime"] = this->rtt;
		jsonObject["packetCount"]   = this->transmissionCounter.GetPacketCount();
		jsonObject["byteCount"]     = this->transmissionCounter.GetBytes();
		jsonObject["bitrate"]       = this->transmissionCounter.GetBitrate(now);
	}

	bool RtpStreamSend::ReceivePacket(RTC::RtpPacket* packet)
	{
		MS_TRACE();

		// Call the parent method.
		if (!RtpStream::ReceivePacket(packet))
			return false;

		// If it's a key frame clear the RTP retransmission buffer to avoid
		// congesting the receiver by sending useless retransmissions (now that we
		// are sending a newer key frame).
		if (packet->IsKeyFrame())
			ClearBuffer();

		// If bufferSize was given, store the packet into the buffer.
		if (!this->storage.empty())
			StorePacket(packet);

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

			for (auto* storageItem : RetransmissionContainer)
			{
				if (storageItem == nullptr)
					break;

				// Note that this is an already RTX encoded packet if RTX is used
				// (FillRetransmissionContainer() did it).
				auto* packet = storageItem->packet;

				MS_ERROR("----- RETRANSMITTING PACKET seq:%" PRIu16, packet->GetSequenceNumber());

				// Retransmit the packet.
				static_cast<RTC::RtpStreamSend::Listener*>(this->listener)
				  ->OnRtpStreamRetransmitRtpPacket(this, packet);

				// Mark the packet as retransmitted.
				RTC::RtpStream::PacketRetransmitted(packet);

				// Mark the packet as repaired (only if this is the first retransmission).
				if (storageItem->sentTimes == 1)
					RTC::RtpStream::PacketRepaired(packet);
			}
		}
	}

	void RtpStreamSend::ReceiveKeyFrameRequest(RTC::RTCP::FeedbackPs::MessageType messageType)
	{
		MS_TRACE();

		switch (messageType)
		{
			case RTC::RTCP::FeedbackPs::MessageType::PLI:
				this->pliCount++;
				break;

			case RTC::RTCP::FeedbackPs::MessageType::FIR:
				this->firCount++;
				break;

			default:;
		}
	}

	void RtpStreamSend::ReceiveRtcpReceiverReport(RTC::RTCP::ReceiverReport* report)
	{
		MS_TRACE();

		/* Calculate RTT. */

		// Get the NTP representation of the current timestamp.
		uint64_t now = DepLibUV::GetTime();
		auto ntp     = Utils::Time::TimeMs2Ntp(now);

		// Get the compact NTP representation of the current timestamp.
		uint32_t compactNtp = (ntp.seconds & 0x0000FFFF) << 16;

		compactNtp |= (ntp.fractions & 0xFFFF0000) >> 16;

		uint32_t lastSr = report->GetLastSenderReport();
		uint32_t dlsr   = report->GetDelaySinceLastSenderReport();

		// RTT in 1/2^16 second fractions.
		uint32_t rtt{ 0 };

		if (compactNtp > dlsr + lastSr)
			rtt = compactNtp - dlsr - lastSr;
		else
			rtt = 0;

		// RTT in milliseconds.
		this->rtt = (rtt >> 16) * 1000;
		this->rtt += (static_cast<float>(rtt & 0x0000FFFF) / 65536) * 1000;

		this->packetsLost  = report->GetTotalLost();
		this->fractionLost = report->GetFractionLost();

		// Update the score with the received RR.
		UpdateScore(report);
	}

	RTC::RTCP::SenderReport* RtpStreamSend::GetRtcpSenderReport(uint64_t now)
	{
		MS_TRACE();

		if (this->transmissionCounter.GetPacketCount() == 0u)
			return nullptr;

		auto ntp    = Utils::Time::TimeMs2Ntp(now);
		auto report = new RTC::RTCP::SenderReport();

		report->SetSsrc(GetSsrc());
		report->SetPacketCount(this->transmissionCounter.GetPacketCount());
		report->SetOctetCount(this->transmissionCounter.GetBytes());
		report->SetRtpTs(this->maxPacketTs);
		report->SetNtpSec(ntp.seconds);
		report->SetNtpFrac(ntp.fractions);

		return report;
	}

	RTC::RTCP::SdesChunk* RtpStreamSend::GetRtcpSdesChunk()
	{
		MS_TRACE();

		auto& cname     = GetCname();
		auto* sdesChunk = new RTC::RTCP::SdesChunk(GetSsrc());
		auto* sdesItem =
		  new RTC::RTCP::SdesItem(RTC::RTCP::SdesItem::Type::CNAME, cname.size(), cname.c_str());

		sdesChunk->AddItem(sdesItem);

		return sdesChunk;
	}

	void RtpStreamSend::Pause()
	{
		MS_TRACE();

		ClearBuffer();
	}

	void RtpStreamSend::Resume()
	{
		MS_TRACE();
	}

	uint32_t RtpStreamSend::GetBitrate(uint64_t /*now*/, uint8_t /*spatialLayer*/, uint8_t /*temporalLayer*/)
	{
		MS_ABORT("invalid method call");
	}

	uint32_t RtpStreamSend::GetLayerBitrate(
	  uint64_t /*now*/, uint8_t /*spatialLayer*/, uint8_t /*temporalLayer*/)
	{
		MS_ABORT("invalid method call");
	}

	void RtpStreamSend::StorePacket(RTC::RtpPacket* packet)
	{
		MS_ERROR("<begin> seq:%" PRIu16, packet->GetSequenceNumber());

		MS_TRACE();

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

		auto seq          = packet->GetSequenceNumber();
		auto* storageItem = this->buffer[seq];

		// Buffer is empty.
		if (this->bufferSize == 0)
		{
			MS_ERROR("  --1-- seq:%" PRIu16 ", buffer empty, first item", packet->GetSequenceNumber());

			MS_ASSERT(this->bufferStartIdx == 0, "bufferStartIdx be 0");
			MS_ASSERT(storageItem == nullptr, "storageItem should be nullptr");

			// Take the first storage position.
			storageItem       = std::addressof(this->storage[0]);
			this->buffer[seq] = storageItem;

			// Increase buffer size and set start index.
			this->bufferSize++;
			this->bufferStartIdx = seq;
		}
		// The buffer item is already used. Check whether we should replace its
		// storage with the new packet or just ignore it (if duplicated packet).
		else if (storageItem)
		{
			MS_ERROR("  --2-- seq:%" PRIu16 ", buffer item used", packet->GetSequenceNumber());

			auto* storedPacket = storageItem->packet;

			if (packet->GetTimestamp() == storedPacket->GetTimestamp())
			{
				MS_ERROR("  --2.1-- seq:%" PRIu16 ", duplicated packet, ignore", packet->GetSequenceNumber());

				return;
			}

			MS_ERROR("  --2.2-- seq:%" PRIu16 ", replacing stored packet", packet->GetSequenceNumber());

			// Reset the storage item.
			ResetStorageItem(storageItem);

			// If this was the item referenced by the buffer start index, move it to
			// the next one.
			if (this->bufferStartIdx == seq)
				UpdateBufferStartIdx();
		}
		// Buffer not yet full, add an entry.
		else if (this->bufferSize < this->storage.size())
		{
			MS_ERROR("  --3-- seq:%" PRIu16 ", buffer not full yet", packet->GetSequenceNumber());

			// Take the next storage position.
			storageItem       = std::addressof(this->storage[this->bufferSize]);
			this->buffer[seq] = storageItem;

			// Increase buffer size.
			this->bufferSize++;
		}
		// Buffer full, remove oldest entry and add new one.
		else
		{
			MS_ERROR("  --4-- seq:%" PRIu16 ", buffer full, removing oldest entry", packet->GetSequenceNumber());

			auto* firstStorageItem = this->buffer[this->bufferStartIdx];

			// Reset the first storage item.
			ResetStorageItem(firstStorageItem);

			// Unfill the buffer start item.
			this->buffer[this->bufferStartIdx] = nullptr;

			// Move the buffer start index.
			UpdateBufferStartIdx();

			// Take the freed storage item.
			storageItem       = firstStorageItem;
			this->buffer[seq] = storageItem;
		}

		// TODO: TMP
		MS_ASSERT(storageItem, "upps 1");
		MS_ASSERT(storageItem->packet == nullptr, "upps 2");
		MS_ASSERT(storageItem->resentAtTime == 0, "upps 3");
		MS_ASSERT(storageItem->sentTimes == 0, "upps 4");
		MS_ASSERT(storageItem->rtxEncoded == false, "upps 5");

		// Clone the packet into the retrieved storage item.
		storageItem->packet = packet->Clone(storageItem->store);

		// TODO
		MS_ERROR(
			"<BUFFER DUMP> bufferStartIdx:%" PRIu16 ", bufferSize:%zu",
			this->bufferStartIdx, this->bufferSize);

		size_t bufferItemCount{ 0 };

		for (size_t seq{ 0 }; seq < this->buffer.size(); ++seq)
		{
			auto* storageItem = this->buffer[seq];

			if (storageItem)
			{
				// MS_ERROR(
				// 	"  - buffer item with storage item [seq:%zu, packet.seq:%" PRIu16 "]",
				// 	seq, storageItem->packet->GetSequenceNumber());

				bufferItemCount++;
			}
		}

		MS_ERROR("</BUFFER DUMP> items:%zu", bufferItemCount);

		MS_ERROR("<end> seq:%" PRIu16, packet->GetSequenceNumber());
	}

	void RtpStreamSend::ClearBuffer()
	{
		MS_TRACE();

		if (this->storage.empty())
			return;

		for (uint32_t idx{ 0 }; idx < this->buffer.size(); ++idx)
		{
			auto* storageItem = this->buffer[idx];

			if (!storageItem)
			{
				// TODO
				MS_ASSERT(this->buffer[idx] == nullptr, "key should be NULL");

				continue;
			}

			// Reset (free RTP packet) the storage item.
			ResetStorageItem(storageItem);

			// Unfill the buffer item.
			this->buffer[idx] = nullptr;
		}

		// Reset buffer.
		this->bufferStartIdx = 0;
		this->bufferSize     = 0;
	}

	inline void RtpStreamSend::ResetStorageItem(StorageItem* storageItem)
	{
		MS_TRACE();

		MS_ASSERT(storageItem, "storageItem cannot be nullptr");

		delete storageItem->packet;

		storageItem->packet       = nullptr;
		storageItem->resentAtTime = 0;
		storageItem->sentTimes    = 0;
		storageItem->rtxEncoded   = false;
	}

	/**
	 * Iterates the buffer starting from the current start idx + 1 until next
	 * used one. It takes into account that the buffer is circular.
	 */
	inline void RtpStreamSend::UpdateBufferStartIdx()
	{
		uint16_t seq = this->bufferStartIdx + 1;

		for (uint32_t idx{ 0 }; idx < this->buffer.size(); ++idx, ++seq)
		{
			auto* storageItem = this->buffer[seq];

			if (storageItem)
			{
				MS_ERROR("new bufferStartIdx: %" PRIu16, seq);

				this->bufferStartIdx = seq;

				break;
			}
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
		uint64_t now = DepLibUV::GetTime();
		uint16_t rtt = (this->rtt != 0u ? this->rtt : DefaultRtt);
		bool requested{ true };
		size_t containerIdx{ 0 };

		// Variables for debugging.
		uint16_t origBitmask = bitmask;
		uint16_t sentBitmask{ 0b0000000000000000 };
		bool isFirstPacket{ true };
		bool firstPacketSent{ false };
		uint8_t bitmaskCounter{ 0 };
		bool tooOldPacketFound{ false };

		while (requested || bitmask != 0)
		{
			bool sent = false;

			if (requested)
			{
				auto* storageItem = this->buffer[seq];
				RTC::RtpPacket* packet{ nullptr };
				uint32_t diffTs;
				uint32_t diffMs;

				// Calculate how the elapsed time between the max timestampt seen and
				// the requested packet's timestampt (in ms).
				if (storageItem)
				{
					packet = storageItem->packet;
					diffTs = this->maxPacketTs - packet->GetTimestamp();
					diffMs = diffTs * 1000 / this->params.clockRate;
				}

				// Packet not found.
				if (!packet)
				{
					// Do nothing.
				}
				// Don't resend the packet if older than MaxRetransmissionDelay ms.
				else if (diffMs > MaxRetransmissionDelay)
				{
					if (!tooOldPacketFound)
					{
						MS_WARN_TAG(
						  rtx,
						  "ignoring retransmission for too old packet "
						  "[seq:%" PRIu16 ", max age:%" PRIu32 "ms, packet age:%" PRIu32 "ms]",
						  packet->GetSequenceNumber(),
						  MaxRetransmissionDelay,
						  diffMs);

						tooOldPacketFound = true;
					}
				}
				// Don't resent the packet if it was resent in the last RTT ms.
				// clang-format off
				else if (
					storageItem->resentAtTime != 0u &&
					now - storageItem->resentAtTime <= static_cast<uint64_t>(rtt)
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
					// If we use RTX and the packet has not yet been resent, encode it
					// now.
					if (HasRtx() && !storageItem->rtxEncoded)
					{
						packet->RtxEncode(this->params.rtxPayloadType, this->params.rtxSsrc, ++this->rtxSeq);

						storageItem->rtxEncoded = true;
					}

					// Save when this packet was resent.
					storageItem->resentAtTime = now;

					// Increase the number of times this packet was sent.
					storageItem->sentTimes++;

					// Store the storage item in the container and then increment its index.
					RetransmissionContainer[containerIdx++] = storageItem;

					sent = true;

					if (isFirstPacket)
						firstPacketSent = true;
				}
			}

			requested = (bitmask & 1) != 0;
			bitmask >>= 1;
			++seq;

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
			MS_DEBUG_TAG(
			  rtx,
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
			MS_DEBUG_TAG(
			  rtx,
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
		auto sent      = totalSent - this->sentPrior;

		this->sentPrior = totalSent;

		// Calculate number of packets lost in this interval.
		uint32_t totalLost = report->GetTotalLost() > 0 ? report->GetTotalLost() : 0;
		uint32_t lost;

		if (totalLost < this->lostPrior)
			lost = 0;
		else
			lost = totalLost - this->lostPrior;

		this->lostPrior = totalLost;

		// Calculate number of packets repaired in this interval.
		auto totalRepaired = this->packetsRepaired;
		uint32_t repaired  = totalRepaired - this->repairedPrior;

		this->repairedPrior = totalRepaired;

		// Calculate number of packets retransmitted in this interval.
		auto totatRetransmitted = this->packetsRetransmitted;
		uint32_t retransmitted  = totatRetransmitted - this->retransmittedPrior;

		this->retransmittedPrior = totatRetransmitted;

		// We didn't send any packet.
		if (sent == 0)
		{
			RTC::RtpStream::UpdateScore(10);

			return;
		}

		if (lost > sent)
			lost = sent;

		if (repaired > lost)
		{
			if (HasRtx())
			{
				repaired = lost;
				retransmitted -= repaired - lost;
			}
			else
			{
				lost = repaired;
			}
		}

#ifdef MS_LOG_DEV
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

		float repairedRatio = static_cast<float>(repaired) / static_cast<float>(sent);
		auto repairedWeight = std::pow(1 / (repairedRatio + 1), 4);

		MS_ASSERT(retransmitted >= repaired, "repaired packets cannot be more than retransmitted ones");

		if (retransmitted > 0)
			repairedWeight *= repaired / retransmitted;

		lost -= repaired * repairedWeight;

		float deliveredRatio = static_cast<float>(sent - lost) / static_cast<float>(sent);
		auto score           = std::round(std::pow(deliveredRatio, 4) * 10);

#ifdef MS_LOG_DEV
		MS_DEBUG_TAG(
		  score,
		  "[deliveredRatio:%f, repairedRatio:%f, repairedWeight:%f, new lost:%" PRIu32 ", score: %lf]",
		  deliveredRatio,
		  repairedRatio,
		  repairedWeight,
		  lost,
		  score);
#endif

		RtpStream::UpdateScore(score);
	}
} // namespace RTC
