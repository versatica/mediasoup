#define MS_CLASS "TcpServer"
// #define MS_LOG_DEV

#include "handles/TcpServer.hpp"
#include "Utils.hpp"
#include "DepLibUV.hpp"
#include "MediaSoupError.hpp"
#include "Logger.hpp"

/* Static methods for UV callbacks. */

static inline
void on_connection(uv_stream_t* handle, int status)
{
	static_cast<TcpServer*>(handle->data)->onUvConnection(status);
}

static inline
void on_close(uv_handle_t* handle)
{
	static_cast<TcpServer*>(handle->data)->onUvClosed();
}

static inline
void on_error_close(uv_handle_t* handle)
{
	delete handle;
}

/* Instance methods. */

TcpServer::TcpServer(const std::string &ip, uint16_t port, int backlog)
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

TcpServer::TcpServer(uv_tcp_t* uvHandle, int backlog) :
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

TcpServer::~TcpServer()
{
	MS_TRACE();

	if (this->uvHandle)
		delete this->uvHandle;
}

void TcpServer::Destroy()
{
	MS_TRACE();

	if (this->isClosing)
		return;

	this->isClosing = true;

	// If there are no connections then close now.
	if (this->connections.empty())
	{
		uv_close((uv_handle_t*)this->uvHandle, (uv_close_cb)on_close);
	}
	// Otherwise close all the connections (but not the TCP server).
	else
	{
		MS_DEBUG_DEV("closing %zu active connections", this->connections.size());

		for (auto it = this->connections.begin(); it != this->connections.end(); ++it)
		{
			TcpConnection* connection = *it;
			connection->Destroy();
		}
	}
}

void TcpServer::Dump() const
{
	MS_DUMP("<TcpServer>");
	MS_DUMP("  [TCP, local:%s :%" PRIu16 ", status:%s, connections:%zu]",
		this->localIP.c_str(), (uint16_t)this->localPort,
		(!this->isClosing) ? "open" : "closed",
		this->connections.size());
	MS_DUMP("</TcpServer>");
}

bool TcpServer::SetLocalAddress()
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
void TcpServer::onUvConnection(int status)
{
	MS_TRACE();

	if (this->isClosing)
		return;

	int err;

	if (status)
	{
		MS_ERROR("error while receiving a new TCP connection: %s", uv_strerror(status));

		return;
	}

	// Notify the subclass so it provides an allocated derived class of TCPConnection.
	TcpConnection* connection = nullptr;
	userOnTcpConnectionAlloc(&connection);
	MS_ASSERT(connection != nullptr, "TcpConnection pointer was not allocated by the user");

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

	// Insert the TcpConnection in the set.
	this->connections.insert(connection);

	// Start receiving data.
	try
	{
		connection->Start();
	}
	catch (const MediaSoupError &error)
	{
		MS_ERROR("cannot run the TCP connection, closing the connection: %s", error.what());

		connection->Destroy();

		// NOTE: Don't return here so the user won't be notified about a TCP connection
		// closure for which there was not a previous creation event.
	}

	// Notify the subclass.
	userOnNewTcpConnection(connection);
}

inline
void TcpServer::onUvClosed()
{
	MS_TRACE();

	// Motify the subclass.
	userOnTcpServerClosed();

	// And delete this.
	delete this;
}

inline
void TcpServer::onTcpConnectionClosed(TcpConnection* connection, bool is_closed_by_peer)
{
	MS_TRACE();

	// NOTE:
	// Worst scenario is that in which this is the latest connection,
	// which is remotely closed (no TcpServer.Destroy() was called) and the user
	// call TcpServer.Destroy() on userOnTcpConnectionClosed() callback, so Destroy()
	// is called with zero connections and calls uv_close(), but then
	// onTcpConnectionClosed() continues and finds that isClosing is true and
	// there are zero connections, so calls uv_close() again and get a crash.
	//
	// SOLUTION:
	// Check isClosing value *before* onTcpConnectionClosed() callback.

	bool wasClosing = this->isClosing;

	MS_DEBUG_DEV("TCP connection closed");

	// Remove the TcpConnection from the set.
	this->connections.erase(connection);

	// Notify the subclass.
	userOnTcpConnectionClosed(connection, is_closed_by_peer);

	// Check if the server was closing connections, and if this is the last
	// connection then close the server now.
	if (wasClosing && this->connections.empty())
		uv_close((uv_handle_t*)this->uvHandle, (uv_close_cb)on_close);
}
