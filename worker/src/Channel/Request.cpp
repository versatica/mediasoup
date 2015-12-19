#define MS_CLASS "Channel::Request"

#include "Channel/Request.h"
#include "Logger.h"

namespace Channel
{
	/* Static methods. */

	Channel::Request* Request::Factory(Channel::UnixStreamSocket* channel, Json::Value& json)
	{
		MS_TRACE();

		unsigned int id;
		std::string method;
		Json::Value data;

		// TODO: TMP
		Json::FastWriter fastWriter;
		fastWriter.dropNullPlaceholders();
		fastWriter.omitEndingLineFeed();
		MS_DEBUG("JSON msg: %s", fastWriter.write(json).c_str());

		// Check fields.

		if (json["id"].isUInt())
		{
			id = json["id"].asUInt();
		}
		else
		{
			MS_ERROR("json has no numeric .id field");

			return nullptr;
		}

		if (json["method"].isString())
		{
			method = json["method"].asString();
		}
		else
		{
			MS_ERROR("json has no string .method field");

			return nullptr;
		}

		if (json["data"].isObject())
			data = json["data"];
		else
			data = Json::Value(Json::objectValue);

		return new Request(channel, id, method, data);
	}

	/* Instance methods. */

	Request::Request(Channel::UnixStreamSocket* channel, unsigned int id, std::string& method, Json::Value& data) :
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

	void Request::Accept()
	{
		MS_TRACE();

		static Json::Value no_data(Json::nullValue);

		Accept(no_data);
	}

	void Request::Accept(Json::Value &data)
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
		json["status"] = 200;

		if (data.isObject())
			json["data"] = data;

		this->channel->Send(json);
	}

	void Request::Reject(unsigned int status, std::string& reason)
	{
		MS_TRACE();

		Reject(status, reason.c_str());
	}

	void Request::Reject(unsigned int status, const char* reason)
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

		if (reason)
			json["reason"] = reason;

		this->channel->Send(json);
	}
}
