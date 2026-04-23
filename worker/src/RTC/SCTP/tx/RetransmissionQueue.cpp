#define MS_CLASS "RTC::SCTP::RetransmissionQueue"
// TODO: SCTP: Comment.
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/tx/RetransmissionQueue.hpp"
#include "Logger.hpp"
#include "RTC/Consts.hpp"
#include "RTC/SCTP/packet/Parameter.hpp"
#include "RTC/SCTP/packet/chunks/DataChunk.hpp"
#include "RTC/SCTP/packet/chunks/IDataChunk.hpp"
#include "RTC/SCTP/packet/parameters/ReconfigurationResponseParameter.hpp"

namespace RTC
{
	namespace SCTP
	{
		/* Instance methods. */

		RetransmissionQueue::RetransmissionQueue(
		  Listener* listener,
		  AssociationListener& associationListener,
		  uint32_t localInitialTsn,
		  uint32_t remoteAdvertisedReceiverWindowCredit,
		  // TODO: SCTP: Implement
		  // SendQueue& sendQueue,
		  BackoffTimerHandle* t3RtxTimer,
		  const SctpOptions& sctpOptions,
		  // TODO: SCTP: I don't like these defaults in dcsctp (true and false),
		  // let's be explicit.
		  bool supportsPartialReliability,
		  bool useMessageInterleaving)
		  : listener(listener),
		    associationListener(associationListener),
		    sctpOptions(sctpOptions),
		    supportsPartialReliability(supportsPartialReliability),
		    dataChunkHeaderLength(
		      useMessageInterleaving ? IDataChunk::IDataChunkHeaderLength
		                             : DataChunk::DataChunkHeaderLength),
		    t3RtxTimer(t3RtxTimer),
		    cwnd(sctpOptions.initialCwndMtus * sctpOptions.mtu),
		    rwnd(remoteAdvertisedReceiverWindowCredit),
		    // https://datatracker.ietf.org/doc/html/rfc9260#section-7.2.1
		    //
		    // "The initial value of ssthresh MAY be arbitrarily high (for example,
		    // implementations MAY use the size of the receiver advertised window)."
		    ssthresh(this->rwnd),
		    // TODO: SCTP: Implement.
		    // sendQueue(sendQueue),
		    outstandingData(
		      this->dataChunkHeaderLength,
		      this->tsnUnwrapper.Unwrap(localInitialTsn - 1),
		      [/*this*/](uint16_t /*streamId*/, uint32_t /*outgoingMessageId*/)
		      {
			      // TODO: SCTP: Implement.
			      // return this->sendQueue.Discard(streamId, outgoingMessageId);

			      // TODO: SCTP: Remove when the above is uncommented.
			      return false;
		      })
		{
			MS_TRACE();
		}

		RetransmissionQueue::~RetransmissionQueue()
		{
			MS_TRACE();
		}

		bool RetransmissionQueue::HandleReceivedSackChunk(uint64_t nowMs, const SackChunk* receivedSackChunk)
		{
			MS_TRACE();

			if (!IsSackChunkValid(receivedSackChunk))
			{
				return false;
			}

			UnwrappedTsn oldLastCumulativeTsnAck = this->outstandingData.GetLastCumulativeTsnAck();
			size_t oldUnackedPacketBytes         = this->outstandingData.GetUnackedPacketBytes();
#if MS_LOG_DEV_LEVEL == 3
			size_t oldRwnd = this->rwnd;
#endif
			UnwrappedTsn cumulativeTsnAck =
			  this->tsnUnwrapper.Unwrap(receivedSackChunk->GetCumulativeTsnAck());

			if (receivedSackChunk->GetValidatedGapAckBlocks().empty())
			{
				UpdateRttMs(nowMs, cumulativeTsnAck);
			}

			// Exit fast recovery before continuing processing, in case it needs to go
			// into fast recovery again due to new reported packet loss.
			MayExitFastRecovery(cumulativeTsnAck);

			OutstandingData::AckInfo ackInfo = this->outstandingData.HandleSack(
			  cumulativeTsnAck, receivedSackChunk->GetValidatedGapAckBlocks(), IsInFastRecovery());

			// Add lifecycle events for delivered messages.
			for (const uint64_t lifecycleId : ackInfo.ackedLifecycleIds)
			{
				MS_DEBUG_TAG(
				  sctp,
				  "triggering OnAssociationLifecycleMessageDelivered() [lifecycleId:%" PRIu64 "]",
				  lifecycleId);

				this->associationListener.OnAssociationLifecycleMessageDelivered(lifecycleId);
				this->associationListener.OnAssociationLifecycleMessageEnd(lifecycleId);
			}

			for (const uint64_t lifecycleId : ackInfo.abandonedLifecycleIds)
			{
				MS_DEBUG_TAG(
				  sctp,
				  "triggering OnLifecycleMessageExpired() [lifecycleId:%" PRIu64 ", maybeDelivered:true]",
				  lifecycleId);

				this->associationListener.OnAssociationLifecycleMessageExpired(
				  lifecycleId, /*maybeDelivered*/ true);
				this->associationListener.OnAssociationLifecycleMessageEnd(lifecycleId);
			}

			// Update of this->outstandingData is now done. Congestion control remains.
			UpdateReceiverWindow(receivedSackChunk->GetAdvertisedReceiverWindowCredit());

			MS_DEBUG_DEV(
			  "Received SACK [cumulativeTsnAck:%" PRIu32 ", oldLastCumulativeTsnAck:%" PRIu32
			  ", unackedPacketBytes:%zu, oldUnackedPacketBytes:%zu, rwnd:%zu, oldRwnd:%zu]",
			  cumulativeTsnAck.Wrap(),
			  oldLastCumulativeTsnAck.Wrap(),
			  this->outstandingData.GetUnackedPacketBytes(),
			  oldUnackedPacketBytes,
			  this->rwnd,
			  oldRwnd);

			if (cumulativeTsnAck > oldLastCumulativeTsnAck)
			{
				// https://datatracker.ietf.org/doc/html/rfc9260#section-6.3.2
				//
				// "Whenever a SACK is received that acknowledges the DATA chunk with
				// the earliest outstanding TSN for that address, restart the T3-rtx
				// timer for that address with its current RTO (if there is still
				// outstanding data on that address)."
				this->t3RtxTimer->Stop();

				HandleIncreasedCumulativeTsnAck(oldUnackedPacketBytes, ackInfo.bytesAcked);
			}

			if (ackInfo.hasPacketLoss)
			{
				HandlePacketLoss(ackInfo.highestTsnAcked);
			}

			// https://datatracker.ietf.org/doc/html/rfc9260#section-8.2
			//
			// "When an outstanding TSN is acknowledged [...] the endpoint shall clear
			// the error counter ...".
			if (ackInfo.bytesAcked > 0)
			{
				this->listener->OnRetransmissionQueueClearRetransmissionCounter();
			}

			StartT3RtxTimerIfOutstandingData();

			return true;
		}
	} // namespace SCTP
} // namespace RTC
