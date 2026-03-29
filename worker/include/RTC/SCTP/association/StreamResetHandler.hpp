#ifndef MS_RTC_SCTP_STREAM_RESET_HANDLER_HPP
#define MS_RTC_SCTP_STREAM_RESET_HANDLER_HPP

#include "common.hpp"
#include "RTC/SCTP/association/TCBContext.hpp"
#include "RTC/SCTP/common/UnwrappedSequenceNumber.hpp"
#include "RTC/SCTP/packet/chunks/ReConfigChunk.hpp"
#include "RTC/SCTP/packet/parameters/ReconfigurationResponseParameter.hpp"
#include "RTC/SCTP/public/AssociationListener.hpp"
#include "handles/BackoffTimerHandle.hpp"
#include <span>
#include <vector>

namespace RTC
{
	namespace SCTP
	{
		/**
		 * StreamResetHandler handles sending outgoing stream reset requests (to
		 * close an SCTP stream, which translates to closing a data channel in
		 * WebRTC).
		 *
		 * It also handles incoming "outgoing stream reset requests", when the peer
		 * wants to close its streams.
		 *
		 * Resetting streams is an asynchronous operation where the client will
		 * request a request a stream to be reset, but then it might not be
		 * performed exactly at this point. First, the sender might need to discard
		 * all messages that have been enqueued for this stream, or it may select to
		 * wait until all have been sent. At least, it must wait for the currently
		 * sending fragmented message to be fully sent, because a stream can't be
		 * reset while having received half a message. In the stream reset request,
		 * the "sender's last assigned TSN" is provided, which is simply the TSN for
		 * which the receiver should've received all messages before this value,
		 * before the stream can be reset. Since fragments can get lost or sent
		 * out-of-order, the receiver of a request may not have received all the
		 * data just yet, and then it will respond to the sender: "In progress". In
		 * other words, try again. The sender will then need to start a timer and
		 * try the very same request again (but with a new sequence number) until
		 * the receiver successfully performs the operation.
		 *
		 * All this can take some time, and may be driven by timers, so the client
		 * will ultimately be notified using callbacks.
		 *
		 * In this implementation, when a stream is reset, the queued but
		 * not-yet-sent messages will be discarded, but that may change in the future.
		 * RFC8831 allows both behaviors.
		 */
		class StreamResetHandler : public TCBContext, public BackoffTimerHandle::Listener
		{
		private:
			enum class ReqSeqNbrValidationResult : uint8_t
			{
				VALID,
				RETRANSMISSION,
				BADSEQUENCE_NUMBER,
			};

			/**
			 * Represents a stream request operation. There can only be one ongoing at
			 * any time, and a sent request may either succeed, fail or result in the
			 * receiver signaling that it can't process it right now, and then it will
			 * be retried.
			 */
			class CurrentRequest
			{
			public:
				CurrentRequest(uint32_t senderLastAssignedTsn, std::vector<uint16_t> streamIds)
				  : reqSeqNbr(std::nullopt),
				    senderLastAssignedTsn(senderLastAssignedTsn),
				    streamIds(std::move(streamIds))
				{
				}

				/**
				 * Returns the current request sequence number, if this request has been
				 * sent (check `HasBeenFirst()` first). Will return 0 if the request is
				 * just prepared (or scheduled for retransmission) but not yet sent.
				 */
				uint32_t GetReqSeqNbr() const
				{
					return this->reqSeqNbr.value_or(0);
				}

				/**
				 * The sender's last assigned TSN, from the retransmission queue. The
				 * receiver uses this to know when all data up to this TSN has been
				 * received, to know when to safely reset the stream.
				 */
				uint32_t GetSenderLastAssignedTsn() const
				{
					return senderLastAssignedTsn;
				}

				/**
				 * The streams that are to be reset.
				 */
				const std::vector<uint16_t>& GetStreamIds() const
				{
					return this->streamIds;
				}

				/**
				 * If this request has been sent yet. If not, then it's either because
				 * it has only been prepared and not yet sent, or because the received
				 * couldn't apply the request, and then the exact same request will be
				 * retried, but with a new sequence number.
				 */
				bool HasBeenSent() const
				{
					return this->reqSeqNbr.has_value();
				}

