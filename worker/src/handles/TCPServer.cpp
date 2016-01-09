#define MS_CLASS "TCPServer"

#include "handles/TCPServer.h"
#include "Utils.h"
#include "DepLibUV.h"
#include "MediaSoupError.h"
#include "Logger.h"

#define RETURN_IF_CLOSING()  \
	if (this->isClosing)  \
	{  \
		MS_DEBUG("in closing state, doing nothing");  \
		return;  \
	}

/* Static methods for UV callbacks. */

static inline
void on_connection(uv_stream_t* handle, int status)
{
	static_cast<TCPServer*>(handle->data)->onUvConnection(status);
}

static inline
void on_close(uv_handle_t* handle)
{
	static_cast<TCPServer*>(handle->data)->onUvClosed();
}

static inline
void on_error_close(uv_handle_t* handle)
{
	delete handle;
}

/* Instance methods. */

TCPServer::TCPServer(const std::string &ip, MS_PORT port, int backlog)
{
	MS_TRACE();

	int err;
	int flags = 0;

	this->uvHandle = new uv_tcp_t;
	this->uvHandle->data = (void*)this;

	err = uv_tcp_init(DepLibUV::GetLoop(), this->uvHandle);
	if (err)
	{
		delete this->uvHandle;
		this->uvHandle = nullptr;
		MS_THROW_ERROR("uv_tcp_init() failed: %s", uv_strerror(err));
	}

	struct sockaddr_storage bind_addr;

	switch (Utils::IP::GetFamily(ip))
	{
		case AF_INET:
			err = uv_ip4_addr(ip.c_str(), (int)port, (struct sockaddr_in*)&bind_addr);
			if (err)
				MS_ABORT("uv_ipv4_addr() failed: %s", uv_strerror(err));
			break;

		case AF_INET6:
			err = uv_ip6_addr(ip.c_str(), (int)port, (struct sockaddr_in6*)&bind_addr);
			if (err)
				MS_ABORT("uv_ipv6_addr() failed: %s", uv_strerror(err));
			// Don't also bind into IPv4 when listening in IPv6.
			flags |= UV_TCP_IPV6ONLY;
			break;

		default:
			uv_close((uv_handle_t*)this->uvHandle, (uv_close_cb)on_error_close);
			MS_THROW_ERROR("invalid binding IP '%s'", ip.c_str());
			break;
	}

	err = uv_tcp_bind(this->uvHandle, (const struct sockaddr*)&bind_addr, flags);
	if (err)
	{
		uv_close((uv_handle_t*)this->uvHandle, (uv_close_cb)on_error_close);
		MS_THROW_ERROR("uv_tcp_bind() failed: %s", uv_strerror(err));
	}

	err = uv_listen((uv_stream_t*)this->uvHandle, backlog, (uv_connection_cb)on_connection);
	if (err)
	{
		uv_close((uv_handle_t*)this->uvHandle, (uv_close_cb)on_error_close);
		MS_THROW_ERROR("uv_listen() failed: %s", uv_strerror(err));
	}

	// Set local address.
	if (!SetLocalAddress())
	{
		uv_close((uv_handle_t*)this->uvHandle, (uv_close_cb)on_error_close);
		MS_THROW_ERROR("error setting local IP and port");
	}
}

TCPServer::TCPServer(uv_tcp_t* uvHandle, int backlog) :
	uvHandle(uvHandle)
{
	MS_TRACE();

	int err;

	this->uvHandle->data = (void*)this;

	err = uv_listen((uv_stream_t*)this->uvHandle, backlog, (uv_connection_cb)on_connection);
	if (err)
	{
		uv_close((uv_handle_t*)this->uvHandle, (uv_close_cb)on_error_close);
		MS_THROW_ERROR("uv_listen() failed: %s", uv_strerror(err));
	}

	// Set local address.
	if (!SetLocalAddress())
	{
		uv_close((uv_handle_t*)this->uvHandle, (uv_close_cb)on_error_close);
		MS_THROW_ERROR("error setting local IP and port");
	}
}

TCPServer::~TCPServer()
{
	MS_TRACE();

	if (this->uvHandle)
		delete this->uvHandle;
}

