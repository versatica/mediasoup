#define MS_CLASS "Channel::Request"
// #define MS_LOG_DEV

#include "Channel/Request.hpp"
#include "Logger.hpp"
#include "MediaSoupError.hpp"

namespace Channel
{
	/* Class variables. */

	// clang-format off
	std::unordered_map<std::string, Request::MethodId> Request::string2MethodId =
	{
		{ "worker.dump",                       Request::MethodId::WORKER_DUMP                          },
		{ "worker.updateSettings",             Request::MethodId::WORKER_UPDATE_SETTINGS               },
		{ "worker.createRouter",               Request::MethodId::WORKER_CREATE_ROUTER                 },
		{ "router.dump",                       Request::MethodId::ROUTER_DUMP                          },
		{ "router.createWebRtcTransport",      Request::MethodId::ROUTER_CREATE_WEBRTC_TRANSPORT       },
		{ "router.createPlainRtpTransport",    Request::MethodId::ROUTER_CREATE_PLAIN_RTP_TRANSPORT    },
		{ "transport.close",                   Request::MethodId::TRANSPORT_CLOSE                      },
		{ "transport.dump",                    Request::MethodId::TRANSPORT_DUMP                       },
		{ "transport.getStats",                Request::MethodId::TRANSPORT_GET_STATS                  },
		{ "transport.connect",                 Request::MethodId::TRANSPORT_CONNECT                    },
		{ "transport.setMaxIncomingBitrate",   Request::MethodId::TRANSPORT_SET_MAX_INCOMING_BITRATE    },
		{ "transport.restartIce",              Request::MethodId::TRANSPORT_RESTART_ICE                },
		{ "transport.produce",                 Request::MethodId::TRANSPORT_PRODUCE                    },
		{ "transport.consume",                 Request::MethodId::TRANSPORT_CONSUME                    },
		{ "producer.close",                    Request::MethodId::PRODUCER_CLOSE                       },
		{ "producer.dump",                     Request::MethodId::PRODUCER_DUMP                        },
		{ "producer.getStats",                 Request::MethodId::PRODUCER_GET_STATS                   },
		{ "producer.pause",                    Request::MethodId::PRODUCER_PAUSE                       },
		{ "producer.resume" ,                  Request::MethodId::PRODUCER_RESUME                      },
		{ "consumer.close",                    Request::MethodId::CONSUMER_CLOSE                       },
		{ "consumer.dump",                     Request::MethodId::CONSUMER_DUMP                        },
		{ "consumer.getStats",                 Request::MethodId::CONSUMER_GET_STATS                   },
		{ "consumer.start",                    Request::MethodId::CONSUMER_START                       },
		{ "consumer.pause",                    Request::MethodId::CONSUMER_PAUSE                       },
		{ "consumer.resume",                   Request::MethodId::CONSUMER_RESUME                      },
		{ "consumer.setPreferredSpatialLayer", Request::MethodId::CONSUMER_SET_PREFERRED_SPATIAL_LAYER },
		{ "consumer.setEncodingPreferences",   Request::MethodId::CONSUMER_SET_ENCODING_PREFERENCES    },
		{ "consumer.requestKeyFrame",          Request::MethodId::CONSUMER_REQUEST_KEY_FRAME           }
	};
	// clang-format on

	/* Instance methods. */

	Request::Request(Channel::UnixStreamSocket* channel, json& body) : channel(channel)
	{
		MS_TRACE();

		auto jsonIdIt       = body.find("id");
		auto jsonMethodIt   = body.find("method");
		auto jsonInternalIt = body.find("internal");
		auto jsonDataIt     = body.find("data");

		if (jsonIdIt == body.end() || !jsonIdIt->is_number_unsigned())
			MS_THROW_ERROR("invalid id");

		this->id = jsonIdIt->get<uint32_t>();

		if (jsonMethodIt == body.end() || !jsonMethodIt->is_string())
			MS_THROW_ERROR("invalid method");

		this->method = jsonMethodIt->get<std::string>();

		auto methodIdIt = Request::string2MethodId.find(this->method);

		if (methodIdIt == Request::string2MethodId.end())
		{
			Reject("unknown method");

			MS_THROW_ERROR("unknown method '%s'", this->method.c_str());
		}

		this->methodId = methodIdIt->second;

		if (jsonInternalIt != body.end() && jsonInternalIt->is_object())
			this->internal = *jsonInternalIt;

		if (jsonDataIt != body.end() && jsonDataIt->is_object())
			this->data = *jsonDataIt;
	}

	Request::~Request()
	{
		MS_TRACE();
	}

	void Request::Accept()
	{
		MS_TRACE();

		static json noData(nullptr);

		Accept(noData);
	}

	void Request::Accept(json& data)
	{
		MS_TRACE();

		MS_ASSERT(!this->replied, "request already replied");

		this->replied = true;

		json body{ json::object() };

		body["id"]       = this->id;
		body["accepted"] = true;

		if (data.is_structured())
			body["data"] = data;

		this->channel->Send(body);
	}

	void Request::Reject(std::string& reason)
	{
		MS_TRACE();

		Reject(reason.c_str());
	}

	/**
	 * Reject the Request.
	 *
	 * @param reason  Description string.
	 */
	void Request::Reject(const char* reason)
	{
		MS_TRACE();

		MS_ASSERT(!this->replied, "request already replied");

		this->replied = true;

		json body{ json::object() };

		body["id"]       = this->id;
		body["rejected"] = true;

		if (reason != nullptr)
			body["reason"] = reason;

		this->channel->Send(body);
	}
} // namespace Channel
