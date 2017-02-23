#ifndef MS_TCP_CONNECTION_HPP
#define MS_TCP_CONNECTION_HPP

#include "common.hpp"
#include <string>
#include <uv.h>

class TcpConnection
{
public:
	class Listener
	{
	public:
		virtual ~Listener() {};

	public:
		virtual void onTcpConnectionClosed(TcpConnection* connection, bool is_closed_by_peer) = 0;
	};

public:
	/* Struct for the data field of uv_req_t when writing into the connection. */
	struct UvWriteData
	{
		TcpConnection* connection;
		uv_write_t     req;
		uint8_t        store[1];
	};

public:
	explicit TcpConnection(size_t bufferSize);
	virtual ~TcpConnection();

	void Close();
	virtual void Dump();
	void Setup(Listener* listener, struct sockaddr_storage* localAddr, const std::string &localIP, uint16_t localPort);
	bool IsClosing();
	uv_tcp_t* GetUvHandle();
	void Start();
	void Write(const uint8_t* data, size_t len);
	void Write(const uint8_t* data1, size_t len1, const uint8_t* data2, size_t len2);
	void Write(const std::string &data);
	const struct sockaddr* GetLocalAddress();
	const std::string& GetLocalIP();
	uint16_t GetLocalPort();
	const struct sockaddr* GetPeerAddress();
	const std::string& GetPeerIP();
	uint16_t GetPeerPort();

private:
	bool SetPeerAddress();

/* Callbacks fired by UV events. */
public:
	void onUvReadAlloc(size_t suggested_size, uv_buf_t* buf);
	void onUvRead(ssize_t nread, const uv_buf_t* buf);
	void onUvWriteError(int error);
	void onUvShutdown(uv_shutdown_t* req, int status);
	void onUvClosed();

/* Pure virtual methods that must be implemented by the subclass. */
protected:
	virtual void userOnTcpConnectionRead() = 0;

private:
	// Passed by argument.
	Listener* listener = nullptr;
	// Allocated by this.
	uv_tcp_t* uvHandle = nullptr;
	// Others.
	struct sockaddr_storage* localAddr = nullptr;
	bool isClosing = false;
	bool isClosedByPeer = false;
	bool hasError = false;

protected:
	// Passed by argument.
	size_t bufferSize = 0;
	// Allocated by this.
	uint8_t* buffer = nullptr;
	// Others.
	size_t bufferDataLen = 0;
	std::string localIP;
	uint16_t localPort = 0;
	struct sockaddr_storage peerAddr;
	std::string peerIP;
	uint16_t peerPort = 0;
};

/* Inline methods. */

inline
bool TcpConnection::IsClosing()
{
	return this->isClosing;
}

inline
uv_tcp_t* TcpConnection::GetUvHandle()
{
	return this->uvHandle;
}

inline
void TcpConnection::Write(const std::string &data)
{
	Write((const uint8_t*)data.c_str(), data.size());
}

inline
const struct sockaddr* TcpConnection::GetLocalAddress()
{
	return (const struct sockaddr*)this->localAddr;
}

inline
const std::string& TcpConnection::GetLocalIP()
{
	return this->localIP;
}

inline
uint16_t TcpConnection::GetLocalPort()
{
	return this->localPort;
}

inline
const struct sockaddr* TcpConnection::GetPeerAddress()
{
	return (const struct sockaddr*)&this->peerAddr;
}

inline
const std::string& TcpConnection::GetPeerIP()
{
	return this->peerIP;
}

inline
uint16_t TcpConnection::GetPeerPort()
{
	return this->peerPort;
}

#endif
