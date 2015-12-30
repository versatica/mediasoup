#define MS_CLASS "Channel::Request"

#include "Channel/Request.h"
#include "MediaSoupError.h"
#include "Logger.h"

namespace Channel
{
	/* Static variables. */

	static Json::Value empty_data(Json::objectValue);

	/* Class variables. */

	std::unordered_map<std::string, Request::MethodId> Request::string2MethodId =
	{
		{ "dumpWorker",                Request::MethodId::dumpWorker                },
		{ "updateSettings",            Request::MethodId::updateSettings            },
		{ "createRoom",                Request::MethodId::createRoom                },
		{ "closeRoom",                 Request::MethodId::closeRoom                 },
		{ "createPeer",                Request::MethodId::createPeer                },
		{ "closePeer",                 Request::MethodId::closePeer                 },
		{ "createTransport",           Request::MethodId::createTransport           },
		{ "createAssociatedTransport", Request::MethodId::createAssociatedTransport },
		{ "closeTransport",            Request::MethodId::closeTransport            }
	};

	/* Instance methods. */

	Request::Request(Channel::UnixStreamSocket* channel, Json::Value& json) :
		channel(channel)
	{
		MS_TRACE();

		if (json["id"].isUInt())
			this->id = json["id"].asUInt();
		else
			MS_THROW_ERROR("json has no numeric .id field");

		if (json["method"].isString())
			this->method = json["method"].asString();
		else
			MS_THROW_ERROR("json has no string .method field");

		auto it = Request::string2MethodId.find(method);

		if (it != Request::string2MethodId.end())
		{
			this->methodId = it->second;
		}
		else
		{
			Reject(405, "method not allowed");

			MS_THROW_ERROR("unknown .method '%s'", method.c_str());
		}

		if (json["data"].isObject())
			this->data = json["data"];
		else
			this->data = Json::Value(Json::objectValue);
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
