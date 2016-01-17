#ifndef MS_UDP_SOCKET_H
#define MS_UDP_SOCKET_H

#include "common.h"
#include <string>
#include <uv.h>

class UDPSocket
{
public:
	/* Struct for the data field of uv_req_t when sending a datagram. */
	struct UvSendData
	{
		UDPSocket*    socket;
		uv_udp_send_t req;
		uint8_t       store[1];
	};

private:
	static uint8_t readBuffer[];

public:
	UDPSocket(const std::string &ip, uint16_t port);
	/**
	 * uvHandle must be an already initialized and binded uv_udp_t pointer.
	 */
	UDPSocket(uv_udp_t* uvHandle);
	virtual ~UDPSocket();

	void Close();
	virtual void Dump();
	void Send(const uint8_t* data, size_t len, const struct sockaddr* addr);
	void Send(const std::string &data, const struct sockaddr* addr);
	void Send(const uint8_t* data, size_t len, const std::string &ip, uint16_t port);
	void Send(const std::string &data, const std::string &ip, uint16_t port);
	const struct sockaddr* GetLocalAddress();
	const std::string& GetLocalIP();
	uint16_t GetLocalPort();

private:
	bool SetLocalAddress();

/* Callbacks fired by UV events. */
public:
	void onUvRecvAlloc(size_t suggested_size, uv_buf_t* buf);
	void onUvRecv(ssize_t nread, const uv_buf_t* buf, const struct sockaddr* addr, unsigned int flags);
	void onUvSendError(int error);
	void onUvClosed();

/* Pure virtual methods that must be implemented by the subclass. */
protected:
	virtual void userOnUDPDatagramRecv(const uint8_t* data, size_t len, const struct sockaddr* addr) = 0;
	virtual void userOnUDPSocketClosed() = 0;

private:
	// Allocated by this (may be passed by argument).
	uv_udp_t* uvHandle = nullptr;
	// Others.
	bool isClosing = false;

protected:
	struct sockaddr_storage localAddr;
	std::string localIP;
	uint16_t localPort = 0;
};

/* Inline methods. */

inline
void UDPSocket::Send(const std::string &data, const struct sockaddr* addr)
{
	Send((const uint8_t*)data.c_str(), data.size(), addr);
}

inline
void UDPSocket::Send(const std::string &data, const std::string &ip, uint16_t port)
{
	Send((const uint8_t*)data.c_str(), data.size(), ip, port);
}

inline
const struct sockaddr* UDPSocket::GetLocalAddress()
{
	return (const struct sockaddr*)&this->localAddr;
}

inline
const std::string& UDPSocket::GetLocalIP()
{
	return this->localIP;
}

inline
uint16_t UDPSocket::GetLocalPort()
{
	return this->localPort;
}

#endif