const struct sockaddr* TCPServer::GetLocalAddress()
{
	MS_TRACE();

	return (const struct sockaddr*)&this->localAddr;
}

const std::string& TCPServer::GetLocalIP()
{
	MS_TRACE();

	return this->localIP;
}

MS_PORT TCPServer::GetLocalPort()
{
	MS_TRACE();

	return this->localPort;
}

void TCPServer::Close()
{
	MS_TRACE();

	RETURN_IF_CLOSING();

	this->isClosing = true;

	// If there are no connections then close now.
	if (this->connections.empty())
	{
		uv_close((uv_handle_t*)this->uvHandle, (uv_close_cb)on_close);
	}
	// Otherwise close all the connections (but not the TCP server).
	else
	{
		MS_DEBUG("closing %zu active connections", this->connections.size());

		for (auto it = this->connections.begin(); it != this->connections.end(); ++it)
		{
			TCPConnection* connection = *it;
			connection->Close();
		}
	}
}

void TCPServer::Dump()
{
	MS_DEBUG("[TCP | local address: %s : %u | status: %s | num connections: %zu]",
		this->localIP.c_str(), (unsigned int)this->localPort,
		(!this->isClosing) ? "open" : "closed",
		this->connections.size());
}

bool TCPServer::SetLocalAddress()
{
	MS_TRACE();

	int err;
	int len = sizeof(this->localAddr);

	err = uv_tcp_getsockname(this->uvHandle, (struct sockaddr*)&this->localAddr, &len);
	if (err)
	{
		MS_ERROR("uv_tcp_getsockname() failed: %s", uv_strerror(err));
		return false;
	}

	int family;
	Utils::IP::GetAddressInfo((const struct sockaddr*)&this->localAddr, &family, this->localIP, &this->localPort);

	return true;
}

inline
void TCPServer::onUvConnection(int status)
{
	MS_TRACE();

	RETURN_IF_CLOSING();

	int err;

	if (status)
	{
		MS_ERROR("error while receiving a new TCP connection: %s", uv_strerror(status));
		return;
	}

	// Notify the subclass so it provides an allocated derived class of TCPConnection.
	TCPConnection* connection = nullptr;
	userOnTCPConnectionAlloc(&connection);
	MS_ASSERT(connection != nullptr, "TCPConnection pointer was not allocated by the user");

	try
	{
		connection->Setup(this, &(this->localAddr), this->localIP, this->localPort);
	}
	catch (const MediaSoupError &error)
	{
		delete connection;
		return;
	}

	// Accept the connection.
	err = uv_accept((uv_stream_t*)this->uvHandle, (uv_stream_t*)connection->GetUvHandle());
	if (err)
		MS_ABORT("uv_accept() failed: %s", uv_strerror(err));

	// Insert the TCPConnection in the set.
	this->connections.insert(connection);

	// Start receiving data.
	try
	{
		connection->Start();
	}
	catch (const MediaSoupError &error)
	{
		MS_ERROR("cannot run the TCP connection: %s | closing the connection", error.what());
		connection->Close();
		// NOTE: Don't return here so the user won't be notified about a "onclose" for a TCP connection
		// for which there was not a previous "onnew" event.
	}

	MS_DEBUG("new TCP connection:");
	connection->Dump();

	// Notify the subclass.
	userOnNewTCPConnection(connection);
}

inline
void TCPServer::onUvClosed()
{
	MS_TRACE();

	// Motify the subclass.
	userOnTCPServerClosed();

	// And delete this.
	delete this;
}

inline
void TCPServer::onTCPConnectionClosed(TCPConnection* connection, bool is_closed_by_peer)
{
	MS_TRACE();

	MS_DEBUG("TCP connection closed:");
	connection->Dump();

	// Remove the TCPConnection from the set.
	this->connections.erase(connection);

	// Notify the subclass.
	userOnTCPConnectionClosed(connection, is_closed_by_peer);

	// Check if the server is closing connections and if this is the last
	// connection then close the server now.
	if (this->isClosing && this->connections.empty())
	{
		MS_DEBUG("last active connection closed");

		uv_close((uv_handle_t*)this->uvHandle, (uv_close_cb)on_close);
	}
}
