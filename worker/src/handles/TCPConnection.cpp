#define MS_CLASS "TCPConnection"

#include "handles/TCPConnection.h"
#include "Utils.h"
#include "DepLibUV.h"
#include "MediaSoupError.h"
#include "Logger.h"
#include <cstring>  // std::memcpy()
#include <cstdlib>  // std::malloc(), std::free()

/* Static methods for UV callbacks. */

static inline
void on_alloc(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf)
{
	static_cast<TCPConnection*>(handle->data)->onUvReadAlloc(suggested_size, buf);
}

static inline
void on_read(uv_stream_t* handle, ssize_t nread, const uv_buf_t* buf)
{
	static_cast<TCPConnection*>(handle->data)->onUvRead(nread, buf);
}

static inline
void on_write(uv_write_t* req, int status)
{
	TCPConnection::UvWriteData* write_data = static_cast<TCPConnection::UvWriteData*>(req->data);
	TCPConnection* connection = write_data->connection;

	// Delete the UvWriteData struct (which includes the uv_req_t and the store char[]).
	std::free(write_data);

	// Just notify the TCPConnection when error.
	if (status)
		connection->onUvWriteError(status);
}

static inline
void on_shutdown(uv_shutdown_t* req, int status)
{
	static_cast<TCPConnection*>(req->data)->onUvShutdown(req, status);
}

static inline
void on_close(uv_handle_t* handle)
{
	static_cast<TCPConnection*>(handle->data)->onUvClosed();
}

/* Instance methods. */

TCPConnection::TCPConnection(size_t bufferSize) :
	bufferSize(bufferSize)
{
	MS_TRACE();

	this->uvHandle = new uv_tcp_t;
	this->uvHandle->data = (void*)this;

	// NOTE: Don't allocate the buffer here. Instead wait for the first uv_alloc_cb().
}

TCPConnection::~TCPConnection()
{
	MS_TRACE();

	if (this->uvHandle)
		delete this->uvHandle;
	if (this->buffer)
		delete[] this->buffer;
}

void TCPConnection::Setup(Listener* listener, struct sockaddr_storage* localAddr, const std::string &localIP, uint16_t localPort)
{
	MS_TRACE();

	int err;

	// Set the UV handle.
	err = uv_tcp_init(DepLibUV::GetLoop(), this->uvHandle);
	if (err)
	{
		delete this->uvHandle;
		this->uvHandle = nullptr;
		MS_THROW_ERROR("uv_tcp_init() failed: %s", uv_strerror(err));
	}

	// Set the listener.
	this->listener = listener;

	// Set the local address.
	this->localAddr = localAddr;
	this->localIP = localIP;
	this->localPort = localPort;
}

void TCPConnection::Close()
{
	MS_TRACE();

	if (this->isClosing)
		return;

	int err;

	this->isClosing = true;

	// Don't read more.
	err = uv_read_stop((uv_stream_t*)this->uvHandle);
	if (err)
		MS_ABORT("uv_read_stop() failed: %s", uv_strerror(err));

	// If there is no error and the peer didn't close its connection side then close gracefully.
	if (!this->hasError && !this->isClosedByPeer)
	{
		// Use uv_shutdown() so pending data to be written will be sent to the peer
		// before closing.
		uv_shutdown_t* req = new uv_shutdown_t;
		req->data = (void*)this;
		err = uv_shutdown(req, (uv_stream_t*)this->uvHandle, (uv_shutdown_cb)on_shutdown);
		if (err)
			MS_ABORT("uv_shutdown() failed: %s", uv_strerror(err));
	}
	// Otherwise directly close the socket.
	else
	{
		uv_close((uv_handle_t*)this->uvHandle, (uv_close_cb)on_close);
	}
}

void TCPConnection::Dump()
{
	MS_DEBUG("[TCP, local:%s :%" PRIu16 ", remote:%s :%" PRIu16 ", status:%s]",
		this->localIP.c_str(), (uint16_t)this->localPort,
		this->peerIP.c_str(), (uint16_t)this->peerPort,
		(!this->isClosing) ? "open" : "closed");
}

