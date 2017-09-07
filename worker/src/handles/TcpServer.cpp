#define MS_CLASS "TcpServer"
// #define MS_LOG_DEV

#include "handles/TcpServer.hpp"
#include "DepLibUV.hpp"
#include "Logger.hpp"
#include "MediaSoupError.hpp"
#include "Utils.hpp"

/* Static methods for UV callbacks. */

inline static void onConnection(uv_stream_t* handle, int status)
{
	static_cast<TcpServer*>(handle->data)->OnUvConnection(status);
}

inline static void onClose(uv_handle_t* handle)
{
	static_cast<TcpServer*>(handle->data)->OnUvClosed();
}

inline static void onErrorClose(uv_handle_t* handle)
{
	delete handle;
}

/* Instance methods. */

TcpServer::TcpServer(const std::string& ip, uint16_t port, int backlog)
{
	MS_TRACE();

	int err;
	int flags = 0;

	this->uvHandle       = new uv_tcp_t;
	this->uvHandle->data = (void*)this;

	err = uv_tcp_init(DepLibUV::GetLoop(), this->uvHandle);
	if (err != 0)
	{
		delete this->uvHandle;
		this->uvHandle = nullptr;

		MS_THROW_ERROR("uv_tcp_init() failed: %s", uv_strerror(err));
	}

	// clang-format off
	struct sockaddr_storage bindAddr{};
	// clang-format on

	switch (Utils::IP::GetFamily(ip))
	{
		case AF_INET:
			err = uv_ip4_addr(
			  ip.c_str(), static_cast<int>(port), reinterpret_cast<struct sockaddr_in*>(&bindAddr));
			if (err != 0)
				MS_ABORT("uv_ipv4_addr() failed: %s", uv_strerror(err));
			break;

		case AF_INET6:
			err = uv_ip6_addr(
			  ip.c_str(), static_cast<int>(port), reinterpret_cast<struct sockaddr_in6*>(&bindAddr));
			if (err != 0)
				MS_ABORT("uv_ipv6_addr() failed: %s", uv_strerror(err));
			// Don't also bind into IPv4 when listening in IPv6.
			flags |= UV_TCP_IPV6ONLY;
			break;

		default:
			uv_close(reinterpret_cast<uv_handle_t*>(this->uvHandle), static_cast<uv_close_cb>(onErrorClose));
			MS_THROW_ERROR("invalid binding IP '%s'", ip.c_str());
			break;
	}

	err = uv_tcp_bind(this->uvHandle, reinterpret_cast<const struct sockaddr*>(&bindAddr), flags);
	if (err != 0)
	{
		uv_close(reinterpret_cast<uv_handle_t*>(this->uvHandle), static_cast<uv_close_cb>(onErrorClose));
		MS_THROW_ERROR("uv_tcp_bind() failed: %s", uv_strerror(err));
	}

	err = uv_listen(
	  reinterpret_cast<uv_stream_t*>(this->uvHandle),
	  backlog,
	  static_cast<uv_connection_cb>(onConnection));
	if (err != 0)
	{
		uv_close(reinterpret_cast<uv_handle_t*>(this->uvHandle), static_cast<uv_close_cb>(onErrorClose));
		MS_THROW_ERROR("uv_listen() failed: %s", uv_strerror(err));
	}

	// Set local address.
	if (!SetLocalAddress())
	{
		uv_close(reinterpret_cast<uv_handle_t*>(this->uvHandle), static_cast<uv_close_cb>(onErrorClose));
		MS_THROW_ERROR("error setting local IP and port");
	}
}

TcpServer::TcpServer(uv_tcp_t* uvHandle, int backlog) : uvHandle(uvHandle)
{
	MS_TRACE();

	int err;

	this->uvHandle->data = (void*)this;

	err = uv_listen(
	  reinterpret_cast<uv_stream_t*>(this->uvHandle),
	  backlog,
	  static_cast<uv_connection_cb>(onConnection));
	if (err != 0)
	{
		uv_close(reinterpret_cast<uv_handle_t*>(this->uvHandle), static_cast<uv_close_cb>(onErrorClose));
		MS_THROW_ERROR("uv_listen() failed: %s", uv_strerror(err));
	}

	// Set local address.
	if (!SetLocalAddress())
	{
		uv_close(reinterpret_cast<uv_handle_t*>(this->uvHandle), static_cast<uv_close_cb>(onErrorClose));
		MS_THROW_ERROR("error setting local IP and port");
	}
}

