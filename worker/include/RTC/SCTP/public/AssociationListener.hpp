#ifndef MS_RTC_SCTP_ASSOCIATION_LISTENER_HPP
#define MS_RTC_SCTP_ASSOCIATION_LISTENER_HPP

#include "common.hpp"
#include "RTC/SCTP/public/Message.hpp"
#include "RTC/SCTP/public/SctpTypes.hpp"
#include <span>
#include <string_view>

namespace RTC
{
	namespace SCTP
	{
		class AssociationListener
		{
		public:
			virtual ~AssociationListener() = default;

		public:
			/**
			 * Called when an SCTP Packet must be sent to the remote endpoint.
			 *
			 * @return
			 * - `true` if the packet was successfully sent. However, since
			 *   sending is unreliable, there are no guarantees that the Packet was
			 *   actually delivered.
			 * - `false` if the Packet failed to be sent.
			 *
			 * @remarks
			 * - It is NOT allowed to call methods in Association within this callback.
			 */
			virtual bool OnAssociationSendData(const uint8_t* data, size_t len) = 0;

			/**
			 * Called when calling Connect().
			 *
			 * @remarks
			 * - It is allowed to call methods in Association within this callback.
			 */
			virtual void OnAssociationConnecting() = 0;

			/**
			 * Called when calling Connect() succeeds and also for incoming successful
			 * connection attempts.
			 *
			 * @remarks
			 * - It is allowed to call methods in Association within this callback.
			 */
			virtual void OnAssociationConnected() = 0;

			/**
			 * Called when calling Connect() and also for incoming connection attempts
			 * in case the association fails.
			 *
			 * @remarks
			 * - It is allowed to call methods in Association within this callback.
			 */
			virtual void OnAssociationFailed(Types::ErrorKind errorKind, std::string_view errorMessage) = 0;

			/**
			 * Called when the Association is closed in a controlled way or when the
			 * Association has aborted - either as decided by this Association due to
			 * e.g. too many retransmission attempts or by the peer when receiving an
			 * ABORT command. No other callbacks will be done after this callback,
			 * unless reconnecting.
			 *
			 * @remarks
			 * - It is allowed to call methods in Association within this callback.
			 */
			virtual void OnAssociationClosed(Types::ErrorKind errorKind, std::string_view errorMessage) = 0;

			/**
			 * Called on connection restarted (by peer). This is just a notification,
			 * and the association is expected to work fine after this call, but there
			 * could have been packet loss as a result of restarting the association.
			 *
			 * @remarks
			 * - It is allowed to call methods in Association within this callback.
			 */
			virtual void OnAssociationRestarted() = 0;

			/**
			 * Triggered when an non-fatal error is reported by either this library or
			 * from the other peer (by sending an ERROR command). These should be
			 * logged, but no other action need to be taken as the association is still
			 * viable.
			 *
			 * @remarks
			 * - It is allowed to call methods in Association within this callback.
			 */
			virtual void OnAssociationError(Types::ErrorKind errorKind, std::string_view errorMessage) = 0;

			/**
			 * Called when an SCTP message in full has been received.
			 *
			 * @remarks
			 * - It is allowed to call methods in Association within this callback.
			 */
			virtual void OnAssociationMessageReceived(Message message) = 0;

			/**
			 * Indicates that a stream reset request has been performed.
			 *
			 * @remarks
			 * - It is allowed to call methods in Association within this callback.
			 */
			virtual void OnAssociationStreamsResetPerformed(std::span<const uint16_t> outboundStreamIds) = 0;

			/**
			 * Indicates that a stream reset request has failed.
			 *
			 * @remarks
			 * - It is allowed to call methods in Association within this callback.
			 */
			virtual void OnAssociationStreamsResetFailed(
			  std::span<const uint16_t> outboundStreamIds, std::string_view errorMessage) = 0;

			/**
			 * When a peer has reset some of its outbound streams, this will be
			 * called. An empty list indicates that all streams have been reset.
			 *
			 * @remarks
			 * - It is allowed to call methods in Association within this callback.
			 */
			virtual void OnAssociationInboundStreamsReset(std::span<const uint16_t> inboundStreamIds) = 0;

			/**
			 * Called when the amount of data buffered to be sent falls to or below
			 * the threshold set when calling SetStreamBufferedAmountLowThreshold().
			 *
			 * @remarks
			 * - It is allowed to call methods in Association within this callback.
			 */
			virtual void OnAssociationStreamBufferedAmountLow(uint16_t streamId) = 0;

