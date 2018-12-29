#define MS_CLASS "TcpConnection"
// #define MS_LOG_DEV

#include "handles/TcpConnection.hpp"
#include "DepLibUV.hpp"
#include "Logger.hpp"
#include "MediaSoupError.hpp"
#include "Utils.hpp"
#include <cstdlib> // std::malloc(), std::free()
#include <cstring> // std::memcpy()

/* Static methods for UV callbacks. */

inline static void onAlloc(uv_handle_t* handle, size_t suggestedSize, uv_buf_t* buf)
{
	static_cast<TcpConnection*>(handle->data)->OnUvReadAlloc(suggestedSize, buf);
}

inline static void onRead(uv_stream_t* handle, ssize_t nread, const uv_buf_t* buf)
{
	static_cast<TcpConnection*>(handle->data)->OnUvRead(nread, buf);
}

inline static void onWrite(uv_write_t* req, int status)
{
	auto* writeData           = static_cast<TcpConnection::UvWriteData*>(req->data);
	TcpConnection* connection = writeData->connection;

	// Delete the UvWriteData struct (which includes the uv_req_t and the store char[]).
	std::free(writeData);

	// Just notify the TcpConnection when error.
	if (status != 0)
		connection->OnUvWriteError(status);
}

inline static void onClose(uv_handle_t* handle)
{
	delete handle;
}

inline static void onShutdown(uv_shutdown_t* req, int status)
{
	auto* handle = req->handle;

	delete req;

	// Now do close the handle.
	uv_close(reinterpret_cast<uv_handle_t*>(handle), static_cast<uv_close_cb>(onClose));
}

/* Instance methods. */

TcpConnection::TcpConnection(size_t bufferSize) : bufferSize(bufferSize)
{
	MS_TRACE();

	this->uvHandle       = new uv_tcp_t;
	this->uvHandle->data = (void*)this;

	// NOTE: Don't allocate the buffer here. Instead wait for the first uv_alloc_cb().
}

TcpConnection::~TcpConnection()
{
	MS_TRACE();

	if (!this->closed)
		Close();

	delete[] this->buffer;
}

void TcpConnection::Close()
{
	MS_TRACE();

	if (this->closed)
		return;

	int err;

	this->closed = true;

	// Don't read more.
	err = uv_read_stop(reinterpret_cast<uv_stream_t*>(this->uvHandle));
	if (err != 0)
		MS_ABORT("uv_read_stop() failed: %s", uv_strerror(err));

	// If there is no error and the peer didn't close its connection side then close gracefully.
	if (!this->hasError && !this->isClosedByPeer)
	{
		// Use uv_shutdown() so pending data to be written will be sent to the peer
		// before closing.
		auto req  = new uv_shutdown_t;
		req->data = (void*)this;
		err       = uv_shutdown(
      req, reinterpret_cast<uv_stream_t*>(this->uvHandle), static_cast<uv_shutdown_cb>(onShutdown));
		if (err != 0)
			MS_ABORT("uv_shutdown() failed: %s", uv_strerror(err));
	}
	// Otherwise directly close the socket.
	else
	{
		uv_close(reinterpret_cast<uv_handle_t*>(this->uvHandle), static_cast<uv_close_cb>(onClose));
	}

	// Notify the listener.
	this->listener->OnTcpConnectionClosed(this, this->isClosedByPeer);
}

void TcpConnection::Dump() const
{
	MS_DEBUG_DEV("<TcpConnection>");
	MS_DEBUG_DEV(
	  "  [TCP, local:%s :%" PRIu16 ", remote:%s :%" PRIu16 ", status:%s]",
	  this->localIP.c_str(),
	  static_cast<uint16_t>(this->localPort),
	  this->peerIP.c_str(),
	  static_cast<uint16_t>(this->peerPort),
	  (!this->closed) ? "open" : "closed");
	MS_DEBUG_DEV("</TcpConnection>");
}

