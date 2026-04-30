#define MS_CLASS "RTC::SCTP::OutstandingData"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/tx/OutstandingData.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "Utils.hpp"
#include "RTC/SCTP/packet/chunks/ForwardTsnChunk.hpp"
#include "RTC/SCTP/packet/chunks/IForwardTsnChunk.hpp"
#include <algorithm>
#include <map>

namespace RTC
{
	namespace SCTP
	{
		/* Static. */

		// The number of times a packet must be NACKed before it's retransmitted.
		//
		// @see https://datatracker.ietf.org/doc/html/rfc9260#section-7.2.4
		constexpr uint8_t NumberOfNacksForRetransmission{ 3 };

		/* Instance methods. */

		OutstandingData::Item::Item(
		  uint32_t messageId,
		  UserData data,
		  uint64_t timeSentMs,
		  uint16_t maxRetransmissions,
		  uint64_t expiresAtMs,
		  uint64_t lifecycleId)
		  : messageId(messageId),
		    timeSentMs(timeSentMs),
		    maxRetransmissions(maxRetransmissions),
		    expiresAtMs(expiresAtMs),
		    lifecycleId(lifecycleId),
		    data(std::move(data))
		{
			MS_TRACE();
		}

		void OutstandingData::Item::Ack()
		{
			MS_TRACE();

			if (this->lifecycle != Lifecycle::ABANDONED)
			{
				this->lifecycle = Lifecycle::ACTIVE;
			}

			this->ackState = AckState::ACKED;
		}

		OutstandingData::Item::NackAction OutstandingData::Item::Nack(bool retransmitNow)
		{
			MS_TRACE();

			this->ackState = AckState::NACKED;
			++this->nackCount;

			if (!ShouldBeRetransmitted() && !IsAbandoned() && (retransmitNow || this->nackCount >= NumberOfNacksForRetransmission))
			{
				// Nacked enough times, it's considered lost.
				if (this->numRetransmissions < this->maxRetransmissions)
				{
					this->lifecycle = Lifecycle::TO_BE_RETRANSMITTED;

					return NackAction::RETRANSMIT;
				}

				Abandon();

				return NackAction::ABANDON;
			}

			return NackAction::NOTHING;
		}

		void OutstandingData::Item::MarkAsRetransmitted()
		{
			MS_TRACE();

			this->lifecycle = Lifecycle::ACTIVE;
			this->ackState  = AckState::UNACKED;
			this->nackCount = 0;
			++this->numRetransmissions;
		}

		void OutstandingData::Item::Abandon()
		{
			MS_TRACE();

			MS_ASSERT(
			  this->expiresAtMs != OutstandingData::ExpiresAtMsInfinite ||
			    this->maxRetransmissions != OutstandingData::MaxRetransmitsNoLimit,
			  "item should not have infinite expiration time or its retransmission times shouldn't be the maximum");

			this->lifecycle = Lifecycle::ABANDONED;
		}

		OutstandingData::OutstandingData(
		  size_t dataChunkHeaderLength,
		  UnwrappedTsn lastCumulativeTsnAck,
		  std::function<bool(uint16_t /*streamId*/, uint32_t /*outgoingMessageId*/)> discardFromSendQueue)
		  : dataChunkHeaderLength(dataChunkHeaderLength),
		    lastCumulativeTsnAck(lastCumulativeTsnAck),
		    discardFromSendQueue(std::move(discardFromSendQueue))
		{
			MS_TRACE();
		}

		OutstandingData::AckInfo OutstandingData::HandleSack(
		  UnwrappedTsn cumulativeTsnAck,
		  std::span<const SackChunk::GapAckBlock> gapAckBlocks,
		  bool isInFastRecovery)
		{
			MS_TRACE();

			const bool cumulativeTsnAckAdvanced = cumulativeTsnAck > this->lastCumulativeTsnAck;

			OutstandingData::AckInfo ackInfo(cumulativeTsnAck);

			// Erase all items up to cumulativeTsnAck.
			RemoveAcked(cumulativeTsnAck, ackInfo);

			// ACK packets reported in the gap ack blocks.
			AckGapBlocks(cumulativeTsnAck, gapAckBlocks, ackInfo);

			// NACK and possibly mark for retransmit Chunks that weren't acked.
			NackBetweenAckBlocks(
			  cumulativeTsnAck, gapAckBlocks, isInFastRecovery, cumulativeTsnAckAdvanced, ackInfo);

			AssertIsConsistent();

			return ackInfo;
		}