TcpServer::~TcpServer()
{
	MS_TRACE();

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
		uv_close(reinterpret_cast<uv_handle_t*>(this->uvHandle), static_cast<uv_close_cb>(onClose));
	}
	// Otherwise close all the connections (but not the TCP server).
	else
	{
		MS_DEBUG_DEV("closing %zu active connections", this->connections.size());

		for (auto connection : this->connections)
		{
			connection->Destroy();
		}
	}
}

void TcpServer::Dump() const
{
	MS_DUMP("<TcpServer>");
	MS_DUMP(
	  "  [TCP, local:%s :%" PRIu16 ", status:%s, connections:%zu]",
	  this->localIP.c_str(),
	  static_cast<uint16_t>(this->localPort),
	  (!this->isClosing) ? "open" : "closed",
	  this->connections.size());
	MS_DUMP("</TcpServer>");
}

bool TcpServer::SetLocalAddress()
{
	MS_TRACE();

	int err;
	int len = sizeof(this->localAddr);

	err =
	  uv_tcp_getsockname(this->uvHandle, reinterpret_cast<struct sockaddr*>(&this->localAddr), &len);
	if (err != 0)
	{
		MS_ERROR("uv_tcp_getsockname() failed: %s", uv_strerror(err));

		return false;
	}

	int family;
	Utils::IP::GetAddressInfo(
	  reinterpret_cast<const struct sockaddr*>(&this->localAddr),
	  &family,
	  this->localIP,
	  &this->localPort);

	return true;
}

inline void TcpServer::OnUvConnection(int status)
{
	MS_TRACE();

	if (this->isClosing)
		return;

	int err;

	if (status != 0)
	{
		MS_ERROR("error while receiving a new TCP connection: %s", uv_strerror(status));

		return;
	}

	// Notify the subclass so it provides an allocated derived class of TCPConnection.
	TcpConnection* connection = nullptr;
	UserOnTcpConnectionAlloc(&connection);

	MS_ASSERT(connection != nullptr, "TcpConnection pointer was not allocated by the user");

	try
	{
		connection->Setup(this, &(this->localAddr), this->localIP, this->localPort);
	}
	catch (const MediaSoupError& error)
	{
		delete connection;

		return;
	}

	// Accept the connection.
	err = uv_accept(
	  reinterpret_cast<uv_stream_t*>(this->uvHandle),
	  reinterpret_cast<uv_stream_t*>(connection->GetUvHandle()));
	if (err != 0)
		MS_ABORT("uv_accept() failed: %s", uv_strerror(err));

	// Insert the TcpConnection in the set.
	this->connections.insert(connection);

	// Start receiving data.
	try
	{
		connection->Start();
	}
	catch (const MediaSoupError& error)
	{
		MS_ERROR("cannot run the TCP connection, closing the connection: %s", error.what());

		connection->Destroy();

		// NOTE: Don't return here so the user won't be notified about a TCP connection
		// closure for which there was not a previous creation event.
	}

	// Notify the subclass.
	UserOnNewTcpConnection(connection);
}

inline void TcpServer::OnUvClosed()
{
	MS_TRACE();

	// Motify the subclass.
	UserOnTcpServerClosed();

	// And delete this.
	delete this;
}

inline void TcpServer::OnTcpConnectionClosed(TcpConnection* connection, bool isClosedByPeer)
{
	MS_TRACE();

	// NOTE:
	// Worst scenario is that in which this is the latest connection,
	// which is remotely closed (no TcpServer.Destroy() was called) and the user
	// call TcpServer.Destroy() on UserOnTcpConnectionClosed() callback, so Destroy()
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
	UserOnTcpConnectionClosed(connection, isClosedByPeer);

	// Check if the server was closing connections, and if this is the last
	// connection then close the server now.
	if (wasClosing && this->connections.empty())
		uv_close(reinterpret_cast<uv_handle_t*>(this->uvHandle), static_cast<uv_close_cb>(onClose));
}
