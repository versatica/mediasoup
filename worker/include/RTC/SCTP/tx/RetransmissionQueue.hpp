#ifndef MS_RTC_SCTP_RETRANSMISSION_QUEUE_HPP
#define MS_RTC_SCTP_RETRANSMISSION_QUEUE_HPP

#include "common.hpp"
#include "RTC/SCTP/common/UnwrappedSequenceNumber.hpp"
#include "RTC/SCTP/packet/Packet.hpp"
#include "RTC/SCTP/packet/UserData.hpp"
#include "RTC/SCTP/packet/chunks/ForwardTsnChunk.hpp"
#include "RTC/SCTP/packet/chunks/IForwardTsnChunk.hpp"
#include "RTC/SCTP/packet/chunks/SackChunk.hpp"
#include "RTC/SCTP/public/AssociationListener.hpp"
#include "RTC/SCTP/public/SctpOptions.hpp"
#include "RTC/SCTP/tx/OutstandingData.hpp"
#include "handles/BackoffTimerHandleInterface.hpp"
#include <vector>

namespace RTC
{
	namespace SCTP
	{
		/**
		 * The RetransmissionQueue manages all DATA/I-DATA chunks that are in-flight
		 * and schedules them to be retransmitted if necessary. Chunks are
		 * retransmitted when they have been lost for a number of consecutive SACKs,
		 * or when the retransmission timer expires.
		 *
		 * As congestion control is tightly connected with the state of transmitted
		 * packets, that's also managed here to limit the amount of data that is
		 * in-flight (sent, but not yet acknowledged).
		 */
		class RetransmissionQueue
		{
		public:
			class Listener
			{
			public:
				virtual ~Listener() = default;

			public:
				virtual void OnRetransmissionQueueNewRttMs(uint64_t newRttMs)  = 0;
				virtual void OnRetransmissionQueueClearRetransmissionCounter() = 0;
			};

		public:
			using UnwrappedTsn = UnwrappedSequenceNumber<uint32_t>;

		private:
			enum class CongestionAlgorithmPhase : uint8_t
			{
				SLOW_START,
				CONGESTION_AVOIDANCE,
			};

		public:
			/**
			 * Creates a RetransmissionQueue which will send data using
			 * `localInitialTsn` as the first TSN to use for sent fragments. It will
			 * poll data from `sendQueue`. When SACKs are received, it will estimate
			 * the RTT and call `listener->OnRetransmissionQueueNewRttMs()`. When an
			 * outstanding Chunk has been acked, it will call
			 * `listener->OnRetransmissionQueueClearRetransmissionCounter() and will
			 * also use `t3RtxTimer`, which is the SCTP retransmission timer to manage
			 * retransmissions.
			 */
			RetransmissionQueue(
			  Listener* listener,
			  AssociationListener& associationListener,
			  uint32_t localInitialTsn,
			  uint32_t remoteAdvertisedReceiverWindowCredit,
			  // TODO: SCTP: Implement
			  // SendQueue& sendQueue,
			  BackoffTimerHandleInterface* t3RtxTimer,
			  const SctpOptions& sctpOptions,
			  // TODO: SCTP: I don't like these defaults (true and false), let's be explicit.
			  bool supportsPartialReliability,
			  bool useMessageInterleaving);

			~RetransmissionQueue();

		public:
			/**
			 * Handles a received SACK. Returns true if the SACK was processed and
			 * false if it was discarded due to received out-of-order and not relevant.
			 */
			bool HandleReceivedSackChunk(uint64_t nowMs, const SackChunk* receivedSackChunk);

			/**
			 * Handles an expired retransmission timer.
			 */
			void HandleT3RtxTimerExpiry();

			bool HasDataToBeFastRetransmitted() const
			{
				return this->outstandingData.HasDataToBeFastRetransmitted();
			}

