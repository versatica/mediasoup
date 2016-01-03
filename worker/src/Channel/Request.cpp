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
		{ "worker.dump",                    Request::MethodId::worker_dump                    },
		{ "worker.updateSettings",          Request::MethodId::worker_updateSettings          },
		{ "worker.createRoom",              Request::MethodId::worker_createRoom              },
		{ "room.close",                     Request::MethodId::room_close                     },
		{ "room.dump",                      Request::MethodId::room_dump                      },
		{ "room.createPeer",                Request::MethodId::room_createPeer                },
		{ "peer.close",                     Request::MethodId::peer_close                     },
		{ "peer.dump",                      Request::MethodId::peer_dump                      },
		{ "peer.createTransport",           Request::MethodId::peer_createTransport           },
		{ "peer.createAssociatedTransport", Request::MethodId::peer_createAssociatedTransport },
		{ "transport.close",                Request::MethodId::transport_close                },
		{ "transport.dump",                 Request::MethodId::transport_dump                 },
		{ "transport.start",                Request::MethodId::transport_start                }
	};

	/* Instance methods. */

	Request::Request(Channel::UnixStreamSocket* channel, Json::Value& json) :
		channel(channel)
	{
		MS_TRACE();

		static const Json::StaticString k_id("id");
		static const Json::StaticString k_method("method");
		static const Json::StaticString k_data("data");

		if (json[k_id].isUInt())
			this->id = json[k_id].asUInt();
		else
			MS_THROW_ERROR("json has no numeric .id field");

		if (json[k_method].isString())
			this->method = json[k_method].asString();
		else
			MS_THROW_ERROR("json has no string .method field");

		auto it = Request::string2MethodId.find(this->method);

		if (it != Request::string2MethodId.end())
		{
			this->methodId = it->second;
		}
		else
		{
			Reject(405, "method not allowed");

			MS_THROW_ERROR("unknown .method '%s'", this->method.c_str());
		}

		if (json[k_data].isObject())
			this->data = json[k_data];
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

		static const Json::StaticString k_id("id");
		static const Json::StaticString k_status("status");
		static const Json::StaticString k_data("data");

		Json::Value json;

		json[k_id] = this->id;
		json[k_status] = 200;

		if (data.isObject())
			json[k_data] = data;
		else
			json[k_data] = empty_data;

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

		static const Json::StaticString k_id("id");
		static const Json::StaticString k_status("status");
		static const Json::StaticString k_reason("reason");

		Json::Value json;

		json[k_id] = this->id;
		json[k_status] = status;

		if (reason)
			json[k_reason] = reason;

		this->channel->Send(json);
	}
}
