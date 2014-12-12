#define MS_CLASS "Utils::Socket"

#include "Utils.h"
#include "MediaSoupError.h"
#include "Logger.h"
#include <cerrno>
#include <fcntl.h>


namespace Utils {


void Socket::BuildSocketPair(int family, int type, int* fds) {
	MS_TRACE();

	// Old Linux/BSD kernels and OSX don't support setting SOCK_NONBLOCK and/or
	// SOCK_CLOEXEC during socket creation. In those cases use fcntl().

	#if defined(SOCK_NONBLOCK)
		type |= SOCK_NONBLOCK;
	#endif

	#if defined(SOCK_CLOEXEC)
		type |= SOCK_CLOEXEC;
	#endif

	// Create a pair of connected sockets.
	if (socketpair(family, type, 0, fds))
		MS_THROW_ERROR("socketpair() failed: %s", std::strerror(errno));

	#if !defined(SOCK_NONBLOCK)
		if (! (SetNonBlock(fds[0]) && SetNonBlock(fds[1])))
			MS_THROW_ERROR("error setting sockets as non-blocking");
	#endif

	#if !defined(SOCK_CLOEXEC)
		if (! (SetCloExec(fds[0]) && SetCloExec(fds[1])))
			MS_THROW_ERROR("error setting sockets as close-exec");
	#endif
}


bool Socket::SetNonBlock(int fd) {
	MS_TRACE();

	int flags;
	int ret;

	flags = fcntl(fd, F_GETFL);
	if (flags == -1) {
		MS_ERROR("fcntl() failed when getting flags");
		return false;
	}

	ret = fcntl(fd, F_SETFL, flags | O_NONBLOCK);
	if (ret == -1) {
		MS_ERROR("fcntl() failed when setting O_NONBLOCK: %s", std::strerror(errno));
		return false;
	}

	return true;
}


bool Socket::SetCloExec(int fd) {
	MS_TRACE();

	int flags;
	int ret;

	flags = fcntl(fd, F_GETFD);
	if (flags == -1) {
		MS_ERROR("fcntl() failed when getting flags");
		return false;
	}

	ret = fcntl(fd, F_SETFD, flags | FD_CLOEXEC);
	if (ret == -1) {
		MS_ERROR("fcntl() failed when setting FD_CLOEXEC: %s", std::strerror(errno));
		return false;
	}

	return true;
}


}  // namespace Utils