			/**
			 * Called when the total amount of data buffered (in the entire send
			 * buffer, for all streams) falls to or below the threshold specified in
			 * SctpOptions::totalBufferedAmountLowThreshold`.
			 *
			 * @remarks
			 * - It is allowed to call methods in Association within this callback.
			 */
			virtual void OnAssociationTotalBufferedAmountLow() = 0;

			/**
			 * SCTP message lifecycle events.
			 *
			 * If a `lifecycleId` is provided as `MessageSendOptions`, lifecycle
			 * callbacks will be triggered as the message is processed by the library.
			 *
			 * The possible transitions are shown in the graph below:
			 *
			 * Association::SendMessage() ────────────────────────┐
			 *                │                                   │
			 *                │                                   │
			 *                v                                   v
			 * OnAssociationLifecycleMessageFullySent ──> OnAssociationLifecycleMessageExpired
			 *                │                                   │
			 *                │                                   │
			 *                v                                   v
			 * OnAssociationLifeCycleMessageDelivered ──>   OnAssociationLifecycleEnd
			 */

			/**
			 * Called when a message has been fully sent, meaning that the last
			 * fragment has been produced from the send queue and sent on the network.
			 * Note that this will trigger at most once per message even if the
			 * message was retransmitted due to packet loss.
			 *
			 * @remarks
			 * - This is a message lifecycle event.
			 * - It is NOT allowed to call methods in Association within this callback.
			 */
			virtual void OnAssociationLifecycleMessageFullySent(uint64_t lifecycleId) {};

			/**
			 * Called when a message has expired. If it was expired with data
			 * remaining in the send queue that had not been sent ever,
			 * `maybeDelivered` will be set to false. If `maybeDelivered` is true,
			 * the message has at least once been sent and may have been correctly
			 * received by the peer, but it has expired before the receiver managed
			 * to acknowledge it. This means that if `maybeDelivered` is true, it's
			 * unknown if the message was lost or was delivered, and if
			 * `maybeDelivered` is false, it's guaranteed to not be delivered.
			 *
			 * @remarks
			 * - This is a message lifecycle event.
			 * - It's guaranteed that OnAssociationLifecycleMessageDelivered() is not called
			 *   if this callback has triggered.
			 * - It is NOT allowed to call methods in Association within this callback.
			 */
			virtual void OnAssociationLifecycleMessageExpired(uint64_t lifecycleId, bool maybeDelivered)
			{
			}

			/**
			 * Called whena non-expired message has been acknowledged by the peer as
			 * delivered.
			 *
			 * Note that this will trigger only when the peer moves its cumulative
			 * TSN ack beyond this message, and will not fire for messages acked using
			 * gap-ack-blocks as those are renegable. This means that this may fire a
			 * bit later than the message was actually first "acked" by the peer, as
			 * according to the protocol, those acks may be unacked later by the
			 * client.
			 *
			 * @remarks
			 * - This is a message lifecycle event.
			 * - It's guaranteed that OnAssociationLifecycleMessageEnd() is not called if
			 *   this callback has triggered.
			 * - It is NOT allowed to call methods in Association within this callback.
			 */
			virtual void OnAssociationLifecycleMessageDelivered(uint64_t lifecycleId)
			{
			}

			/**
			 * Called when a lifecycle event has reached its end. It will be called
			 * when processing of a message is complete, no matter how it completed.
			 * It will be called after all other lifecycle events, if any.
			 *
			 * Note that it's possible that this callback triggers without any other
			 * lifecycle callbacks having been called before in case of errors, such
			 * as attempting to send an empty message or failing to enqueue a message
			 * if the send queue is full.
			 *
			 * @remarks:
			 * - This is a message lifecycle event.
			 * - When the Association is deallocated, there will be no
			 *   OnAssociationLifecycleMessageEnd() callbacks sent for messages that were
			 *   enqueued. But as long as the Association is alive, these callbacks are
			 *   guaranteed to be sent as messages are either expired or successfully
			 *   acknowledged.
			 * - It is NOT allowed to call methods in Association within this callback.
			 */
			virtual void OnAssociationLifecycleMessageEnd(uint64_t lifecycleId)
			{
			}
		};
	} // namespace SCTP
} // namespace RTC

#endif
