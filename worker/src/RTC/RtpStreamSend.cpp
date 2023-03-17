#define MS_CLASS "RTC::RtpStreamSend"
// TODO: Comment.
#define MS_LOG_DEV_LEVEL 3

#include "RTC/RtpStreamSend.hpp"
#include "Logger.hpp"
#include "Utils.hpp"
#include "RTC/RtpDictionaries.hpp"
#include "RTC/SeqManager.hpp"

namespace RTC
{
	/* Static. */

	// Limit retransmission buffer max size to 2500 items.
	static constexpr size_t RetransmissionBufferMaxItems{ 2500u };
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
	  : RTC::RtpStream::RtpStream(listener, params, 10), mid(mid)
	{
		MS_TRACE();

		if (this->params.useNack)
		{
			uint32_t maxRetransmissionDelayMs;

			switch (params.mimeType.type)
			{
				case RTC::RtpCodecMimeType::Type::VIDEO:
				{
					maxRetransmissionDelayMs = RtpStreamSend::MaxRetransmissionDelayForVideoMs;

					break;
				}

				case RTC::RtpCodecMimeType::Type::AUDIO:
				{
					maxRetransmissionDelayMs = RtpStreamSend::MaxRetransmissionDelayForAudioMs;

					break;
				}

				case RTC::RtpCodecMimeType::Type::UNSET:
				{
					MS_ABORT("codec mimeType not set");
				}
			}

			this->retransmissionBuffer = new RetransmissionBuffer(
			  RetransmissionBufferMaxItems, maxRetransmissionDelayMs, params.clockRate);
		}
	}

	RtpStreamSend::~RtpStreamSend()
	{
		MS_TRACE();

		// Delete retransmission buffer.
		delete this->retransmissionBuffer;
		this->retransmissionBuffer = nullptr;
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

		MS_ASSERT(
		  packet->GetSsrc() == this->params.ssrc, "RTP packet SSRC does not match the encodings SSRC");

		// Call the parent method.
		if (!RtpStream::ReceiveStreamPacket(packet))
		{
			return false;
		}

		// If NACK is enabled, store the packet into the buffer.
		if (this->retransmissionBuffer)
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
		if (this->retransmissionBuffer)
		{
			this->retransmissionBuffer->Clear();
		}
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

		this->retransmissionBuffer->Insert(packet, sharedPacket);
	}

