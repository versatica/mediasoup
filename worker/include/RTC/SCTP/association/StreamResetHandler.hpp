#ifndef MS_RTC_SCTP_STREAM_RESET_HANDLER_HPP
#define MS_RTC_SCTP_STREAM_RESET_HANDLER_HPP

#include "common.hpp"
#include "RTC/SCTP/association/TCBContext.hpp"
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
			void Dump(int indentation = 0) const;

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
			const std::unique_ptr<BackoffTimerHandle> reconfigTimer;
		};
	} // namespace SCTP
} // namespace RTC

#endif
