#define MS_CLASS "RTC::NackGenerator"
// #define MS_LOG_DEV

#include "RTC/NackGenerator.hpp"
#include "DepLibUV.hpp"
#include "Logger.hpp"

namespace RTC
{
	/* Static. */

	constexpr uint32_t MaxPacketAge{ 5000 };
	constexpr size_t MaxNackPackets{ 1000 };
	constexpr uint32_t DefaultRtt{ 100 };
	constexpr uint8_t MaxNackRetries{ 8 };
	constexpr uint64_t TimerInterval{ 50 };

	/* Instance methods. */

	NackGenerator::NackGenerator(Listener* listener) : listener(listener), rtt(DefaultRtt)
	{
		MS_TRACE();

		// Set the timer.
		this->timer = new Timer(this);
	}

	NackGenerator::~NackGenerator()
	{
		MS_TRACE();

		// Close the timer.
		this->timer->Destroy();
	}

	// Returns true if this is a found nacked packet. False otherwise.
	bool NackGenerator::ReceivePacket(RTC::RtpPacket* packet)
	{
		MS_TRACE();

		uint16_t seq = packet->GetSequenceNumber();

		if (!this->started)
		{
			this->lastSeq = seq;
			this->started   = true;

			return false;
		}

		// If a key frame remove all the items in the nack list older than this seq.
		if (packet->IsKeyFrame())
			RemoveFromNackListOlderThan(packet);

		// Obviously never nacked, so ignore.
		if (seq == this->lastSeq)
		{
			return false;
		}
		if (seq == this->lastSeq + 1)
		{
			this->lastSeq++;

			return false;
		}

		// May be an out of order packet, or already handled retransmitted packet,
		// or a retransmitted packet.
		if (SeqManager<uint16_t>::IsSeqLowerThan(seq, this->lastSeq))
		{
			auto it = this->nackList.find(seq);

			// It was a nacked packet.
			if (it != this->nackList.end())
			{
				MS_DEBUG_TAG(
				  rtx,
				  "NACKed packet received [ssrc:%" PRIu32 ", seq:%" PRIu16 "]",
				  packet->GetSsrc(),
				  packet->GetSequenceNumber());

				this->nackList.erase(it);

				return true;
			}

			// Out of order packet or already handled NACKed packet.
			MS_DEBUG_TAG(
			  rtx,
			  "ignoring old packet not present in the NACK list [ssrc:%" PRIu32
			  ", seq:%" PRIu16 "]",
			  packet->GetSsrc(),
			  packet->GetSequenceNumber());

			return false;
		}

		// Otherwise we may have lost some packets.
		AddPacketsToNackList(this->lastSeq + 1, seq);
		this->lastSeq = seq;

		// Check if there are any nacks that are waiting for this seq number.
		std::vector<uint16_t> nackBatch = GetNackBatch(NackFilter::SEQ);

		if (!nackBatch.empty())
			this->listener->OnNackGeneratorNackRequired(nackBatch);

		MayRunTimer();

		return false;
	}

