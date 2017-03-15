#ifndef MS_TCP_SERVER_HPP
#define MS_TCP_SERVER_HPP

#include "common.hpp"
#include "handles/TcpConnection.hpp"
#include <string>
#include <unordered_set>
#include <uv.h>

class TcpServer : public TcpConnection::Listener
{
public:
	TcpServer(const std::string &ip, uint16_t port, int backlog);
	/**
	 * uvHandle must be an already initialized and binded uv_tcp_t pointer.
	 */
	TcpServer(uv_tcp_t* uvHandle, int backlog);
	virtual ~TcpServer();

	void Close();
	virtual void Dump();
	bool IsClosing();
	const struct sockaddr* GetLocalAddress();
	int GetLocalFamily();
	const std::string& GetLocalIP();
	uint16_t GetLocalPort();
	size_t GetNumConnections();

private:
	bool SetLocalAddress();

/* Pure virtual methods that must be implemented by the subclass. */
protected:
	virtual void userOnTcpConnectionAlloc(TcpConnection** connection) = 0;
	virtual void userOnNewTcpConnection(TcpConnection* connection) = 0;
	virtual void userOnTcpConnectionClosed(TcpConnection* connection, bool is_closed_by_peer) = 0;
	virtual void userOnTcpServerClosed() = 0;

/* Callbacks fired by UV events. */
public:
	void onUvConnection(int status);
	void onUvClosed();

/* Methods inherited from TcpConnection::Listener. */
public:
	virtual void onTcpConnectionClosed(TcpConnection* connection, bool is_closed_by_peer);

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

inline
bool TcpServer::IsClosing()
{
	return this->isClosing;
}

inline
size_t TcpServer::GetNumConnections()
{
	return this->connections.size();
}

inline
const struct sockaddr* TcpServer::GetLocalAddress()
{
	return (const struct sockaddr*)&this->localAddr;
}

inline
int TcpServer::GetLocalFamily()
{
	return ((const struct sockaddr*)&this->localAddr)->sa_family;
}

inline
const std::string& TcpServer::GetLocalIP()
{
	return this->localIP;
}

inline
uint16_t TcpServer::GetLocalPort()
{
	return this->localPort;
}


#endif
