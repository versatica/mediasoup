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
			virtual void onSTUNDataRecv(RTC::UDPSocket *socket, const MS_BYTE* data, size_t len, const struct sockaddr* remote_addr) = 0;
			virtual void onDTLSDataRecv(RTC::UDPSocket *socket, const MS_BYTE* data, size_t len, const struct sockaddr* remote_addr) = 0;
			virtual void onRTPDataRecv(RTC::UDPSocket *socket, const MS_BYTE* data, size_t len, const struct sockaddr* remote_addr) = 0;
			virtual void onRTCPDataRecv(RTC::UDPSocket *socket, const MS_BYTE* data, size_t len, const struct sockaddr* remote_addr) = 0;
		};

	public:
		static void ClassInit();
		static RTC::UDPSocket* New(int address_family);
		static void NewPair(int address_family, RTC::UDPSocket* sockets[]);

	private:
		static void RandomizePort(int address_family, uv_udp_t* uvHandles[], bool pair);

	private:
		static struct sockaddr_storage sockaddrStorageIPv4;
		static struct sockaddr_storage sockaddrStorageIPv6;
		static MS_PORT minPort;
		static MS_PORT maxPort;
		typedef std::unordered_map<MS_PORT, bool> AvailablePorts;
		static AvailablePorts availableIPv4Ports;
		static AvailablePorts availableIPv6Ports;

	public:
		UDPSocket(uv_udp_t* uvHandle);

		void SetListener(Listener* listener);

	/* Pure virtual methods inherited from ::UDPSocket. */
	public:
		virtual void userOnUDPDatagramRecv(const MS_BYTE* data, size_t len, const struct sockaddr* addr) override;
		virtual void userOnUDPSocketClosed() override;

	private:
		// Passed by argument:
		Listener* listener = nullptr;
	};

	/* Inline methods. */

	inline
	void UDPSocket::SetListener(Listener* listener)
	{
		this->listener = listener;
	}
}  // namespace RTC

#endif
