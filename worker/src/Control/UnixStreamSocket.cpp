#define MS_CLASS "Control::UnixStreamSocket"

#include "Control/UnixStreamSocket.h"
#include "Logger.h"
#include <json/json.h>
#include <picojson.h>

#define MESSAGE_MAX_SIZE 65536  // TODO: set proper buffer size

namespace Control
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

	void UnixStreamSocket::userOnUnixStreamRead(const MS_BYTE* data, size_t len)
	{
		MS_TRACE();

		std::string kk((const char*)data, len);

		MS_WARN("---------- readed data: %s", kk.c_str());

		this->bufferDataLen = 0;


		Json::Value json;
		Json::Reader reader;

		if (reader.parse((const char*)data, (const char*)data + len, json))
		{
			MS_DEBUG("jsoncpp: IT IS VALID JSON:\n%s", json.toStyledString().c_str());

			MS_DEBUG("jsoncpp: method      : %s", json["method"].asCString());
			MS_DEBUG("jsoncpp: room.id     : %d", json["room"]["id"].asInt());
			MS_DEBUG("jsoncpp: room.closed : %s", json["room"]["closed"].asBool() ? "true" : "false");

			if (json["method"].isString())
				MS_DEBUG("jsoncpp: method is a string");

			if (!json.isMember("kk"))
				MS_DEBUG("jsoncpp: kk is not member");

			if (json["parent"]["foo"]["bar"].isNull())
				MS_DEBUG("jsoncpp: parent.foo.bar is null");
		}
		else
		{
			MS_ERROR("jsoncpp: INVALID JSON !!!!");
		}


		picojson::value v;
		std::string err;
		picojson::parse(v, (const char*)data, (const char*)data + len, &err);

		if (!err.empty()) {
		  MS_ERROR("picojson: INVALID JSON: %s", err.c_str());
		}

		MS_DEBUG("picojson: IT IS VALID JSON:\n%s", v.serialize().c_str());

		MS_DEBUG("picojson: method      : %s", v.get("method").get<std::string>().c_str());
		MS_DEBUG("picojson: room.id     : %d", (int)v.get("room").get("id").get<double>());
		MS_DEBUG("picojson: room.closed : %s", v.get("room").get("closed").get<bool>() ? "true" : "false");

		if (v.get("method").is<std::string>())
			MS_DEBUG("picojson: method is a string");

		if (v.get("kk").is<picojson::null>())
			MS_DEBUG("picojson: kk is null");

		// ESTO CASCA: ganador: jsoncpp
		if (v.get("parent").get("foo").get("bar").is<picojson::null>())
			MS_DEBUG("picojson: parent.foo.bar is null");
	}

	void UnixStreamSocket::userOnUnixStreamSocketClosed(bool is_closed_by_peer)
	{
		MS_TRACE();

		// Notify the listener.
		// this->listener->onControlUnixStreamSocketClosed(this, is_closed_by_peer);
	}
}  // namespace ControlProtocol
