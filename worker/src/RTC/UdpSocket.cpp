#define MS_CLASS "RTC::UdpSocket"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/UdpSocket.hpp"
#include "Logger.hpp"
#include "RTC/PortManager.hpp"
#include <string>

namespace RTC
{
	/* Instance methods. */

	UdpSocket::UdpSocket(Listener* listener, std::string& ip, bool enableMulticast)
	  : // This may throw.
	    ::UdpSocketHandle::UdpSocketHandle(PortManager::BindUdp(ip, enableMulticast)),
	    listener(listener)
	{
		MS_TRACE();
	}

	UdpSocket::UdpSocket(Listener* listener, std::string& ip, uint16_t port, bool enableMulticast)
	  : // This may throw.
	    ::UdpSocketHandle::UdpSocketHandle(PortManager::BindUdp(ip, port, enableMulticast)),
	    listener(listener), fixedPort(true)
	{
		MS_TRACE();
	}

	UdpSocket::~UdpSocket()
	{
		MS_TRACE();

		if (!fixedPort)
		{
			PortManager::UnbindUdp(this->localIp, this->localPort);
		}
	}

	void UdpSocket::UserOnUdpDatagramReceived(const uint8_t* data, size_t len, const struct sockaddr* addr)
	{
		MS_TRACE();

		if (!this->listener)
		{
			MS_ERROR("no listener set");

			return;
		}

		// Notify the reader.
		this->listener->OnUdpSocketPacketReceived(this, data, len, addr);
	}
} // namespace RTC