		std::vector<std::pair<uint32_t /*tsn*/, UserData>> OutstandingData::GetChunksToBeFastRetransmitted(
		  size_t maxLength)
		{
			MS_TRACE();

			std::vector<std::pair<uint32_t /*tsn*/, UserData>> result =
			  ExtractChunksThatCanFit(this->toBeFastRetransmitted, maxLength);

			// https://datatracker.ietf.org/doc/html/rfc9260#section-7.2.4
			//
			// "Those TSNs marked for retransmission due to the Fast-Retransmit
			// algorithm that did not fit in the sent datagram carrying K other TSNs
			// are also marked as ineligible for a subsequent Fast Retransmit.
			// However, as they are marked for retransmission they will be
			// retransmitted later on as soon as cwnd allows."
			if (!this->toBeFastRetransmitted.empty())
			{
				this->toBeRetransmitted.insert(
				  this->toBeFastRetransmitted.begin(), this->toBeFastRetransmitted.end());

				this->toBeFastRetransmitted.clear();
			}

			AssertIsConsistent();

			return result;
		}

		std::vector<std::pair<uint32_t /*tsn*/, UserData>> OutstandingData::GetChunksToBeRetransmitted(
		  size_t maxLength)
		{
			MS_TRACE();

			// Chunks scheduled for fast retransmission must be sent first.
			MS_ASSERT(this->toBeFastRetransmitted.empty(), "this->toBeFastRetransmitted is not empty");

			return ExtractChunksThatCanFit(this->toBeFastRetransmitted, maxLength);
		}

		void OutstandingData::ExpireOutstandingChunks(uint64_t nowMs)
		{
			MS_TRACE();

			std::vector<UnwrappedTsn> tsnsToExpire;
			UnwrappedTsn tsn = this->lastCumulativeTsnAck;

			for (const Item& item : this->outstandingData)
			{
				tsn.Increment();

				// Chunks that are nacked can be expired. Care should be taken not to
				// expire unacked (in-flight) Chunks as they might have been received,
				// but the SACK is either delayed or in-flight and may be received
				// later.
				if (item.IsAbandoned())
				{
					// Already abandoned.
				}
				else if (item.IsNacked() && item.HasExpired(nowMs))
				{
					tsnsToExpire.push_back(tsn);
				}
				else
				{
					// A non-expired Chunk. No need to iterate any further.
					break;
				}
			}

			for (const UnwrappedTsn tsnToExpire : tsnsToExpire)
			{
				// The item is retrieved by TSN, as AbandonAllFor() may have modified
				// `this->outstandingData` and invalidated iterators from the first
				// loop.
				const Item& item = GetItem(tsnToExpire);

				MS_WARN_TAG(
				  sctp,
				  "marking nacked Chunk %" PRIu32 " and message %" PRIu32 " as expired",
				  tsnToExpire.Wrap(),
				  item.GetData().GetMessageId());

				AbandonAllFor(item);
			}

			AssertIsConsistent();
		}

		OutstandingData::UnwrappedTsn OutstandingData::GetHighestOutstandingTsn() const
		{
			MS_TRACE();

			return UnwrappedTsn::AddTo(this->lastCumulativeTsnAck, this->outstandingData.size());
		}

		std::optional<OutstandingData::UnwrappedTsn> OutstandingData::Insert(
		  uint32_t messageId,
		  const UserData& data,
		  uint64_t timeSentMs,
		  uint16_t maxRetransmissions,
		  uint64_t expiresAtMs,
		  uint64_t lifecycleId)
		{
			MS_TRACE();

			// All Chunks are always padded to be even divisible by 4.
			const size_t chunkLength = GetSerializedChunkLength(data);

			this->unackedPayloadBytes += data.GetPayloadLength();
			this->unackedPacketBytes += chunkLength;
			++this->unackedItems;

			const UnwrappedTsn tsn = GetNextTsn();
			const Item& item       = this->outstandingData.emplace_back(
			  messageId, data.Clone(), timeSentMs, maxRetransmissions, expiresAtMs, lifecycleId);

			if (item.HasExpired(timeSentMs))
			{
				// No need to send it, it was expired when it was in the send queue.
				MS_WARN_TAG(
				  sctp,
				  "marking freshly produced Chunk %" PRIu32 " and message %" PRIu32 " as expired",
				  tsn.Wrap(),
				  item.GetData().GetMessageId());

				AbandonAllFor(item);

				AssertIsConsistent();

				return std::nullopt;
			}

			AssertIsConsistent();

			return tsn;
		}

