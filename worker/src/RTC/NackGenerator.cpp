#define MS_CLASS "RTC::NackGenerator"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/NackGenerator.hpp"
#include "Logger.hpp"
#if MS_LOG_DEV_LEVEL == 3
#include <iterator> // std::ostream_iterator
#include <sstream>  // std::ostringstream
#endif

namespace RTC
{
	/* Static. */

	static constexpr size_t MaxPacketAge{ 10000 };
	static constexpr size_t MaxNackPackets{ 1000 };
	static constexpr uint8_t MaxNackRetries{ 10 };
	static constexpr int64_t TimerIntervalMs{ 40 };

	/* Instance methods. */

	NackGenerator::NackGenerator(Listener* listener, SharedInterface* shared, int64_t sendNackDelayMs)
	  : listener(listener),
	    shared(shared),
	    sendNackDelayMs(sendNackDelayMs),
	    timer(shared->CreateTimer(this, "nack-generator"))
	{
		MS_TRACE();
	}

	NackGenerator::~NackGenerator()
	{
		MS_TRACE();
	}

	/**
	 * Returns true if this is a found nacked packet. False otherwise.
	 *
	 * @remarks
	 * - It also returns true if packet comes via RTX and contains a sequence
	 *   number higher than the highest seen.
	 */
	bool NackGenerator::ReceivePacket(const RTC::RTP::Packet* packet, bool isRecovered)
	{
		MS_TRACE();

		const uint16_t seq    = packet->GetSequenceNumber();
		const bool isKeyFrame = packet->IsKeyFrame();

		if (!this->started)
		{
			// Do not start with a retransmission, whose sequence number may be far
			// behind the current one.
			if (isRecovered || this->requestedList.contains(seq))
			{
				return false;
			}

			this->started = true;
			this->lastSeq = seq;

			if (isKeyFrame)
			{
				this->keyFrameList.insert(seq);
			}

			return false;
		}

		// Obviously never nacked, so ignore.
		if (seq == this->lastSeq)
		{
			return false;
		}

		// May be an out of order packet, or already handled retransmitted packet,
		// or a retransmitted packet.
		if (SeqManager<uint16_t>::IsSeqLowerThan(seq, this->lastSeq))
		{
			// It was a nacked packet.
			if (this->nackList.erase(seq) != 0)
			{
				// Remember it, so any later copy of it is known to be a retransmission.
				this->requestedList.insert(seq);

				// Remove old ones so we don't accumulate requested packets.
				const auto it = this->requestedList.lower_bound(this->lastSeq - MaxPacketAge);

				if (it != this->requestedList.begin())
				{
					this->requestedList.erase(this->requestedList.begin(), it);
				}

				MS_DEBUG_DEV(
				  "NACKed packet received [ssrc:%" PRIu32 ", seq:%" PRIu16 ", recovered:%s]",
				  packet->GetSsrc(),
				  packet->GetSequenceNumber(),
				  isRecovered ? "true" : "false");

				// NOTE: Accept the packet since it was in the `nackList`, regardless
				// the NACK requesting this packet was not sent yet (this is, if
				// nackInfo.retries == 0) because we would request it later anyway.

				return true;
			}

			// Out of order packet or already handled NACKed packet.
			if (!isRecovered)
			{
				MS_WARN_DEV(
				  "ignoring older packet not present in the NACK list [ssrc:%" PRIu32 ", seq:%" PRIu16 "]",
				  packet->GetSsrc(),
				  packet->GetSequenceNumber());
			}

			return false;
		}

		// If we are here it means that we may have lost some packets so seq is
		// newer than the latest seq seen.

		if (isKeyFrame)
		{
			this->keyFrameList.insert(seq);
		}

		// Remove old keyframes.
		{
			const auto it = this->keyFrameList.lower_bound(seq - MaxPacketAge);

			if (it != this->keyFrameList.begin())
			{
				this->keyFrameList.erase(this->keyFrameList.begin(), it);
			}
		}

		if (isRecovered)
		{
			const bool inserted = this->recoveredList.insert(seq).second;

			// Packet already recovered, ignore it.
			if (!inserted)
			{
				return false;
			}

			// Remove old ones so we don't accumulate recovered packets.
			const auto it = this->recoveredList.lower_bound(seq - MaxPacketAge);

			if (it != this->recoveredList.begin())
			{
				this->recoveredList.erase(this->recoveredList.begin(), it);
			}

			// NOTE: It may happen that this packet received via RTX contains a real
			// RTP packet that (with highest seq not seen yet) whose transmission
			// failed so we didn't receive it. So do not return false here but let
			// the packet go through.
		}

		AddPacketsToNackList(this->lastSeq + 1, seq);

		this->lastSeq = seq;

		// Check if there are any nacks that are waiting for this seq number.
		const std::vector<uint16_t> nackBatch = GetNackBatch(NackFilter::SEQ);

		if (!nackBatch.empty())
		{
			this->listener->OnNackGeneratorNackRequired(nackBatch);
		}

		// This is important. Otherwise the running timer (filter:TIME) would be
		// interrupted and NACKs would never been sent more than once for each seq.
		if (!this->timer->IsActive())
		{
			MayRunTimer();
		}

		// libwebrtc may use RTX for probation and such packets may contain
		// RTX-encoded real RTP packets that were sent before but didn't arrive yet
		// to us or they were lost. Let's deal with them as normal packets.
		return isRecovered;
	}

