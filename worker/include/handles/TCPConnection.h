#ifndef MS_TCP_CONNECTION_H
#define MS_TCP_CONNECTION_H

#include "common.h"
#include <string>
#include <uv.h>

class TCPConnection
{
public:
	class Listener
	{
	public:
		virtual void onTCPConnectionClosed(TCPConnection* connection, bool is_closed_by_peer) = 0;
	};

public:
	/* Struct for the data field of uv_req_t when writing into the connection. */
	struct UvWriteData
	{
		TCPConnection* connection;
		uv_write_t     req;
		MS_BYTE        store[1];
	};

public:
	TCPConnection(size_t bufferSize);
	virtual ~TCPConnection();

	void Setup(Listener* listener, struct sockaddr_storage* localAddr, const std::string &localIP, MS_PORT localPort);
	uv_tcp_t* GetUvHandle();
	void Start();
	void Write(const MS_BYTE* data, size_t len);
	void Write(const MS_BYTE* data1, size_t len1, const MS_BYTE* data2, size_t len2);
	void Write(const std::string &data);
	const struct sockaddr* GetLocalAddress();
	const std::string& GetLocalIP();
	MS_PORT GetLocalPort();
	const struct sockaddr* GetPeerAddress();
	const std::string& GetPeerIP();
	MS_PORT GetPeerPort();
	void Close();
	bool IsClosing();
	virtual void Dump();

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
	virtual void userOnTCPConnectionRead() = 0;

private:
	// Allocated by this.
	uv_tcp_t* uvHandle = nullptr;
	// Passed by argument.
	Listener* listener = nullptr;
	// Others.
	struct sockaddr_storage* localAddr = nullptr;
	bool isClosing = false;
	bool isClosedByPeer = false;
	bool hasError = false;

protected:
	// Allocated by this.
	MS_BYTE* buffer = nullptr;
	// Passed by argument.
	size_t bufferSize = 0;
	// Others.
	size_t bufferDataLen = 0;
	std::string localIP;
	MS_PORT localPort = 0;
	struct sockaddr_storage peerAddr;
	std::string peerIP;
	MS_PORT peerPort = 0;
};

/* Inline methods. */

inline
uv_tcp_t* TCPConnection::GetUvHandle()
{
	return this->uvHandle;
}

inline
void TCPConnection::Write(const std::string &data)
{
	Write((const MS_BYTE*)data.c_str(), data.size());
}

inline
const struct sockaddr* TCPConnection::GetLocalAddress()
{
	return (const struct sockaddr*)this->localAddr;
}

inline
const std::string& TCPConnection::GetLocalIP()
{
	return this->localIP;
}

inline
MS_PORT TCPConnection::GetLocalPort()
{
	return this->localPort;
}

inline
const struct sockaddr* TCPConnection::GetPeerAddress()
{
	return (const struct sockaddr*)&this->peerAddr;
}

inline
const std::string& TCPConnection::GetPeerIP()
{
	return this->peerIP;
}

inline
MS_PORT TCPConnection::GetPeerPort()
{
	return this->peerPort;
}

inline
bool TCPConnection::IsClosing()
{
	return this->isClosing;
}

#endif