		void OutstandingData::NackAll()
		{
			MS_TRACE();

			UnwrappedTsn tsn = this->lastCumulativeTsnAck;

			// A two-pass algorithm is needed, as NackItem will invalidate iterators.
			std::vector<UnwrappedTsn> tsnsToNack;

			for (const Item& item : this->outstandingData)
			{
				tsn.Increment();

				if (!item.IsAcked())
				{
					tsnsToNack.push_back(tsn);
				}
			}

			for (const UnwrappedTsn tsnToNack : tsnsToNack)
			{
				NackItem(
				  tsnToNack,
				  /*retransmitNow*/ true,
				  /*doFastRetransmit*/ false);
			}

			AssertIsConsistent();
		}

		void OutstandingData::CreateForwardTsn(Packet* packet) const
		{
			MS_TRACE();

			std::map<uint16_t /*streamId*/, uint16_t /*ssn*/> skippedPerOrderedStream;
			UnwrappedTsn newCumulativeAck = this->lastCumulativeTsnAck;
			UnwrappedTsn tsn              = this->lastCumulativeTsnAck;

			for (const Item& item : this->outstandingData)
			{
				tsn.Increment();

				if (
				  this->streamResetBreakpointTsns.contains(tsn) ||
				  (tsn != newCumulativeAck.GetNextValue()) || !item.IsAbandoned())
				{
					break;
				}

				newCumulativeAck = tsn;

				if (
				  !item.GetData().IsUnordered() && item.GetData().GetStreamSequenceNumber() >
				                                     skippedPerOrderedStream[item.GetData().GetStreamId()])
				{
					skippedPerOrderedStream[item.GetData().GetStreamId()] =
					  item.GetData().GetStreamSequenceNumber();
				}
			}

			auto* forwardTsnChunk = packet->BuildChunkInPlace<ForwardTsnChunk>();

			forwardTsnChunk->SetNewCumulativeTsn(newCumulativeAck.Wrap());

			for (const auto& [streamId, ssn] : skippedPerOrderedStream)
			{
				forwardTsnChunk->AddStream(streamId, ssn);
			}

			forwardTsnChunk->Consolidate();
		}

		void OutstandingData::CreateIForwardTsn(Packet* packet) const
		{
			MS_TRACE();

			std::map<std::pair<uint16_t /*streamId*/, bool /*isUnordered*/>, uint32_t /*mid*/> skippedPerStream;
			UnwrappedTsn newCumulativeAck = this->lastCumulativeTsnAck;
			UnwrappedTsn tsn              = this->lastCumulativeTsnAck;

			for (const Item& item : this->outstandingData)
			{
				tsn.Increment();

				if (
				  this->streamResetBreakpointTsns.contains(tsn) ||
				  (tsn != newCumulativeAck.GetNextValue()) || !item.IsAbandoned())
				{
					break;
				}

				newCumulativeAck = tsn;

				const std::pair<uint16_t /*streamId*/, bool /*isUnordered*/> stream =
				  std::make_pair(item.GetData().GetStreamId(), item.GetData().IsUnordered());

				skippedPerStream[stream] = std::max(item.GetData().GetMessageId(), skippedPerStream[stream]);
			}

			auto* iForwardTsnChunk = packet->BuildChunkInPlace<IForwardTsnChunk>();

			iForwardTsnChunk->SetNewCumulativeTsn(newCumulativeAck.Wrap());

			for (const auto& [stream, mid] : skippedPerStream)
			{
				iForwardTsnChunk->AddStream(stream.first, stream.second, mid);
			}

			iForwardTsnChunk->Consolidate();
		}

