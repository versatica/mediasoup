#ifndef MS_TCP_SERVER_HPP
#define MS_TCP_SERVER_HPP

#include "common.hpp"
#include "handles/TcpConnection.hpp"
#include <uv.h>
#include <string>
#include <unordered_set>

class TcpServer : public TcpConnection::Listener
{
public:
	TcpServer(const std::string& ip, uint16_t port, int backlog);
	/**
	 * uvHandle must be an already initialized and binded uv_tcp_t pointer.
	 */
	TcpServer(uv_tcp_t* uvHandle, int backlog);

protected:
	virtual ~TcpServer();

public:
	void Destroy();
	virtual void Dump() const;
	bool IsClosing() const;
	const struct sockaddr* GetLocalAddress() const;
	int GetLocalFamily() const;
	const std::string& GetLocalIP() const;
	uint16_t GetLocalPort() const;
	size_t GetNumConnections() const;

private:
	bool SetLocalAddress();

	/* Pure virtual methods that must be implemented by the subclass. */
protected:
	virtual void userOnTcpConnectionAlloc(TcpConnection** connection)                      = 0;
	virtual void userOnNewTcpConnection(TcpConnection* connection)                         = 0;
	virtual void userOnTcpConnectionClosed(TcpConnection* connection, bool isClosedByPeer) = 0;
	virtual void userOnTcpServerClosed()                                                   = 0;

	/* Callbacks fired by UV events. */
public:
	void onUvConnection(int status);
	void onUvClosed();

	/* Methods inherited from TcpConnection::Listener. */
public:
	virtual void onTcpConnectionClosed(TcpConnection* connection, bool isClosedByPeer);

private:
	// Allocated by this (may be passed by argument).
	uv_tcp_t* uvHandle = nullptr;
	// Others.
	std::unordered_set<TcpConnection*> connections;
	bool isClosing = false;

protected:
	struct sockaddr_storage localAddr;
	std::string localIP;
	uint16_t localPort = 0;
};

/* Inline methods. */

inline bool TcpServer::IsClosing() const
{
	return this->isClosing;
}

inline size_t TcpServer::GetNumConnections() const
{
	return this->connections.size();
}

inline const struct sockaddr* TcpServer::GetLocalAddress() const
{
	return (const struct sockaddr*)&this->localAddr;
}

inline int TcpServer::GetLocalFamily() const
{
	return ((const struct sockaddr*)&this->localAddr)->sa_family;
}

inline const std::string& TcpServer::GetLocalIP() const
{
	return this->localIP;
}

inline uint16_t TcpServer::GetLocalPort() const
{
	return this->localPort;
}

#endif
