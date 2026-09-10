#define MS_CLASS "RTC::BWE::SendPacketHistory"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/BWE/SendPacketHistory.hpp"
#include "Logger.hpp"

namespace RTC
{
	namespace BWE
	{
		/* Instance methods. */

		SendPacketHistory::SendPacketHistory() : SendPacketHistory(SendPacketHistoryOptions{})
		{
			MS_TRACE();
		}

		SendPacketHistory::SendPacketHistory(SendPacketHistoryOptions options) : options(options)
		{
			MS_TRACE();
		}

		int64_t SendPacketHistory::AddPacket(
		  uint32_t ssrc, uint16_t seq, size_t size, bool audio, int64_t sentAtUs)
		{
			MS_TRACE();

			// Drop the packets held for longer than the window, since a feedback
			// reporting on them is not of any use anymore.
			while (!this->history.empty() && sentAtUs - this->history.begin()->second.sentPacket.sendTimeUs >
			                                   this->options.windowDurationUs)
			{
				const auto& entry = this->history.begin()->second;

				// Only the packets that no feedback has reported on yet count as in
				// flight, since those already reported had their bytes removed then.
				if (entry.sentPacket.sequenceNumber > this->lastAckSequenceNumber)
				{
					RemoveOutstandingBytes(entry);
				}

				RemoveSsrcAndSeq(entry);

				this->history.erase(this->history.begin());
			}

			const int64_t sequenceNumber = this->nextSequenceNumber++;

			SentPacketEntry entry;

			entry.sentPacket.sequenceNumber = sequenceNumber;
			entry.sentPacket.sendTimeUs     = sentAtUs;
			entry.sentPacket.size           = size;
			entry.sentPacket.audio          = audio;
			entry.ssrc                      = ssrc;
			entry.seq                       = seq;

			// The very same SSRC and RTP sequence number can be sent twice, as an audio
			// retransmission does, so let the latest send take the mapping over.
			this->ssrcAndSeqToSequenceNumber[SsrcAndSeq{ .ssrc = ssrc, .seq = seq }] = sequenceNumber;

			this->outstandingBytes += size;

			entry.sentPacket.dataInFlight = this->outstandingBytes;

			this->history.emplace(sequenceNumber, entry);

			return sequenceNumber;
		}

		std::optional<Types::SentPacket> SendPacketHistory::RetrievePacket(
		  int64_t sequenceNumber, bool received)
		{
			MS_TRACE();

			// A sequence number this history never gave out cannot be resolved, and
			// must not move the acknowledged mark either.
			if (sequenceNumber < 0 || sequenceNumber >= this->nextSequenceNumber)
			{
				MS_DEBUG_DEV(
				  "sequence number was never given out [sequenceNumber:%" PRIi64 "]", sequenceNumber);

				return std::nullopt;
			}

			// The feedback settles whether every packet up to the reported one arrived
			// or not, so none of them is in flight anymore.
			if (sequenceNumber > this->lastAckSequenceNumber)
			{
				for (auto it = this->history.upper_bound(this->lastAckSequenceNumber);
				     it != this->history.upper_bound(sequenceNumber);
				     ++it)
				{
					RemoveOutstandingBytes(it->second);
				}

				this->lastAckSequenceNumber = sequenceNumber;
			}

			const auto it = this->history.find(sequenceNumber);

			if (it == this->history.end())
			{
				// Either the window already dropped it or a previous feedback reported it
				// as received.
				MS_DEBUG_DEV("packet not held anymore [sequenceNumber:%" PRIi64 "]", sequenceNumber);

				return std::nullopt;
			}

			const auto sentPacket = it->second.sentPacket;

			// A packet reported as lost is kept, since a later feedback may still
			// report it as received.
			if (received)
			{
				RemoveSsrcAndSeq(it->second);

				this->history.erase(it);
			}

			return sentPacket;
		}

		std::optional<Types::SentPacket> SendPacketHistory::RetrievePacket(
		  uint32_t ssrc, uint16_t seq, bool received)
		{
			MS_TRACE();

			const auto it = this->ssrcAndSeqToSequenceNumber.find(SsrcAndSeq{ .ssrc = ssrc, .seq = seq });

			if (it == this->ssrcAndSeqToSequenceNumber.end())
			{
				MS_DEBUG_DEV("packet not held anymore [ssrc:%" PRIu32 ", seq:%" PRIu16 "]", ssrc, seq);

				return std::nullopt;
			}

			return RetrievePacket(it->second, received);
		}

		std::optional<int64_t> SendPacketHistory::GetLastSequenceNumber() const
		{
			MS_TRACE();

			if (this->nextSequenceNumber == 0)
			{
				return std::nullopt;
			}

			return this->nextSequenceNumber - 1;
		}

		void SendPacketHistory::RemoveOutstandingBytes(const SentPacketEntry& entry)
		{
			MS_TRACE();

			// The acknowledged mark only moves forward, so the bytes of a packet are
			// removed at most once.
			MS_ASSERT(
			  this->outstandingBytes >= entry.sentPacket.size,
			  "less bytes in flight than the size of the packet being removed");

			this->outstandingBytes -= entry.sentPacket.size;
		}

		void SendPacketHistory::RemoveSsrcAndSeq(const SentPacketEntry& entry)
		{
			MS_TRACE();

			const auto it =
			  this->ssrcAndSeqToSequenceNumber.find(SsrcAndSeq{ .ssrc = entry.ssrc, .seq = entry.seq });

			// A later send of the very same SSRC and RTP sequence number took the
			// mapping over, so it's not this packet's to drop.
			if (it == this->ssrcAndSeqToSequenceNumber.end() || it->second != entry.sentPacket.sequenceNumber)
			{
				return;
			}

			this->ssrcAndSeqToSequenceNumber.erase(it);
		}
	} // namespace BWE
} // namespace RTC