		std::optional<uint64_t> OutstandingData::MeasureRtt(uint64_t nowMs, UnwrappedTsn tsn) const
		{
			MS_TRACE();

			if (tsn > this->lastCumulativeTsnAck && tsn < GetNextTsn())
			{
				const Item& item = GetItem(tsn);

				if (!item.HasBeenRetransmitted())
				{
					// https://datatracker.ietf.org/doc/html/rfc9260#section-6.3.1
					//
					// "Karn's algorithm: RTT measurements MUST NOT be made using packets
					// that were retransmitted (and thus for which it is ambiguous
					// whether the reply was for the first instance of the Chunk or for a
					// later instance)"
					return nowMs - item.GetTimeSentMs();
				}
			}

			return std::nullopt;
		}

		bool OutstandingData::ShouldSendForwardTsn() const
		{
			MS_TRACE();

			if (!this->outstandingData.empty())
			{
				return this->outstandingData.front().IsAbandoned();
			}
			else
			{
				return false;
			}
		}

		void OutstandingData::BeginResetStreams()
		{
			MS_TRACE();

			this->streamResetBreakpointTsns.insert(GetNextTsn());
		}

#ifdef MS_TEST
		std::vector<
		  std::pair<uint32_t /*tsn*/, OutstandingData::State>> OutstandingData::GetChunkStatesForTesting() const
		{
			MS_TRACE();

			std::vector<std::pair<uint32_t /*tsn*/, State>> states;

			states.emplace_back(this->lastCumulativeTsnAck.Wrap(), State::ACKED);

			UnwrappedTsn tsn = this->lastCumulativeTsnAck;

			for (const Item& item : this->outstandingData)
			{
				tsn.Increment();

				State state;

				if (item.IsAbandoned())
				{
					state = State::ABANDONED;
				}
				else if (item.ShouldBeRetransmitted())
				{
					state = State::TO_BE_RETRANSMITTED;
				}
				else if (item.IsAcked())
				{
					state = State::ACKED;
				}
				else if (item.IsNacked())
				{
					state = State::NACKED;
				}
				else if (item.IsOutstanding())
				{
					state = State::IN_FLIGHT;
				}
				else
				{
					MS_THROW_ERROR("should not end here");
				}

				states.emplace_back(tsn.Wrap(), state);
			}

			return states;
		}
#endif

		size_t OutstandingData::GetSerializedChunkLength(const UserData& data) const
		{
			MS_TRACE();

			return Utils::Byte::PadTo4Bytes<size_t>(this->dataChunkHeaderLength + data.GetPayloadLength());
		}

		OutstandingData::Item& OutstandingData::GetItem(UnwrappedTsn tsn)
		{
			MS_TRACE();

			MS_ASSERT(
			  tsn > this->lastCumulativeTsnAck, "tsn must be higher than this->lastCumulativeTsnAck");
			MS_ASSERT(tsn < GetNextTsn(), "tsn must be higher than GetNextTsn()");

			const size_t index = UnwrappedTsn::Difference(tsn, this->lastCumulativeTsnAck) - 1;

			MS_ASSERT(index >= 0, "index must be equal or higher than 0");
			MS_ASSERT(
			  index < this->outstandingData.size(),
			  "index must be lower than this->outstandingData.size()");

			return this->outstandingData[index];
		}

		const OutstandingData::Item& OutstandingData::GetItem(UnwrappedTsn tsn) const
		{
			MS_TRACE();

			MS_ASSERT(
			  tsn > this->lastCumulativeTsnAck, "tsn must be higher than this->lastCumulativeTsnAck");
			MS_ASSERT(tsn < GetNextTsn(), "tsn must be higher than GetNextTsn()");

			const size_t index = UnwrappedTsn::Difference(tsn, this->lastCumulativeTsnAck) - 1;

			MS_ASSERT(index >= 0, "index must be equal or higher than 0");
			MS_ASSERT(
			  index < this->outstandingData.size(),
			  "index must be lower than this->outstandingData.size()");

			return this->outstandingData[index];
		}

		void OutstandingData::RemoveAcked(UnwrappedTsn cumulativeTsnAck, AckInfo& ackInfo)
		{
			MS_TRACE();

			while (!this->outstandingData.empty() && this->lastCumulativeTsnAck < cumulativeTsnAck)
			{
				const UnwrappedTsn tsn = this->lastCumulativeTsnAck.GetNextValue();

				Item& item = this->outstandingData.front();

				AckChunk(ackInfo, tsn, item);

				if (item.GetLifecycleId() != 0)
				{
					MS_ASSERT(item.GetData().IsEnd(), "item.GetData().IsEnd() must be true");

					if (item.IsAbandoned())
					{
						ackInfo.abandonedLifecycleIds.push_back(item.GetLifecycleId());
					}
					else
					{
						ackInfo.ackedLifecycleIds.push_back(item.GetLifecycleId());
					}
				}

				this->outstandingData.pop_front();
				this->lastCumulativeTsnAck.Increment();
			}

			this->streamResetBreakpointTsns.erase(
			  this->streamResetBreakpointTsns.begin(),
			  this->streamResetBreakpointTsns.upper_bound(cumulativeTsnAck.GetNextValue()));
		}