void TcpConnection::Setup(
  Listener* listener, struct sockaddr_storage* localAddr, const std::string& localIP, uint16_t localPort)
{
	MS_TRACE();

	// Set the listener.
	this->listener = listener;

	// Set the local address.
	this->localAddr = localAddr;
	this->localIP   = localIP;
	this->localPort = localPort;

	int err;

	// Set the UV handle.
	err = uv_tcp_init(DepLibUV::GetLoop(), this->uvHandle);
	if (err != 0)
	{
		delete this->uvHandle;
		this->uvHandle = nullptr;

		MS_THROW_ERROR("uv_tcp_init() failed: %s", uv_strerror(err));
	}
}

void TcpConnection::Start()
{
	MS_TRACE();

	if (this->closed)
		return;

	int err;

	err = uv_read_start(
	  reinterpret_cast<uv_stream_t*>(this->uvHandle),
	  static_cast<uv_alloc_cb>(onAlloc),
	  static_cast<uv_read_cb>(onRead));
	if (err != 0)
		MS_THROW_ERROR("uv_read_start() failed: %s", uv_strerror(err));

	// Get the peer address.
	if (!SetPeerAddress())
		MS_THROW_ERROR("error setting peer IP and port");
}

void TcpConnection::Write(const uint8_t* data, size_t len)
{
	MS_TRACE();

	if (this->closed)
		return;

	if (len == 0)
		return;

	uv_buf_t buffer;
	int written;
	int err;

	// First try uv_try_write(). In case it can not directly write all the given
	// data then build a uv_req_t and use uv_write().

	buffer  = uv_buf_init(reinterpret_cast<char*>(const_cast<uint8_t*>(data)), len);
	written = uv_try_write(reinterpret_cast<uv_stream_t*>(this->uvHandle), &buffer, 1);

	// All the data was written. Done.
	if (written == static_cast<int>(len))
	{
		return;
	}
	// Cannot write any data at first time. Use uv_write().
	if (written == UV_EAGAIN || written == UV_ENOSYS)
	{
		// Set written to 0 so pendingLen can be properly calculated.
		written = 0;
	}
	// Error. Should not happen.
	else if (written < 0)
	{
		MS_WARN_DEV("uv_try_write() failed, closing the connection: %s", uv_strerror(written));

		Close();

		return;
	}

	// MS_DEBUG_DEV(
	// 	"could just write %zu bytes (%zu given) at first time, using uv_write() now",
	// 	static_cast<size_t>(written), len);

	size_t pendingLen = len - written;
	// Allocate a special UvWriteData struct pointer.
	auto* writeData = static_cast<UvWriteData*>(std::malloc(sizeof(UvWriteData) + pendingLen));

	writeData->connection = this;
	std::memcpy(writeData->store, data + written, pendingLen);
	writeData->req.data = (void*)writeData;

	buffer = uv_buf_init(reinterpret_cast<char*>(writeData->store), pendingLen);

	err = uv_write(
	  &writeData->req,
	  reinterpret_cast<uv_stream_t*>(this->uvHandle),
	  &buffer,
	  1,
	  static_cast<uv_write_cb>(onWrite));
	if (err != 0)
		MS_ABORT("uv_write() failed: %s", uv_strerror(err));
}

