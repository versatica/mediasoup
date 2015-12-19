#define MS_CLASS "Channel::Request"

#include "Channel/Request.h"
#include "Logger.h"

namespace Channel
{
	/* Static methods. */

	Channel::Request* Request::Factory(Channel::UnixStreamSocket* channel, Json::Value& msg)
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

		return new Request(channel, id, method, msg["data"]);
	}

	/* Instance methods. */

	Request::Request(Channel::UnixStreamSocket* channel, std::string& id, std::string& method, Json::Value& data) :
		channel(channel),
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

		if (this->replied)
		{
			MS_ERROR("already replied");

			return;
		}
		this->replied = true;

		Json::Value json;

		json["id"] = this->id;
		json["status"] = status;

		this->channel->Send(json);
	}
}
