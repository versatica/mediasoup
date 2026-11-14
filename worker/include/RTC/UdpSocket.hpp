#ifndef MS_RTC_UDP_SOCKET_HPP
#define MS_RTC_UDP_SOCKET_HPP

#include "common.hpp"
#include "handles/UdpSocketHandle.hpp"
#include "RTC/PortManager.hpp"
#include "RTC/Transport.hpp"
#include "SharedInterface.hpp"
#include <string>

namespace RTC
{
	class UdpSocket : public UdpSocketHandle
	{
	public:
		class Listener
		{
		public:
			virtual ~Listener() = default;

		public:
			virtual void OnUdpSocketPacketReceived(
			  RTC::UdpSocket* socket,
			  const uint8_t* data,
			  size_t len,
			  size_t bufferLen,
			  const sockaddr* remoteAddr,
			  int64_t receivedAtUs) = 0;
		};

	public:
		UdpSocket(
		  Listener* listener,
		  SharedInterface* shared,
		  std::string& ip,
		  const std::string& announcedAddress,
		  uint16_t port,
		  const RTC::Transport::SocketFlags& flags);

		UdpSocket(
		  Listener* listener,
		  SharedInterface* shared,
		  std::string& ip,
		  const std::string& announcedAddress,
		  uint16_t minPort,
		  uint16_t maxPort,
		  const RTC::Transport::SocketFlags& flags,
		  RTC::PortManager::PortRangeKey& portRangeKey);

		~UdpSocket() override;

		const std::string& GetAnnouncedAddress() const
		{
			return this->announcedAddress;
		}

		/* Pure virtual methods inherited from UdpSocketHandle. */
	public:
		void UserOnUdpDatagramReceived(
		  const uint8_t* data, size_t len, size_t bufferLen, const sockaddr* addr) override;

	private:
		// Passed by argument.
		Listener* const listener;
		SharedInterface* const shared;
		const std::string announcedAddress;
		bool fixedPort{ false };
		RTC::PortManager::PortRangeKey portRangeKey;
	};
} // namespace RTC

#endif
