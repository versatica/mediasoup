#ifndef MS_UNIX_STREAM_SOCKET_H
#define MS_UNIX_STREAM_SOCKET_H

#include "common.h"
#include <string>
#include <uv.h>

class UnixStreamSocket
{
public:
	/* Struct for the data field of uv_req_t when writing data. */
	struct UvWriteData
	{
		UnixStreamSocket* socket;
		uv_write_t        req;
		uint8_t           store[1];
	};

public:
	UnixStreamSocket(int fd, size_t bufferSize);
	virtual ~UnixStreamSocket();

	void Close();
	bool IsClosing();
	void Write(const uint8_t* data, size_t len);
	void Write(const std::string &data);

/* Callbacks fired by UV events. */
public:
	void onUvReadAlloc(size_t suggested_size, uv_buf_t* buf);
	void onUvRead(ssize_t nread, const uv_buf_t* buf);
	void onUvWriteError(int error);
	void onUvShutdown(uv_shutdown_t* req, int status);
	void onUvClosed();

/* Pure virtual methods that must be implemented by the subclass. */
protected:
	virtual void userOnUnixStreamRead() = 0;
	virtual void userOnUnixStreamSocketClosed(bool is_closed_by_peer) = 0;

private:
	// Allocated by this.
	uv_pipe_t* uvHandle = nullptr;
	// Others.
	bool isClosing = false;
	bool isClosedByPeer = false;
	bool hasError = false;

protected:
	// Passed by argument.
	size_t bufferSize = 0;
	// Allocated by this.
	uint8_t* buffer = nullptr;
	// Others.
	size_t bufferDataLen = 0;
};

/* Inline methods. */

inline
bool UnixStreamSocket::IsClosing()
{
	return this->isClosing;
}

inline
void UnixStreamSocket::Write(const std::string &data)
{
	Write((const uint8_t*)data.c_str(), data.size());
}

#endif
