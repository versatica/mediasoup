#ifndef MS_RTC_TCP_SERVER_HPP
#define MS_RTC_TCP_SERVER_HPP

#include "common.hpp"
#include "RTC/TcpConnection.hpp"
#include "handles/TcpConnectionHandler.hpp"
#include "handles/TcpServerHandler.hpp"
#include <string>

namespace RTC
{
	class TcpServer : public ::TcpServerHandler
	{
	public:
		class Listener
		{
		public:
			virtual ~Listener() = default;

		public:
			virtual void OnRtcTcpConnectionClosed(
			  RTC::TcpServer* tcpServer, RTC::TcpConnection* connection) = 0;
		};

	public:
		TcpServer(Listener* listener, RTC::TcpConnection::Listener* connListener, std::string& ip);
		TcpServer(
		  Listener* listener, RTC::TcpConnection::Listener* connListener, std::string& ip, uint16_t port);
		~TcpServer() override;

		/* Pure virtual methods inherited from ::TcpServerHandler. */
	public:
		void UserOnTcpConnectionAlloc() override;
		void UserOnTcpConnectionClosed(::TcpConnectionHandler* connection) override;

	private:
		// Passed by argument.
		Listener* listener{ nullptr };
		RTC::TcpConnection::Listener* connListener{ nullptr };
		bool fixedPort{ false };
	};
} // namespace RTC

#endif
