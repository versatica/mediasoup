#ifndef MS_TCP_SERVER_HPP
#define MS_TCP_SERVER_HPP

#include "common.hpp"
#include "handles/TcpConnectionHandler.hpp"
#include <uv.h>
#include <absl/container/flat_hash_set.h>
#include <string>

class TcpServerHandler : public TcpConnectionHandler::Listener
{
public:
	/**
	 * uvHandle must be an already initialized and binded uv_tcp_t pointer.
	 */
	TcpServerHandler(uv_tcp_t* uvHandle, int backlog);
	virtual ~TcpServerHandler() override;

public:
	void Close();
	virtual void Dump() const;
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
	size_t GetNumConnections() const
	{
		return this->connections.size();
	}

protected:
	void AcceptTcpConnection(TcpConnectionHandler* connection);

private:
	bool SetLocalAddress();

	/* Pure virtual methods that must be implemented by the subclass. */
protected:
	virtual void UserOnTcpConnectionAlloc()                                  = 0;
	virtual void UserOnTcpConnectionClosed(TcpConnectionHandler* connection) = 0;

	/* Callbacks fired by UV events. */
public:
	void OnUvConnection(int status);

	/* Methods inherited from TcpConnectionHandler::Listener. */
public:
	void OnTcpConnectionClosed(TcpConnectionHandler* connection) override;

protected:
	struct sockaddr_storage localAddr;
	std::string localIp;
	uint16_t localPort{ 0u };

private:
	// Allocated by this (may be passed by argument).
	uv_tcp_t* uvHandle{ nullptr };
	// Others.
	absl::flat_hash_set<TcpConnectionHandler*> connections;
	bool closed{ false };
};

#endif
