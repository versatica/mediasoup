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
	/**
	 * uvHandle must be an already initialized and binded uv_tcp_t pointer.
	 */
	TcpServer(uv_tcp_t* uvHandle, int backlog);
	virtual ~TcpServer() override;

public:
	void Close();
	virtual void Dump() const;
	const struct sockaddr* GetLocalAddress() const;
	int GetLocalFamily() const;
	const std::string& GetLocalIp() const;
	uint16_t GetLocalPort() const;
	size_t GetNumConnections() const;

protected:
	void AcceptTcpConnection(TcpConnection* connection);

private:
	bool SetLocalAddress();

	/* Pure virtual methods that must be implemented by the subclass. */
protected:
	virtual void UserOnTcpConnectionAlloc()                           = 0;
	virtual void UserOnTcpConnectionClosed(TcpConnection* connection) = 0;

	/* Callbacks fired by UV events. */
public:
	void OnUvConnection(int status);

	/* Methods inherited from TcpConnection::Listener. */
public:
	void OnTcpConnectionClosed(TcpConnection* connection) override;

protected:
	struct sockaddr_storage localAddr;
	std::string localIp;
	uint16_t localPort{ 0 };

private:
	// Allocated by this (may be passed by argument).
	uv_tcp_t* uvHandle{ nullptr };
	// Others.
	std::unordered_set<TcpConnection*> connections;
	bool closed{ false };
};

/* Inline methods. */

inline size_t TcpServer::GetNumConnections() const
{
	return this->connections.size();
}

inline const struct sockaddr* TcpServer::GetLocalAddress() const
{
	return reinterpret_cast<const struct sockaddr*>(&this->localAddr);
}

inline int TcpServer::GetLocalFamily() const
{
	return reinterpret_cast<const struct sockaddr*>(&this->localAddr)->sa_family;
}

inline const std::string& TcpServer::GetLocalIp() const
{
	return this->localIp;
}

inline uint16_t TcpServer::GetLocalPort() const
{
	return this->localPort;
}

#endif
