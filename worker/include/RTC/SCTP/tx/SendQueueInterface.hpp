#ifndef MS_RTC_SCTP_SEND_QUEUE_INTERFACE_HPP
#define MS_RTC_SCTP_SEND_QUEUE_INTERFACE_HPP

#include "common.hpp"
#include "RTC/SCTP/packet/UserData.hpp"
#include "RTC/SCTP/public/SctpTypes.hpp"
#include <vector>

namespace RTC
{
	namespace SCTP
	{
		class SendQueueInterface
		{
		public:
			/**
			 * Container for a data chunk that is produced by the send queue.
			 */
			struct DataToSend
			{
				DataToSend(uint32_t outgoingMessageId, UserData data)
				  : outgoingMessageId(outgoingMessageId), data(std::move(data))
				{
				}

				uint32_t outgoingMessageId;

				/**
				 * The data to send, including all parameters.
				 */
				UserData data;

				/**
				 * Partial reliability (RFC 3758).
				 */
				uint16_t maxRetransmissions{ Types::MaxRetransmitsNoLimit };

				/**
				 * Time when it expires.
				 */
				uint64_t expiresAtMs{ Types::ExpiresAtMsInfinite };

				/**
				 * Lifecycle. Set for the last fragment and `std::nullopt` for all
				 * other fragments.
				 */
				std::optional<uint64_t> lifecycleId;
			};

		public:
			virtual ~SendQueueInterface() = default;

			/**
			 * Configures the send queue to support interleaved message sending as
			 * described in RFC 8260. Every send queue starts with this value set as
			 * disabled, but can later change it when the capabilities of the
			 * connection have been negotiated. This affects the behavior of the
			 * `Produce()` method.
			 */
			virtual void EnableMessageInterleaving(bool enabled) = 0;

			/**
			 * Produce a chunk to be sent. `maxLength` refers to how many payload bytes
			 * that may be produced, not including any headers.
			 *
			 * @todo
			 * - As in dcsctp, this interface is obviously missing an "AddMessage()"
			 *   method, but that is postponed a bit until the story around how to
			 *   model message prioritization, which is important for any advanced
			 *   stream scheduler, is further clarified.
			 */
			virtual std::optional<DataToSend> Produce(uint64_t nowMs, size_t maxLength) = 0;

			/**
			 * Discards a partially sent message identified by the parameters `streamId`
			 * and `outgoingMessageId`. The `outgoingMessageId` comes from the returned
			 * information when having called `Produce()`. A partially sent message
			 * means that it has had at least one fragment of it returned when
			 * `Produce()` was called prior to calling this method.
			 *
			 * This is used when a message has been found to be expired (by the partial
			 * reliability extension), and the retransmission queue will signal the
			 * receiver that any partially received message fragments should be
			 * skipped. This means that any remaining fragments in the send queue must
			 * be removed as well so that they are not sent.
			 *
			 * This method returns true if this message had unsent fragments still in
			 * the queue that were discarded, and false if there were no such
			 * fragments.
			 */
			virtual bool Discard(uint16_t streamId, uint32_t outgoingMessageId) = 0;

			/**
			 * Prepares the stream to be reset. This is used to close a SCTP stream
			 * and will be signaled to the other side.
			 *
			 * Concretely, it discards all whole (not partly sent) messages in the
			 * given stream and pauses that stream so that future added messages are
			 * not produced until resumed.
			 *
			 * This method can be called multiple times to add more streams to be
			 * reset, and paused while they are resetting. This is the first part of
			 * the two-phase commit protocol to reset streams, where the caller
			 * completes the procedure by either calling `CommitResetStreams()` or
			 * `RollbackResetStreams()`.
			 *
			 * @todo
			 * - As in dcsctp, investigate if it really should discard any message at
			 *   all. RFC 8831 only mentions that "RFC 6525 also guarantees that all
			 *   the messages are delivered (or abandoned) before the stream is reset".
			 */
			virtual void PrepareResetStream(uint16_t streamId) = 0;

			/**
			 * Indicates if there are any streams that are ready to be reset.
			 */
			virtual bool HasStreamsReadyToBeReset() const = 0;

			/**
			 * Returns a list of streams that are ready to be included in an outgoing
			 * stream reset request. Any streams that are returned here must be
			 * included in an outgoing stream reset request, and there must not be
			 * concurrent requests.
			 */
			virtual std::vector<uint16_t> GetStreamsReadyToBeReset() = 0;

			/**
			 * Called to commit to reset the streams returned by
			 * `GetStreamsReadyToBeReset()`. It will reset the stream sequence numbers
			 * (SSNs) and message identifiers (MIDs) and resume the paused streams.
			 */
			virtual void CommitResetStreams() = 0;

			/**
			 * Called to abort the resetting of streams returned by
			 * `GetStreamsReadyToBeReset()`. Will resume the paused streams without
			 * resetting the stream sequence numbers (SSNs) or message identifiers
			 * (MIDs). Note that the non-partial messages that were discarded when
			 * calling `PrepareResetStreams()` will not be recovered, to better match
			 * the intention from the sender to "close the channel".
			 */
			virtual void RollbackResetStreams() = 0;

			/**
			 * Resets all message identifier counters (MID, SSN) and makes all
			 * partially messages be ready to be re-sent in full. This is used when
			 * the peer has been detected to have restarted and is used to try to
			 * minimize the amount of data loss. However, data loss cannot be
			 * completely guaranteed when a peer restarts.
			 */
			virtual void Reset() = 0;

			/**
			 * Returns the amount of buffered data. This doesn't include packets that
			 * are e.g. inflight.
			 */
			virtual size_t GetStreamBufferedAmount(uint16_t streamId) const = 0;

			/**
			 * Returns the total amount of buffer data, for all streams.
			 */
			virtual size_t GetTotalBufferedAmount() const = 0;

			/**
			 * Returns the limit for the `OnAssociationStreamBufferedAmountLow()`
			 * event. Default value is 0.
			 */
			virtual size_t GetStreamBufferedAmountLowThreshold(uint16_t streamId) const = 0;

			/**
			 * Sets a limit for the `OnAssociationStreamBufferedAmountLow()` event.
			 * @param streamId [description]
			 * @param bytes    [description]
			 */
			virtual void SetStreamBufferedAmountLowThreshold(uint16_t streamId, size_t bytes) = 0;
		};
	} // namespace SCTP
} // namespace RTC

#endif
