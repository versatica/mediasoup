#ifndef MS_RTC_TCP_SERVER_H
#define MS_RTC_TCP_SERVER_H

#include "common.h"
#include "handles/TCPServer.h"
#include "handles/TCPConnection.h"
#include "RTC/TCPConnection.h"
#include <unordered_map>
#include <uv.h>

namespace RTC
{
	class TCPServer : public ::TCPServer
	{
	public:
		class Listener
		{
		public:
			virtual void onRTCTCPConnectionClosed(RTC::TCPServer* tcpServer, RTC::TCPConnection* connection, bool is_closed_by_peer) = 0;
		};

	public:
		static void ClassInit();
		static RTC::TCPServer* New(int address_family);
		static void NewPair(int address_family, RTC::TCPServer* servers[]);

	private:
		static void RandomizePort(int address_family, uv_tcp_t* uvHandles[], bool pair);

	private:
		static struct sockaddr_storage sockaddrStorageIPv4;
		static struct sockaddr_storage sockaddrStorageIPv6;
		static MS_PORT minPort;
		static MS_PORT maxPort;
		typedef std::unordered_map<MS_PORT, bool> AvailablePorts;
		static AvailablePorts availableIPv4Ports;
		static AvailablePorts availableIPv6Ports;

	public:
		TCPServer(uv_tcp_t* uvHandle);

		void SetListeners(Listener* listener, RTC::TCPConnection::Reader* reader);

	/* Pure virtual methods inherited from ::TCPServer. */
	public:
		virtual void userOnTCPConnectionAlloc(::TCPConnection** connection) override;
		virtual void userOnNewTCPConnection(::TCPConnection* connection) override;
		virtual void userOnTCPConnectionClosed(::TCPConnection* connection, bool is_closed_by_peer) override;
		virtual void userOnTCPServerClosed() override;

	private:
		// Passed by argument.
		Listener* listener = nullptr;
		RTC::TCPConnection::Reader* reader = nullptr;
	};

	/* Inline methods. */

	inline
	void TCPServer::SetListeners(Listener* listener, RTC::TCPConnection::Reader* reader)
	{
		this->listener = listener;
		this->reader = reader;
	}
}

#endif