	void NackGenerator::AddPacketsToNackList(uint16_t seqStart, uint16_t seqEnd)
	{
		MS_TRACE();

		// Remove old packets.
		const auto it = this->nackList.lower_bound(seqEnd - MaxPacketAge);

		this->nackList.erase(this->nackList.begin(), it);

		const auto requestedIt = this->requestedList.lower_bound(seqEnd - MaxPacketAge);

		this->requestedList.erase(this->requestedList.begin(), requestedIt);

		// If the nack list is too large, remove packets from the nack list until
		// the latest first packet of a keyframe. If the list is still too large,
		// clear it and request a keyframe.
		const uint16_t numNewNacks = seqEnd - seqStart;

		if (this->nackList.size() + numNewNacks > MaxNackPackets)
		{
			while (RemoveNackItemsUntilKeyFrame() && this->nackList.size() + numNewNacks > MaxNackPackets)
			{
			}

			if (this->nackList.size() + numNewNacks > MaxNackPackets)
			{
				MS_WARN_TAG(
				  rtx, "NACK list full, clearing it and requesting a key frame [seqEnd:%" PRIu16 "]", seqEnd);

				// The packets already requested may still arrive, as retransmissions.
				for (const auto& kv : this->nackList)
				{
					const auto& nackInfo = kv.second;

					this->requestedList.insert(nackInfo.seq);
				}

				this->nackList.clear();

				this->listener->OnNackGeneratorKeyFrameRequired();

				return;
			}
		}

		for (uint16_t seq = seqStart; seq != seqEnd; ++seq)
		{
			MS_ASSERT(!this->nackList.contains(seq), "packet already in the NACK list");

			// Do not send NACK for packets that are already recovered by RTX.
			if (this->recoveredList.contains(seq))
			{
				continue;
			}

			this->nackList.emplace(
			  seq,
			  NackInfo{
			    this->shared->GetTimeMs(),
			    seq,
			    seq,
			  });
		}
	}

	bool NackGenerator::RemoveNackItemsUntilKeyFrame()
	{
		MS_TRACE();

		while (!this->keyFrameList.empty())
		{
			const auto it = this->nackList.lower_bound(*this->keyFrameList.begin());

			if (it != this->nackList.begin())
			{
				// We have found a keyframe that actually is newer than at least one
				// packet in the nack list.
				//
				// NOTE: The packets already requested may still arrive, as
				// retransmissions.
				for (auto removedIt = this->nackList.begin(); removedIt != it; ++removedIt)
				{
					const auto& nackInfo = removedIt->second;

					this->requestedList.insert(nackInfo.seq);
				}

				this->nackList.erase(this->nackList.begin(), it);

				return true;
			}

			// If this keyframe is so old it does not remove any packets from the list,
			// remove it from the list of keyframes and try the next keyframe.
			this->keyFrameList.erase(this->keyFrameList.begin());
		}

		return false;
	}

