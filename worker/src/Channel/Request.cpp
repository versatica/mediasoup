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
		{ "worker.createRouter",               Request::MethodId::WORKER_CREATE_ROTER                  },
		{ "router.close",                      Request::MethodId::ROUTER_CLOSE                         },
		{ "router.dump",                       Request::MethodId::ROUTER_DUMP                          },
		{ "router.createTransport",            Request::MethodId::ROUTER_CREATE_TRANSPORT              },
		{ "router.createProducer",             Request::MethodId::ROUTER_CREATE_PRODUCER               },
		{ "router.createConsumer",             Request::MethodId::ROUTER_CREATE_CONSUMER               },
		{ "router.setAudioLevelsEvent",        Request::MethodId::ROUTER_SET_AUDIO_LEVELS_EVENT        },
		{ "transport.close",                   Request::MethodId::TRANSPORT_CLOSE                      },
		{ "transport.dump",                    Request::MethodId::TRANSPORT_DUMP                       },
		{ "transport.setRemoteDtlsParameters", Request::MethodId::TRANSPORT_SET_REMOTE_DTLS_PARAMETERS },
		{ "transport.setMaxBitrate",           Request::MethodId::TRANSPORT_SET_MAX_BITRATE            },
		{ "transport.changeUfragPwd",          Request::MethodId::TRANSPORT_CHANGE_UFRAG_PWD           },
		{ "producer.close",                    Request::MethodId::PRODUCER_CLOSE                       },
		{ "producer.dump",                     Request::MethodId::PRODUCER_DUMP                        },
		{ "producer.receive",                  Request::MethodId::PRODUCER_RECEIVE                     },
		{ "producer.pause",                    Request::MethodId::PRODUCER_PAUSE                       },
		{ "producer.resume" ,                  Request::MethodId::PRODUCER_RESUME                      },
		{ "producer.setRtpRawEvent",           Request::MethodId::PRODUCER_SET_RTP_RAW_EVENT           },
		{ "producer.setRtpObjectEvent",        Request::MethodId::PRODUCER_SET_RTP_OBJECT_EVENT        },
		{ "consumer.close",                    Request::MethodId::CONSUMER_CLOSE                       },
		{ "consumer.dump",                     Request::MethodId::CONSUMER_DUMP                        },
		{ "consumer.enable",                   Request::MethodId::CONSUMER_ENABLE                      },
		{ "consumer.pause",                    Request::MethodId::CONSUMER_PAUSE                       },
		{ "consumer.resume",                   Request::MethodId::CONSUMER_RESUME                      }
	};
	// clang-format on

	/* Instance methods. */

	Request::Request(Channel::UnixStreamSocket* channel, Json::Value& json) : channel(channel)
	{
		MS_TRACE();

		static const Json::StaticString JsonStringId{ "id" };
		static const Json::StaticString JsonStringMethod{ "method" };
		static const Json::StaticString JsonStringInternal{ "internal" };
		static const Json::StaticString JsonStringData{ "data" };

		if (json[JsonStringId].isUInt())
			this->id = json[JsonStringId].asUInt();
		else
			MS_THROW_ERROR("json has no numeric id field");

		if (json[JsonStringMethod].isString())
			this->method = json[JsonStringMethod].asString();
		else
			MS_THROW_ERROR("json has no string .method field");

		auto it = Request::string2MethodId.find(this->method);

		if (it != Request::string2MethodId.end())
		{
			this->methodId = it->second;
		}
		else
		{
			Reject("unknown method");

			MS_THROW_ERROR("unknown request.method '%s'", this->method.c_str());
		}

		if (json[JsonStringInternal].isObject())
			this->internal = json[JsonStringInternal];
		else
			this->internal = Json::Value(Json::objectValue);

		if (json[JsonStringData].isObject())
			this->data = json[JsonStringData];
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

		static Json::Value emptyData(Json::objectValue);

		Accept(emptyData);
	}

	void Request::Accept(Json::Value& data)
	{
		MS_TRACE();

		static Json::Value emptyData(Json::objectValue);
		static const Json::StaticString JsonStringId{ "id" };
		static const Json::StaticString JsonStringAccepted{ "accepted" };
		static const Json::StaticString JsonStringData{ "data" };

		MS_ASSERT(!this->replied, "Request already replied");

		this->replied = true;

		Json::Value json(Json::objectValue);

		json[JsonStringId]       = Json::UInt{ this->id };
		json[JsonStringAccepted] = true;

		if (data.isObject())
			json[JsonStringData] = data;
		else
			json[JsonStringData] = emptyData;

		this->channel->Send(json);
	}

	void Request::Reject(std::string& reason)
	{
		MS_TRACE();

		Reject(reason.c_str());
	}

	/**
	 * Reject the Request.
	 * @param reason  Description string.
	 */
	void Request::Reject(const char* reason)
	{
		MS_TRACE();

		static const Json::StaticString JsonStringId{ "id" };
		static const Json::StaticString JsonStringRejected{ "rejected" };
		static const Json::StaticString JsonStringReason{ "reason" };

		MS_ASSERT(!this->replied, "Request already replied");

		this->replied = true;

		Json::Value json(Json::objectValue);

		json[JsonStringId]       = Json::UInt{ this->id };
		json[JsonStringRejected] = true;

		if (reason != nullptr)
			json[JsonStringReason] = reason;

		this->channel->Send(json);
	}
} // namespace Channel