	void NackGenerator::AddPacketsToNackList(uint16_t seqStart, uint16_t seqEnd)
	{
		MS_TRACE();

		if (seqEnd > MaxPacketAge)
		{
			uint32_t numItemsBefore = this->nackList.size();

			// Remove old packets.
			auto it = this->nackList.lower_bound(seqEnd - MaxPacketAge);

			this->nackList.erase(this->nackList.begin(), it);

			uint32_t numItemsRemoved = numItemsBefore - this->nackList.size();

			if (numItemsRemoved > 0)
			{
				MS_DEBUG_TAG(
				  rtx,
				  "removed %" PRIu32 " NACK items due to too old seq number [seqEnd:%" PRIu16 "]",
				  numItemsRemoved,
				  seqEnd);
			}
		}

		// If the nack list is too large, clear it and request a key frame.
		uint16_t numNewNacks = seqEnd - seqStart;

		if (this->nackList.size() + numNewNacks > MaxNackPackets)
		{
			MS_DEBUG_TAG(
			  rtx,
			  "NACK list too large, clearing it and requesting a key frame [seqEnd:%" PRIu16 "]",
			  seqEnd);

			this->nackList.clear();
			this->listener->OnNackGeneratorKeyFrameRequired();

			return;
		}

		for (uint16_t seq = seqStart; seq != seqEnd; ++seq)
		{
			// NOTE: Let the packet become out of order for a while without requesting
			// it into a NACK.
			// TODO: To be done.
			uint16_t sendAtSeq = seq + 0;
			NackInfo nackInfo(seq, sendAtSeq);

			MS_ASSERT(
			  this->nackList.find(seq) == this->nackList.end(), "packet already in the NACK list");

			this->nackList[seq] = nackInfo;
		}
	}

	// Delete all the entries in the NACK list whose key (seq) is older than
	// the given one.
	void NackGenerator::RemoveFromNackListOlderThan(RTC::RtpPacket* packet)
	{
		MS_TRACE();

		uint16_t seq          = packet->GetSequenceNumber();
		uint32_t numItemsBefore = this->nackList.size();

		auto it = this->nackList.lower_bound(seq);

		this->nackList.erase(this->nackList.begin(), it);

		uint32_t numItemsRemoved = numItemsBefore - this->nackList.size();

		if (numItemsRemoved > 0)
		{
			MS_DEBUG_TAG(
			  rtx,
			  "removed %" PRIu32 " old NACK items older than received key frame [seq:%" PRIu16 "]",
			  numItemsRemoved,
			  packet->GetSequenceNumber());
		}
	}

	std::vector<uint16_t> NackGenerator::GetNackBatch(NackFilter filter)
	{
		MS_TRACE();

		uint64_t now = DepLibUV::GetTime();
		std::vector<uint16_t> nackBatch;
		auto it = this->nackList.begin();

		while (it != this->nackList.end())
		{
			NackInfo& nackInfo = it->second;
			uint16_t seq       = nackInfo.seq;

			if (filter == NackFilter::SEQ && nackInfo.sentAtTime == 0 &&
				SeqManager<uint16_t>::IsSeqHigherThan(this->lastSeq, nackInfo.sendAtSeq))
			{
				nackInfo.retries++;
				nackInfo.sentAtTime = now;

				if (nackInfo.retries >= MaxNackRetries)
				{
					MS_WARN_TAG(
					  rtx,
					  "sequence number removed from the NACK list due to max retries [seq:%" PRIu16 "]",
					  seq);

					it = this->nackList.erase(it);
				}
				else
				{
					nackBatch.emplace_back(seq);
					++it;
				}

				continue;
			}

			if (filter == NackFilter::TIME && nackInfo.sentAtTime + this->rtt < now)
			{
				nackInfo.retries++;
				nackInfo.sentAtTime = now;

				if (nackInfo.retries >= MaxNackRetries)
				{
					MS_WARN_TAG(
					  rtx,
					  "sequence number removed from the NACK list due to max retries [seq:%" PRIu16 "]",
					  seq);

					it = this->nackList.erase(it);
				}
				else
				{
					nackBatch.emplace_back(seq);
					++it;
				}

				continue;
			}

			++it;
		}

		return nackBatch;
	}

	inline void NackGenerator::MayRunTimer() const
	{
		if (!this->nackList.empty())
			this->timer->Start(TimerInterval);
	}

	inline void NackGenerator::OnTimer(Timer* /*timer*/)
	{
		MS_TRACE();

		std::vector<uint16_t> nackBatch = GetNackBatch(NackFilter::TIME);

		if (!nackBatch.empty())
			this->listener->OnNackGeneratorNackRequired(nackBatch);

		MayRunTimer();
	}
} // namespace RTC
