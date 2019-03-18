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

	static uint8_t RtxPacketBuffer[RTC::RtpBufferSize];
	// 17: 16 bit mask + the initial sequence number.
	static constexpr size_t MaxRequestedPackets{ 17 };
	static std::vector<RTC::RtpPacket*> RetransmissionContainer(MaxRequestedPackets + 1);
	// Don't retransmit packets older than this (ms).
	static constexpr uint32_t MaxRetransmissionDelay{ 2000 };
	static constexpr uint32_t DefaultRtt{ 100 };

	/* Instance methods. */

	RtpStreamSend::RtpStreamSend(
	  RTC::RtpStreamSend::Listener* listener, RTC::RtpStream::Params& params, size_t bufferSize)
	  : RTC::RtpStream::RtpStream(listener, params, 10), storage(bufferSize)
	{
		MS_TRACE();
	}

	RtpStreamSend::~RtpStreamSend()
	{
		MS_TRACE();

		// Clear the RTP buffer.
		ClearRetransmissionBuffer();
	}

	void RtpStreamSend::FillJsonStats(json& jsonObject)
	{
		MS_TRACE();

		RTC::RtpStream::FillJsonStats(jsonObject);

		jsonObject["type"]          = "outbound-rtp";
		jsonObject["roundTripTime"] = this->rtt;
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
			ClearRetransmissionBuffer();

		// If bufferSize was given, store the packet into the buffer.
		if (!this->storage.empty())
			StorePacket(packet);

		return true;
	}

	void RtpStreamSend::ReceiveNack(RTC::RTCP::FeedbackRtpNackPacket* nackPacket)
	{
		MS_TRACE();

		this->nackCount++;

		for (auto it = nackPacket->Begin(); it != nackPacket->End(); ++it)
		{
			RTC::RTCP::FeedbackRtpNackItem* item = *it;

			this->nackRtpPacketCount += item->CountRequestedPackets();

			FillRetransmissionContainer(item->GetPacketId(), item->GetLostPacketBitmask());

			auto it2 = RetransmissionContainer.begin();

			for (; it2 != RetransmissionContainer.end(); ++it2)
			{
				RTC::RtpPacket* packet = *it2;

				if (packet == nullptr)
					break;

				// Retransmit the packet.
				RetransmitPacket(packet);

				// Mark the packet as repaired.
				PacketRepaired(packet);
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

		ClearRetransmissionBuffer();
	}

	void RtpStreamSend::Resume()
	{
		MS_TRACE();
	}

	void RtpStreamSend::ClearRetransmissionBuffer()
	{
		MS_TRACE();

		// Delete cloned packets.
		for (auto& bufferItem : this->buffer)
		{
			delete bufferItem.packet;
		}

		// Clear list.
		this->buffer.clear();
	}

	void RtpStreamSend::RetransmitPacket(RTC::RtpPacket* packet)
	{
		MS_TRACE();

		RTC::RtpPacket* rtxPacket{ nullptr };

		if (HasRtx())
		{
			rtxPacket = packet->Clone(RtxPacketBuffer);

			rtxPacket->RtxEncode(this->params.rtxPayloadType, this->params.rtxSsrc, ++this->rtxSeq);

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

		// Send the packet.
		static_cast<RTC::RtpStreamSend::Listener*>(this->listener)
		  ->OnRtpStreamRetransmitRtpPacket(this, rtxPacket);

		// Delete the RTX RtpPacket if it was cloned.
		if (rtxPacket != packet)
			delete rtxPacket;

		// Mark the packet as retransmitted.
		PacketRetransmitted(packet);
	}

	void RtpStreamSend::StorePacket(RTC::RtpPacket* packet)
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

		// Sum the packet seq number and the number of 16 bits cycles.
		auto packetSeq = packet->GetSequenceNumber();
		BufferItem bufferItem;

		bufferItem.seq = packetSeq;

		// If empty do it easy.
		if (this->buffer.empty())
		{
			auto store = this->storage[0].store;

			bufferItem.packet = packet->Clone(store);
			this->buffer.push_back(bufferItem);

			return;
		}

		// Otherwise, do the stuff.

		std::list<BufferItem>::iterator newBufferIt;
		uint8_t* store{ nullptr };

		// Iterate the buffer in reverse order and find the proper place to store the
		// packet.
		auto bufferItReverse = this->buffer.rbegin();

		for (; bufferItReverse != this->buffer.rend(); ++bufferItReverse)
		{
			auto currentSeq = (*bufferItReverse).seq;

			if (RTC::SeqManager<uint16_t>::IsSeqHigherThan(packetSeq, currentSeq))
			{
				// Get a forward iterator pointing to the same element.
				auto it = bufferItReverse.base();

				newBufferIt = this->buffer.insert(it, bufferItem);

				break;
			}

			// Packet is already stored.
			if (packetSeq == currentSeq)
				return;
		}

		// The packet was older than anything in the buffer, store it at the beginning.
		if (bufferItReverse == this->buffer.rend())
			newBufferIt = this->buffer.insert(this->buffer.begin(), bufferItem);

		// If the buffer is not full use the next free storage item.
		if (this->buffer.size() - 1 < this->storage.size())
		{
			store = this->storage[this->buffer.size() - 1].store;
		}
		// Otherwise remove the first packet of the buffer and replace its storage area.
		else
		{
			auto& firstBufferItem = *(this->buffer.begin());
			auto firstPacket      = firstBufferItem.packet;

			// Store points to the store used by the first packet.
			store = const_cast<uint8_t*>(firstPacket->GetData());
			// Free the first packet.
			delete firstPacket;
			// Remove the first element in the list.
			this->buffer.pop_front();
		}

		// Update the new buffer item so it points to the cloned packed.
		(*newBufferIt).packet = packet->Clone(store);
	}

	// This method looks for the requested RTP packets and inserts them into the
	// RetransmissionContainer vector (and set to null the next position).
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

		// If the buffer is empty just return.
		if (this->buffer.empty())
			return;

		uint16_t firstSeq = seq;
		uint16_t lastSeq  = firstSeq + MaxRequestedPackets - 1;

		// Number of requested packets cannot be greater than the container size - 1.
		MS_ASSERT(
		  RetransmissionContainer.size() - 1 >= MaxRequestedPackets, "RtpPacket container is too small");

		auto bufferIt           = this->buffer.begin();
		auto bufferItReverse    = this->buffer.rbegin();
		uint16_t bufferFirstSeq = (*bufferIt).seq;
		uint16_t bufferLastSeq  = (*bufferItReverse).seq;

		// Requested packet range not found.
		if (
		  RTC::SeqManager<uint16_t>::IsSeqHigherThan(firstSeq, bufferLastSeq) ||
		  RTC::SeqManager<uint16_t>::IsSeqLowerThan(lastSeq, bufferFirstSeq))
		{
			MS_WARN_TAG(
			  rtx,
			  "requested packet range not in the buffer [seq:%" PRIu16 ", bufferFirstseq:%" PRIu16
			  ", bufferLastseq:%" PRIu16 "]",
			  seq,
			  bufferFirstSeq,
			  bufferLastSeq);

			return;
		}

		// Look for each requested packet.
		uint64_t now = DepLibUV::GetTime();
		uint16_t rtt = (this->rtt != 0u ? this->rtt : DefaultRtt);
		bool requested{ true };
		size_t containerIdx{ 0 };

		// Some variables for debugging.
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
				for (; bufferIt != this->buffer.end(); ++bufferIt)
				{
					auto currentSeq = (*bufferIt).seq;

					// Found.
					if (currentSeq == seq)
					{
						auto currentPacket = (*bufferIt).packet;
						// Calculate how the elapsed time between the max timestampt seen and
						// the requested packet's timestampt (in ms).
						uint32_t diffTs = this->maxPacketTs - currentPacket->GetTimestamp();
						uint32_t diffMs = diffTs * 1000 / this->params.clockRate;

						// Just provide the packet if no older than MaxRetransmissionDelay ms.
						if (diffMs > MaxRetransmissionDelay)
						{
							if (!tooOldPacketFound)
							{
								// TODO: May we ask for a key frame in this case?

								MS_WARN_TAG(
								  rtx,
								  "ignoring retransmission for too old packet "
								  "[seq:%" PRIu16 ", max age:%" PRIu32 "ms, packet age:%" PRIu32 "ms]",
								  currentPacket->GetSequenceNumber(),
								  MaxRetransmissionDelay,
								  diffMs);

								tooOldPacketFound = true;
							}

							break;
						}

						// Don't resent the packet if it was resent in the last RTT ms.
						auto resentAtTime = (*bufferIt).resentAtTime;

						if ((resentAtTime != 0u) && now - resentAtTime <= static_cast<uint64_t>(rtt))
						{
							MS_DEBUG_TAG(
							  rtx,
							  "ignoring retransmission for a packet already resent in the last RTT ms "
							  "[seq:%" PRIu16 ", rtt:%" PRIu32 "]",
							  currentPacket->GetSequenceNumber(),
							  rtt);

							break;
						}

						// Store the packet in the container and then increment its index.
						RetransmissionContainer[containerIdx++] = currentPacket;

						// Save when this packet was resent.
						(*bufferIt).resentAtTime = now;

						sent = true;

						if (isFirstPacket)
							firstPacketSent = true;

						break;
					}

					// It can not be after this packet.
					if (RTC::SeqManager<uint16_t>::IsSeqHigherThan(currentSeq, seq))
						break;
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

		// Calculate number of packets lost in the source in this interval.
		auto totalExpected = GetExpectedPackets();
		auto expected      = totalExpected - this->expectedPrior;

		// We didn't send new packets.
		if (expected == 0)
			return;

		this->expectedPrior = totalExpected;

		// TODO: REMOVE
		// std::cout << "RtpStreamSend::UpdateScore() 1: totalExpected:" << totalExpected << ",
		// expected:" << expected << "\n";

		auto totalSent       = this->transmissionCounter.GetPacketCount();
		auto totalSourceLost = totalExpected > totalSent ? totalExpected - totalSent : 0;

		// Current lost may be less now than before.
		if (totalSourceLost < this->sourceLostPrior)
			totalSourceLost = this->sourceLostPrior;

		auto sourceLost = totalSourceLost - this->sourceLostPrior;

		this->sourceLostPrior = totalSourceLost;

		// TODO: REMOVE
		// std::cout << "RtpStreamSend::UpdateScore() 2: totalSent:" << totalSent << ",
		// totalSourceLost:" << totalSourceLost << ", sourceLost:" << sourceLost << "\n";

		// Calculate number of packets lost in the edge in this interval.
		uint32_t totalLost = report->GetTotalLost();

		// Do not trust whatever the received RR says.
		if (totalLost < this->lostPrior)
			totalLost = this->lostPrior;

		auto lost = totalLost - this->lostPrior;

		this->lostPrior = totalLost;

		// TODO: REMOVE
		// std::cout << "RtpStreamSend::UpdateScore() 3: totalLost:" << totalLost << ", lost:" << lost << "\n";

		// Substract number of packets lost at the source.
		if (lost > sourceLost)
			lost = sourceLost;

		// TODO: REMOVE
		// std::cout << "RtpStreamSend::UpdateScore() 4: lost -= sourceLost:" << lost << "\n";

		// Calculate number of packets repaired in this interval.
		auto totalRepaired = this->packetsRepaired;
		uint32_t repaired  = totalRepaired - this->repairedPrior;

		// TODO: REMOVE
		// std::cout << "RtpStreamSend::UpdateScore() 5: totalRepaired:" << totalRepaired << ",
		// repaired:" << repaired << "\n";

		this->repairedPrior = totalRepaired;

		if (repaired >= lost)
			lost = 0;
		else
			lost -= repaired;

		// TODO: REMOVE
		// std::cout << "RtpStreamSend::UpdateScore() 6: lost:" << lost << "\n";

		if (repaired > lost)
		{
			MS_DEBUG_TAG(
			  score, "repaired greater than lost [repaired:%" PRIu32 ", lost:%" PRIu32 "]", repaired, lost);
		}

		// Calculate packet loss percentage in this interva.
		float lossPercentage = lost * 100 / expected;

		// TODO: REMOVE
		// std::cout << "RtpStreamSend::UpdateScore() 7: lossPercentage:" << lossPercentage << "\n";

		/*
		 * Calculate score. Starting from a score of 100:
		 *
		 * - Each loss porcentual point has a weight of 1.0f.
		 */
		float base100Score{ 100 };

		base100Score -= (lossPercentage * 1.0f);

		// TODO: REMOVE
		// std::cout << "RtpStreamSend::UpdateScore() 8: base100Score:" << base100Score << "\n";

		// Get base 10 score.
		auto score = static_cast<uint8_t>(std::lround(base100Score / 10));

		// TODO: REMOVE
		// std::cout << "RtpStreamSend::UpdateScore() 9: score:" << int{ score } << "\n";

#ifdef MS_LOG_DEV
		MS_DEBUG_TAG(
		  score,
		  "[totalExpected:%" PRIu32 ", totalSent:%zu, totalSourceLost:%zu, totalLost:%" PRIi32
		  ", totalRepaired:%zu, expected:%" PRIu32 ", repaired:%" PRIu32
		  ", lossPercentage:%f, score:%" PRIu8 "]",
		  totalExpected,
		  totalSent,
		  totalSourceLost,
		  totalLost,
		  totalRepaired,
		  expected,
		  repaired,
		  lossPercentage,
		  score);

		report->Dump();
#endif

		RtpStream::UpdateScore(score);
	}
} // namespace RTC
