#define MS_CLASS "TcpServerHandler"
// #define MS_LOG_DEV_LEVEL 3

#include "handles/TcpServerHandler.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "Utils.hpp"

/* Static methods for UV callbacks. */

inline static void onConnection(uv_stream_t* handle, int status)
{
	auto* server = static_cast<TcpServerHandler*>(handle->data);

	if (server)
		server->OnUvConnection(status);
}

inline static void onClose(uv_handle_t* handle)
{
	delete handle;
}

/* Instance methods. */

// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
TcpServerHandler::TcpServerHandler(uv_tcp_t* uvHandle, int backlog) : uvHandle(uvHandle)
{
	MS_TRACE();

	int err;

	this->uvHandle->data = static_cast<void*>(this);

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

TcpServerHandler::~TcpServerHandler()
{
	MS_TRACE();

	if (!this->closed)
		Close();
}

void TcpServerHandler::Close()
{
	MS_TRACE();

	if (this->closed)
		return;

	this->closed = true;

	// Tell the UV handle that the TcpServerHandler has been closed.
	this->uvHandle->data = nullptr;

	MS_DEBUG_DEV("closing %zu active connections", this->connections.size());

	for (auto* connection : this->connections)
	{
		delete connection;
	}

	uv_close(reinterpret_cast<uv_handle_t*>(this->uvHandle), static_cast<uv_close_cb>(onClose));
}

void TcpServerHandler::Dump() const
{
	MS_DUMP("<TcpServerHandler>");
	MS_DUMP(
	  "  [TCP, local:%s :%" PRIu16 ", status:%s, connections:%zu]",
	  this->localIp.c_str(),
	  static_cast<uint16_t>(this->localPort),
	  (!this->closed) ? "open" : "closed",
	  this->connections.size());
	MS_DUMP("</TcpServerHandler>");
}

void TcpServerHandler::AcceptTcpConnection(TcpConnectionHandler* connection)
{
	MS_TRACE();

	MS_ASSERT(connection != nullptr, "TcpConnectionHandler pointer was not allocated by the user");

	try
	{
		connection->Setup(this, &(this->localAddr), this->localIp, this->localPort);
	}
	catch (const MediaSoupError& error)
	{
		delete connection;

		return;
	}

	// Accept the connection.
	int err = uv_accept(
	  reinterpret_cast<uv_stream_t*>(this->uvHandle),
	  reinterpret_cast<uv_stream_t*>(connection->GetUvHandle()));

	if (err != 0)
		MS_ABORT("uv_accept() failed: %s", uv_strerror(err));

	// Start receiving data.
	try
	{
		// NOTE: This may throw.
		connection->Start();
	}
	catch (const MediaSoupError& error)
	{
		delete connection;

		return;
	}

	// Store it.
	this->connections.insert(connection);
}

bool TcpServerHandler::SetLocalAddress()
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
	  reinterpret_cast<const struct sockaddr*>(&this->localAddr), family, this->localIp, this->localPort);

	return true;
}

inline void TcpServerHandler::OnUvConnection(int status)
{
	MS_TRACE();

	if (this->closed)
		return;

	if (status != 0)
	{
		MS_ERROR("error while receiving a new TCP connection: %s", uv_strerror(status));

		return;
	}

	// Notify the subclass about a new TCP connection attempt.
	UserOnTcpConnectionAlloc();
}

inline void TcpServerHandler::OnTcpConnectionClosed(TcpConnectionHandler* connection)
{
	MS_TRACE();

	MS_DEBUG_DEV("TCP connection closed");

	// Remove the TcpConnectionHandler from the set.
	this->connections.erase(connection);

	// Notify the subclass.
	UserOnTcpConnectionClosed(connection);

	// Delete it.
	delete connection;
}
