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
		uv_udp_send_t req;
		uint8_t store[1];
	};

public:
	/**
	 * uvHandle must be an already initialized and binded uv_udp_t pointer.
	 */
	explicit UdpSocket(uv_udp_t* uvHandle);
	UdpSocket& operator=(const UdpSocket&) = delete;
	UdpSocket(const UdpSocket&)            = delete;
	virtual ~UdpSocket();

public:
	void Close();
	virtual void Dump() const;
	void Send(const uint8_t* data, size_t len, const struct sockaddr* addr);
	void Send(const std::string& data, const struct sockaddr* addr);
	void Send(const uint8_t* data, size_t len, const std::string& ip, uint16_t port);
	void Send(const std::string& data, const std::string& ip, uint16_t port);
	const struct sockaddr* GetLocalAddress() const;
	int GetLocalFamily() const;
	const std::string& GetLocalIp() const;
	uint16_t GetLocalPort() const;
	size_t GetRecvBytes() const;
	size_t GetSentBytes() const;

private:
	bool SetLocalAddress();

	/* Callbacks fired by UV events. */
public:
	void OnUvRecvAlloc(size_t suggestedSize, uv_buf_t* buf);
	void OnUvRecv(ssize_t nread, const uv_buf_t* buf, const struct sockaddr* addr, unsigned int flags);
	void OnUvSendError(int error);

	/* Pure virtual methods that must be implemented by the subclass. */
protected:
	virtual void UserOnUdpDatagramReceived(
	  const uint8_t* data, size_t len, const struct sockaddr* addr) = 0;

protected:
	struct sockaddr_storage localAddr;
	std::string localIp;
	uint16_t localPort{ 0 };

private:
	// Allocated by this (may be passed by argument).
	uv_udp_t* uvHandle{ nullptr };
	// Others.
	bool closed{ false };
	size_t recvBytes{ 0 };
	size_t sentBytes{ 0 };
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

inline const std::string& UdpSocket::GetLocalIp() const
{
	return this->localIp;
}

inline uint16_t UdpSocket::GetLocalPort() const
{
	return this->localPort;
}

inline size_t UdpSocket::GetRecvBytes() const
{
	return this->recvBytes;
}

inline size_t UdpSocket::GetSentBytes() const
{
	return this->sentBytes;
}

#endif
