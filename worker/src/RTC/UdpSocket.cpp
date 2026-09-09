#define MS_CLASS "RTC::UdpSocket"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/UdpSocket.hpp"
#include "Logger.hpp"
#include "RTC/PortManager.hpp"
#include <string>

namespace RTC
{
	/* Instance methods. */

	UdpSocket::UdpSocket(
	  Listener* listener,
	  SharedInterface* shared,
	  std::string& ip,
	  uint16_t port,
	  RTC::Transport::SocketFlags& flags)
	  : // This may throw.
	    ::UdpSocketHandle::UdpSocketHandle(RTC::PortManager::BindUdp(ip, port, flags)),
	    listener(listener),
	    shared(shared),
	    fixedPort(true)
	{
		MS_TRACE();
	}

	UdpSocket::UdpSocket(
	  Listener* listener,
	  SharedInterface* shared,
	  std::string& ip,
	  uint16_t minPort,
	  uint16_t maxPort,
	  RTC::Transport::SocketFlags& flags,
	  RTC::PortManager::PortRangeKey& portRangeKey)
	  : // This may throw.
	    ::UdpSocketHandle::UdpSocketHandle(
	      RTC::PortManager::BindUdp(ip, minPort, maxPort, flags, portRangeKey)),
	    listener(listener),
	    shared(shared)
	{
		MS_TRACE();

		this->portRangeKey = portRangeKey;
	}

	UdpSocket::~UdpSocket()
	{
		MS_TRACE();

		if (!this->fixedPort)
		{
			RTC::PortManager::Unbind(this->portRangeKey, this->localPort);
		}
	}

	void UdpSocket::UserOnUdpDatagramReceived(
	  const uint8_t* data, size_t len, size_t bufferLen, const struct sockaddr* addr)
	{
		MS_TRACE();

		// NOTE: Take the arrival time before anything else is done with the
		// datagram, so that it doesn't include the cost of processing it.
		const int64_t receivedAtUs = this->shared->GetTimeUs();

		if (!this->listener)
		{
			MS_ERROR("no listener set");

			return;
		}

		// Notify the reader.
		this->listener->OnUdpSocketPacketReceived(this, data, len, bufferLen, addr, receivedAtUs);
	}
} // namespace RTC