		void OutstandingData::AckGapBlocks(
		  UnwrappedTsn cumulativeTsnAck,
		  std::span<const SackChunk::GapAckBlock> gapAckBlocks,
		  AckInfo& ackInfo)
		{
			MS_TRACE();

			// Mark all non-gaps as ACKED (but they can't be removed) as (from RFC)
			// "SCTP considers the information carried in the Gap Ack Blocks in the
			// SACK Chunk as advisory". Note that when NR-SACK is supported, this can
			// be handled differently.

			for (const auto& block : gapAckBlocks)
			{
				const UnwrappedTsn start = UnwrappedTsn::AddTo(cumulativeTsnAck, block.start);
				const UnwrappedTsn end   = UnwrappedTsn::AddTo(cumulativeTsnAck, block.end);

				for (UnwrappedTsn tsn = start; tsn <= end; tsn = tsn.GetNextValue())
				{
					if (tsn > this->lastCumulativeTsnAck && tsn < GetNextTsn())
					{
						Item& item = GetItem(tsn);

						AckChunk(ackInfo, tsn, item);
					}
				}
			}
		}

		void OutstandingData::NackBetweenAckBlocks(
		  UnwrappedTsn cumulativeTsnAck,
		  std::span<const SackChunk::GapAckBlock> gapAckBlocks,
		  bool isInFastRecovery,
		  bool cumulativeTsnAckedAdvanced,
		  AckInfo& ackInfo)
		{
			MS_TRACE();

			// Mark everything between the blocks as NACKED/TO_BE_RETRANSMITTED.
			//
			// https://datatracker.ietf.org/doc/html/rfc9260#section-7.2.4
			//
			// "Mark the DATA chunk(s) with three miss indications for retransmission."
			// "For each incoming SACK, miss indications are incremented only for
			// missing TSNs prior to the highest TSN newly acknowledged in the SACK."
			//
			// What this means is that only when there is a increasing stream of data
			// received and there are new packets seen (since last time), packets that
			// are in-flight and between gaps should be nacked. This means that SCTP
			// relies on the T3-RTX-timer to re-send packets otherwise.
			UnwrappedTsn maxTsnToNack = ackInfo.highestTsnAcked;

			if (isInFastRecovery && cumulativeTsnAckedAdvanced)
			{
				// https://datatracker.ietf.org/doc/html/rfc9260#section-7.2.4
				//
				// "If an endpoint is in Fast Recovery and a SACK arrives that advances
				// the Cumulative TSN Ack Point, the miss indications are incremented
				// for all TSNs reported missing in the SACK."
				maxTsnToNack = UnwrappedTsn::AddTo(
				  cumulativeTsnAck, gapAckBlocks.empty() ? 0 : gapAckBlocks.rbegin()->end);
			}

			UnwrappedTsn prevBlockLastAcked = cumulativeTsnAck;

			for (const auto& block : gapAckBlocks)
			{
				const UnwrappedTsn curBlockFirstAcked = UnwrappedTsn::AddTo(cumulativeTsnAck, block.start);

				for (UnwrappedTsn tsn = prevBlockLastAcked.GetNextValue();
				     tsn < curBlockFirstAcked && tsn <= maxTsnToNack && tsn < GetNextTsn();
				     tsn = tsn.GetNextValue())
				{
					ackInfo.hasPacketLoss |= NackItem(
					  tsn,
					  /*retransmitNow*/ false,
					  /*doFastRetransmit*/ !isInFastRecovery);
				}

				prevBlockLastAcked = UnwrappedTsn::AddTo(cumulativeTsnAck, block.end);
			}

			// Note that packets are not NACKED which are above the highest
			// gap-ack-block (or above the cumulative ack TSN if no gap-ack-blocks)
			// as only packets up until the `highestTsnAcked` (see above) should be
			// considered when NACKing.
		}

