#define MS_CLASS "RTC::NackGenerator"
// #define MS_LOG_DEV

#include "RTC/NackGenerator.hpp"
#include "DepLibUV.hpp"
#include "Logger.hpp"

namespace RTC
{
	constexpr uint32_t kMaxPacketAge = 10000;
	constexpr size_t kMaxNackPackets = 500;
	constexpr uint32_t kDefaultRttMs = 100;
	constexpr uint8_t kMaxNackRetries = 8;
	constexpr uint64_t kProcessIntervalMs = 25;

	/* Instance methods. */

	NackGenerator::NackGenerator(Listener* listener) :
		listener(listener),
		rtt(kDefaultRttMs)
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
			this->last_seq32 = seq32;
			this->started = true;

			// Start the periodic timer now.
			this->timer->Start(kProcessIntervalMs);

			return;
		}

		// Obviously never nacked, so ignore.
		if (seq32 == this->last_seq32)
		{
			return;
		}
		else if (seq32 == this->last_seq32 + 1)
		{
			this->last_seq32++;
			return;
		}

		// May be an out of order packet or a retransmitted packet (without RTX).
		if (seq32 < this->last_seq32)
		{
			auto it = this->nack_list.find(seq32);

			// It was a nacked packet.
			if (it != this->nack_list.end())
			{
				MS_DEBUG_TAG(rtx,
					"nacked packet received [ssrc:%" PRIu32 ", seq:%" PRIu16 "]",
					packet->GetSsrc(), packet->GetSequenceNumber());

				this->nack_list.erase(it);
			}
			// Out of order packet.
			else
			{
				MS_DEBUG_TAG(rtx,
					"out of order packet received [ssrc:%" PRIu32 ", seq:%" PRIu16 "]",
					packet->GetSsrc(), packet->GetSequenceNumber());
			}

			return;
		}

		// Otherwise we may have lost some packets.
		AddPacketsToNackList(this->last_seq32 + 1, seq32);
		this->last_seq32 = seq32;

		// Check if there are any nacks that are waiting for this seq number.
		std::vector<uint16_t> nack_batch = GetNackBatch(NackFilter::SEQ);

		if (!nack_batch.empty())
			this->listener->onNackRequired(nack_batch);
	}

	void NackGenerator::AddPacketsToNackList(uint32_t seq32_start, uint32_t seq32_end)
	{
		MS_TRACE();

		// Remove old packets.
		auto it = this->nack_list.lower_bound(seq32_end - kMaxPacketAge);

		this->nack_list.erase(this->nack_list.begin(), it);

		// If the nack list is too large, clear it and request a full frame.
		uint32_t num_new_nacks = seq32_end - seq32_start;

		if (this->nack_list.size() + num_new_nacks > kMaxNackPackets)
		{
			MS_DEBUG_TAG(rtx, "nack list too large, clearing it and requesting a full frame");

			this->nack_list.clear();
			this->listener->onFullFrameRequired();

			return;
		}

		for (uint32_t seq32 = seq32_start; seq32 != seq32_end; ++seq32)
		{
			NackInfo nack_info(seq32);

			MS_ASSERT(this->nack_list.find(seq32) == this->nack_list.end(), "packet already in the NACK list");

			this->nack_list[seq32] = nack_info;
		}
	}

	std::vector<uint16_t> NackGenerator::GetNackBatch(NackFilter filter)
	{
		uint64_t now = DepLibUV::GetTime();
		std::vector<uint16_t> nack_batch;
		auto it = this->nack_list.begin();

		while (it != this->nack_list.end())
		{
			NackInfo& nack_info = it->second;
			uint16_t seq = nack_info.seq32 % (1<<16);

			if (filter == NackFilter::SEQ && nack_info.sent_at_time == 0)
			{
				nack_info.retries++;
				nack_info.sent_at_time = now;

				if (nack_info.retries >= kMaxNackRetries)
				{
					MS_WARN_TAG(rtx,
						"sequence number removed from the NACK list due to max retries [seq:%" PRIu16 "]", seq);

					it = this->nack_list.erase(it);
				}
				else
				{
					nack_batch.emplace_back(seq);
					++it;
				}

				continue;
			}

			if (filter == NackFilter::TIME && nack_info.sent_at_time + this->rtt < now)
			{
				nack_info.retries++;
				nack_info.sent_at_time = now;

				if (nack_info.retries >= kMaxNackRetries)
				{
					MS_WARN_TAG(rtx,
						"sequence number removed from the NACK list due to max retries [seq:%" PRIu16 "]", seq);

					it = this->nack_list.erase(it);
				}
				else
				{
					nack_batch.emplace_back(seq);
					++it;
				}

				continue;
			}

			++it;
		}

		return nack_batch;
	}

	inline
	void NackGenerator::onTimer(Timer* timer)
	{
		MS_TRACE();

		std::vector<uint16_t> nack_batch = GetNackBatch(NackFilter::TIME);

		if (!nack_batch.empty())
			this->listener->onNackRequired(nack_batch);

		this->timer->Start(kProcessIntervalMs);
	}
}
