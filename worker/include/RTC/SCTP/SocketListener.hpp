#ifndef MS_RTC_SCTP_SOCKET_LISTENER_HPP
#define MS_RTC_SCTP_SOCKET_LISTENER_HPP

#include "common.hpp"
#include "RTC/SCTP/Message.hpp"
#include "RTC/SCTP/packet/Packet.hpp"

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
			 *  connection attempts.
			 *
			 * @remarks
			 * - It is allowed to call methods in Socket within this callback.
			 */
			virtual void OnConnected(const Socket* socket) = 0;

			/**
			 * Called when an SCTP message in full has been received.
			 */
			virtual void OnMessageReceived(const Socket* socket, Message message) = 0;
		};
	} // namespace SCTP
} // namespace RTC

#endif
