#define MS_CLASS "Channel::Request"

#include "Channel/Request.h"
#include "Channel/UnixStreamSocket.h"
#include "Logger.h"

namespace Channel
{
	/* Static methods. */

	Channel::Request* Request::Factory(Json::Value& msg)
	{
		MS_TRACE();

		// TODO: TMP
		Json::FastWriter fastWriter;
		fastWriter.dropNullPlaceholders();
		fastWriter.omitEndingLineFeed();
		MS_DEBUG("JSON msg: %s", fastWriter.write(msg).c_str());

		// TODO: Check keys.

		std::string id = msg["id"].asString();
		std::string method = msg["method"].asString();

		return new Request(id, method, msg["data"]);
	}

	/* Instance methods. */

	Request::Request(std::string& id, std::string& method, Json::Value& data) :
		id(id),
		method(method),
		data(data)
	{
		MS_TRACE();
	}

	Request::~Request()
	{
		MS_TRACE();
	}

	void Request::Reply(int status)
	{
		MS_TRACE();

		Json::Value json;

		json["id"] = this->id;
		json["status"] = status;

		Channel::channel->Send(json);
	}
}