	std::vector<uint16_t> NackGenerator::GetNackBatch(NackFilter filter)
	{
		MS_TRACE();

		const int64_t nowMs = this->shared->GetTimeMs();

		std::vector<uint16_t> nackBatch;

		auto it = this->nackList.begin();

		while (it != this->nackList.end())
		{
			auto& nackInfo = it->second;

			const auto seq = nackInfo.seq;

			if (this->sendNackDelayMs > 0 && nowMs - nackInfo.createdAtMs < this->sendNackDelayMs)
			{
				++it;
				continue;
			}

			if (
			  filter == NackFilter::SEQ && nackInfo.sentAtMs == 0 &&
			  (nackInfo.sendAtSeq == this->lastSeq ||
				 SeqManager<uint16_t>::IsSeqHigherThan(this->lastSeq, nackInfo.sendAtSeq)))
			{
				nackBatch.emplace_back(seq);
				nackInfo.retries++;
				nackInfo.sentAtMs = nowMs;

				if (nackInfo.retries >= MaxNackRetries)
				{
					MS_WARN_TAG(
					  rtx,
					  "sequence number removed from the NACK list due to max retries [filter:seq, seq:%" PRIu16
					  "]",
					  seq);

					// The NACKs just sent may still be answered.
					this->requestedList.insert(seq);

					it = this->nackList.erase(it);
				}
				else
				{
					++it;
				}

				continue;
			}

			if (
			  filter == NackFilter::TIME &&
			  (nackInfo.sentAtMs == 0 ||
				 nowMs - nackInfo.sentAtMs >= (this->rttMs > 0 ? this->rttMs : DefaultRttMs)))
			{
				nackBatch.emplace_back(seq);
				nackInfo.retries++;
				nackInfo.sentAtMs = nowMs;

				if (nackInfo.retries >= MaxNackRetries)
				{
					MS_WARN_TAG(
					  rtx,
					  "sequence number removed from the NACK list due to max retries [filter:time, seq:%" PRIu16
					  "]",
					  seq);

					// The NACKs just sent may still be answered.
					this->requestedList.insert(seq);

					it = this->nackList.erase(it);
				}
				else
				{
					++it;
				}

				continue;
			}

			++it;
		}

#if MS_LOG_DEV_LEVEL == 3
		if (!nackBatch.empty())
		{
			std::ostringstream seqsStream;

			std::ranges::copy(
			  nackBatch.begin(), nackBatch.end() - 1, std::ostream_iterator<uint32_t>(seqsStream, ","));
			seqsStream << nackBatch.back();

			if (filter == NackFilter::SEQ)
			{
				MS_DEBUG_DEV("[filter:SEQ, asking seqs:%s]", seqsStream.str().c_str());
			}
			else
			{
				MS_DEBUG_DEV("[filter:TIME, asking seqs:%s]", seqsStream.str().c_str());
			}
		}
#endif

		return nackBatch;
	}

	void NackGenerator::UpdateRttMs(int64_t rttMs)
	{
		MS_TRACE();

		this->rttMs = rttMs;
	}

	void NackGenerator::Reset()
	{
		MS_TRACE();

		this->nackList.clear();
		this->keyFrameList.clear();
		this->recoveredList.clear();
		this->requestedList.clear();
		this->started = false;
		this->lastSeq = 0;
	}

	void NackGenerator::Pause()
	{
		MS_TRACE();

		// The packets still requested may arrive later, as retransmissions.
		for (const auto& kv : this->nackList)
		{
			const auto& nackInfo = kv.second;

			this->requestedList.insert(nackInfo.seq);
		}

		this->nackList.clear();
		this->keyFrameList.clear();
		this->recoveredList.clear();
		this->started = false;
		this->lastSeq = 0;
	}

	inline void NackGenerator::MayRunTimer() const
	{
		MS_TRACE();

		if (this->nackList.empty())
		{
			this->timer->Stop();
		}
		else
		{
			this->timer->Start(TimerIntervalMs);
		}
	}

	inline void NackGenerator::OnTimer(TimerHandleInterface* /*timer*/)
	{
		MS_TRACE();

		const std::vector<uint16_t> nackBatch = GetNackBatch(NackFilter::TIME);

		if (!nackBatch.empty())
		{
			this->listener->OnNackGeneratorNackRequired(nackBatch);
		}

		MayRunTimer();
	}
} // namespace RTC