		void OutstandingData::AckChunk(AckInfo& ackInfo, UnwrappedTsn tsn, Item& item)
		{
			MS_TRACE();

			if (!item.IsAcked())
			{
				const size_t serializedLength = GetSerializedChunkLength(item.GetData());

				ackInfo.bytesAcked += serializedLength;

				if (item.IsOutstanding())
				{
					this->unackedPayloadBytes -= item.GetData().GetPayloadLength();
					this->unackedPacketBytes -= serializedLength;
					--this->unackedItems;
				}

				if (item.ShouldBeRetransmitted())
				{
					MS_ASSERT(
					  !this->toBeFastRetransmitted.contains(tsn),
					  "tsn should not be present in this->toBeFastRetransmitted");

					this->toBeRetransmitted.erase(tsn);
				}

				item.Ack();

				ackInfo.highestTsnAcked = std::max(ackInfo.highestTsnAcked, tsn);
			}
		}

		bool OutstandingData::NackItem(UnwrappedTsn tsn, bool retransmitNow, bool doFastRetransmit)
		{
			MS_TRACE();

			Item& item = GetItem(tsn);

			// Ignore NACKs for chunks that have already been acknowledged.
			if (item.IsAcked())
			{
				return false;
			}

			const bool wasOutstanding     = item.IsOutstanding();
			const Item::NackAction action = item.Nack(retransmitNow);

			if (wasOutstanding && !item.IsOutstanding())
			{
				this->unackedPayloadBytes -= item.GetData().GetPayloadLength();
				this->unackedPacketBytes -= GetSerializedChunkLength(item.GetData());
				--this->unackedItems;
			}

			switch (action)
			{
				case Item::NackAction::NOTHING:
				{
					return false;
				}

				case Item::NackAction::RETRANSMIT:
				{
					if (doFastRetransmit)
					{
						this->toBeFastRetransmitted.insert(tsn);
					}
					else
					{
						this->toBeRetransmitted.insert(tsn);
					}

					MS_DEBUG_TAG(sctp, "tsn %" PRIu32 " marked for retransmission", tsn.Wrap());

					break;
				}

				case Item::NackAction::ABANDON:
				{
					MS_DEBUG_TAG(sctp, "tsn %" PRIu32 " nacked, resulted in abandoning", tsn.Wrap());

					AbandonAllFor(item);

					break;
				}
			}

			return true;
		}

		void OutstandingData::AbandonAllFor(const OutstandingData::Item& item)
		{
			MS_TRACE();

			// Erase all remaining chunks from the producer, if any.
			if (this->discardFromSendQueue(item.GetData().GetStreamId(), item.GetMessageId()))
			{
				// There were remaining chunks to be produced for this message. Since the
				// receiver may have already received all chunks (up till now) for this
				// message, we can't just FORWARD-TSN to the last fragment in this
				// (abandoned) message and start sending a new message, as the receiver will
				// then see a new message before the end of the previous one was seen (or
				// skipped over). So create a new fragment, representing the end, that the
				// received will never see as it is abandoned immediately and used as cum
				// TSN in the sent FORWARD-TSN.
				UserData messageEnd(
				  item.GetData().GetStreamId(),
				  item.GetData().GetStreamSequenceNumber(),
				  item.GetData().GetMessageId(),
				  item.GetData().GetFragmentSequenceNumber(),
				  item.GetData().GetPayloadProtocolId(),
				  std::vector<uint8_t>(),
				  /*isBeginning*/ false,
				  /*isEnd*/ true,
				  /*isUnordered*/ item.GetData().IsUnordered());

				const UnwrappedTsn tsn = GetNextTsn();

				Item& addedItem = this->outstandingData.emplace_back(
				  item.GetMessageId(),
				  std::move(messageEnd),
				  /*timeSentMs*/ 0,
				  /*maxRetransmissions*/ 0,
				  /*expiresAtMs*/ OutstandingData::ExpiresAtMsInfinite,
				  /*lifecycleId*/ 0);

				// The added Chunk shouldn't be included in `this->unackedPacketBytes`,
				// so set it as acked.
				addedItem.Ack();

				MS_DEBUG_TAG(sctp, "adding unsent end placeholder for message at TSN %" PRIu32, tsn.Wrap());
			}

			UnwrappedTsn tsn = this->lastCumulativeTsnAck;

			for (Item& other : this->outstandingData)
			{
				tsn.Increment();

				if (
				  !other.IsAbandoned() && other.GetData().GetStreamId() == item.GetData().GetStreamId() &&
				  other.GetMessageId() == item.GetMessageId())
				{
					MS_WARN_TAG(sctp, "marking Chunk %" PRIu32 " as abandoned", tsn.Wrap());

					if (other.ShouldBeRetransmitted())
					{
						this->toBeFastRetransmitted.erase(tsn);
						this->toBeRetransmitted.erase(tsn);
					}

					const bool wasOutstanding = other.IsOutstanding();

					other.Abandon();

					if (wasOutstanding)
					{
						this->unackedPayloadBytes -= other.GetData().GetPayloadLength();
						this->unackedPacketBytes -= GetSerializedChunkLength(other.GetData());
						--this->unackedItems;
					}
				}
			}
		}

