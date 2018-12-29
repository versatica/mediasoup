#define MS_CLASS "UdpSocket"
// #define MS_LOG_DEV

#include "handles/UdpSocket.hpp"
#include "DepLibUV.hpp"
#include "Logger.hpp"
#include "MediaSoupError.hpp"
#include "Utils.hpp"

/* Static. */

static constexpr size_t ReadBufferSize{ 65536 };
static uint8_t ReadBuffer[ReadBufferSize];

/* Static methods for UV callbacks. */

inline static void onAlloc(uv_handle_t* handle, size_t suggestedSize, uv_buf_t* buf)
{
	static_cast<UdpSocket*>(handle->data)->OnUvRecvAlloc(suggestedSize, buf);
}

inline static void onRecv(
  uv_udp_t* handle, ssize_t nread, const uv_buf_t* buf, const struct sockaddr* addr, unsigned int flags)
{
	static_cast<UdpSocket*>(handle->data)->OnUvRecv(nread, buf, addr, flags);
}

inline static void onSend(uv_udp_send_t* req, int status)
{
	auto* sendData    = static_cast<UdpSocket::UvSendData*>(req->data);
	UdpSocket* socket = sendData->socket;

	// Delete the UvSendData struct (which includes the uv_req_t and the store char[]).
	std::free(sendData);

	// Just notify the UdpSocket when error.
	if (status != 0)
		socket->OnUvSendError(status);
}

inline static void onClose(uv_handle_t* handle)
{
	delete handle;
}

/* Instance methods. */

UdpSocket::UdpSocket(const std::string& ip, uint16_t port)
{
	MS_TRACE();

	int err;
	int flags = 0;

	this->uvHandle       = new uv_udp_t;
	this->uvHandle->data = (void*)this;

	err = uv_udp_init(DepLibUV::GetLoop(), this->uvHandle);
	if (err != 0)
	{
		delete this->uvHandle;
		this->uvHandle = nullptr;

		MS_THROW_ERROR("uv_udp_init() failed: %s", uv_strerror(err));
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
			flags |= UV_UDP_IPV6ONLY;

			break;
		}

		default:
		{
			uv_close(reinterpret_cast<uv_handle_t*>(this->uvHandle), static_cast<uv_close_cb>(onClose));

			MS_THROW_ERROR("invalid binding IP '%s'", ip.c_str());

			break;
		}
	}

	err = uv_udp_bind(this->uvHandle, reinterpret_cast<const struct sockaddr*>(&bindAddr), flags);
	if (err != 0)
	{
		uv_close(reinterpret_cast<uv_handle_t*>(this->uvHandle), static_cast<uv_close_cb>(onClose));
		MS_THROW_ERROR("uv_udp_bind() failed: %s", uv_strerror(err));
	}

	err = uv_udp_recv_start(
	  this->uvHandle, static_cast<uv_alloc_cb>(onAlloc), static_cast<uv_udp_recv_cb>(onRecv));
	if (err != 0)
	{
		uv_close(reinterpret_cast<uv_handle_t*>(this->uvHandle), static_cast<uv_close_cb>(onClose));
		MS_THROW_ERROR("uv_udp_recv_start() failed: %s", uv_strerror(err));
	}

	// Set local address.
	if (!SetLocalAddress())
	{
		uv_close(reinterpret_cast<uv_handle_t*>(this->uvHandle), static_cast<uv_close_cb>(onClose));
		MS_THROW_ERROR("error setting local IP and port");
	}
}

UdpSocket::UdpSocket(uv_udp_t* uvHandle) : uvHandle(uvHandle)
{
	MS_TRACE();

	int err;

	this->uvHandle->data = (void*)this;

	err = uv_udp_recv_start(
	  this->uvHandle, static_cast<uv_alloc_cb>(onAlloc), static_cast<uv_udp_recv_cb>(onRecv));
	if (err != 0)
	{
		uv_close(reinterpret_cast<uv_handle_t*>(this->uvHandle), static_cast<uv_close_cb>(onClose));
		MS_THROW_ERROR("uv_udp_recv_start() failed: %s", uv_strerror(err));
	}

	// Set local address.
	if (!SetLocalAddress())
	{
		uv_close(reinterpret_cast<uv_handle_t*>(this->uvHandle), static_cast<uv_close_cb>(onClose));
		MS_THROW_ERROR("error setting local IP and port");
	}
}

UdpSocket::~UdpSocket()
{
	MS_TRACE();

	if (!this->closed)
		Close();
}

void UdpSocket::Close()
{
	MS_TRACE();

	if (this->closed)
		return;

	int err;

	this->closed = true;

	// Don't read more.
	err = uv_udp_recv_stop(this->uvHandle);
	if (err != 0)
		MS_ABORT("uv_udp_recv_stop() failed: %s", uv_strerror(err));

	uv_close(reinterpret_cast<uv_handle_t*>(this->uvHandle), static_cast<uv_close_cb>(onClose));
}

void UdpSocket::Dump() const
{
	MS_DEBUG_DEV("<UdpSocket>");
	MS_DEBUG_DEV(
	  "  [UDP, local:%s :%" PRIu16 ", status:%s]",
	  this->localIP.c_str(),
	  static_cast<uint16_t>(this->localPort),
	  (!this->closed) ? "open" : "closed");
	MS_DEBUG_DEV("</UdpSocket>");
}

