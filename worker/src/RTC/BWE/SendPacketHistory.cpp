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

		int64_t SendPacketHistory::AddPacket(const AddPacketOptions& addPacketOptions)
		{
			MS_TRACE();

			while (!this->history.empty())
			{
				const auto it           = this->history.begin();
				const auto& oldestEntry = it->second;

				// The packets are held for as long as the window, beyond which a
				// feedback reporting on them is not of any use anymore.
				if (addPacketOptions.createdAtUs - oldestEntry.createdAtUs <= this->options.windowDurationUs)
				{
					break;
				}

				// Only the packets that no feedback has reported on yet count as in
				// flight, since those already reported had their bytes removed then.
				if (oldestEntry.sentPacket.sequenceNumber > this->lastAckSequenceNumber)
				{
					RemoveOutstandingBytes(oldestEntry);
				}

				RemoveSsrcAndSeq(oldestEntry);

				this->history.erase(it);
			}

			// The unwrapper is what counts, so that what goes on the wire and what this
			// history is keyed by can never be two different counts.
			const int64_t sequenceNumber =
			  this->sequenceNumberUnwrapper.Unwrap(this->nextWrappedSequenceNumber++).GetValue();

			SentPacketEntry entry;

			entry.sentPacket.sequenceNumber = sequenceNumber;
			entry.sentPacket.size           = addPacketOptions.size;
			entry.sentPacket.isAudio        = addPacketOptions.isAudio;
			entry.sentPacket.probeCluster   = addPacketOptions.probeCluster;
			entry.createdAtUs               = addPacketOptions.createdAtUs;
			entry.ssrc                      = addPacketOptions.ssrc;
			entry.seq                       = addPacketOptions.seq;
			entry.isRetransmission          = addPacketOptions.isRetransmission;
			entry.sentWithEct1              = addPacketOptions.sentWithEct1;

			// The very same SSRC and RTP sequence number can be sent twice, as an audio
			// retransmission does. What a feedback then reports as the arrival time of
			// either of them may be of the other one, so both are marked and the
			// mapping is left on the first of them.
			const auto [it, inserted] = this->ssrcAndSeqToSequenceNumber.emplace(
			  SsrcAndSeq{ .ssrc = addPacketOptions.ssrc, .seq = addPacketOptions.seq }, sequenceNumber);

			if (!inserted)
			{
				entry.ambiguousReceiveTime = true;

				const int64_t previousSequenceNumber = it->second;
				const auto previousIt                = this->history.find(previousSequenceNumber);

				if (previousIt != this->history.end())
				{
					auto& previousEntry = previousIt->second;

					previousEntry.ambiguousReceiveTime = true;
				}
			}
			// A retransmission that goes without RTX carries the very same SSRC and RTP
			// sequence number as what it retransmits, so it is just as ambiguous even
			// when the packet it repeats is not held anymore.
			else if (
			  addPacketOptions.isRetransmission &&
			  (!addPacketOptions.originalSsrc.has_value() ||
				 addPacketOptions.originalSsrc.value() == addPacketOptions.ssrc))
			{
				entry.ambiguousReceiveTime = true;
			}

			this->history.emplace(sequenceNumber, entry);

			this->highestSequenceNumber =
			  std::max(this->highestSequenceNumber.value_or(sequenceNumber), sequenceNumber);

			return sequenceNumber;
		}

		int64_t SendPacketHistory::UnwrapSequenceNumber(uint16_t wrappedSequenceNumber)
		{
			MS_TRACE();

			return this->sequenceNumberUnwrapper.Unwrap(wrappedSequenceNumber).GetValue();
		}

		std::optional<Types::SentPacket> SendPacketHistory::ProcessSentPacket(
		  int64_t sequenceNumber, int64_t sentAtUs)
		{
			MS_TRACE();

			const auto it = this->history.find(sequenceNumber);

			if (it == this->history.end())
			{
				MS_DEBUG_DEV("packet not held anymore [sequenceNumber:%" PRIi64 "]", sequenceNumber);

				return std::nullopt;
			}

			auto& entry = it->second;

			// It already left, so taking note of it again would count its bytes twice.
			const bool alreadySent = entry.sentPacket.sendTimeUs != Types::TimeUsInfinite;

			entry.sentPacket.sendTimeUs = sentAtUs;

			this->lastSendTimeUs = std::max(this->lastSendTimeUs.value_or(sentAtUs), sentAtUs);

			// The bytes that left untracked belong to whoever leaves next, which is
			// this packet.
			//
			// TODO: (libwebrtc) Don't do this on retransmit.
			if (this->pendingUntrackedBytes > 0)
			{
				if (this->lastUntrackedSendTimeUs.has_value() && sentAtUs < this->lastUntrackedSendTimeUs.value())
				{
					MS_WARN_DEV("attributing untracked bytes to a packet that left before them");
				}

				entry.sentPacket.priorUnackedData += this->pendingUntrackedBytes;

				this->pendingUntrackedBytes = 0;
			}

			if (alreadySent)
			{
				return std::nullopt;
			}

			if (entry.sentPacket.sequenceNumber > this->lastAckSequenceNumber)
			{
				this->outstandingBytes += entry.sentPacket.size;
			}

			entry.sentPacket.dataInFlight = this->outstandingBytes;

			return entry.sentPacket;
		}

		void SendPacketHistory::ProcessSentUntrackedPacket(size_t size, int64_t sentAtUs)
		{
			MS_TRACE();

			if (this->lastSendTimeUs.has_value() && sentAtUs < this->lastSendTimeUs.value())
			{
				MS_WARN_DEV("untracked bytes left before the latest tracked packet did");
			}

			this->pendingUntrackedBytes += size;

			this->lastUntrackedSendTimeUs =
			  std::max(this->lastUntrackedSendTimeUs.value_or(sentAtUs), sentAtUs);
		}

		std::optional<SendPacketHistory::SentPacketEntry> SendPacketHistory::RetrievePacket(
		  int64_t sequenceNumber, bool received)
		{
			MS_TRACE();

			// A sequence number this history never gave out cannot be resolved, and
			// must not move the acknowledged mark either.
			if (
			  sequenceNumber < 0 || !this->highestSequenceNumber.has_value() ||
			  sequenceNumber > this->highestSequenceNumber.value())
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
					const auto& entry = it->second;

					RemoveOutstandingBytes(entry);
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

			auto& storedEntry = it->second;

			// A feedback cannot report on a packet that was never taken note of as
			// having left.
			if (storedEntry.sentPacket.sendTimeUs == Types::TimeUsInfinite)
			{
				MS_DEBUG_DEV(
				  "feedback reports on a packet that never left [sequenceNumber:%" PRIi64 "]",
				  sequenceNumber);

				return std::nullopt;
			}

			const auto entry = storedEntry;

			// A packet reported as lost is kept, since a later feedback may still
			// report it as received.
			if (received)
			{
				RemoveSsrcAndSeq(storedEntry);

				this->history.erase(it);
			}
			// Marked after the copy was taken, so that the caller sees the value the
			// packet had when this feedback reported on it and can hence tell a first
			// loss from the same loss reported again.
			else
			{
				storedEntry.previouslyReportedLost = true;
			}

			return entry;
		}

		std::optional<SendPacketHistory::SentPacketEntry> SendPacketHistory::RetrievePacket(
		  uint32_t ssrc, uint16_t seq, bool received)
		{
			MS_TRACE();

			const auto it = this->ssrcAndSeqToSequenceNumber.find(SsrcAndSeq{ .ssrc = ssrc, .seq = seq });

			if (it == this->ssrcAndSeqToSequenceNumber.end())
			{
				MS_DEBUG_DEV("packet not held anymore [ssrc:%" PRIu32 ", seq:%" PRIu16 "]", ssrc, seq);

				return std::nullopt;
			}

			const int64_t sequenceNumber = it->second;

			return RetrievePacket(sequenceNumber, received);
		}

		void SendPacketHistory::RemoveOutstandingBytes(const SentPacketEntry& entry)
		{
			MS_TRACE();

			// A packet that never left never counted as in flight.
			if (entry.sentPacket.sendTimeUs == Types::TimeUsInfinite)
			{
				return;
			}

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

			this->ssrcAndSeqToSequenceNumber.erase(SsrcAndSeq{ .ssrc = entry.ssrc, .seq = entry.seq });
		}
	} // namespace BWE
} // namespace RTC
