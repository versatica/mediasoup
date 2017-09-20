#ifndef MS_UDP_SOCKET_HPP
#define MS_UDP_SOCKET_HPP

#include "common.hpp"
#include <uv.h>
#include <string>

class UdpSocket
{
public:
	/* Struct for the data field of uv_req_t when sending a datagram. */
	struct UvSendData
	{
		UdpSocket* socket{ nullptr };
		uv_udp_send_t req;
		uint8_t store[1];
	};

public:
	UdpSocket(const std::string& ip, uint16_t port);
	/**
	 * uvHandle must be an already initialized and binded uv_udp_t pointer.
	 */
	explicit UdpSocket(uv_udp_t* uvHandle);
	UdpSocket& operator=(const UdpSocket&) = delete;
	UdpSocket(const UdpSocket&)            = delete;

protected:
	virtual ~UdpSocket();

public:
	void Destroy();
	virtual void Dump() const;
	void Send(const uint8_t* data, size_t len, const struct sockaddr* addr);
	void Send(const std::string& data, const struct sockaddr* addr);
	void Send(const uint8_t* data, size_t len, const std::string& ip, uint16_t port);
	void Send(const std::string& data, const std::string& ip, uint16_t port);
	const struct sockaddr* GetLocalAddress() const;
	int GetLocalFamily() const;
	const std::string& GetLocalIP() const;
	uint16_t GetLocalPort() const;

private:
	bool SetLocalAddress();

	/* Callbacks fired by UV events. */
public:
	void OnUvRecvAlloc(size_t suggestedSize, uv_buf_t* buf);
	void OnUvRecv(ssize_t nread, const uv_buf_t* buf, const struct sockaddr* addr, unsigned int flags);
	void OnUvSendError(int error);
	void OnUvClosed();

	/* Pure virtual methods that must be implemented by the subclass. */
protected:
	virtual void UserOnUdpDatagramRecv(const uint8_t* data, size_t len, const struct sockaddr* addr) = 0;
	virtual void UserOnUdpSocketClosed() = 0;

private:
	// Allocated by this (may be passed by argument).
	uv_udp_t* uvHandle{ nullptr };
	// Others.
	bool isClosing{ false };

protected:
	struct sockaddr_storage localAddr{};
	std::string localIP;
	uint16_t localPort{ 0 };
};

/* Inline methods. */

inline void UdpSocket::Send(const std::string& data, const struct sockaddr* addr)
{
	Send(reinterpret_cast<const uint8_t*>(data.c_str()), data.size(), addr);
}

inline void UdpSocket::Send(const std::string& data, const std::string& ip, uint16_t port)
{
	Send(reinterpret_cast<const uint8_t*>(data.c_str()), data.size(), ip, port);
}

inline const struct sockaddr* UdpSocket::GetLocalAddress() const
{
	return reinterpret_cast<const struct sockaddr*>(&this->localAddr);
}

inline int UdpSocket::GetLocalFamily() const
{
	return reinterpret_cast<const struct sockaddr*>(&this->localAddr)->sa_family;
}

inline const std::string& UdpSocket::GetLocalIP() const
{
	return this->localIP;
}

inline uint16_t UdpSocket::GetLocalPort() const
{
	return this->localPort;
}

#endif