void UdpSocket::Send(const uint8_t* data, size_t len, const struct sockaddr* addr)
{
	MS_TRACE();

	if (this->closed)
		return;

	if (len == 0)
		return;

	uv_buf_t buffer;
	int sent;
	int err;

	// First try uv_udp_try_send(). In case it can not directly send the datagram
	// then build a uv_req_t and use uv_udp_send().

	buffer = uv_buf_init(reinterpret_cast<char*>(const_cast<uint8_t*>(data)), len);
	sent   = uv_udp_try_send(this->uvHandle, &buffer, 1, addr);

	// Entire datagram was sent. Done.
	if (sent == static_cast<int>(len))
	{
		// Update sent bytes.
		this->sentBytes += sent;

		return;
	}
	if (sent >= 0)
	{
		MS_WARN_DEV("datagram truncated (just %d of %zu bytes were sent)", sent, len);

		// Update sent bytes.
		this->sentBytes += sent;

		return;
	}
	// Error,
	if (sent != UV_EAGAIN)
	{
		MS_WARN_DEV("uv_udp_try_send() failed: %s", uv_strerror(sent));

		return;
	}
	// Otherwise UV_EAGAIN was returned so cannot send data at first time. Use uv_udp_send().

	// MS_DEBUG_DEV("could not send the datagram at first time, using uv_udp_send() now");

	// Allocate a special UvSendData struct pointer.
	auto* sendData = static_cast<UvSendData*>(std::malloc(sizeof(UvSendData) + len));

	sendData->socket = this;
	std::memcpy(sendData->store, data, len);
	sendData->req.data = (void*)sendData;

	buffer = uv_buf_init(reinterpret_cast<char*>(sendData->store), len);

	err = uv_udp_send(
	  &sendData->req, this->uvHandle, &buffer, 1, addr, static_cast<uv_udp_send_cb>(onSend));
	if (err != 0)
	{
		// NOTE: uv_udp_send() returns error if a wrong INET family is given
		// (IPv6 destination on a IPv4 binded socket), so be ready.
		MS_WARN_DEV("uv_udp_send() failed: %s", uv_strerror(err));

		// Delete the UvSendData struct (which includes the uv_req_t and the store char[]).
		std::free(sendData);
	}
	else
	{
		// Update sent bytes.
		this->sentBytes += len;
	}
}

void UdpSocket::Send(const uint8_t* data, size_t len, const std::string& ip, uint16_t port)
{
	MS_TRACE();

	if (this->closed)
		return;

	int err;

	if (len == 0)
		return;

	struct sockaddr_storage addr;

	switch (Utils::IP::GetFamily(ip))
	{
		case AF_INET:
		{
			err = uv_ip4_addr(
			  ip.c_str(), static_cast<int>(port), reinterpret_cast<struct sockaddr_in*>(&addr));

			if (err != 0)
				MS_ABORT("uv_ipv4_addr() failed: %s", uv_strerror(err));

			break;
		}

		case AF_INET6:
		{
			err = uv_ip6_addr(
			  ip.c_str(), static_cast<int>(port), reinterpret_cast<struct sockaddr_in6*>(&addr));

			if (err != 0)
				MS_ABORT("uv_ipv6_addr() failed: %s", uv_strerror(err));

			break;
		}

		default:
		{
			MS_ERROR("invalid destination IP '%s'", ip.c_str());

			return;
		}
	}

	Send(data, len, reinterpret_cast<struct sockaddr*>(&addr));
}

bool UdpSocket::SetLocalAddress()
{
	MS_TRACE();

	int err;
	int len = sizeof(this->localAddr);

	err =
	  uv_udp_getsockname(this->uvHandle, reinterpret_cast<struct sockaddr*>(&this->localAddr), &len);
	if (err != 0)
	{
		MS_ERROR("uv_udp_getsockname() failed: %s", uv_strerror(err));

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

inline void UdpSocket::OnUvRecvAlloc(size_t /*suggestedSize*/, uv_buf_t* buf)
{
	MS_TRACE();

	// Tell UV to write into the static buffer.
	buf->base = reinterpret_cast<char*>(ReadBuffer);
	// Give UV all the buffer space.
	buf->len = ReadBufferSize;
}

inline void UdpSocket::OnUvRecv(
  ssize_t nread, const uv_buf_t* buf, const struct sockaddr* addr, unsigned int flags)
{
	MS_TRACE();

	if (this->closed)
		return;

	// NOTE: libuv calls twice to alloc & recv when a datagram is received, the
	// second one with nread = 0 and addr = NULL. Ignore it.
	if (nread == 0)
		return;

	// Check flags.
	if ((flags & UV_UDP_PARTIAL) != 0u)
	{
		MS_ERROR("received datagram was truncated due to insufficient buffer, ignoring it");

		return;
	}

	// Data received.
	if (nread > 0)
	{
		// Update received bytes.
		this->recvBytes += nread;

		// Notify the subclass.
		UserOnUdpDatagramRecv(reinterpret_cast<uint8_t*>(buf->base), nread, addr);
	}
	// Some error.
	else
	{
		MS_DEBUG_DEV("read error: %s", uv_strerror(nread));
	}
}

inline void UdpSocket::OnUvSendError(int /*error*/)
{
	MS_TRACE();

	if (this->closed)
		return;

	MS_DEBUG_DEV("send error: %s", uv_strerror(error));
}
