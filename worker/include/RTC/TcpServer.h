#ifndef MS_RTC_TCP_SERVER_H
#define MS_RTC_TCP_SERVER_H

#include "common.h"
#include "handles/TcpServer.h"
#include "handles/TcpConnection.h"
#include "RTC/TcpConnection.h"
#include <unordered_map>
#include <uv.h>

namespace RTC
{
	class TcpServer :
		public ::TcpServer
	{
	public:
		class Listener
		{
		public:
			virtual ~Listener() {};

		public:
			virtual void onRtcTcpConnectionClosed(RTC::TcpServer* tcpServer, RTC::TcpConnection* connection, bool is_closed_by_peer) = 0;
		};

	public:
		static void ClassInit();

	private:
		static uv_tcp_t* GetRandomPort(int address_family);

	private:
		static struct sockaddr_storage sockaddrStorageIPv4;
		static struct sockaddr_storage sockaddrStorageIPv6;
		static uint16_t minPort;
		static uint16_t maxPort;
		static std::unordered_map<uint16_t, bool> availableIPv4Ports;
		static std::unordered_map<uint16_t, bool> availableIPv6Ports;

	public:
		TcpServer(Listener* listener, RTC::TcpConnection::Listener* connListener, int address_family);
		virtual ~TcpServer() {};

	/* Pure virtual methods inherited from ::TcpServer. */
	public:
		virtual void userOnTcpConnectionAlloc(::TcpConnection** connection) override;
		virtual void userOnNewTcpConnection(::TcpConnection* connection) override;
		virtual void userOnTcpConnectionClosed(::TcpConnection* connection, bool is_closed_by_peer) override;
		virtual void userOnTcpServerClosed() override;

	private:
		// Passed by argument.
		Listener* listener = nullptr;
		RTC::TcpConnection::Listener* connListener = nullptr;
	};
}

#endif