				/**
				 * If the receiver can't apply the request yet (and answered "In
				 * Progress"), this will be called to prepare the request to be
				 * retransmitted at a later time.
				 */
				void PrepareRetransmission()
				{
					this->reqSeqNbr = std::nullopt;
				}

				/**
				 * If the request hasn't been sent yet, this assigns it a request
				 * number.
				 */
				void PrepareToSend(uint32_t newReqSeqNbr)
				{
					this->reqSeqNbr = newReqSeqNbr;
				}

				void SetDeferred(bool isDeferred)
				{
					this->isDeferred = isDeferred;
				}

				bool IsDeferred() const
				{
					return this->isDeferred;
				}

			private:
				// If this is set, this request has been sent. If it's not set, the
				// request has been prepared, but has not yet been sent. This is
				// typically used when the peer responded "in progress" and the same
				// request (but a different request number) must be sent again.
				std::optional<uint32_t> reqSeqNbr{ 0 };
				// The sender's (that's us) last assigned TSN, from the retransmission
				// queue.
				uint32_t senderLastAssignedTsn{ 0 };
				// The streams that are to be reset in this request.
				std::vector<uint16_t> streamIds;
				// If the request is deferred (received "In Progress"), the next timeout
				// should not be treated as a timeout.
				bool isDeferred{ false };
			};

		public:
			StreamResetHandler(
			  AssociationListener& associationListener, TCBContext* tcbContext
			  // TODO: SCTP: Implement
			  // DataTracker* dataTracker,
			  // ReassemblyQueue* reassemblyQueue,
			  // RetransmissionQueue* retransmissionQueue
			);

			~StreamResetHandler() override;

		public:
			/**
			 * Initiates reset of the provided streams. While there can only be one
			 * ongoing stream reset request at any time, this method can be called at
			 * any time and also multiple times. It will enqueue requests that can't
			 * be directly fulfilled, and will asynchronously process them when any
			 * ongoing request has completed.
			 */
			void ResetStreams(std::span<const uint16_t> outgoingStreamIds);

			/**
			 * Creates a Reset Streams request that must be sent if returned. Will
			 * start the reconfig timer. Will return `nullptr` if there is no need
			 * to create a request (no streams to reset) or if there already is an
			 * ongoing stream reset request that hasn't completed yet.
			 */
			// TODO: SCTP: Do we really want to return a Chunk? Maybe we should pass
			// a Packet to use BuilChunkInPlace().
			ReConfigChunk* MakeStreamResetRequest();

			/**
			 * Called when handling and incoming RE-CONFIG chunk.
			 */
			void ProcessReceivedReConfigChunk(const ReConfigChunk* receivedReConfigChunk);

		private:
			void OnReconfigTimer(uint64_t& baseTimeoutMs, bool& stop);

			/* Pure virtual methods inherited from BackoffTimerHandle::Listener. */
		public:
			void OnTimer(BackoffTimerHandle* backoffTimer, uint64_t& baseTimeoutMs, bool& stop) override;

		private:
			AssociationListener& associationListener;
			TCBContext* tcbContext{ nullptr };
			// TODO: SCTP: Implement
			// DataTracker* dataTracker{ nullptr };,
			// ReassemblyQueue* reassemblyQueue{ nullptr };,
			// RetransmissionQueue* retransmissionQueue{ nullptr };
			UnwrappedSequenceNumber<uint32_t>::Unwrapper incomingReconfigRequestSnUnwrapper;
			const std::unique_ptr<BackoffTimerHandle> reconfigTimer;
			// The next sequence number for outgoing stream requests.
			uint32_t nextOutgoingReqSeqNbr{ 0 };
			// The current stream request operation.
			std::optional<CurrentRequest> currentRequest;
			// For incoming requests. Last processed request sequence number.
			UnwrappedSequenceNumber<uint32_t> lastProcessedReqSeqNbr;
			// The result from last processed incoming request.
			ReconfigurationResponseParameter::Result lastProcessedReqResult;
		};
	} // namespace SCTP
} // namespace RTC

#endif