			/**
			 * Returns a list of Chunks to "fast retransmit" that would fit in
			 * `maxLength` (bytes). The current value of `cwnd` is ignored.
			 */
			std::vector<std::pair<uint32_t /*tsn*/, UserData>> GetChunksForFastRetransmit(size_t maxLength);

			/**
			 * Returns a list of Chunks to send that would fit in `maxLength`
			 * (bytes). This may be further limited by the congestion control windows.
			 * Note that `ShouldSendForwardTsn()` must be called prior to this method,
			 * to abandon expired Chunks, as this method will not expire any Chunks.
			 */
			std::vector<std::pair<uint32_t /*tsn*/, UserData>> GetChunksToSend(
			  uint64_t nowMs, size_t maxLength);

			/**
			 * Returns the next TSN that will be allocated for sent DATA Chunks.
			 */
			uint32_t GetNextTsn() const
			{
				return this->outstandingData.GetNextTsn().Wrap();
			}

			uint32_t GetLastAssignedTsn() const
			{
				return UnwrappedTsn::AddTo(this->outstandingData.GetNextTsn(), -1).Wrap();
			}

			/**
			 * Returns the size of the congestion window, in bytes. This is the number
			 * of bytes that may be in-flight.
			 */
			size_t GetCwnd() const
			{
				return this->cwnd;
			}

			/**
			 * Overrides the current congestion window size.
			 */
			void SetCwnd(size_t cwnd)
			{
				this->cwnd = cwnd;
			}

			/**
			 * Returns the current receiver window size.
			 */
			size_t GetRwnd() const
			{
				return this->rwnd;
			}

			size_t GetRtxPacketsCount() const
			{
				return this->rtxPacketsCount;
			}

			uint64_t GetRtxBytesCount() const
			{
				return this->rtxBytesCount;
			}

			/**
			 * How many inflight bytes there are, as sent on the wire as packets.
			 */
			size_t GetUnackedPacketBytes() const
			{
				return this->outstandingData.GetUnackedPacketBytes();
			}

			/**
			 * Returns the number of DATA/I-DATA chunks that are in-flight.
			 */
			size_t GetUnackedItems() const
			{
				return this->outstandingData.GetUnackedItems();
			}

			/**
			 * Given the current time `nowMs`, it will evaluate if there are Chunks
			 * that have expired and that need to be discarded. It returns true if a
			 * FORWARD-TSN should be sent.
			 */
			bool ShouldSendForwardTsn(uint64_t nowMs);

			/**
			 * Creates a FORWARD-TSN Chunk and adds it to the given Packet.
			 */
			void CreateForwardTsn(Packet* packet) const
			{
				this->outstandingData.CreateForwardTsn(packet);
			}

			/**
			 * Creates an I-FORWARD-TSN Chunk and adds it to the given Packet.
			 */
			void CreateIForwardTsn(Packet* packet) const
			{
				this->outstandingData.CreateIForwardTsn(packet);
			}

			/**
			 * @see SendQueue for a longer description of these methods related
			 * to stream resetting.
			 */
			void PrepareResetStream(uint16_t streamId);
			bool HasStreamsReadyToBeReset() const;
			std::vector<uint16_t /*streamId*/> BeginResetStreams();
			void CommitResetStreams();
			void RollbackResetStreams();

#ifdef MS_TEST
			/**
			 * Returns the internal state of all queued Chunks.
			 *
			 * @remarks
			 * - This is only used in tests.
			 */
			std::vector<std::pair<uint32_t /*tsn*/, OutstandingData::State>> GetChunkStatesForTesting() const
			{
				return this->outstandingData.GetChunkStatesForTesting();
			}
#endif

		private:
			/**
			 * Returns how large a chunk will be, serialized, carrying the data.
			 */
			size_t GetSerializedChunkLength(const UserData& data) const;

			/**
			 * Indicates if the congestion control algorithm is in "fast recovery".
			 */
			bool IsInFastRecovery() const
			{
				return this->fastRecoveryExitTsn.has_value();
			}