void TCPConnection::Start()
{
	MS_TRACE();

	if (this->isClosing)
		return;

	int err;

	err = uv_read_start((uv_stream_t*)this->uvHandle, (uv_alloc_cb)on_alloc, (uv_read_cb)on_read);
	if (err)
		MS_THROW_ERROR("uv_read_start() failed: %s", uv_strerror(err));

	// Get the peer address.
	if (!SetPeerAddress())
		MS_THROW_ERROR("error setting peer IP and port");
}

void TCPConnection::Write(const uint8_t* data, size_t len)
{
	MS_TRACE();

	if (this->isClosing)
		return;

	if (len == 0)
		return;

	uv_buf_t buffer;
	int written;
	int err;

	// First try uv_try_write(). In case it can not directly write all the given
	// data then build a uv_req_t and use uv_write().

	buffer = uv_buf_init((char*)data, len);
	written = uv_try_write((uv_stream_t*)this->uvHandle, &buffer, 1);

	// All the data was written. Done.
	if (written == (int)len)
	{
		return;
	}
	// Cannot write any data at first time. Use uv_write().
	else if (written == UV_EAGAIN || written == UV_ENOSYS)
	{
		// Set written to 0 so pending_len can be properly calculated.
		written = 0;
	}
	// Error. Should not happen.
	else if (written < 0)
	{
		MS_WARN("uv_try_write() failed, closing the connection: %s", uv_strerror(written));

		Close();
		return;
	}

	// MS_DEBUG("could just write %zu bytes (%zu given) at first time, using uv_write() now", (size_t)written, len);

	size_t pending_len = len - written;

	// Allocate a special UvWriteData struct pointer.
	UvWriteData* write_data = (UvWriteData*)std::malloc(sizeof(UvWriteData) + pending_len);

	write_data->connection = this;
	std::memcpy(write_data->store, data + written, pending_len);
	write_data->req.data = (void*)write_data;

	buffer = uv_buf_init((char*)write_data->store, pending_len);

	err = uv_write(&write_data->req, (uv_stream_t*)this->uvHandle, &buffer, 1, (uv_write_cb)on_write);
	if (err)
		MS_ABORT("uv_write() failed: %s", uv_strerror(err));
}

void TCPConnection::Write(const uint8_t* data1, size_t len1, const uint8_t* data2, size_t len2)
{
	MS_TRACE();

	if (this->isClosing)
		return;

	if (len1 == 0 && len2 == 0)
		return;

	size_t total_len = len1 + len2;
	uv_buf_t buffers[2];
	int written;
	int err;

	// First try uv_try_write(). In case it can not directly write all the given
	// data then build a uv_req_t and use uv_write().

	buffers[0] = uv_buf_init((char*)data1, len1);
	buffers[1] = uv_buf_init((char*)data2, len2);
	written = uv_try_write((uv_stream_t*)this->uvHandle, buffers, 2);

	// All the data was written. Done.
	if (written == (int)total_len)
	{
		return;
	}
	// Cannot write any data at first time. Use uv_write().
	else if (written == UV_EAGAIN || written == UV_ENOSYS)
	{
		// Set written to 0 so pending_len can be properly calculated.
		written = 0;
	}
	// Error. Should not happen.
	else if (written < 0)
	{
		MS_WARN("uv_try_write() failed, closing the connection: %s", uv_strerror(written));

		Close();
		return;
	}

	// MS_DEBUG("could just write %zu bytes (%zu given) at first time, using uv_write() now", (size_t)written, total_len);

	size_t pending_len = total_len - written;

	// Allocate a special UvWriteData struct pointer.
	UvWriteData* write_data = (UvWriteData*)std::malloc(sizeof(UvWriteData) + pending_len);

	write_data->connection = this;
	// If the first buffer was not entirely written then splice it.
	if ((size_t)written < len1)
	{
		std::memcpy(write_data->store, data1 + (size_t)written, len1 - (size_t)written);
		std::memcpy(write_data->store + (len1 - (size_t)written), data2, len2);
	}
	// Otherwise just take the pending data in the second buffer.
	else
	{
		std::memcpy(write_data->store, data2 + ((size_t)written - len1), len2 - ((size_t)written - len1));
	}
	write_data->req.data = (void*)write_data;

	uv_buf_t buffer = uv_buf_init((char*)write_data->store, pending_len);

	err = uv_write(&write_data->req, (uv_stream_t*)this->uvHandle, &buffer, 1, (uv_write_cb)on_write);
	if (err)
		MS_ABORT("uv_write() failed: %s", uv_strerror(err));
}

