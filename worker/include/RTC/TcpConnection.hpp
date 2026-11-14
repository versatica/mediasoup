#ifndef MS_RTC_TCP_CONNECTION_HPP
#define MS_RTC_TCP_CONNECTION_HPP

#include "common.hpp"
#include "handles/SendCallbacks.hpp"
#include "handles/TcpConnectionHandle.hpp"
#include "SharedInterface.hpp"
#include <string>

namespace RTC
{
	class TcpConnection : public TcpConnectionHandle
	{
	public:
		class Listener
		{
		public:
			virtual ~Listener() = default;

		public:
			virtual void OnTcpConnectionPacketReceived(
			  RTC::TcpConnection* connection,
			  const uint8_t* data,
			  size_t len,
			  size_t bufferLen,
			  int64_t receivedAtUs) = 0;
		};

	public:
		TcpConnection(
		  Listener* listener,
		  SharedInterface* shared,
		  const std::string& announcedAddress,
		  size_t bufferSize);

		~TcpConnection() override;

		const std::string& GetAnnouncedAddress() const
		{
			return this->announcedAddress;
		}

	public:
		void Send(const uint8_t* data, size_t len, onSendCallback cb);

		/* Pure virtual methods inherited from TcpConnectionHandle. */
	public:
		void UserOnTcpConnectionRead() override;

	private:
		// Passed by argument.
		Listener* const listener;
		SharedInterface* const shared;
		const std::string announcedAddress;
		// Others.
		size_t frameStart{ 0 }; // Where the latest frame starts.
	};
} // namespace RTC

#endif
