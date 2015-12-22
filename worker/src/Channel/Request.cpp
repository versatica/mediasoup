#define MS_CLASS "Channel::Request"

#include "Channel/Request.h"
#include "Logger.h"

namespace Channel
{
	/* Static variables. */

	static Json::Value empty_data(Json::objectValue);

	/* Class variables. */

	std::unordered_map<std::string, Request::MethodId> Request::string2MethodId =
	{
		{ "updateSettings", Request::MethodId::updateSettings },
		{ "createRoom",     Request::MethodId::createRoom     },
		{ "createPeer",     Request::MethodId::createPeer     }
	};

	/* Class methods. */

	Channel::Request* Request::Factory(Channel::UnixStreamSocket* channel, Json::Value& json)
	{
		MS_TRACE();

		unsigned int id;
		std::string method;
		MethodId methodId;
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

		auto it = Request::string2MethodId.find(method);

		if (it != Request::string2MethodId.end())
		{
			methodId = it->second;
		}
		else
		{
			MS_ERROR("unknwon .method '%s'", method.c_str());

			return nullptr;
		}

		if (json["data"].isObject())
			data = json["data"];
		else
			data = Json::Value(Json::objectValue);

		return new Request(channel, id, method, methodId, data);
	}

	/* Instance methods. */

	Request::Request(Channel::UnixStreamSocket* channel, unsigned int id, std::string& method, MethodId methodId, Json::Value& data) :
		channel(channel),
		id(id),
		method(method),
		methodId(methodId),
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

		Accept(empty_data);
	}

	void Request::Accept(Json::Value &data)
	{
		MS_TRACE();

		MS_ASSERT(!this->replied, "Request already replied");
		this->replied = true;

		Json::Value json;

		json["id"] = this->id;
		json["status"] = 200;

		if (data.isObject())
			json["data"] = data;
		else
			json["data"] = empty_data;

		this->channel->Send(json);
	}

	void Request::Reject(unsigned int status, std::string& reason)
	{
		MS_TRACE();

		Reject(status, reason.c_str());
	}

	/**
	 * Reject the Request.
	 * @param status  4XX means normal rejection, 5XX means error.
	 * @param reason  Description string.
	 */
	void Request::Reject(unsigned int status, const char* reason)
	{
		MS_TRACE();

		MS_ASSERT(!this->replied, "Request already replied");
		this->replied = true;

		MS_ASSERT(status >= 400 && status <= 599, "status must be between 400 and 500");

		Json::Value json;

		json["id"] = this->id;
		json["status"] = status;

		if (reason)
			json["reason"] = reason;

		this->channel->Send(json);
	}
}