bool TCPConnection::SetPeerAddress()
{
	MS_TRACE();

	int err;
	int len = sizeof(this->peerAddr);

	err = uv_tcp_getpeername(this->uvHandle, (struct sockaddr*)&this->peerAddr, &len);
	if (err)
	{
		MS_ERROR("uv_tcp_getpeername() failed: %s", uv_strerror(err));

		return false;
	}

	int family;
	Utils::IP::GetAddressInfo((const struct sockaddr*)&this->peerAddr, &family, this->peerIP, &this->peerPort);

	return true;
}

inline
void TCPConnection::onUvReadAlloc(size_t suggested_size, uv_buf_t* buf)
{
	MS_TRACE();

	// If this is the first call to onUvReadAlloc() then allocate the receiving buffer now.
	if (!this->buffer)
		this->buffer = new uint8_t[this->bufferSize];

	// Tell UV to write after the last data byte in the buffer.
	buf->base = (char *)(this->buffer + this->bufferDataLen);
	// Give UV all the remaining space in the buffer.
	if (this->bufferSize > this->bufferDataLen)
	{
		buf->len = this->bufferSize - this->bufferDataLen;
	}
	else
	{
		buf->len = 0;

		MS_WARN("no available space in the buffer");
	}
}

inline
void TCPConnection::onUvRead(ssize_t nread, const uv_buf_t* buf)
{
	MS_TRACE();

	if (this->isClosing)
		return;

	if (nread == 0)
		return;

	// Data received.
	if (nread > 0)
	{
		// Update the buffer data length.
		this->bufferDataLen += (size_t)nread;

		// Notify the subclass.
		userOnTCPConnectionRead();
	}
	// Client disconneted.
	else if (nread == UV_EOF || nread == UV_ECONNRESET)
	{
		MS_DEBUG("connection closed by peer, closing server side");

		this->isClosedByPeer = true;

		// Close server side of the connection.
		Close();
	}
	// Some error.
	else
	{
		MS_DEBUG("read error, closing the connection: %s", uv_strerror(nread));

		this->hasError = true;

		// Close server side of the connection.
		Close();
	}
}

inline
void TCPConnection::onUvWriteError(int error)
{
	MS_TRACE();

	if (this->isClosing)
		return;

	if (error == UV_EPIPE || error == UV_ENOTCONN)
	{
		MS_DEBUG("write error, closing the connection: %s", uv_strerror(error));
	}
	else
	{
		MS_DEBUG("write error, closing the connection: %s", uv_strerror(error));

		this->hasError = true;
	}

	Close();
}

inline
void TCPConnection::onUvShutdown(uv_shutdown_t* req, int status)
{
	MS_TRACE();

	delete req;

	if (status == UV_EPIPE || status == UV_ENOTCONN || status == UV_ECANCELED)
		MS_DEBUG("shutdown error: %s", uv_strerror(status));
	else if (status)
		MS_DEBUG("shutdown error: %s", uv_strerror(status));

	// Now do close the handle.
	uv_close((uv_handle_t*)this->uvHandle, (uv_close_cb)on_close);
}

inline
void TCPConnection::onUvClosed()
{
	MS_TRACE();

	// Notify the listener.
	this->listener->onTCPConnectionClosed(this, this->isClosedByPeer);

	// And delete this.
	delete this;
}