void TcpConnection::Write(const uint8_t* data1, size_t len1, const uint8_t* data2, size_t len2)
{
	MS_TRACE();

	if (this->closed)
		return;

	if (len1 == 0 && len2 == 0)
		return;

	size_t totalLen = len1 + len2;
	uv_buf_t buffers[2];
	int written;
	int err;

	// First try uv_try_write(). In case it can not directly write all the given
	// data then build a uv_req_t and use uv_write().

	buffers[0] = uv_buf_init(reinterpret_cast<char*>(const_cast<uint8_t*>(data1)), len1);
	buffers[1] = uv_buf_init(reinterpret_cast<char*>(const_cast<uint8_t*>(data2)), len2);
	written    = uv_try_write(reinterpret_cast<uv_stream_t*>(this->uvHandle), buffers, 2);

	// All the data was written. Done.
	if (written == static_cast<int>(totalLen))
	{
		return;
	}
	// Cannot write any data at first time. Use uv_write().
	if (written == UV_EAGAIN || written == UV_ENOSYS)
	{
		// Set written to 0 so pendingLen can be properly calculated.
		written = 0;
	}
	// Error. Should not happen.
	else if (written < 0)
	{
		MS_WARN_DEV("uv_try_write() failed, closing the connection: %s", uv_strerror(written));

		Close();

		return;
	}

	// MS_DEBUG_DEV(
	// 	"could just write %zu bytes (%zu given) at first time, using uv_write() now",
	// 	static_cast<size_t>(written), totalLen);

	size_t pendingLen = totalLen - written;

	// Allocate a special UvWriteData struct pointer.
	auto* writeData = static_cast<UvWriteData*>(std::malloc(sizeof(UvWriteData) + pendingLen));

	writeData->connection = this;
	// If the first buffer was not entirely written then splice it.
	if (static_cast<size_t>(written) < len1)
	{
		std::memcpy(
		  writeData->store, data1 + static_cast<size_t>(written), len1 - static_cast<size_t>(written));
		std::memcpy(writeData->store + (len1 - static_cast<size_t>(written)), data2, len2);
	}
	// Otherwise just take the pending data in the second buffer.
	else
	{
		std::memcpy(
		  writeData->store,
		  data2 + (static_cast<size_t>(written) - len1),
		  len2 - (static_cast<size_t>(written) - len1));
	}
	writeData->req.data = (void*)writeData;

	uv_buf_t buffer = uv_buf_init(reinterpret_cast<char*>(writeData->store), pendingLen);

	err = uv_write(
	  &writeData->req,
	  reinterpret_cast<uv_stream_t*>(this->uvHandle),
	  &buffer,
	  1,
	  static_cast<uv_write_cb>(onWrite));
	if (err != 0)
		MS_ABORT("uv_write() failed: %s", uv_strerror(err));
}

bool TcpConnection::SetPeerAddress()
{
	MS_TRACE();

	int err;
	int len = sizeof(this->peerAddr);

	err = uv_tcp_getpeername(this->uvHandle, reinterpret_cast<struct sockaddr*>(&this->peerAddr), &len);
	if (err != 0)
	{
		MS_ERROR("uv_tcp_getpeername() failed: %s", uv_strerror(err));

		return false;
	}

	int family;
	Utils::IP::GetAddressInfo(
	  reinterpret_cast<const struct sockaddr*>(&this->peerAddr), &family, this->peerIP, &this->peerPort);

	return true;
}

inline void TcpConnection::OnUvReadAlloc(size_t /*suggestedSize*/, uv_buf_t* buf)
{
	MS_TRACE();

	if (this->closed)
		return;

	// If this is the first call to onUvReadAlloc() then allocate the receiving buffer now.
	if (this->buffer == nullptr)
		this->buffer = new uint8_t[this->bufferSize];

	// Tell UV to write after the last data byte in the buffer.
	buf->base = reinterpret_cast<char*>(this->buffer + this->bufferDataLen);
	// Give UV all the remaining space in the buffer.
	if (this->bufferSize > this->bufferDataLen)
	{
		buf->len = this->bufferSize - this->bufferDataLen;
	}
	else
	{
		buf->len = 0;

		MS_WARN_DEV("no available space in the buffer");
	}
}

inline void TcpConnection::OnUvRead(ssize_t nread, const uv_buf_t* /*buf*/)
{
	MS_TRACE();

	if (this->closed)
		return;

	if (nread == 0)
		return;

	// Data received.
	if (nread > 0)
	{
		// Update the buffer data length.
		this->bufferDataLen += static_cast<size_t>(nread);

		// Notify the subclass.
		UserOnTcpConnectionRead();
	}
	// Client disconneted.
	else if (nread == UV_EOF || nread == UV_ECONNRESET)
	{
		MS_DEBUG_DEV("connection closed by peer, closing server side");

		this->isClosedByPeer = true;

		// Close server side of the connection.
		Close();
	}
	// Some error.
	else
	{
		MS_WARN_DEV("read error, closing the connection: %s", uv_strerror(nread));

		this->hasError = true;

		// Close server side of the connection.
		Close();
	}
}

inline void TcpConnection::OnUvWriteError(int error)
{
	MS_TRACE();

	if (this->closed)
		return;

	if (error != UV_EPIPE && error != UV_ENOTCONN)
		this->hasError = true;

	MS_WARN_DEV("write error, closing the connection: %s", uv_strerror(error));

	Close();
}
