#define MS_CLASS "RTC::SCTP::RetransmissionQueue"
// TODO: SCTP: Comment.
#define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/tx/RetransmissionQueue.hpp"
#include "Logger.hpp"
#include "Utils.hpp"
#include "RTC/SCTP/packet/chunks/DataChunk.hpp"
#include "RTC/SCTP/packet/chunks/IDataChunk.hpp"
#include <cmath>   // std::min()
#include <numeric> // std::accumulate()
#include <string>

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
		  BackoffTimerHandleInterface* t3RtxTimer,
		  const SctpOptions& sctpOptions,
		  // NOTE: I don't like default argument values in dcsctp (true and false),
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

			const UnwrappedTsn oldLastCumulativeTsnAck = this->outstandingData.GetLastCumulativeTsnAck();
			const size_t oldUnackedPacketBytes         = this->outstandingData.GetUnackedPacketBytes();
#if MS_LOG_DEV_LEVEL == 3
			const size_t oldRwnd = this->rwnd;
#endif
			const UnwrappedTsn cumulativeTsnAck =
			  this->tsnUnwrapper.Unwrap(receivedSackChunk->GetCumulativeTsnAck());

			if (receivedSackChunk->GetValidatedGapAckBlocks().empty())
			{
				UpdateRttMs(nowMs, cumulativeTsnAck);
			}

			// Exit fast recovery before continuing processing, in case it needs to go
			// into fast recovery again due to new reported packet loss.
			MayExitFastRecovery(cumulativeTsnAck);

			const OutstandingData::AckInfo ackInfo = this->outstandingData.HandleSack(
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

		void RetransmissionQueue::HandleT3RtxTimerExpiry()
		{
			MS_TRACE();

			const size_t oldCwnd               = this->cwnd;
			const size_t oldUnackedPacketBytes = GetUnackedPacketBytes();

			// https://datatracker.ietf.org/doc/html/rfc9260#section-6.3.3
			//
			// "For the destination address for which the timer expires, adjust
			// its ssthresh with rules defined in Section 7.2.3 and set the cwnd
			// <- MTU."
			this->ssthresh = std::max(this->cwnd / 2, 4 * this->sctpOptions.mtu);
			this->cwnd     = 1 * this->sctpOptions.mtu;

			// Errata: https://datatracker.ietf.org/doc/html/rfc8540#section-3.11
			this->partialBytesAcked = 0;

			// https://datatracker.ietf.org/doc/html/rfc9260#section-6.3.3
			//
			// "For the destination address for which the timer expires, set RTO
			// <- RTO * 2 ("back off the timer").  The maximum value discussed in
			// rule C7 above (RTO.max) may be used to provide an upper bound to this
			// doubling operation."

			// Already done by the BackoffTimerHandle implementation.

			// https://datatracker.ietf.org/doc/html/rfc9260#section-6.3.3
			//
			// "Determine how many of the earliest (i.e., lowest TSN) outstanding
			// DATA chunks for the address for which the T3-rtx has expired will fit
			// into a single Packet"

			// https://datatracker.ietf.org/doc/html/rfc9260#section-6.3.3
			//
			// "Note: Any DATA chunks that were sent to the address for which the
			// T3-rtx timer expired but did not fit in one MTU (rule E3 above) should
			// be marked for retransmission and sent as soon as cwnd allows (normally,
			// when a SACK arrives)."
			this->outstandingData.NackAll();

			// https://datatracker.ietf.org/doc/html/rfc9260#section-6.3.3
			//
			// "Start the retransmission timer T3-rtx on the destination address to
			// which the retransmission is sent, if rule R1 above indicates to do so."

			// Already done by the BackoffTimerHandle implementation.

			MS_DEBUG_TAG(
			  sctp,
			  "T3-rtx timer has expired [cwnd:%zu, oldCwnd:%zu, ssthresh:%zu, unackedPacketBytes:%zu, oldUnackedPacketBytes:%zu]",
			  this->cwnd,
			  oldCwnd,
			  this->ssthresh,
			  GetUnackedPacketBytes(),
			  oldUnackedPacketBytes);
		}

		std::vector<std::pair<uint32_t /*tsn*/, UserData>> RetransmissionQueue::GetChunksForFastRetransmit(
		  size_t maxLength)
		{
			MS_TRACE();

			MS_ASSERT(
			  this->outstandingData.HasDataToBeFastRetransmitted(), "no data to be fast-retransmitted");
			MS_ASSERT(
			  Utils::Byte::IsPaddedTo4Bytes(maxLength),
			  "given maxLength %zu is not divisible by 4",
			  maxLength);

			std::vector<std::pair<uint32_t /*tsn*/, UserData>> result;

#if MS_LOG_DEV_LEVEL == 3
			const size_t oldUnackedPacketBytes = GetUnackedPacketBytes();
#endif

			result = this->outstandingData.GetChunksToBeFastRetransmitted(maxLength);

			MS_ASSERT(!result.empty(), "result cannot be empty");

			// https://datatracker.ietf.org/doc/html/rfc9260#section-7.2.4
			//
			// "4)  Restart the T3-rtx timer only if ... the endpoint is retransmitting
			// the first outstanding DATA chunk sent to that address."
			if (result[0].first == this->outstandingData.GetLastCumulativeTsnAck().GetNextValue().Wrap())
			{
				MS_DEBUG_DEV("first outstanding data to be retransmitted, restarting T3-rtx timer");

				this->t3RtxTimer->Stop();
			}

			// https://datatracker.ietf.org/doc/html/rfc9260#section-6.3.2
			//
			// "Every time a DATA chunk is sent to any address (including a
			// retransmission), if the T3-rtx timer of that address is not running,
			// start it running so that it will expire after the RTO of that address."
			if (!this->t3RtxTimer->IsRunning())
			{
				this->t3RtxTimer->Start();
			}

			const size_t bytesRetransmitted = std::accumulate(
			  result.begin(),
			  result.end(),
			  size_t{ 0 },
			  [&](size_t r, const std::pair<uint32_t /*tsn*/, UserData>& data)
			  {
				  return r + GetSerializedChunkLength(data.second);
			  });

			++this->rtxPacketsCount;
			this->rtxBytesCount += bytesRetransmitted;

#if MS_LOG_DEV_LEVEL == 3
			std::string tsnList;

			for (const auto& [tsn, data] : result)
			{
				if (!tsnList.empty())
				{
					tsnList += ',';
				}

				tsnList += std::to_string(tsn);
			}

			MS_DEBUG_DEV(
			  "fast-retransmitting TSN %s - %zu bytes [unackedPacketBytes:%zu, oldUnackedPacketBytes:%zu]",
			  tsnList.c_str(),
			  bytesRetransmitted,
			  GetUnackedPacketBytes(),
			  oldUnackedPacketBytes);
#endif

			return result;
		}

		std::vector<std::pair<uint32_t /*tsn*/, UserData>> RetransmissionQueue::GetChunksToSend(
		  uint64_t /*nowMs*/, size_t maxLength)
		{
			MS_TRACE();

			MS_ASSERT(
			  Utils::Byte::IsPaddedTo4Bytes(maxLength),
			  "given maxLength %zu is not divisible by 4",
			  maxLength);

			std::vector<std::pair<uint32_t /*tsn*/, UserData>> result;

			const size_t oldUnackedPacketBytes = GetUnackedPacketBytes();
#if MS_LOG_DEV_LEVEL == 3
			const size_t oldRwnd = this->rwnd;
#endif

			// Calculate the bandwidth budget (how many bytes that is allowed to be
			// sent).
			const size_t maxPacketBytesAllowedByCwnd =
			  oldUnackedPacketBytes >= this->cwnd ? 0 : this->cwnd - oldUnackedPacketBytes;

			size_t maxPacketBytesAllowedByRwnd =
			  Utils::Byte::PadTo4Bytes(GetRwnd() + this->dataChunkHeaderLength);

			// https://datatracker.ietf.org/doc/html/rfc9260#section-6.1
			//
			// "However, regardless of the value of rwnd (including if it is 0), the
			// data sender can always have one DATA chunk in flight to the receiver if
			// allowed by cwnd (see rule B, below)."
			if (this->outstandingData.GetUnackedItems() == 0)
			{
				maxPacketBytesAllowedByRwnd = this->sctpOptions.mtu;
			}

			size_t maxBytes = Utils::Byte::PadDownTo4Bytes(
			  std::min({ maxPacketBytesAllowedByCwnd, maxPacketBytesAllowedByRwnd, maxLength }));

			result = this->outstandingData.GetChunksToBeRetransmitted(maxBytes);

			const size_t bytesRetransmitted = std::accumulate(
			  result.begin(),
			  result.end(),
			  size_t{ 0 },
			  [&](size_t r, const std::pair<uint32_t /*tsn*/, UserData>& data)
			  {
				  return r + GetSerializedChunkLength(data.second);
			  });

			maxBytes -= bytesRetransmitted;

			if (!result.empty())
			{
				++this->rtxPacketsCount;
				this->rtxBytesCount += bytesRetransmitted;
			}

			while (maxBytes > this->dataChunkHeaderLength)
			{
				MS_ASSERT(
				  Utils::Byte::IsPaddedTo4Bytes(maxBytes),
				  "computed maxBytes %zu during the loop is not divisible by 4",
				  maxBytes);

				// TODO: SCTP: Implement and uncomment.

				// std::optional<SendQueue::DataToSend> chunkOpt =
				//   this->sendQueue.Produce(nowMs, maxBytes - this->dataChunkHeaderLength);

				// if (!chunkOpt.has_value())
				// {
				// 	break;
				// }

				// const size_t chunkSize = GetSerializedChunkSize(chunkOpt->data);

				// maxBytes -= chunkSize;

				// this->rwnd -= chunkOpt->data.size();

				// const std::optional<UnwrappedTsn> tsn = this->outstandingData.Insert(
				//   chunkOpt->messageId,
				//   chunkOpt->data,
				//   nowMs,
				//   this->supportsPartialReliability ? chunkOpt->maxRetransmissions
				//                                    : OutstandingData::MaxRetransmitsNoLimit,
				//   this->supportsPartialReliability ? chunkOpt->expiresAtMs
				//                                    : OutstandingData::ExpiresAtMsInfinite,
				//   chunkOpt->lifecycleId);

				// if (tsn.has_value())
				// {
				// 	if (chunkOpt->lifecycleId != 0)
				// 	{
				// 		MS_ASSERT(chunkOpt->data.IsEnd(), "data.IsEnd() should return true");

				// 		this->associationListener.OnAssociationLifecycleMessageFullySent(chunkOpt->lifecycleId);
				// 	}

				// 	result.emplace_back(tsn->Wrap(), std::move(chunkOpt->data));
				// }
			}

			// https://tools.ietf.org/html/rfc9260#section-6.3.2
			//
			// "Every time a DATA chunk is sent to any address (including a
			// retransmission), if the T3-rtx timer of that address is not running,
			// start it running so that it will expire after the RTO of that address."
			if (!result.empty())
			{
				if (!this->t3RtxTimer->IsRunning())
				{
					this->t3RtxTimer->Start();
				}

#if MS_LOG_DEV_LEVEL == 3
				std::string tsnList;

				for (const auto& [tsn, data] : result)
				{
					if (!tsnList.empty())
					{
						tsnList += ',';
					}

					tsnList += std::to_string(tsn);
				}

				const size_t bytesRetransmitted = std::accumulate(
				  result.begin(),
				  result.end(),
				  size_t{ 0 },
				  [&](size_t r, const std::pair<uint32_t, UserData>& d)
				  {
					  return r + GetSerializedChunkLength(d.second);
				  });

				MS_DEBUG_DEV(
				  "sending TSN %s - %zu bytes [unackedPacketBytes:%zu, oldUnackedPacketBytes:%zu, cwnd:%zu, rwnd:%zu, oldRwnd:%zu]",
				  tsnList.c_str(),
				  bytesRetransmitted,
				  GetUnackedPacketBytes(),
				  oldUnackedPacketBytes,
				  cwnd,
				  rwnd,
				  oldRwnd);
#endif
			}

			return result;
		}

		bool RetransmissionQueue::ShouldSendForwardTsn(uint64_t nowMs)
		{
			MS_TRACE();

			if (!this->supportsPartialReliability)
			{
				return false;
			}

			this->outstandingData.ExpireOutstandingChunks(nowMs);

			return this->outstandingData.ShouldSendForwardTsn();
		}

		void RetransmissionQueue::PrepareResetStream(uint16_t /*streamId*/)
		{
			MS_TRACE();

			// TODO: As per TODO comment in same method in dcsctp:
			//
			// "These calls are now only affecting the send queue. The packet buffer
			// can also change behavior - for example draining the chunk producer and
			// eagerly assign TSNs so that an "Outgoing SSN Reset Request" can be sent
			// quickly, with a known `sender_last_assigned_tsn`.
			// TODO: SCTP: Implement it.
			// this->sendQueue.PrepareResetStream(streamId);
		}

		bool RetransmissionQueue::HasStreamsReadyToBeReset() const
		{
			MS_TRACE();

			// TODO: SCTP: Implement it.
			// return this->sendQueue.HasStreamsReadyToBeReset();

			// TODO: SCTP: Remove.
			return false;
		}

		std::vector<uint16_t /*streamId*/> RetransmissionQueue::BeginResetStreams()
		{
			MS_TRACE();

			this->outstandingData.BeginResetStreams();

			// TODO: SCTP: Implement it.
			// this->sendQueue.GetStreamsReadyToBeReset();

			// TODO: SCTP: Remove.
			return {};
		}

		void RetransmissionQueue::CommitResetStreams()
		{
			MS_TRACE();

			// TODO: SCTP: Implement it.
			// this->sendQueue.CommitResetStreams();
		}

		void RetransmissionQueue::RollbackResetStreams()
		{
			MS_TRACE();

			// TODO: SCTP: Implement it.
			// this->sendQueue.RollbackResetStreams();
		}

		size_t RetransmissionQueue::GetSerializedChunkLength(const UserData& data) const
		{
			MS_TRACE();

			return Utils::Byte::PadTo4Bytes(this->dataChunkHeaderLength + data.GetPayloadLength());
		}

		bool RetransmissionQueue::IsSackChunkValid(const SackChunk* sackChunk) const
		{
			MS_TRACE();

			// https://tools.ietf.org/html/rfc9260#section-6.2.1
			//
			// "If Cumulative TSN Ack is less than the Cumulative TSN Ack Point, then
			// drop the SACK. Since Cumulative TSN Ack is monotonically increasing, a
			// SACK whose Cumulative TSN Ack is less than the Cumulative TSN Ack Point
			// indicates an out-of- order SACK."
			//
			// @remarks
			// - Important not to drop SACKs with identical TSN to that previously
			//   received, as the gap-ack-blocks or dup tsn fields may have changed.
			const UnwrappedTsn cumulativeTsnAck =
			  this->tsnUnwrapper.PeekUnwrap(sackChunk->GetCumulativeTsnAck());

			if (cumulativeTsnAck < this->outstandingData.GetLastCumulativeTsnAck())
			{
				// https://tools.ietf.org/html/rfc9260#section-6.2.1
				//
				// "If Cumulative TSN Ack is less than the Cumulative TSN Ack Point,
				// then drop the SACK.  Since Cumulative TSN Ack is monotonically
				// increasing, a SACK whose Cumulative TSN Ack is less than the
				// Cumulative TSN Ack Point indicates an out-of- order SACK."
				return false;
			}
			else if (cumulativeTsnAck > this->outstandingData.GetHighestOutstandingTsn())
			{
				return false;
			}

			return std::ranges::all_of(
			  sackChunk->GetValidatedGapAckBlocks(),
			  [&](const auto& block)
			  {
				  return UnwrappedTsn::AddTo(cumulativeTsnAck, block.end) <=
				         this->outstandingData.GetHighestOutstandingTsn();
			  });
		}

		void RetransmissionQueue::UpdateRttMs(uint64_t nowMs, UnwrappedTsn cumulativeTsnAck)
		{
			MS_TRACE();

			// RTT updating is flawed in SCTP, as explained in e.g. Pedersen J, Griwodz C,
			// Halvorsen P (2006) Considerations of SCTP retransmission delays for thin
			// streams.
			// Due to delayed acknowledgement, the SACK may be sent much later which
			// increases the calculated RTT.
			//
			// TODO: As per TODO comment in same method in dcsctp:
			//
			// "Consider occasionally sending DATA chunks with I-bit set and use only
			// those packets for measurement.

			const auto rttMs = this->outstandingData.MeasureRtt(nowMs, cumulativeTsnAck);

			if (rttMs.has_value())
			{
				this->listener->OnRetransmissionQueueNewRttMs(rttMs.value());
			}
		}

		void RetransmissionQueue::MayExitFastRecovery(UnwrappedTsn cumulativeTsnAck)
		{
			MS_TRACE();

			// https://tools.ietf.org/html/rfc9260#section-7.2.4
			//
			// "When a SACK acknowledges all TSNs up to and including this [fast
			// recovery] exit point, Fast Recovery is exited."
			if (this->fastRecoveryExitTsn.has_value() && cumulativeTsnAck >= this->fastRecoveryExitTsn.value())
			{
				MS_DEBUG_DEV(
				  "exit point %" PRIu32 " reached, exiting fast recovery",
				  this->fastRecoveryExitTsn.value().Wrap());

				this->fastRecoveryExitTsn = std::nullopt;
			}
		}

		void RetransmissionQueue::StopT3RtxTimerOnIncreasedCumulativeTsnAck(UnwrappedTsn /*cumulativeTsnAck*/)
		{
			MS_TRACE();

			// TODO: This method is NOT defined in dcsctp!
			//
			// @see https://issues.webrtc.org/issues/505751236
		}

		void RetransmissionQueue::HandleIncreasedCumulativeTsnAck(
		  size_t unackedPacketBytes, size_t totalBytesAcked)
		{
			MS_TRACE();

			// Allow some margin for classifying as fully utilized, due to e.g. that
			// too small packets (less than kMinimumFragmentedPayload) are not sent +
			// overhead.
			const bool isFullyUtilized = unackedPacketBytes + this->sctpOptions.mtu >= this->cwnd;
#if MS_LOG_DEV_LEVEL == 3
			const size_t oldCwnd = this->cwnd;
#endif

			if (GetCongestionAlgorithmPhase() == CongestionAlgorithmPhase::SLOW_START)
			{
				if (isFullyUtilized && !IsInFastRecovery())
				{
					// https://tools.ietf.org/html/rfc9260#section-7.2.1
					//
					// "Only when these three conditions are met can the cwnd be
					// increased; otherwise, the cwnd MUST not be increased. If these
					// conditions are met, then cwnd MUST be increased by, at most, the
					// lesser of 1) the total size of the previously outstanding DATA
					// chunk(s) acknowledged, and 2) the destination's path MTU."
					this->cwnd += std::min(totalBytesAcked, this->sctpOptions.mtu);

					MS_DEBUG_DEV("SS increase [cwnd:%zu, oldCwnd:%zu]", this->cwnd, oldCwnd);
				}
			}
			else if (GetCongestionAlgorithmPhase() == CongestionAlgorithmPhase::CONGESTION_AVOIDANCE)
			{
// https://tools.ietf.org/html/rfc9260#section-7.2.2
//
// "Whenever cwnd is greater than ssthresh, upon each SACK arrival
// that advances the Cumulative TSN Ack Point, increase
// partial_bytes_acked by the total number of bytes of all new chunks
// acknowledged in that SACK including chunks acknowledged by the new
// Cumulative TSN Ack and by Gap Ack Blocks."
#if MS_LOG_DEV_LEVEL == 3
				const size_t oldPba = this->partialBytesAcked;
#endif

				this->partialBytesAcked += totalBytesAcked;

				if (this->partialBytesAcked >= this->cwnd && isFullyUtilized)
				{
					// https://tools.ietf.org/html/rfc9260#section-7.2.2
					//
					// "When partial_bytes_acked is equal to or greater than cwnd and
					// before the arrival of the SACK the sender had cwnd or more bytes of
					// data outstanding (i.e., before arrival of the SACK, flightsize was
					// greater than or equal to cwnd), increase cwnd by MTU, and reset
					// partial_bytes_acked to (partial_bytes_acked - cwnd)."

					// Errata: https://datatracker.ietf.org/doc/html/rfc8540#section-3.12
					this->partialBytesAcked -= this->cwnd;
					this->cwnd += this->sctpOptions.mtu;

					MS_DEBUG_DEV(
					  "CA increase [cwnd:%zu, oldCwnd:%zu, ssthresh:%zu, pba:%zu, oldPba:%zu]",
					  this->cwnd,
					  oldCwnd,
					  this->ssthresh,
					  this->partialBytesAcked,
					  oldPba);
				}
				else
				{
					MS_DEBUG_DEV(
					  "CA unchanged [cwnd:%zu, oldCwnd:%zu, ssthresh:%zu, pba:%zu, oldPba:%zu]",
					  this->cwnd,
					  oldCwnd,
					  this->ssthresh,
					  this->partialBytesAcked,
					  oldPba);
				}
			}
		}

		void RetransmissionQueue::HandlePacketLoss(UnwrappedTsn /*highestTsnAcked*/)
		{
			MS_TRACE();

			// https://tools.ietf.org/html/rfc9260#section-7.2.4
			//
			// "If not in Fast Recovery, adjust the ssthresh and cwnd of the
			// destination address(es) to which the missing DATA chunks were last
			// sent, according to the formula described in Section 7.2.3."
			if (!IsInFastRecovery())
			{
#if MS_LOG_DEV_LEVEL == 3
				const size_t oldCwnd = this->cwnd;
				const size_t oldPba  = this->partialBytesAcked;
#endif

				this->ssthresh =
				  std::max(this->cwnd / 2, this->sctpOptions.minCwndMtus * this->sctpOptions.mtu);
				this->cwnd              = this->ssthresh;
				this->partialBytesAcked = 0;

				MS_DEBUG_DEV(
				  "packet loss detected (not fast recovery) [cwnd:%zu, oldCwnd:%zu, ssthresh:%zu, pba:%zu, oldPba:%zu]",
				  this->cwnd,
				  oldCwnd,
				  this->ssthresh,
				  this->partialBytesAcked,
				  oldPba);

				// https://tools.ietf.org/html/rfc9260#section-7.2.4
				//
				// "If not in Fast Recovery, enter Fast Recovery and mark the highest
				// outstanding TSN as the Fast Recovery exit point."
				this->fastRecoveryExitTsn = this->outstandingData.GetHighestOutstandingTsn();

				MS_DEBUG_DEV(
				  "fast recovery initiated with exit point %" PRIu32,
				  this->fastRecoveryExitTsn.value().Wrap());
			}
			// https://tools.ietf.org/html/rfc9260#section-7.2.4
			//
			// "While in Fast Recovery, the ssthresh and cwnd SHOULD NOT change for
			// any destinations due to a subsequent Fast Recovery event (i.e., one
			// SHOULD NOT reduce the cwnd further due to a subsequent Fast
			// Retransmit)."
			else
			{
				MS_DEBUG_DEV("packet loss detected (fast recovery), no changes");
			}
		}

		void RetransmissionQueue::UpdateReceiverWindow(uint32_t aRwnd)
		{
			MS_TRACE();

			this->rwnd = this->outstandingData.GetUnackedPayloadBytes() >= aRwnd
			               ? 0
			               : aRwnd - this->outstandingData.GetUnackedPayloadBytes();
		}

		void RetransmissionQueue::StartT3RtxTimerIfOutstandingData()
		{
			MS_TRACE();

			// Note: Can't use `GetUnackedPacketBytes()` as that one doesn't count
			// chunks to be retransmitted.

			// https://tools.ietf.org/html/rfc9260#section-6.3.2
			// "Whenever all outstanding data sent to an address have been
			// acknowledged, turn off the T3-rtx timer of that address.
			if (this->outstandingData.IsEmpty())
			{
				// Note: Already stopped in `StopT3RtxTimerOnIncreasedCumulativeTsnAck()`."
				//
				// TODO: As said above, `StopT3RtxTimerOnIncreasedCumulativeTsnAck()`
				// is NOT defined in dcsctp and of course it's never called from
				// anywhere.
			}
			else
			{
				// https://tools.ietf.org/html/rfc9260#section-6.3.2
				//
				// "Whenever a SACK is received that acknowledges the DATA chunk with
				// the earliest outstanding TSN for that address, restart the T3-rtx
				// timer for that address with its current RTO (if there is still
				// outstanding data on that address)."
				// "Whenever a SACK is received missing a TSN that was previously
				// acknowledged via a Gap Ack Block, start the T3-rtx for the
				// destination address to which the DATA chunk was originally
				// transmitted if it is not already running."
				if (!this->t3RtxTimer->IsRunning())
				{
					this->t3RtxTimer->Start();
				}
			}
		}
	} // namespace SCTP
} // namespace RTC
