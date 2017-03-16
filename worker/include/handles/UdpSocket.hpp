#ifndef MS_UDP_SOCKET_HPP
#define MS_UDP_SOCKET_HPP

#include "common.hpp"
#include <string>
#include <uv.h>

class UdpSocket
{
public:
	/* Struct for the data field of uv_req_t when sending a datagram. */
	struct UvSendData
	{
		UdpSocket*    socket;
		uv_udp_send_t req;
		uint8_t       store[1];
	};

private:
	static uint8_t readBuffer[];

public:
	UdpSocket(const std::string &ip, uint16_t port);
	/**
	 * uvHandle must be an already initialized and binded uv_udp_t pointer.
	 */
	explicit UdpSocket(uv_udp_t* uvHandle);
	UdpSocket& operator=(const UdpSocket&) = delete;
	UdpSocket(const UdpSocket&) = delete;
	virtual ~UdpSocket();

	void Close();
	virtual void Dump() const;
	void Send(const uint8_t* data, size_t len, const struct sockaddr* addr);
	void Send(const std::string &data, const struct sockaddr* addr);
	void Send(const uint8_t* data, size_t len, const std::string &ip, uint16_t port);
	void Send(const std::string &data, const std::string &ip, uint16_t port);
	const struct sockaddr* GetLocalAddress() const;
	int GetLocalFamily() const;
	const std::string& GetLocalIP() const;
	uint16_t GetLocalPort() const;

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
	virtual void userOnUdpDatagramRecv(const uint8_t* data, size_t len, const struct sockaddr* addr) = 0;
	virtual void userOnUdpSocketClosed() = 0;

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
void UdpSocket::Send(const std::string &data, const struct sockaddr* addr)
{
	Send((const uint8_t*)data.c_str(), data.size(), addr);
}

inline
void UdpSocket::Send(const std::string &data, const std::string &ip, uint16_t port)
{
	Send((const uint8_t*)data.c_str(), data.size(), ip, port);
}

inline
const struct sockaddr* UdpSocket::GetLocalAddress() const
{
	return (const struct sockaddr*)&this->localAddr;
}

inline
int UdpSocket::GetLocalFamily() const
{
	return ((const struct sockaddr*)&this->localAddr)->sa_family;
}

inline
const std::string& UdpSocket::GetLocalIP() const
{
	return this->localIP;
}

inline
uint16_t UdpSocket::GetLocalPort() const
{
	return this->localPort;
}

#endif
