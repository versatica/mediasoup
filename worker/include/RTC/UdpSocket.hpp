#ifndef MS_RTC_UDP_SOCKET_HPP
#define MS_RTC_UDP_SOCKET_HPP

#include "common.hpp"
#include "handles/UdpSocket.hpp"
#include <uv.h>
#include <unordered_map>

namespace RTC
{
	class UdpSocket : public ::UdpSocket
	{
	public:
		class Listener
		{
		public:
			virtual ~Listener() = default;

		public:
			virtual void OnPacketRecv(
			    RTC::UdpSocket* socket,
			    const uint8_t* data,
			    size_t len,
			    const struct sockaddr* remoteAddr) = 0;
		};

	public:
		static void ClassInit();

	private:
		static uv_udp_t* GetRandomPort(int addressFamily);

	private:
		static struct sockaddr_storage sockaddrStorageIPv4;
		static struct sockaddr_storage sockaddrStorageIPv6;
		static uint16_t minPort;
		static uint16_t maxPort;
		static std::unordered_map<uint16_t, bool> availableIPv4Ports;
		static std::unordered_map<uint16_t, bool> availableIPv6Ports;

	public:
		UdpSocket(Listener* listener, int addressFamily);

	private:
		~UdpSocket() override = default;

		/* Pure virtual methods inherited from ::UdpSocket. */
	public:
		void UserOnUdpDatagramRecv(const uint8_t* data, size_t len, const struct sockaddr* addr) override;
		void UserOnUdpSocketClosed() override;

	private:
		// Passed by argument.
		Listener* listener{ nullptr };
	};
} // namespace RTC

#endif
