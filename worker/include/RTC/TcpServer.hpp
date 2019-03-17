#ifndef MS_RTC_TCP_SERVER_HPP
#define MS_RTC_TCP_SERVER_HPP

#include "common.hpp"
#include "RTC/TcpConnection.hpp"
#include "handles/TcpConnection.hpp"
#include "handles/TcpServer.hpp"
#include <string>

namespace RTC
{
	class TcpServer : public ::TcpServer
	{
	public:
		class Listener
		{
		public:
			virtual void OnRtcTcpConnectionClosed(
			  RTC::TcpServer* tcpServer, RTC::TcpConnection* connection, bool isClosedByPeer) = 0;
		};

	public:
		TcpServer(Listener* listener, RTC::TcpConnection::Listener* connListener, std::string& ip);
		~TcpServer() override;

		/* Pure virtual methods inherited from ::TcpServer. */
	public:
		void UserOnTcpConnectionAlloc(::TcpConnection** connection) override;
		void UserOnNewTcpConnection(::TcpConnection* connection) override;
		void UserOnTcpConnectionClosed(::TcpConnection* connection, bool isClosedByPeer) override;

	private:
		// Passed by argument.
		Listener* listener{ nullptr };
		RTC::TcpConnection::Listener* connListener{ nullptr };
	};
} // namespace RTC

#endif
