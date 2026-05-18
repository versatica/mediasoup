#define MS_CLASS "RTC::SCTP::DataTracker"
// TODO: SCTP: COMMENT
#define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/rx/DataTracker.hpp"
#include "Logger.hpp"
#include "RTC/SCTP/packet/chunks/SackChunk.hpp"
#include <ranges>

namespace RTC
{
	namespace SCTP
	{
		DataTracker::DataTracker(BackoffTimerHandleInterface* delayedAckTimer, uint32_t remoteInitialTsn)
		  : delayedAckTimer(delayedAckTimer),
		    lastCumulativeAckedTsn(this->tsnUnwrapper.Unwrap(remoteInitialTsn - 1))
		{
			MS_TRACE();
		}

		bool DataTracker::IsTsnValid(uint32_t tsn) const
		{
			MS_TRACE();

			// Note that this method doesn't return `false` for old DATA/I-DATA chunks,
			// as those are actually valid, and receiving those may affect the
			// generated SACK response (by setting "duplicate TSNs").

			const Types::UnwrappedTsn unwrappedTsn = this->tsnUnwrapper.PeekUnwrap(tsn);
			const uint32_t difference =
			  Types::UnwrappedTsn::Difference(unwrappedTsn, this->lastCumulativeAckedTsn);

			if (difference > DataTracker::MaxAcceptedOutstandingFragments)
			{
				return false;
			}

			return true;
		}

		bool DataTracker::Observe(uint32_t tsn, bool immediateAck)
		{
			MS_TRACE();

			const Types::UnwrappedTsn unwrappedTsn = this->tsnUnwrapper.Unwrap(tsn);

			// `IsTsnValid()` must be called prior to calling this method.
			MS_ASSERT(
			  Types::UnwrappedTsn::Difference(unwrappedTsn, this->lastCumulativeAckedTsn) <=
			    DataTracker::MaxAcceptedOutstandingFragments,
			  "Types::UnwrappedTsn::Difference(unwrappedTsn, this->lastCumulativeAckedTsn) >     DataTracker::MaxAcceptedOutstandingFragments");

			bool isDuplicate = false;

			// Old chunk already seen before?
			if (unwrappedTsn <= this->lastCumulativeAckedTsn)
			{
				if (this->duplicateTsns.size() < DataTracker::MaxDuplicateTsnReported)
				{
					this->duplicateTsns.insert(unwrappedTsn.Wrap());
				}

				// https://datatracker.ietf.org/doc/html/rfc9260#section-6.2
				//
				// "When a packet arrives with duplicate DATA chunk(s) and with no new
				// DATA chunk(s), the endpoint MUST immediately send a SACK with no
				// delay. If a packet arrives with duplicate DATA chunk(s) bundled with
				// new DATA chunks, the endpoint MAY immediately send a SACK."
				UpdateAckState(AckState::IMMEDIATE, "duplicate data");

				isDuplicate = true;
			}
			else
			{
				if (unwrappedTsn == this->lastCumulativeAckedTsn.GetNextValue())
				{
					this->lastCumulativeAckedTsn = unwrappedTsn;

					// The cumulative acked `tsn` may be moved even further, if a gap was
					// filled.
					if (
					  !this->additionalTsnBlocks.IsEmpty() && this->additionalTsnBlocks.Front().firstTsn ==
					                                            this->lastCumulativeAckedTsn.GetNextValue())
					{
						this->lastCumulativeAckedTsn = this->additionalTsnBlocks.Front().lastTsn;
						this->additionalTsnBlocks.PopFront();
					}
				}
				else
				{
					const bool inserted = this->additionalTsnBlocks.Add(unwrappedTsn);

					if (!inserted)
					{
						// Already seen before.
						if (this->duplicateTsns.size() < DataTracker::MaxDuplicateTsnReported)
						{
							this->duplicateTsns.insert(unwrappedTsn.Wrap());
						}

						// https://datatracker.ietf.org/doc/html/rfc9260#section-6.2
						//
						// "When a packet arrives with duplicate DATA chunk(s) and with no
						// new DATA chunk(s), the endpoint MUST immediately send a SACK with
						// no delay. If a packet arrives with duplicate DATA chunk(s)
						// bundled with new DATA chunks, the endpoint MAY immediately send a
						// SACK."
						//
						// No need to do this. SACKs are sent immediately on packet loss below.
						isDuplicate = true;
					}
				}
			}

			// https://tools.ietf.org/html/rfc9260#section-6.7
			//
			// "Upon the reception of a new DATA chunk, an endpoint shall examine the
			// continuity of the TSNs received.  If the endpoint detects a gap in the
			// received DATA chunk sequence, it SHOULD send a SACK with Gap Ack Blocks
			// immediately. The data receiver continues sending a SACK after receipt
			// of each SCTP packet that doesn't fill the gap."
			if (!this->additionalTsnBlocks.IsEmpty())
			{
				UpdateAckState(AckState::IMMEDIATE, "packet loss");
			}

			// https://tools.ietf.org/html/rfc7053#section-5.2
			//
			// "Upon receipt of an SCTP packet containing a DATA chunk with the I bit
			// set, the receiver SHOULD NOT delay the sending of the corresponding
			// SACK chunk, i.e., the receiver SHOULD immediately respond with the
			// corresponding SACK chunk."
			if (immediateAck)
			{
				UpdateAckState(AckState::IMMEDIATE, "immediate-ack bit set");
			}

			if (!this->packetSeen)
			{
				// https://tools.ietf.org/html/rfc9260#section-5.1
				//
				// "After the reception of the first DATA chunk in an association the
				// endpoint MUST immediately respond with a SACK to acknowledge the DATA
				// chunk."
				this->packetSeen = true;

				UpdateAckState(AckState::IMMEDIATE, "first data chunk");
			}

			// https://tools.ietf.org/html/rfc9260#section-6.2
			//
			// "Specifically, an acknowledgement SHOULD be generated for at least
			// every second packet (not every second DATA chunk) received, and SHOULD
			// be generated within 200 ms of the arrival of any unacknowledged DATA
			// chunk."
			if (this->ackState == AckState::IDLE)
			{
				UpdateAckState(AckState::BECOMING_DELAYED, "received data when idle");
			}
			else if (this->ackState == AckState::DELAYED)
			{
				UpdateAckState(AckState::IMMEDIATE, "received data when already delayed");
			}

			return !isDuplicate;
		}

