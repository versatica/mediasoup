#define MS_CLASS "Channel::UnixStreamSocket"

#include "Channel/UnixStreamSocket.h"
#include "Logger.h"
#include <json/json.h>
#include <netstring.h>

#define MESSAGE_MAX_SIZE 65536  // TODO: set proper buffer size

namespace Channel
{
	/* Instance methods. */

	UnixStreamSocket::UnixStreamSocket(Listener* listener, int fd) :
		::UnixStreamSocket::UnixStreamSocket(fd, MESSAGE_MAX_SIZE),
		listener(listener)
	{
		MS_TRACE();
	}

	UnixStreamSocket::~UnixStreamSocket()
	{
		MS_TRACE();
	}

	void UnixStreamSocket::Reset()
	{
		MS_TRACE();

		// reset parsing stuff, dataLen, etc
	}

	void UnixStreamSocket::userOnUnixStreamRead(const MS_BYTE* data, size_t len)
	{
		MS_TRACE();

		std::string kk((const char*)data, len);

		MS_WARN("---------- readed data: %s", kk.c_str());

		this->bufferDataLen = 0;

		char *ns_start = nullptr;
		size_t ns_length;
		int ns_ret = netstring_read((char *)data, len, &ns_start, &ns_length);

		if (ns_ret != 0)
		{
			switch (ns_ret)
			{
				case NETSTRING_ERROR_TOO_SHORT:
					MS_DEBUG("received netstring is too short, waiting for more data");
					return;
				case NETSTRING_ERROR_TOO_LONG:
					MS_ERROR("NETSTRING_ERROR_TOO_LONG");
					break;
				case NETSTRING_ERROR_NO_COLON:
					MS_ERROR("NETSTRING_ERROR_NO_COLON");
					break;
				case NETSTRING_ERROR_NO_COMMA:
					MS_ERROR("NETSTRING_ERROR_NO_COMMA");
					break;
				case NETSTRING_ERROR_LEADING_ZERO:
					MS_ERROR("NETSTRING_ERROR_LEADING_ZERO");
					break;
				case NETSTRING_ERROR_NO_LENGTH:
					MS_ERROR("NETSTRING_ERROR_NO_LENGTH");
					break;
			}

			Reset();
			return;
		}




		Json::Value json;
		Json::Reader reader;

		if (reader.parse((const char*)ns_start, (const char*)ns_start + ns_length, json))
		{
			MS_INFO("jsoncpp: IT IS VALID JSON:\n\n%s\n", json.toStyledString().c_str());

			// MS_DEBUG("jsoncpp: method      : %s", json["method"].asCString());
			// MS_DEBUG("jsoncpp: room.id     : %d", json["room"]["id"].asInt());
			// MS_DEBUG("jsoncpp: room.closed : %s", json["room"]["closed"].asBool() ? "true" : "false");

			// if (json["method"].isString())
			// 	MS_DEBUG("jsoncpp: method is a string");

			// if (!json.isMember("kk"))
			// 	MS_DEBUG("jsoncpp: kk is not member");

			// if (json["parent"]["foo"]["bar"].isNull())
			// 	MS_DEBUG("jsoncpp: parent.foo.bar is null");
		}
		else
		{
			MS_ERROR("jsoncpp: INVALID JSON !!!!");
		}
	}

	void UnixStreamSocket::userOnUnixStreamSocketClosed(bool is_closed_by_peer)
	{
		MS_TRACE();

		// Notify the listener.
		// this->listener->onChannelUnixStreamSocketClosed(this, is_closed_by_peer);
	}
}
