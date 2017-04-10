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
		{ "worker.dump",                       Request::MethodId::worker_dump                       },
		{ "worker.updateSettings",             Request::MethodId::worker_updateSettings             },
		{ "worker.createRoom",                 Request::MethodId::worker_createRoom                 },
		{ "room.close",                        Request::MethodId::room_close                        },
		{ "room.dump",                         Request::MethodId::room_dump                         },
		{ "room.createPeer",                   Request::MethodId::room_createPeer                   },
		{ "peer.close",                        Request::MethodId::peer_close                        },
		{ "peer.dump",                         Request::MethodId::peer_dump                         },
		{ "peer.setCapabilities",              Request::MethodId::peer_setCapabilities              },
		{ "peer.createTransport",              Request::MethodId::peer_createTransport              },
		{ "peer.createRtpReceiver",            Request::MethodId::peer_createRtpReceiver            },
		{ "transport.close",                   Request::MethodId::transport_close                   },
		{ "transport.dump",                    Request::MethodId::transport_dump                    },
		{ "transport.setRemoteDtlsParameters", Request::MethodId::transport_setRemoteDtlsParameters },
		{ "transport.setMaxBitrate",           Request::MethodId::transport_setMaxBitrate           },
		{ "rtpReceiver.close",                 Request::MethodId::rtpReceiver_close                 },
		{ "rtpReceiver.dump",                  Request::MethodId::rtpReceiver_dump                  },
		{ "rtpReceiver.receive",               Request::MethodId::rtpReceiver_receive               },
		{ "rtpReceiver.setRtpRawEvent",        Request::MethodId::rtpReceiver_setRtpRawEvent        },
		{ "rtpReceiver.setRtpObjectEvent",     Request::MethodId::rtpReceiver_setRtpObjectEvent     },
		{ "rtpSender.dump",                    Request::MethodId::rtpSender_dump                    },
		{ "rtpSender.setTransport",            Request::MethodId::rtpSender_setTransport            },
		{ "rtpSender.disable",                 Request::MethodId::rtpSender_disable                 }
	};
	// clang-format on

	/* Instance methods. */

	Request::Request(Channel::UnixStreamSocket* channel, Json::Value& json) : channel(channel)
	{
		MS_TRACE();

		static const Json::StaticString k_id("id");
		static const Json::StaticString k_method("method");
		static const Json::StaticString k_internal("internal");
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
			Reject("method not allowed");

			MS_THROW_ERROR("unknown .method '%s'", this->method.c_str());
		}

		if (json[k_internal].isObject())
			this->internal = json[k_internal];
		else
			this->internal = Json::Value(Json::objectValue);

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

		static Json::Value EmptyData(Json::objectValue);

		Accept(EmptyData);
	}

	void Request::Accept(Json::Value& data)
	{
		MS_TRACE();

		static Json::Value EmptyData(Json::objectValue);
		static const Json::StaticString k_id("id");
		static const Json::StaticString k_accepted("accepted");
		static const Json::StaticString k_data("data");

		MS_ASSERT(this->replied == false, "Request already replied");

		this->replied = true;

		Json::Value json(Json::objectValue);

		json[k_id]       = (Json::UInt)this->id;
		json[k_accepted] = true;

		if (data.isObject())
			json[k_data] = data;
		else
			json[k_data] = EmptyData;

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

		static const Json::StaticString k_id("id");
		static const Json::StaticString k_rejected("rejected");
		static const Json::StaticString k_reason("reason");

		MS_ASSERT(this->replied == false, "Request already replied");

		this->replied = true;

		Json::Value json(Json::objectValue);

		json[k_id]       = (Json::UInt)this->id;
		json[k_rejected] = true;

		if (reason)
			json[k_reason] = reason;

		this->channel->Send(json);
	}
}
