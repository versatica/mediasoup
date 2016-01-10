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
		MS_BYTE           store[1];
	};

public:
	UnixStreamSocket(int fd, size_t bufferSize);
	virtual ~UnixStreamSocket();

	void Write(const MS_BYTE* data, size_t len);
	void Write(const std::string &data);
	void Close();
	bool IsClosing();
	virtual void Dump();

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
	// Allocated by this.
	MS_BYTE* buffer = nullptr;
	// Passed by argument.
	size_t bufferSize = 0;
	// Others.
	size_t bufferDataLen = 0;
};

/* Inline methods. */

inline
void UnixStreamSocket::Write(const std::string &data)
{
	Write((const MS_BYTE*)data.c_str(), data.size());
}

inline
bool UnixStreamSocket::IsClosing()
{
	return this->isClosing;
}


#endif
