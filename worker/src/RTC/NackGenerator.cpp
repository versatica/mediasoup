#define MS_CLASS "RTC::NackGenerator"
// #define MS_LOG_DEV

#include "RTC/NackGenerator.hpp"
#include "DepLibUV.hpp"
#include "Logger.hpp"

namespace RTC
{
	/* Static. */

	constexpr uint32_t MaxPacketAge  = 10000;
	constexpr size_t MaxNackPackets  = 500;
	constexpr uint32_t DefaultRtt    = 100;
	constexpr uint8_t MaxNackRetries = 8;
	constexpr uint64_t TimerInterval = 25;

	/* Instance methods. */

	NackGenerator::NackGenerator(Listener* listener)
	    : listener(listener)
	    , rtt(DefaultRtt)
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

	void NackGenerator::ReceivePacket(RTC::RtpPacket* packet)
	{
		MS_TRACE();

		uint32_t seq32 = packet->GetExtendedSequenceNumber();

		if (!this->started)
		{
			this->lastSeq32 = seq32;
			this->started   = true;

			return;
		}

		// Obviously never nacked, so ignore.
		if (seq32 == this->lastSeq32)
		{
			return;
		}
		else if (seq32 == this->lastSeq32 + 1)
		{
			this->lastSeq32++;

			return;
		}

		// May be an out of order packet or a retransmitted packet (without RTX).
		if (seq32 < this->lastSeq32)
		{
			auto it = this->nackList.find(seq32);

			// It was a nacked packet.
			if (it != this->nackList.end())
			{
				MS_DEBUG_TAG(
				    rtx,
				    "nacked packet received [ssrc:%" PRIu32 ", seq:%" PRIu16 "]",
				    packet->GetSsrc(),
				    packet->GetSequenceNumber());

				this->nackList.erase(it);
			}
			// Out of order packet.
			else
			{
				MS_DEBUG_TAG(
				    rtx,
				    "out of order packet received [ssrc:%" PRIu32 ", seq:%" PRIu16 "]",
				    packet->GetSsrc(),
				    packet->GetSequenceNumber());
			}

			return;
		}

		// Otherwise we may have lost some packets.
		AddPacketsToNackList(this->lastSeq32 + 1, seq32);
		this->lastSeq32 = seq32;

		// Check if there are any nacks that are waiting for this seq number.
		std::vector<uint16_t> nackBatch = GetNackBatch(NackFilter::SEQ);

		if (!nackBatch.empty())
			this->listener->onNackRequired(nackBatch);

		MayRunTimer();
	}

	void NackGenerator::AddPacketsToNackList(uint32_t seq32Start, uint32_t seq32End)
	{
		MS_TRACE();

		// Remove old packets.
		auto it = this->nackList.lower_bound(seq32End - MaxPacketAge);

		this->nackList.erase(this->nackList.begin(), it);

		// If the nack list is too large, clear it and request a full frame.
		uint32_t numNewNacks = seq32End - seq32Start;

		if (this->nackList.size() + numNewNacks > MaxNackPackets)
		{
			MS_DEBUG_TAG(rtx, "nack list too large, clearing it and requesting a full frame");

			this->nackList.clear();
			this->listener->onFullFrameRequired();

			return;
		}

		for (uint32_t seq32 = seq32Start; seq32 != seq32End; ++seq32)
		{
			// NOTE: Let the packet become out of order for a while without requesting
			// it into a NACK.
			// TODO: To be done.
			uint32_t sendAtSeqNum = seq32 + 0;

			NackInfo nackInfo(seq32, sendAtSeqNum);

			MS_ASSERT(
			    this->nackList.find(seq32) == this->nackList.end(), "packet already in the NACK list");

			this->nackList[seq32] = nackInfo;
		}
	}

	std::vector<uint16_t> NackGenerator::GetNackBatch(NackFilter filter)
	{
		uint64_t now = DepLibUV::GetTime();
		std::vector<uint16_t> nackBatch;
		auto it = this->nackList.begin();

		while (it != this->nackList.end())
		{
			NackInfo& nackInfo = it->second;
			uint16_t seq       = nackInfo.seq32 % (1 << 16);

			if (filter == NackFilter::SEQ && nackInfo.sentAtTime == 0 &&
			    this->lastSeq32 >= nackInfo.sendAtSeqNum)
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
		if (this->nackList.size() > 0)
			this->timer->Start(TimerInterval);
	}

	inline void NackGenerator::onTimer(Timer* timer)
	{
		MS_TRACE();

		std::vector<uint16_t> nackBatch = GetNackBatch(NackFilter::TIME);

		if (!nackBatch.empty())
			this->listener->onNackRequired(nackBatch);

		MayRunTimer();
	}
}
