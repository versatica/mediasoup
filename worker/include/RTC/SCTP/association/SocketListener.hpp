#ifndef MS_RTC_SCTP_SOCKET_LISTENER_HPP
#define MS_RTC_SCTP_SOCKET_LISTENER_HPP

#include "common.hpp"
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
			virtual void OnSocketSendSctpPacket(const Socket* socket, Packet* packet) const = 0;

			/**
			 * Called when calling Connect() succeeds and also for incoming successful
			 *  connection attempts.
			 *
			 * @remarks
			 * - It is allowed to call methods in Socket within this callback.
			 */
			virtual void OnConnected() = 0;
		};
	} // namespace SCTP
} // namespace RTC

#endif
