#ifndef MS_RTC_TCP_CONNECTION_HPP
#define MS_RTC_TCP_CONNECTION_HPP

#include "common.hpp"
#include "handles/TcpConnectionHandler.hpp"

namespace RTC
{
	class TcpConnection : public ::TcpConnectionHandler
	{
	public:
		class Listener
		{
		public:
			virtual ~Listener() = default;

		public:
			virtual void OnTcpConnectionPacketReceived(
			  RTC::TcpConnection* connection, const uint8_t* data, size_t len) = 0;
		};

	public:
		TcpConnection(Listener* listener, size_t bufferSize);
		~TcpConnection() override;

	public:
		void Send(const uint8_t* data, size_t len, ::TcpConnectionHandler::onSendCallback* cb);

		/* Pure virtual methods inherited from ::TcpConnectionHandler. */
	public:
		void UserOnTcpConnectionRead() override;

	private:
		// Passed by argument.
		Listener* listener{ nullptr };
		// Others.
		size_t frameStart{ 0u }; // Where the latest frame starts.
	};
} // namespace RTC

#endif
