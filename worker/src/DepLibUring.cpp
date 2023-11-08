#define MS_CLASS "DepLibUring"

// #define MS_LOG_DEV_LEVEL 3

#include "DepLibUring.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include <sys/eventfd.h>

/* Static variables. */

/* liburing instance per thread. */
thread_local DepLibUring* DepLibUring::liburing{ nullptr };
/* Completion queue entry array used to retrieve processes tasks. */
thread_local struct io_uring_cqe* cqes[DepLibUring::QueueDepth];

/* Static method for UV callback. */

inline static void onFdEvent(uv_poll_t* handle, int status, int events)
{
	auto* liburing = static_cast<DepLibUring*>(handle->data);

	auto count = io_uring_peek_batch_cqe(liburing->GetRing(), cqes, DepLibUring::QueueDepth);

	// libuv uses level triggering, so we need to read from the socket to reset
	// the counter in order to avoid libuv calling this callback indefinitely.
	eventfd_t v;
	int error = eventfd_read(liburing->GetEventFd(), &v);
	if (error < 0)
	{
		MS_ERROR("eventfd_read() failed: %s", std::strerror(-error));
	};

	for (unsigned int i = 0; i < count; ++i)
	{
		struct io_uring_cqe* cqe = cqes[i];
		auto* userData           = static_cast<DepLibUring::UserData*>(io_uring_cqe_get_data(cqe));

		if (cqe->res < 0)
		{
			MS_ERROR("sending failed: %s", std::strerror(-cqe->res));

			if (userData->cb)
			{
				(*userData->cb)(false);
				delete userData->cb;
			}
		}
		else
		{
			if (userData->cb)
			{
				(*userData->cb)(true);
				delete userData->cb;
			}
		}

		io_uring_cqe_seen(liburing->GetRing(), cqe);
		liburing->ReleaseUserDataEntry(userData->idx);
	}
}

void DepLibUring::ClassInit()
{
	const auto mayor = io_uring_major_version();
	const auto minor = io_uring_minor_version();

	MS_DEBUG_TAG(info, "liburing version: \"%i.%i\"", mayor, minor);

	DepLibUring::liburing = new DepLibUring();
}

void DepLibUring::ClassDestroy()
{
	MS_TRACE();

	delete DepLibUring::liburing;
}

DepLibUring::DepLibUring()
{
	MS_TRACE();

	auto error = io_uring_queue_init(DepLibUring::QueueDepth, &this->ring, 0);

	if (error < 0)
	{
		MS_THROW_ERROR("io_uring_queue_init() failed: %s", std::strerror(-error));
	}

	/* Create an eventfd instance */
	this->efd = eventfd(0, 0);

	if (this->efd < 0)
	{
		MS_THROW_ERROR("eventfd() failed: %s", std::strerror(-efd));
	}

	error = io_uring_register_eventfd(&this->ring, this->efd);

	if (error < 0)
	{
		MS_THROW_ERROR("io_uring_register_eventfd() failed: %s", std::strerror(-error));
	}

	// Watch the event file descriptor.
	this->uvHandle = new uv_poll_t;

	error = uv_poll_init(DepLibUV::GetLoop(), uvHandle, efd);

	if (error != 0)
	{
		delete uvHandle;

		MS_THROW_ERROR("uv_poll_init() failed: %s", uv_strerror(error));
	}

	uvHandle->data = this;

	error = uv_poll_start(uvHandle, UV_READABLE, static_cast<uv_poll_cb>(onFdEvent));

	if (error != 0)
	{
		MS_THROW_ERROR("uv_poll_start() failed: %s", uv_strerror(error));
	}

	// Initialize available UserData entries.
	for (size_t i = 0; i < DepLibUring::QueueDepth; ++i)
	{
		this->availableUserDataEntries.push(i);
	}
}

DepLibUring::~DepLibUring()
{
	MS_TRACE();

	// Stop polling the event file descriptor.
	uv_poll_stop(this->uvHandle);

	delete this->uvHandle;

	// Close the event file descriptor.
	close(this->efd);

	// Close the ring.
	io_uring_queue_exit(&this->ring);
}

bool DepLibUring::PrepareSend(
  int sockfd, const void* data, size_t len, const struct sockaddr* addr, onSendCallback* cb)
{
	MS_TRACE();

	auto* userData = this->GetUserData();

	if (!userData)
	{
		MS_WARN_DEV("no user data entry available");

		return false;
	}

	auto* sqe = io_uring_get_sqe(&this->ring);

	if (!sqe)
	{
		MS_WARN_DEV("no sqe available");

		return false;
	}

	std::memcpy(userData->store, data, len);
	userData->cb = cb;

	io_uring_sqe_set_data(sqe, userData);

	socklen_t addrlen = 0;

	if (addr->sa_family == AF_INET)
	{
		addrlen = sizeof(struct sockaddr_in);
	}
	else if (addr->sa_family == AF_INET6)
	{
		addrlen = sizeof(struct sockaddr_in6);
	}

	io_uring_prep_sendto(sqe, sockfd, userData->store, len, 0, addr, addrlen);

	return true;
}

bool DepLibUring::PrepareWrite(
  int sockfd, const void* data1, size_t len1, const void* data2, size_t len2, onSendCallback* cb)
{
	MS_TRACE();

	auto* userData = this->GetUserData();

	if (!userData)
	{
		MS_WARN_DEV("no user data entry available");

		return false;
	}

	auto* sqe = io_uring_get_sqe(&this->ring);

	if (!sqe)
	{
		MS_WARN_DEV("no sqe available");

		return false;
	}

	std::memcpy(userData->store, data1, len1);
	std::memcpy(userData->store + len1, data2, len2);
	userData->cb = cb;

	io_uring_sqe_set_data(sqe, userData);
	io_uring_prep_write(sqe, sockfd, userData->store, len1 + len2, 0);

	return true;
}

void DepLibUring::Submit()
{
	MS_TRACE();

	// Unset active flag.
	this->active = false;

	auto error = io_uring_submit(&this->ring);

	if (error >= 0)
	{
		MS_DEBUG_DEV("%i submission queue entries submitted", ret);
	}
	else
	{
		MS_ERROR("io_uring_submit() failed: %s", std::strerror(-error));
	}
}

DepLibUring::UserData* DepLibUring::GetUserData()
{
	MS_TRACE();

	if (this->availableUserDataEntries.empty())
	{
		MS_WARN_DEV("no user data entry available");

		return nullptr;
	}

	auto idx = this->availableUserDataEntries.front();

	this->availableUserDataEntries.pop();

	auto* userData = &this->userDataBuffer[idx];
	userData->idx  = idx;

	return userData;
}
