#ifndef MS_RTC_SCTP_SOCKET_LISTENER_HPP
#define MS_RTC_SCTP_SOCKET_LISTENER_HPP

#include "common.hpp"
#include "RTC/SCTP/Message.hpp"
#include "RTC/SCTP/Types.hpp"
#include "RTC/SCTP/packet/Packet.hpp"
#include <string_view>

namespace RTC
{
	namespace SCTP
	{
		// Forward declaration.
		class Socket;

		class SocketListener
		{
		public:
			virtual ~SocketListener() = default;

		public:
			/**
			 * Called when a SCTP Packet must be sent to the remote endpoint.
			 *
			 * @return
			 * - `true` if the packet was successfully sent. However, since
			 *   sending is unreliable, there are no guarantees that the Packet was
			 *   actually delivered.
			 * - `false` if the Packet failed to be sent.
			 *
			 * @remarks
			 * - It is NOT allowed to call methods in Socket within this callback.
			 */
			virtual bool OnSocketSendSctpPacket(const Socket* socket, Packet* packet) = 0;

			/**
			 * Called when calling Connect() succeeds and also for incoming successful
			 * connection attempts.
			 *
			 * @remarks
			 * - It is allowed to call methods in Socket within this callback.
			 */
			virtual void OnSocketConnected(const Socket* socket) = 0;

			/**
			 * Called when the Socket is closed in a controlled way. No other callbacks
			 * will be done after this callback, unless reconnecting.
			 *
			 * @remarks
			 * - It is allowed to call methods in Socket within this callback.
			 */
			virtual void OnSocketClosed(const Socket* socket) = 0;

			/**
			 * Called on connection restarted (by peer). This is just a notification,
			 * and the association is expected to work fine after this call, but there
			 * could have been packet loss as a result of restarting the association.
			 *
			 * @remarks
			 * - It is allowed to call methods in Socket within this callback.
			 */
			virtual void OnSocketConnectionRestarted(const Socket* socket) = 0;

			/**
			 * Triggered when an non-fatal error is reported by either this library or
			 * from the other peer (by sending an ERROR command). These should be
			 * logged, but no other action need to be taken as the association is still
			 * viable.
			 *
			 * @remarks
			 * - It is allowed to call methods in Socket within this callback.
			 */
			virtual void OnSocketError(
			  const Socket* socket, Types::ErrorKind errorKind, std::string_view message) = 0;

			/**
			 * Triggered when the socket has aborted - either as decided by this Socket
			 * due to e.g. too many retransmission attempts, or by the peer when
			 * receiving an ABORT command. No other callbacks will be done after this
			 * callback, unless reconnecting.
			 *
			 * @remarks
			 * - It is allowed to call methods in Socket within this callback.
			 */
			virtual void OnSocketAborted(
			  const Socket* socket, Types::ErrorKind errorKind, std::string_view message) = 0;

			/**
			 * Called when an SCTP message in full has been received.
			 */
			virtual void OnSocketMessageReceived(const Socket* socket, Message message) = 0;
		};
	} // namespace SCTP
} // namespace RTC

#endif
