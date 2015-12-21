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

	const struct sockaddr* GetLocalAddress();
	const std::string& GetLocalIP();
	MS_PORT GetLocalPort();
	void SetUserData(void* userData);
	void* GetUserData();
	size_t GetNumConnections();
	void Close();
	bool IsClosing();
	virtual void Dump();

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
	// Passed by argument.
	void* userData = nullptr;
	// Others.
	typedef std::unordered_set<TCPConnection*> TCPConnections;
	TCPConnections connections;
	bool isClosing = false;

protected:
	struct sockaddr_storage localAddr;
	std::string localIP;
	MS_PORT localPort = 0;
};

/* Inline methods. */

inline
void TCPServer::SetUserData(void* userData)
{
	this->userData = userData;
}

inline
void* TCPServer::GetUserData()
{
	return this->userData;
}

inline
size_t TCPServer::GetNumConnections()
{
	return this->connections.size();
}

inline
bool TCPServer::IsClosing()
{
	return this->isClosing;
}

#endif
