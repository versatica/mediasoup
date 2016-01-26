#ifndef MS_RTC_UDP_SOCKET_H
#define MS_RTC_UDP_SOCKET_H

#include "common.h"
#include "handles/UDPSocket.h"
#include <unordered_map>
#include <uv.h>

namespace RTC
{
	class UDPSocket :
		public ::UDPSocket
	{
	public:
		class Listener
		{
		public:
			virtual void onPacketRecv(RTC::UDPSocket *socket, const uint8_t* data, size_t len, const struct sockaddr* remote_addr) = 0;
		};

	public:
		static void ClassInit();

	private:
		static uv_udp_t* GetRandomPort(int address_family);

	private:
		static struct sockaddr_storage sockaddrStorageIPv4;
		static struct sockaddr_storage sockaddrStorageIPv6;
		static uint16_t minPort;
		static uint16_t maxPort;
		static std::unordered_map<uint16_t, bool> availableIPv4Ports;
		static std::unordered_map<uint16_t, bool> availableIPv6Ports;

	public:
		UDPSocket(Listener* listener, int address_family);

	/* Pure virtual methods inherited from ::UDPSocket. */
	public:
		virtual void userOnUDPDatagramRecv(const uint8_t* data, size_t len, const struct sockaddr* addr) override;
		virtual void userOnUDPSocketClosed() override;

	private:
		// Passed by argument.
		Listener* listener = nullptr;
	};
}

#endif
