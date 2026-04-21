#define MS_CLASS "RTC::SCTP::OutstandingData"
// TODO: SCTP: COMMENT
#define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/tx/OutstandingData.hpp"
#include "Logger.hpp"
#include "RTC/SCTP/packet/chunks/ForwardTsnChunk.hpp"
#include "RTC/SCTP/packet/chunks/IForwardTsnChunk.hpp"
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

		OutstandingData::OutstandingData(
		  size_t dataChunkHeaderLength,
		  UnwrappedTsn lastCumulativeTsnAck,
		  std::function<bool(uint16_t /*streamId*/, uint32_t /*outgoingMessageId*/)> discardFromSendQueue)
		  : dataChunkHeaderLength(dataChunkHeaderLength),
		    lastCumulativeTsnAck(lastCumulativeTsnAck),
		    discardFromSendQueue(std::move(discardFromSendQueue)){ MS_TRACE() }

		    OutstandingData::AckInfo OutstandingData::HandleSack(
		      UnwrappedTsn cumulativeTsnAck,
		      std::span<const SackChunk::GapAckBlock> gapAckBlocks,
		      bool isInFastRecovery)
		{
			MS_TRACE();

			bool cumulativeTsnAckAdvanced = cumulativeTsnAck > this->lastCumulativeTsnAck;

			OutstandingData::AckInfo ackInfo(cumulativeTsnAck);

			// Erase all items up to cumulativeTsnAck.
			RemoveAcked(cumulativeTsnAck, ackInfo);

			// ACK packets reported in the gap ack blocks.
			AckGapBlocks(cumulativeTsnAck, gapAckBlocks, ackInfo);

			// NACK and possibly mark for retransmit chunks that weren't acked.
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
				// expire unacked (in-flight) chunks as they might have been received,
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
					// A non-expired chunk. No need to iterate any further.
					break;
				}
			}

			for (UnwrappedTsn tsnToExpire : tsnsToExpire)
			{
				// The item is retrieved by TSN, as AbandonAllFor() may have modified
				// `this->outstandingData` and invalidated iterators from the first
				// loop.
				Item& item = GetItem(tsnToExpire);

				MS_WARN_TAG(
				  sctp,
				  "marking nacked chunk %" PRIu32 " and message %" PRIu32 " as expired",
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

			// All chunks are always padded to be even divisible by 4.
			size_t chunkLength = GetSerializedChunkLength(data);

			this->unackedPayloadBytes += data.GetPayloadLength();
			this->unackedPacketBytes += chunkLength;
			++this->unackedItems;

			UnwrappedTsn tsn = GetNextTsn();
			Item& item       = this->outstandingData.emplace_back(
			  messageId, data.Clone(), timeSentMs, maxRetransmissions, expiresAtMs, lifecycleId);

			if (item.HasExpired(timeSentMs))
			{
				// No need to send it, it was expired when it was in the send queue.
				MS_WARN_TAG(
				  sctp,
				  "marking freshly produced chunk %" PRIu32 " and message %" PRIu32 " as expired",
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

			for (Item& item : this->outstandingData)
			{
				tsn.Increment();

				if (!item.IsAcked())
				{
					tsnsToNack.push_back(tsn);
				}
			}

			for (UnwrappedTsn tsnToNack : tsnsToNack)
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
				std::pair<uint16_t /*streamId*/, bool /*isUnordered*/> stream =
				  std::make_pair(item.GetData().GetStreamId(), item.GetData().IsUnordered());

				if (item.GetData().GetMessageId() > skippedPerStream[stream])
				{
					skippedPerStream[stream] = item.GetData().GetMessageId();
				}
			}

			auto* iForwardTsnChunk = packet->BuildChunkInPlace<IForwardTsnChunk>();

			iForwardTsnChunk->SetNewCumulativeTsn(newCumulativeAck.Wrap());

			for (const auto& [stream, mid] : skippedPerStream)
			{
				iForwardTsnChunk->AddStream(stream.first, stream.second, mid);
			}

			iForwardTsnChunk->Consolidate();
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
