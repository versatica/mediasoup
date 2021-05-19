#ifndef MS_UDP_SOCKET_HPP
#define MS_UDP_SOCKET_HPP

#include "common.hpp"
#include <uv.h>
#include <string>

class UdpSocketHandler
{
protected:
	using onSendCallback = const std::function<void(bool sent)>;

public:
	/* Struct for the data field of uv_req_t when sending a datagram. */
	struct UvSendData
	{
		explicit UvSendData(size_t storeSize)
		{
			this->store = new uint8_t[storeSize];
		}

		// Disable copy constructor because of the dynamically allocated data (store).
		UvSendData(const UvSendData&) = delete;

		~UvSendData()
		{
			delete[] this->store;
			delete this->cb;
		}

		uv_udp_send_t req;
		uint8_t* store{ nullptr };
		UdpSocketHandler::onSendCallback* cb{ nullptr };
	};

public:
	/**
	 * uvHandle must be an already initialized and binded uv_udp_t pointer.
	 */
	explicit UdpSocketHandler(uv_udp_t* uvHandle);
	UdpSocketHandler& operator=(const UdpSocketHandler&) = delete;
	UdpSocketHandler(const UdpSocketHandler&)            = delete;
	virtual ~UdpSocketHandler();

public:
	void Close();
	virtual void Dump() const;
	void Send(
	  const uint8_t* data, size_t len, const struct sockaddr* addr, UdpSocketHandler::onSendCallback* cb);
	const struct sockaddr* GetLocalAddress() const
	{
		return reinterpret_cast<const struct sockaddr*>(&this->localAddr);
	}
	int GetLocalFamily() const
	{
		return reinterpret_cast<const struct sockaddr*>(&this->localAddr)->sa_family;
	}
	const std::string& GetLocalIp() const
	{
		return this->localIp;
	}
	uint16_t GetLocalPort() const
	{
		return this->localPort;
	}
	size_t GetRecvBytes() const
	{
		return this->recvBytes;
	}
	size_t GetSentBytes() const
	{
		return this->sentBytes;
	}

private:
	bool SetLocalAddress();

	/* Callbacks fired by UV events. */
public:
	void OnUvRecvAlloc(size_t suggestedSize, uv_buf_t* buf);
	void OnUvRecv(ssize_t nread, const uv_buf_t* buf, const struct sockaddr* addr, unsigned int flags);
	void OnUvSend(int status, UdpSocketHandler::onSendCallback* cb);

	/* Pure virtual methods that must be implemented by the subclass. */
protected:
	virtual void UserOnUdpDatagramReceived(
	  const uint8_t* data, size_t len, const struct sockaddr* addr) = 0;

protected:
	struct sockaddr_storage localAddr;
	std::string localIp;
	uint16_t localPort{ 0u };

private:
	// Allocated by this (may be passed by argument).
	uv_udp_t* uvHandle{ nullptr };
	// Others.
	bool closed{ false };
	size_t recvBytes{ 0u };
	size_t sentBytes{ 0u };
};

#endif
