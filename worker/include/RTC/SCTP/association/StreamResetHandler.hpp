#ifndef MS_RTC_SCTP_STREAM_RESET_HANDLER_HPP
#define MS_RTC_SCTP_STREAM_RESET_HANDLER_HPP

#include "common.hpp"
#include "SharedInterface.hpp"
#include "RTC/SCTP/association/TCBContext.hpp"
#include "RTC/SCTP/common/UnwrappedSequenceNumber.hpp"
#include "RTC/SCTP/packet/Packet.hpp"
#include "RTC/SCTP/packet/chunks/ReConfigChunk.hpp"
#include "RTC/SCTP/packet/parameters/IncomingSsnResetRequestParameter.hpp"
#include "RTC/SCTP/packet/parameters/OutgoingSsnResetRequestParameter.hpp"
#include "RTC/SCTP/packet/parameters/ReconfigurationResponseParameter.hpp"
#include "RTC/SCTP/public/AssociationListener.hpp"
#include "RTC/SCTP/tx/RetransmissionQueue.hpp"
#include "handles/BackoffTimerHandleInterface.hpp"
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
		class StreamResetHandler : public BackoffTimerHandleInterface::Listener
		{
		private:
			enum class ReqSeqNbrValidationResult : uint8_t
			{
				VALID,
				RETRANSMISSION,
				BAD_SEQUENCE_NUMBER,
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

		private:
			using UnwrappedReConfigRequestSn = UnwrappedSequenceNumber<uint32_t>;

		public:
			StreamResetHandler(
			  AssociationListener& associationListener,
			  SharedInterface* shared,
			  TCBContext* tcbContext,
			  // TODO: SCTP: Implement
			  // DataTracker* dataTracker,
			  // ReassemblyQueue* reassemblyQueue,
			  RetransmissionQueue* retransmissionQueue);

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
			 * Whether a Reset Streams request should be send. Will return `false` if
			 * there is no need to create a request (no streams to reset) or if there
			 * already is an ongoing stream reset request that hasn't completed yet.
			 */
			bool ShouldCreateStreamResetRequest() const;

			/**
			 * Creates a Reset Streams request that must be sent if returned. Will
			 * start the reconfig timer.
			 *
			 * @remarks
			 * - The caller must check `ShouldCreateStreamResetRequest()` first and
			 *   only invoke this method if the former returns `true`.
			 */
			void CreateStreamResetRequest(Packet* packet);

			/**
			 * Called when handling and incoming RE-CONFIG chunk. Processes a stream
			 * reconfiguration chunk and may send a RE-CONFIG back to the peer with
			 * either 1 or 2 responses.
			 */
			void HandleReceivedReConfigChunk(const ReConfigChunk* receivedReConfigChunk);

		private:
			/**
			 * Called to validate a received RE-CONFIG chunk.
			 */
			bool ValidateReceivedReConfigChunk(const ReConfigChunk* receivedReConfigChunk);

			/**
			 * Adds the actual RE-CONFIG chunk to the given Packet. A request (which
			 * set `this->currentRequest`) must have been created prior.
			 */
			void CreateReConfigChunk(Packet* packet);

			/**
			 * Called to validate the `reqSeqNbr`, that it's the next in sequence.
			 */
			ReqSeqNbrValidationResult ValidateReqSeqNbr(UnwrappedReConfigRequestSn reqSeqNbr);

			/**
			 * Called when this Association receives an outgoing stream reset request.
			 * It might either be performed straight away, or have to be deferred, and
			 * the result of that will be put in `responses`.
			 */
			void HandleReceivedOutgoingSsnResetRequestParameter(
			  const OutgoingSsnResetRequestParameter* receivedOutgoingSsnResetRequestParameter,
			  ReConfigChunk* reConfigChunk);

			/**
			 * Called when this Association receives an incoming stream reset request.
			 * This isn't really supported, but a successful response is put in
			 * `responses`.
			 */
			void HandleReceivedIncomingSsnResetRequestParameter(
			  const IncomingSsnResetRequestParameter* receivedIncomingSsnResetRequestParameter,
			  ReConfigChunk* reConfigChunk);

			/**
			 * Called when receiving a response to an outgoing stream reset request.
			 * It will either commit the stream resetting, if the operation was
			 * successful, or will schedule a retry if it was deferred. And if it
			 * failed, the operation will be rolled back.
			 */
			void HandleReceivedReconfigurationResponseParameter(
			  const ReconfigurationResponseParameter* receivedReconfigurationResponseParameter);

			void OnReConfigTimer(uint64_t& baseTimeoutMs, bool& stop);

			/* Pure virtual methods inherited from BackoffTimerHandleInterface::Listener. */
		public:
			void OnTimer(BackoffTimerHandleInterface* backoffTimer, uint64_t& baseTimeoutMs, bool& stop) override;

		private:
			AssociationListener& associationListener;
			SharedInterface* shared;
			TCBContext* tcbContext;
			// TODO: SCTP: Implement
			// DataTracker* dataTracker;,
			// TODO: SCTP: Implement
			// ReassemblyQueue* reassemblyQueue;,
			RetransmissionQueue* retransmissionQueue;
			UnwrappedReConfigRequestSn::Unwrapper incomingReConfigRequestSnUnwrapper;
			const std::unique_ptr<BackoffTimerHandleInterface> reConfigTimer;
			// The next sequence number for outgoing stream requests.
			uint32_t nextOutgoingReqSeqNbr{ 0 };
			// The current stream request operation.
			std::optional<CurrentRequest> currentRequest;
			// For incoming requests. Last processed request sequence number.
			UnwrappedReConfigRequestSn lastProcessedReqSeqNbr;
			// The result from last processed incoming request.
			ReconfigurationResponseParameter::Result lastProcessedReqResult;
		};
	} // namespace SCTP
} // namespace RTC

#endif
