#define MS_CLASS "RTC::NackGenerator"
// #define MS_LOG_DEV

#include "RTC/NackGenerator.hpp"
#include "DepLibUV.hpp"
#include "Logger.hpp"
#include <utility> // std::make_pair()

namespace RTC
{
	/* Static. */

	constexpr size_t MaxPacketAge{ 5000 };
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
		delete this->timer;
	}

	// Returns true if this is a found nacked packet. False otherwise.
	bool NackGenerator::ReceivePacket(RTC::RtpPacket* packet)
	{
		MS_TRACE();

		uint16_t seq    = packet->GetSequenceNumber();
		bool isKeyFrame = packet->IsKeyFrame();

		if (!this->started)
		{
			this->lastSeq = seq;
			this->started = true;

			if (isKeyFrame)
				this->keyFrameList.insert(seq);

			return false;
		}

		// Obviously never nacked, so ignore.
		if (seq == this->lastSeq)
			return false;

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
			  "ignoring old packet not present in the NACK list [ssrc:%" PRIu32 ", seq:%" PRIu16 "]",
			  packet->GetSsrc(),
			  packet->GetSequenceNumber());

			return false;
		}

		// If we are here it means that we may have lost some packets so seq is
		// newer than the latest seq seen.

		CleanOldNackItems(seq);

		// If a key frame remove all items in the nack list older than our previous
		// key frame seq.
		if (isKeyFrame)
		{
			RemoveNackItemsUntilKeyFrame();

			this->keyFrameList.insert(seq);
		}

		// Expected seq number so nothing else to do.
		if (seq == this->lastSeq + 1)
		{
			this->lastSeq++;

			return false;
		}

		AddPacketsToNackList(this->lastSeq + 1, seq);

		// NackGenerator instance may have been reset.
		if (!this->started)
			return false;

		this->lastSeq = seq;

		// Check if there are any nacks that are waiting for this seq number.
		std::vector<uint16_t> nackBatch = GetNackBatch(NackFilter::SEQ);

		if (!nackBatch.empty())
			this->listener->OnNackGeneratorNackRequired(nackBatch);

		MayRunTimer();

		return false;
	}

	void NackGenerator::CleanOldNackItems(uint16_t seq)
	{
		MS_TRACE();

		auto it = this->nackList.lower_bound(seq - MaxPacketAge);

		this->nackList.erase(this->nackList.begin(), it);

		auto it2 = this->keyFrameList.lower_bound(seq - MaxPacketAge);

		this->keyFrameList.erase(this->keyFrameList.begin(), it2);
	}

	void NackGenerator::AddPacketsToNackList(uint16_t seqStart, uint16_t seqEnd)
	{
		MS_TRACE();

		// If the nack list is too large, clear it and request a key frame.
		uint16_t numNewNacks = seqEnd - seqStart;

		if (this->nackList.size() + numNewNacks > MaxNackPackets)
		{
			MS_DEBUG_TAG(
			  rtx,
			  "NACK list too large, clearing it and requesting a key frame [seqEnd:%" PRIu16 "]",
			  seqEnd);

			this->nackList.clear();
			this->keyFrameList.clear();
			this->listener->OnNackGeneratorKeyFrameRequired();

			return;
		}

		for (uint16_t seq = seqStart; seq != seqEnd; ++seq)
		{
			MS_ASSERT(this->nackList.find(seq) == this->nackList.end(), "packet already in the NACK list");

			// NOTE: We may not generate a NACK for this seq right now, but wait a bit
			// assuming that this packet may be in its way.
			// TODO: To be done.
			uint16_t sendAtSeq = seq + 0;

			this->nackList.emplace(std::make_pair(seq, NackInfo{ seq, sendAtSeq }));
		}
	}

	void NackGenerator::RemoveNackItemsUntilKeyFrame()
	{
		MS_TRACE();

		// No previous key frame, so do nothing.
		if (this->keyFrameList.empty())
			return;

		auto it               = this->keyFrameList.begin();
		auto seq              = *it;
		size_t numItemsBefore = this->nackList.size();
		auto it2              = this->nackList.lower_bound(seq);

		this->nackList.erase(this->nackList.begin(), it2);
		this->keyFrameList.erase(seq);

		size_t numItemsRemoved = numItemsBefore - this->nackList.size();

		if (numItemsRemoved > 0)
		{
			MS_DEBUG_TAG(
			  rtx,
			  "removed %zu old NACK items older than received key frame [seq:%" PRIu16 "]",
			  numItemsRemoved,
			  seq);
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

			if (
			  filter == NackFilter::SEQ && nackInfo.sentAtTime == 0 &&
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