		void DataTracker::ObservePacketEnd()
		{
			MS_TRACE();

			if (this->ackState == AckState::BECOMING_DELAYED)
			{
				UpdateAckState(AckState::DELAYED, "packet end");
			}
		}

		bool DataTracker::HandleForwardTsn(uint32_t newCumulativeTsn)
		{
			// Forward-TSN is sent to make the receiver (this association) "forget"
			// about partly received (or not received at all) data, up until
			// `newCumulativeTsn`.

			const Types::UnwrappedTsn unwrappedTsn      = this->tsnUnwrapper.Unwrap(newCumulativeTsn);
			const Types::UnwrappedTsn prevLastCumAckTsn = this->lastCumulativeAckedTsn;

			// Old chunk already seen before?
			if (unwrappedTsn <= this->lastCumulativeAckedTsn)
			{
				// https://tools.ietf.org/html/rfc3758#section-3.6
				//
				// "Note, if the "New Cumulative TSN" value carried in the arrived
				// FORWARD-TSN chunk is found to be behind or at the current cumulative
				// TSN point, the data receiver MUST treat this FORWARD TSN as out-of-date
				// and MUST NOT update its Cumulative TSN.  The receiver SHOULD send a
				// SACK to its peer (the sender of the FORWARD TSN) since such a duplicate
				// may indicate the previous SACK was lost in the network."
				UpdateAckState(AckState::IMMEDIATE, "Forward TSN new cumulative tsn was behind");

				return false;
			}

			// https://tools.ietf.org/html/rfc3758#section-3.6
			//
			// "When a FORWARD TSN chunk arrives, the data receiver MUST first update
			// its cumulative TSN point to the value carried in the FORWARD TSN chunk,
			// and then MUST further advance its cumulative TSN point locally if
			// possible"

			// The `newCumulativeTsn` will become the current
			// `this->lastCumulativeAckedTsn`, and if there have been prior "gaps"
			// that are now overlapping with the new value, remove them.
			this->lastCumulativeAckedTsn = unwrappedTsn;
			this->additionalTsnBlocks.EraseTo(unwrappedTsn);

			// See if the `this->lastCumulativeAckedTsn` can be moved even further.
			if (
			  !this->additionalTsnBlocks.IsEmpty() &&
			  this->additionalTsnBlocks.Front().firstTsn == this->lastCumulativeAckedTsn.GetNextValue())
			{
				this->lastCumulativeAckedTsn = this->additionalTsnBlocks.Front().lastTsn;
				this->additionalTsnBlocks.PopFront();
			}

			MS_DEBUG_DEV(
			  "Forward TSN [prevLastCumAckTsn:%" PRIu32 ", newCumulativeTsn:%" PRIu32
			  ", lastCumulativeAckedTsn:%" PRIu32,
			  prevLastCumAckTsn.Wrap(),
			  newCumulativeTsn,
			  this->lastCumulativeAckedTsn.Wrap());

			// https://tools.ietf.org/html/rfc3758#section-3.6
			//
			// "Any time a FORWARD TSN chunk arrives, for the purposes of sending a
			// SACK, the receiver MUST follow the same rules as if a DATA chunk had
			// been received (i.e., follow the delayed sack rules specified in ...".
			if (this->ackState == AckState::IDLE)
			{
				UpdateAckState(AckState::BECOMING_DELAYED, "received forward TSN when idle");
			}
			else if (this->ackState == AckState::DELAYED)
			{
				UpdateAckState(AckState::IMMEDIATE, "received forward TSN when already delayed");
			}

			return true;
		}

