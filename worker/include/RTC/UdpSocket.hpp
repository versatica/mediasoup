#ifndef MS_RTC_UDP_SOCKET_HPP
#define MS_RTC_UDP_SOCKET_HPP

#include "common.hpp"
#include "handles/UdpSocket.hpp"
#include <string>

namespace RTC
{
	class UdpSocket : public ::UdpSocket
	{
	public:
		class Listener
		{
		public:
			virtual void OnUdpSocketPacketReceived(
			  RTC::UdpSocket* socket, const uint8_t* data, size_t len, const struct sockaddr* remoteAddr) = 0;
		};

	public:
		UdpSocket(Listener* listener, std::string& ip);
		~UdpSocket() override;

		/* Pure virtual methods inherited from ::UdpSocket. */
	public:
		void UserOnUdpDatagramReceived(const uint8_t* data, size_t len, const struct sockaddr* addr) override;

	private:
		// Passed by argument.
		Listener* listener{ nullptr };
	};
} // namespace RTC

#endif