		std::vector<std::pair<uint32_t /*tsn*/, UserData>> OutstandingData::ExtractChunksThatCanFit(
		  std::set<UnwrappedTsn>& chunks, size_t maxLength)
		{
			MS_TRACE();

			std::vector<std::pair<uint32_t /*tsn*/, UserData>> result;

			for (auto it = chunks.begin(); it != chunks.end();)
			{
				const UnwrappedTsn tsn = *it;

				Item& item = GetItem(tsn);

				MS_ASSERT(item.ShouldBeRetransmitted(), "item should be retransmitted");
				MS_ASSERT(!item.IsOutstanding(), "item should not be outstanding");
				MS_ASSERT(!item.IsAbandoned(), "item should not be abandoned");
				MS_ASSERT(!item.IsAcked(), "item should not be acked");

				const size_t serializedLength = GetSerializedChunkLength(item.GetData());

				if (serializedLength <= maxLength)
				{
					item.MarkAsRetransmitted();
					result.emplace_back(tsn.Wrap(), item.GetData().Clone());
					maxLength -= serializedLength;

					this->unackedPayloadBytes += item.GetData().GetPayloadLength();
					this->unackedPacketBytes += serializedLength;
					++this->unackedItems;

					it = chunks.erase(it);
				}
				else
				{
					++it;
				}

				// No point in continuing if the packet is full.
				if (maxLength <= this->dataChunkHeaderLength)
				{
					break;
				}
			}

			return result;
		}

		void OutstandingData::AssertIsConsistent() const
		{
			MS_TRACE();

			size_t actualUnackedPayloadBytes{ 0 };
			size_t actualUnackedPacketBytes{ 0 };
			size_t actualUnackedItems{ 0 };

			std::set<UnwrappedTsn> combinedToBeRetransmitted;

			combinedToBeRetransmitted.insert(this->toBeRetransmitted.begin(), this->toBeRetransmitted.end());
			combinedToBeRetransmitted.insert(
			  this->toBeFastRetransmitted.begin(), this->toBeFastRetransmitted.end());

			std::set<UnwrappedTsn> actualCombinedToBeRetransmitted;
			UnwrappedTsn tsn = this->lastCumulativeTsnAck;

			for (const Item& item : this->outstandingData)
			{
				tsn.Increment();

				if (item.IsOutstanding())
				{
					actualUnackedPayloadBytes += item.GetData().GetPayloadLength();
					actualUnackedPacketBytes += GetSerializedChunkLength(item.GetData());
					++actualUnackedItems;
				}

				if (item.ShouldBeRetransmitted())
				{
					actualCombinedToBeRetransmitted.insert(tsn);
				}
			}

			MS_ASSERT(
			  actualUnackedPayloadBytes == this->unackedPayloadBytes,
			  "actualUnackedPayloadBytes != this->unackedPayloadBytes");
			MS_ASSERT(
			  actualUnackedPacketBytes == this->unackedPacketBytes,
			  "actualUnackedPacketBytes != this->unackedPacketBytes");
			MS_ASSERT(actualUnackedItems == this->unackedItems, "actualUnackedItems != this->unackedItems");
			MS_ASSERT(
			  actualCombinedToBeRetransmitted == combinedToBeRetransmitted,
			  "actualCombinedToBeRetransmitted != combinedToBeRetransmitted");
		}
	} // namespace SCTP
} // namespace RTC