		bool DataTracker::ShouldSendAck(bool alsoIfDelayed)
		{
			MS_TRACE();

			if (
			  this->ackState == AckState::IMMEDIATE ||
			  (alsoIfDelayed &&
			   (this->ackState == AckState::BECOMING_DELAYED || this->ackState == AckState::DELAYED)))
			{
				UpdateAckState(AckState::IDLE, "should send SACK");

				return true;
			}

			return false;
		}

		void DataTracker::ForceImmediateSack()
		{
			MS_TRACE();

			UpdateAckState(AckState::IMMEDIATE, "force immediate SACK");
		}

		bool DataTracker::WillIncreaseCumAckTsn(uint32_t tsn) const
		{
			MS_TRACE();

			const Types::UnwrappedTsn unwrappedTsn = this->tsnUnwrapper.PeekUnwrap(tsn);

			return unwrappedTsn == this->lastCumulativeAckedTsn.GetNextValue();
		}

		void DataTracker::AddSackSelectiveAck(Packet* packet, size_t aRwnd)
		{
			MS_TRACE();

			// Note that in SCTP, the receiver side is allowed to discard received data
			// and signal that to the sender, but only chunks that have previously been
			// reported in the gap-ack-blocks. However, this implementation will never
			// do that. So this SACK produced is more like a NR-SACK as explained in
			// https://ieeexplore.ieee.org/document/4697037 and which there is an RFC
			// draft at https://tools.ietf.org/html/draft-tuexen-tsvwg-sctp-multipath-17.

			auto* sackChunk = packet->BuildChunkInPlace<SackChunk>();

			sackChunk->SetCumulativeTsnAck(this->lastCumulativeAckedTsn.Wrap());
			sackChunk->SetAdvertisedReceiverWindowCredit(aRwnd);

			const auto& tsnBlocks = this->additionalTsnBlocks.GetBlocks();

			for (size_t i{ 0 }; i < tsnBlocks.size() && i < DataTracker::MaxGapAckBlocksReported; ++i)
			{
				const auto startDiff =
				  Types::UnwrappedTsn::Difference(tsnBlocks[i].firstTsn, this->lastCumulativeAckedTsn);
				const auto endDiff =
				  Types::UnwrappedTsn::Difference(tsnBlocks[i].lastTsn, this->lastCumulativeAckedTsn);

				sackChunk->AddAckBlock(startDiff, endDiff);
			}

			std::set<uint32_t> duplicateTsns;

			this->duplicateTsns.swap(duplicateTsns);

			for (const uint32_t tsn : duplicateTsns)
			{
				sackChunk->AddDuplicateTsn(tsn);
			}

			sackChunk->Consolidate();
		}

		void DataTracker::HandleDelayedAckTimerExpiry()
		{
			MS_TRACE();

			UpdateAckState(AckState::IMMEDIATE, "delayed ack timer expired");
		}

