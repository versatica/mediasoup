#define MS_CLASS "TcpServer"
// #define MS_LOG_DEV

#include "handles/TcpServer.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "Utils.hpp"

/* Static methods for UV callbacks. */

inline static void onConnection(uv_stream_t* handle, int status)
{
	auto* server = static_cast<TcpServer*>(handle->data);

	if (server == nullptr)
		return;

	server->OnUvConnection(status);
}

inline static void onClose(uv_handle_t* handle)
{
	delete handle;
}

/* Instance methods. */

// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
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

	// Tell the UV handle that the TcpServer has been closed.
	this->uvHandle->data = nullptr;

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
	MS_DUMP("<TcpServer>");
	MS_DUMP(
	  "  [TCP, local:%s :%" PRIu16 ", status:%s, connections:%zu]",
	  this->localIp.c_str(),
	  static_cast<uint16_t>(this->localPort),
	  (!this->closed) ? "open" : "closed",
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
	  reinterpret_cast<const struct sockaddr*>(&this->localAddr), family, this->localIp, this->localPort);

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
		connection->Setup(this, &(this->localAddr), this->localIp, this->localPort);
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
		MS_ERROR("cannot start the TCP connection, closing the connection: %s", error.what());

		delete connection;
	}
}

inline void TcpServer::OnTcpConnectionClosed(TcpConnection* connection, bool isClosedByPeer)
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