			/**
			 * Indicates if the provided SACK Chunk is valid given what has previously
			 * been received. If it returns false, the SACK is most likely a duplicate
			 * of something already seen, so this returning false doesn't necessarily
			 * mean that the SACK is illegal.
			 */
			bool IsSackChunkValid(const SackChunk* sackChunk) const;

			/**
			 * When a SACK Chunk is received, this method will be called which may
			 * call into the `RetransmissionTimeout` to update the RTO.
			 */
			void UpdateRttMs(uint64_t nowMs, UnwrappedTsn cumulativeTsnAck);

			/**
			 * If the congestion control is in "fast recovery mode", this may be
			 * exited now.
			 */
			void MayExitFastRecovery(UnwrappedTsn cumulativeTsnAck);

			/**
			 * If Chunks have been ACKed, stop the retransmission timer.
			 *
			 * @remarks
			 * - This method is NOT defined in dcsctp! See bug report:
			 *   https://issues.webrtc.org/issues/505751236
			 */
			void StopT3RtxTimerOnIncreasedCumulativeTsnAck(UnwrappedTsn cumulativeTsnAck);

			/**
			 * Update the congestion control algorithm given as the cumulative ack TSN
			 * value has increased, as reported in an incoming SACK Chunk.
			 */
			void HandleIncreasedCumulativeTsnAck(size_t unackedPacketBytes, size_t totalBytesAcked);

			/**
			 * Update the congestion control algorithm, given as packet loss has been
			 * detected, as reported in an incoming SACK Chunk.
			 */
			void HandlePacketLoss(UnwrappedTsn highestTsnAcked);

			/**
			 * Update the view of the receiver window size.
			 */
			void UpdateReceiverWindow(uint32_t aRwnd);

			/**
			 * If there is data sent and not acked, ensure that the retransmission
			 * timer is running.
			 */
			void StartT3RtxTimerIfOutstandingData();

			/**
			 * Returns the current congestion control algorithm phase.
			 */
			CongestionAlgorithmPhase GetCongestionAlgorithmPhase() const
			{
				return (this->cwnd <= this->ssthresh) ? CongestionAlgorithmPhase::SLOW_START
				                                      : CongestionAlgorithmPhase::CONGESTION_AVOIDANCE;
			}

		private:
			Listener* listener;
			AssociationListener& associationListener;
			const SctpOptions sctpOptions;
			// If the peer supports RFC3758 "SCTP Partial Reliability Extension".
			bool supportsPartialReliability;
			// The length of the data chunk (DATA/I-DATA) header that is used.
			const size_t dataChunkHeaderLength;
			// The retransmission timer.
			BackoffTimerHandleInterface* t3RtxTimer;
			// Unwraps TSNs.
			UnwrappedTsn::Unwrapper tsnUnwrapper;
			// Congestion Window. Number of bytes that may be in-flight (sent, not
			// acked).
			size_t cwnd;
			// Receive Window. Number of bytes available in the receiver's RX buffer.
			size_t rwnd;
			// Slow Start Threshold. See RFC 9260.
			size_t ssthresh;
			// Partial Bytes Acked. See RFC 9260.
			size_t partialBytesAcked{ 0 };
			// See `AssociationMetrics`.
			size_t rtxPacketsCount{ 0 };
			uint64_t rtxBytesCount{ 0 };
			// If set, fast recovery is enabled until this TSN has been cumulative
			// acked.
			std::optional<UnwrappedTsn> fastRecoveryExitTsn{ std::nullopt };
			// The send queue.
			// TODO: SCTP: Implement.
			// SendQueue& sendQueue;
			// All the outstanding data Chunks that are in-flight and that have not
			// been cumulative acked. Note that it also contains chunks that have been
			// acked in gap-ack-blocks.
			OutstandingData outstandingData;
			// TODO: SCTP.
		};
	} // namespace SCTP
} // namespace RTC

#endif