		void DataTracker::UpdateAckState(AckState newAckState, std::string_view reason)
		{
			MS_TRACE();

			if (newAckState != this->ackState)
			{
				const auto& previousAckStateStrView = DataTracker::AckStateToString(this->ackState);
				const auto& newAckStateStrView      = DataTracker::AckStateToString(newAckState);

#if MS_LOG_DEV_LEVEL == 3
				MS_DEBUG_DEV(
				  "ack state changed from %.*s to %.*s due to %.*s",
				  static_cast<int>(previousAckStateStrView.size()),
				  previousAckStateStrView.data(),
				  static_cast<int>(newAckStateStrView.size()),
				  newAckStateStrView.data(),
				  static_cast<int>(reason.size()),
				  reason.data());
#endif

				if (this->ackState == AckState::DELAYED)
				{
					this->delayedAckTimer->Stop();
				}
				else if (newAckState == AckState::DELAYED)
				{
					this->delayedAckTimer->Start();
				}
				this->ackState = newAckState;
			}
		}

		bool DataTracker::AdditionalTsnBlocks::Add(Types::UnwrappedTsn tsn)
		{
			MS_TRACE();

			// Find any block to expand. It will look for any block that includes (also
			// when expanded) the provided `tsn`. It will return the block that is greater
			// than, or equal to `tsn`.
			const auto it = std::ranges::lower_bound(
			  this->blocks,
			  tsn,
			  std::less<Types::UnwrappedTsn>{},
			  [](const TsnRange& e)
			  {
				  return e.lastTsn.GetNextValue();
			  });

			// No matching block found. There is no greater than, or equal block,
			// which means that this TSN is greater than any block. It can then be
			// inserted at the end.
			if (it == this->blocks.end())
			{
				this->blocks.emplace_back(tsn, tsn);

				return true;
			}

			// `tsn` is already in this block.
			if (tsn >= it->firstTsn && tsn <= it->lastTsn)
			{
				return false;
			}

			// This block can be expanded to the right, or merged with the next.
			if (it->lastTsn.GetNextValue() == tsn)
			{
				const auto nextIt = it + 1;

				if (nextIt != this->blocks.end() && tsn.GetNextValue() == nextIt->firstTsn)
				{
					// Expanding it would make it adjacent to next block, merge those.
					it->lastTsn = nextIt->lastTsn;

					this->blocks.erase(nextIt);

					return true;
				}

				// Expand to the right.
				it->lastTsn = tsn;

				return true;
			}

			// This block can be expanded to the left. Merging to the left would've
			// been covered by the above "merge to the right". Both blocks (expand a
			// right-most block to the left and expand a left-most block to the right)
			// would match, but the left-most would be returned by std::lower_bound.
			if (it->firstTsn == tsn.GetNextValue())
			{
				MS_ASSERT(
				  it == this->blocks.begin() || (it - 1)->lastTsn.GetNextValue() != tsn,
				  "got wrong iterator");

				it->firstTsn = tsn;

				return true;
			}

			// Need to create a new block in the middle.
			this->blocks.emplace(it, tsn, tsn);

			return true;
		}

		void DataTracker::AdditionalTsnBlocks::EraseTo(Types::UnwrappedTsn tsn)
		{
			MS_TRACE();

			// Find the block that is greater than or equals `tsn`.
			const auto it = std::ranges::lower_bound(
			  this->blocks, tsn, std::less<Types::UnwrappedTsn>{}, &TsnRange::lastTsn);

			// The block that is found is greater or equal (or possibly ::end(), when
			// no block is greater or equal). All blocks before this block can be
			// safely removed. The TSN might be within this block, so possibly truncate
			// it.
			const bool tsnIsWithinBlock = it != this->blocks.end() && tsn >= it->firstTsn;

			this->blocks.erase(this->blocks.begin(), it);

			if (tsnIsWithinBlock)
			{
				this->blocks.front().firstTsn = tsn.GetNextValue();
			}
		}

		void DataTracker::AdditionalTsnBlocks::PopFront()
		{
			MS_TRACE();

			MS_ASSERT(!this->blocks.empty(), "this->blocks is empty");

			this->blocks.erase(this->blocks.begin());
		}
	} // namespace SCTP
} // namespace RTC
