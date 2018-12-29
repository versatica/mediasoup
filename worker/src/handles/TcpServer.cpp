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

	struct sockaddr_storage bindAddr;

	switch (Utils::IP::GetFamily(ip))
	{
		case AF_INET:
		{
			err = uv_ip4_addr(
			  ip.c_str(), static_cast<int>(port), reinterpret_cast<struct sockaddr_in*>(&bindAddr));

			if (err != 0)
				MS_ABORT("uv_ipv4_addr() failed: %s", uv_strerror(err));

			break;
		}

		case AF_INET6:
		{
			err = uv_ip6_addr(
			  ip.c_str(), static_cast<int>(port), reinterpret_cast<struct sockaddr_in6*>(&bindAddr));

			if (err != 0)
				MS_ABORT("uv_ipv6_addr() failed: %s", uv_strerror(err));

			// Don't also bind into IPv4 when listening in IPv6.
			flags |= UV_TCP_IPV6ONLY;

			break;
		}

		default:
		{
			uv_close(reinterpret_cast<uv_handle_t*>(this->uvHandle), static_cast<uv_close_cb>(onClose));

			MS_THROW_ERROR("invalid binding IP '%s'", ip.c_str());

			break;
		}
	}

	err = uv_tcp_bind(this->uvHandle, reinterpret_cast<const struct sockaddr*>(&bindAddr), flags);
	if (err != 0)
	{
		uv_close(reinterpret_cast<uv_handle_t*>(this->uvHandle), static_cast<uv_close_cb>(onClose));
		MS_THROW_ERROR("uv_tcp_bind() failed: %s", uv_strerror(err));
	}

	err = uv_listen(
	  reinterpret_cast<uv_stream_t*>(this->uvHandle),
	  backlog,
	  static_cast<uv_connection_cb>(onConnection));
	if (err != 0)
	{
		uv_close(reinterpret_cast<uv_handle_t*>(this->uvHandle), static_cast<uv_close_cb>(onClose));
		MS_THROW_ERROR("uv_listen() failed: %s", uv_strerror(err));
	}

	// Set local address.
	if (!SetLocalAddress())
	{
		uv_close(reinterpret_cast<uv_handle_t*>(this->uvHandle), static_cast<uv_close_cb>(onClose));
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
		uv_close(reinterpret_cast<uv_handle_t*>(this->uvHandle), static_cast<uv_close_cb>(onClose));
		MS_THROW_ERROR("uv_listen() failed: %s", uv_strerror(err));
	}

	// Set local address.
	if (!SetLocalAddress())
	{
		uv_close(reinterpret_cast<uv_handle_t*>(this->uvHandle), static_cast<uv_close_cb>(onClose));
		MS_THROW_ERROR("error setting local IP and port");
	}
}

TcpServer::~TcpServer()
{
	MS_TRACE();

	if (!this->closed)
		Close();
}

void TcpServer::Close()
{
	MS_TRACE();

	if (this->closed)
		return;

	this->closed = true;

	MS_DEBUG_DEV("closing %zu active connections", this->connections.size());

	for (auto it = this->connections.begin(); it != this->connections.end();)
	{
		auto* connection = *it;

		it = this->connections.erase(it);
		delete connection;
	}

	uv_close(reinterpret_cast<uv_handle_t*>(this->uvHandle), static_cast<uv_close_cb>(onClose));
}

void TcpServer::Dump() const
{
	MS_DEBUG_DEV("<TcpServer>");
	MS_DEBUG_DEV(
	  "  [TCP, local:%s :%" PRIu16 ", status:%s, connections:%zu]",
	  this->localIP.c_str(),
	  static_cast<uint16_t>(this->localPort),
	  (!this->closed) ? "open" : "closed",
	  this->connections.size());
	MS_DEBUG_DEV("</TcpServer>");
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

	if (this->closed)
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

		// Notify the subclass.
		UserOnNewTcpConnection(connection);
	}
	catch (const MediaSoupError& error)
	{
		MS_ERROR("cannot run the TCP connection, closing the connection: %s", error.what());

		delete connection;
	}
}

void TcpServer::OnTcpConnectionClosed(TcpConnection* connection, bool isClosedByPeer)
{
	MS_TRACE();

	MS_DEBUG_DEV("TCP connection closed");

	// Remove the TcpConnection from the set.
	size_t numErased = this->connections.erase(connection);

	// If the closed connection was not present in the set, do nothing else.
	if (numErased == 0)
		return;

	// Notify the subclass.
	UserOnTcpConnectionClosed(connection, isClosedByPeer);
}