	// This method looks for the requested RTP packets and inserts them into the
	// RetransmissionContainer vector (and sets to null the next position).
	//
	// If RTX is used the stored packet will be RTX encoded now (if not already
	// encoded in a previous resend).
	void RtpStreamSend::FillRetransmissionContainer(uint16_t seq, uint16_t bitmask)
	{
		MS_TRACE();

		MS_ERROR_STD("1");

		// Ensure the container's first element is 0.
		RetransmissionContainer[0] = nullptr;

		// If NACK is not supported, exit.
		if (!this->retransmissionBuffer)
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

		MS_ERROR_STD("2");

		while (requested || bitmask != 0)
		{
			bool sent = false;

			if (requested)
			{
				MS_ERROR_STD("3 [currentSeq:%" PRIu16 "]", currentSeq);
				auto* item = this->retransmissionBuffer->Get(currentSeq);
				std::shared_ptr<RTC::RtpPacket> packet{ nullptr };

				// Calculate the elapsed time between the max timestamp seen and the
				// requested packet's timestamp (in ms).
				if (item)
				{
					MS_ERROR_STD(
					  "4 item [currentSeq:%" PRIu16 ", item->packet?:%s]",
					  currentSeq,
					  item->packet ? "yes" : "no");

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
					MS_ERROR_STD("4 no item [currentSeq:%" PRIu16 "]", currentSeq);
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

						MS_ERROR_STD("5 packet->RtxEncode() [currentSeq:%" PRIu16 "]", currentSeq);

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

	RtpStreamSend::RetransmissionBuffer::RetransmissionBuffer(
	  uint16_t maxItems, uint32_t maxRetransmissionDelayMs, uint32_t clockRate)
	  : maxItems(maxItems), maxRetransmissionDelayMs(maxRetransmissionDelayMs), clockRate(clockRate)
	{
		MS_TRACE();

		MS_ASSERT(maxItems > 0u, "maxItems must be greater than 0");
	}

	RtpStreamSend::RetransmissionBuffer::~RetransmissionBuffer()
	{
		MS_TRACE();

		Clear();
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

		auto idx = static_cast<uint16_t>(seq - this->startSeq);

		if (idx > static_cast<uint16_t>(this->buffer.size() - 1))
		{
			return nullptr;
		}

		return this->buffer.at(idx);
	}

	/**
	 * This method tries to insert given packet into the buffer. Here we assume
	 * that packet seq number is legitimate according to the content of the buffer.
	 * We discard the packet if too old and also discard it if its timestamp does
	 * not properly fit (by ensuring that elements in the buffer are not only
	 * ordered by increasing seq but also that their timestamp are incremental).
	 */
	void RtpStreamSend::RetransmissionBuffer::Insert(
	  RTC::RtpPacket* packet, std::shared_ptr<RTC::RtpPacket>& sharedPacket)
	{
		MS_TRACE();

		auto ssrc      = packet->GetSsrc();
		auto seq       = packet->GetSequenceNumber();
		auto timestamp = packet->GetTimestamp();

		MS_ERROR_STD(
		  "BEGIN | packet [seq:%" PRIu16 ", timestamp:%" PRIu32 ", buffer.size:%zu]",
		  seq,
		  timestamp,
		  this->buffer.size());

		// Buffer is empty, so just insert new item.
		if (this->buffer.empty())
		{
			MS_ERROR_STD(
			  "buffer empty [seq:%" PRIu16 ", timestamp:%" PRIu32 ", buffer.size:%zu]",
			  seq,
			  timestamp,
			  this->buffer.size());

			auto* item = new RetransmissionItem();

			this->buffer.push_back(FillItem(item, packet, sharedPacket));

			// Packet's seq number becomes startSeq.
			this->startSeq = seq;

			return;
		}

		// Clear too old packets in the buffer.
		ClearTooOld();

		auto* oldestItem = GetOldest();
		auto* newestItem = GetNewest();

		MS_ASSERT(oldestItem != nullptr, "oldest item doesn't exist");
		MS_ASSERT(newestItem != nullptr, "newest item doesn't exist");

		// Packet arrived in order (its seq is higher than seq of the newest stored
		// packet) so will become the newest one in the buffer.
		if (RTC::SeqManager<uint16_t>::IsSeqHigherThan(seq, newestItem->sequenceNumber))
		{
			MS_ERROR_STD(
			  "packet in order [seq:%" PRIu16 ", timestamp:%" PRIu32 ", buffer.size:%zu]",
			  seq,
			  timestamp,
			  this->buffer.size());

			// Ensure that the timestamp of the packet is equal or higher than the
			// timestamp of the newest stored packet.
			if (RTC::SeqManager<uint32_t>::IsSeqLowerThan(timestamp, newestItem->timestamp))
			{
				MS_WARN_TAG(
				  rtp,
				  "packet has higher seq but less timestamp than newest stored packet, discarding it [ssrc:%" PRIu32
				  ", seq:%" PRIu16 ", timestamp:%" PRIu32 "]",
				  ssrc,
				  seq,
				  timestamp);

				return;
			}

			// Calculate how many blank slots it would be necessary to add when
			// pushing new item to the back of the buffer.
			auto numBlankSlots = static_cast<uint16_t>(seq - newestItem->sequenceNumber - 1);

			// We may have to remove oldest items not to exceed the maximum size of
			// the buffer.
			if (this->buffer.size() + numBlankSlots + 1 > this->maxItems)
			{
				auto numItemsToRemove =
				  static_cast<uint16_t>(this->buffer.size() + numBlankSlots + 1 - this->maxItems);

				// If num of items to be removed exceed buffer size minus one (needed to
				// allocate current packet) then we must clear the entire buffer.
				if (numItemsToRemove > this->buffer.size() - 1)
				{
					MS_WARN_TAG(
					  rtp,
					  "packet has too high seq and forces buffer emptying [ssrc:%" PRIu32 ", seq:%" PRIu16
					  ", timestamp:%" PRIu32 "]",
					  ssrc,
					  seq,
					  timestamp);

					numBlankSlots = 0u;
					Clear();
				}
				else
				{
					MS_ERROR_STD(
					  "calling RemoveFromFrontAtLeast(%" PRIu16 ") [bufferSize:%zu, numBlankSlots:%" PRIu16
					  ", maxItems:%" PRIu16 "]",
					  numItemsToRemove,
					  this->buffer.size(),
					  numBlankSlots,
					  this->maxItems);

					RemoveFromFrontAtLeast(numItemsToRemove);
				}
			}

			// Push blank slots to the back.
			for (uint16_t i{ 0u }; i < numBlankSlots; ++i)
			{
				this->buffer.push_back(nullptr);
			}

			// Push the packet, which becomes the newest one in the buffer.
			auto* item = new RetransmissionItem();

			this->buffer.push_back(FillItem(item, packet, sharedPacket));
		}
		// Packet arrived out order and its seq is less than seq of the oldest
		// stored packet, so will become the oldest one in the buffer.
		else if (RTC::SeqManager<uint16_t>::IsSeqLowerThan(seq, oldestItem->sequenceNumber))
		{
			MS_ERROR_STD(
			  "packet out of order and oldest [seq:%" PRIu16 ", timestamp:%" PRIu32 ", buffer.size:%zu]",
			  seq,
			  timestamp,
			  this->buffer.size());

			// Ensure that packet is not too old to be stored.
			if (IsTooOld(timestamp, newestItem->timestamp))
			{
				return;
			}

			// Ensure that the timestamp of the packet is equal or less than the
			// timestamp of the oldest stored packet.
			if (RTC::SeqManager<uint32_t>::IsSeqHigherThan(timestamp, oldestItem->timestamp))
			{
				MS_WARN_TAG(
				  rtp,
				  "packet has less seq but higher timestamp than oldest stored packet, discarding it [ssrc:%" PRIu32
				  ", seq:%" PRIu16 ", timestamp:%" PRIu32 "]",
				  ssrc,
				  seq,
				  timestamp);

				return;
			}

			// Calculate how many blank slots it would be necessary to add when
			// pushing new item to the fton of the buffer.
			auto numBlankSlots = static_cast<uint16_t>(oldestItem->sequenceNumber - seq - 1);

			// If adding this packet (and needed blank slots) to the front makes the
			// buffer exceed its max size, discard this packet.
			if (this->buffer.size() + numBlankSlots + 1 > this->maxItems)
			{
				MS_WARN_TAG(
				  rtp,
				  "discarding received old packet to not exceed max buffer size [ssrc:%" PRIu32
				  ", seq:%" PRIu16 ", timestamp:%" PRIu32 "]",
				  ssrc,
				  seq,
				  timestamp);

				return;
			}

			// Push blank slots to the front.
			for (uint16_t i{ 0u }; i < numBlankSlots; ++i)
			{
				this->buffer.push_front(nullptr);
			}

			// Insert the packet, which becomes the oldest one in the buffer.
			auto* item = new RetransmissionItem();

			this->buffer.push_front(FillItem(item, packet, sharedPacket));

			// Packet's seq number becomes startSeq.
			this->startSeq = seq;
		}
		// Otherwise packet must be inserted between oldest and newest stored items
		// so there is already an allocated slot for it.
		else
		{
			MS_ERROR_STD(
			  "packet out of order and in between [seq:%" PRIu16 ", timestamp:%" PRIu32
			  ", buffer.size:%zu]",
			  seq,
			  timestamp,
			  this->buffer.size());

			// Let's check if an item already exist in same position. If so, assume
			// it's duplicated.
			auto* item = Get(seq);

			if (item)
			{
				return;
			}

			// idx is the intended position of the received packet in the buffer.
			auto idx = static_cast<uint16_t>(seq - this->startSeq);

			// Validate that packet timestamp is equal or higher than the timestamp of
			// the immediate older packet (if any).
			for (auto idx2 = static_cast<int32_t>(idx - 1); idx2 >= 0; --idx2)
			{
				auto* olderItem = this->buffer.at(idx2);

				// Blank slot, continue.
				if (!olderItem)
				{
					continue;
				}

				// We are done.
				if (timestamp >= olderItem->timestamp)
				{
					break;
				}
				else
				{
					MS_WARN_TAG(
					  rtp,
					  "packet timestamp is less than timestamp of immediate older packet in the buffer, discarding it [ssrc:%" PRIu32
					  ", seq:%" PRIu16 ", timestamp:%" PRIu32 "]",
					  ssrc,
					  seq,
					  timestamp);

					return;
				}
			}

			// Validate that packet timestamp is equal or less than the timestamp of
			// the immediate newer packet (if any).
			for (auto idx2 = static_cast<uint16_t>(idx + 1); idx2 < this->buffer.size(); ++idx2)
			{
				auto* newerItem = this->buffer.at(idx2);

				// Blank slot, continue.
				if (!newerItem)
				{
					continue;
				}

				// We are done.
				if (timestamp <= newerItem->timestamp)
				{
					break;
				}
				else
				{
					MS_WARN_TAG(
					  rtp,
					  "packet timestamp is higher than timestamp of immediate newer packet in the buffer, discarding it [ssrc:%" PRIu32
					  ", seq:%" PRIu16 ", timestamp:%" PRIu32 "]",
					  ssrc,
					  seq,
					  timestamp);

					return;
				}
			}

			// Store the packet.
			item = new RetransmissionItem();

			this->buffer[idx] = FillItem(item, packet, sharedPacket);
		}

#if MS_LOG_DEV_LEVEL == 3
		Dump();
#endif

		MS_ASSERT(
		  this->buffer.size() <= this->maxItems,
		  "RetransmissionBuffer contains %zu items (more than %" PRIu16 " max items)",
		  this->buffer.size(),
		  this->maxItems);
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

	void RtpStreamSend::RetransmissionBuffer::Dump() const
	{
		MS_TRACE();

		MS_DUMP("<RetransmissionBuffer>");
		MS_DUMP("  buffer [size:%zu, maxSize:%" PRIu16 "]", this->buffer.size(), this->maxItems);
		if (this->buffer.size() > 0)
		{
			const auto* oldestItem = GetOldest();
			const auto* newestItem = GetNewest();

			MS_DUMP(
			  "  oldest item [seq:%" PRIu16 ", timestamp:%" PRIu32 "]",
			  oldestItem->sequenceNumber,
			  oldestItem->timestamp);
			MS_DUMP(
			  "  newest item [seq:%" PRIu16 ", timestamp:%" PRIu32 "]",
			  newestItem->sequenceNumber,
			  newestItem->timestamp);
			MS_DUMP(
			  "  buffer window: %" PRIu32 "ms",
			  static_cast<uint32_t>(newestItem->timestamp * 1000 / this->clockRate) -
			    static_cast<uint32_t>(oldestItem->timestamp * 1000 / this->clockRate));
		}
		MS_DUMP("</RetransmissionBuffer>");
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

	// TODO: REMOVE PROBABLY
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

	void RtpStreamSend::RetransmissionBuffer::RemoveFromFrontAtLeast(uint16_t numItems)
	{
		MS_TRACE();

		MS_ASSERT(
		  numItems <= this->buffer.size(),
		  "attempting to remove more items than current buffer size [bufferSize:%zu, numItems:%" PRIu16
		  "]",
		  this->buffer.size(),
		  numItems);

		auto intendedBufferSize = this->buffer.size() - numItems;

		while (this->buffer.size() > intendedBufferSize)
		{
			RemoveOldest();
		}
	}

	void RtpStreamSend::RetransmissionBuffer::ClearTooOld()
	{
		MS_TRACE();

		const auto* newestItem = GetNewest();

		if (!newestItem)
		{
			return;
		}

		RtpStreamSend::RetransmissionItem* oldestItem{ nullptr };

		// Go through all buffer items starting with the first and free all items
		// that contain too old packets.
		while ((oldestItem = GetOldest()))
		{
			if (IsTooOld(oldestItem->timestamp, newestItem->timestamp))
			{
				RemoveOldest();
			}
			// If current oldest stored packet is not too old, exit the loop since we
			// know that packets stored after it are guaranteed to be newer.
			else
			{
				break;
			}
		}
	}

	bool RtpStreamSend::RetransmissionBuffer::IsTooOld(uint32_t timestamp, uint32_t newestTimestamp) const
	{
		MS_TRACE();

		if (RTC::SeqManager<uint32_t>::IsSeqHigherThan(timestamp, newestTimestamp))
		{
			return false;
		}

		const int64_t diffTs = newestTimestamp - timestamp;

		return static_cast<uint32_t>(diffTs * 1000 / this->clockRate) > this->maxRetransmissionDelayMs;
	}

	RtpStreamSend::RetransmissionItem* RtpStreamSend::RetransmissionBuffer::FillItem(
	  RtpStreamSend::RetransmissionItem* item,
	  RTC::RtpPacket* packet,
	  std::shared_ptr<RTC::RtpPacket>& sharedPacket) const
	{
		MS_TRACE();

		// Store original packet into the item. Only clone once and only if
		// necessary.
		// NOTE: This must be done BEFORE assigning item->packet = sharedPacket,
		// otherwise the value being copied in item->packet will remain nullptr.
		// This is because we are copying an **empty** shared_ptr into another
		// shared_ptr (item->packet), so future value assigned via reset() in the
		// former doesn't update the value in the copy.
		if (!sharedPacket.get())
		{
			sharedPacket.reset(packet->Clone());
		}

		// Store original packet and some extra info into the item.
		item->packet         = sharedPacket;
		item->ssrc           = packet->GetSsrc();
		item->sequenceNumber = packet->GetSequenceNumber();
		item->timestamp      = packet->GetTimestamp();

		return item;
	}

	void RtpStreamSend::RetransmissionItem::Reset()
	{
		MS_TRACE();

		this->packet.reset();
		this->ssrc           = 0u;
		this->sequenceNumber = 0u;
		this->timestamp      = 0u;
		this->resentAtMs     = 0u;
		this->sentTimes      = 0u;
	}
} // namespace RTC
