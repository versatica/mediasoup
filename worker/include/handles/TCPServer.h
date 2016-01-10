#ifndef MS_TCP_SERVER_H
#define MS_TCP_SERVER_H

#include "common.h"
#include "handles/TCPConnection.h"
#include <string>
#include <unordered_set>
#include <uv.h>

class TCPServer : public TCPConnection::Listener
{
public:
	TCPServer(const std::string &ip, MS_PORT port, int backlog);
	/**
	 * uvHandle must be an already initialized and binded uv_tcp_t pointer.
	 */
	TCPServer(uv_tcp_t* uvHandle, int backlog);
	virtual ~TCPServer();

	void Close();
	virtual void Dump();
	bool IsClosing();
	const struct sockaddr* GetLocalAddress();
	const std::string& GetLocalIP();
	MS_PORT GetLocalPort();
	size_t GetNumConnections();

private:
	bool SetLocalAddress();

/* Pure virtual methods that must be implemented by the subclass. */
protected:
	virtual void userOnTCPConnectionAlloc(TCPConnection** connection) = 0;
	virtual void userOnNewTCPConnection(TCPConnection* connection) = 0;
	virtual void userOnTCPConnectionClosed(TCPConnection* connection, bool is_closed_by_peer) = 0;
	virtual void userOnTCPServerClosed() = 0;

/* Callbacks fired by UV events. */
public:
	void onUvConnection(int status);
	void onUvClosed();

/* Methods inherited from TCPConnection::Listener. */
public:
	virtual void onTCPConnectionClosed(TCPConnection* connection, bool is_closed_by_peer);

private:
	// Allocated by this (may be passed by argument).
	uv_tcp_t* uvHandle = nullptr;
	// Others.
	std::unordered_set<TCPConnection*> connections;
	bool isClosing = false;

protected:
	struct sockaddr_storage localAddr;
	std::string localIP;
	MS_PORT localPort = 0;
};

/* Inline methods. */

inline
bool TCPServer::IsClosing()
{
	return this->isClosing;
}

inline
size_t TCPServer::GetNumConnections()
{
	return this->connections.size();
}

#endif
