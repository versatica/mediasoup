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
		{ "worker.createRoom",                 Request::MethodId::WORKER_CREATE_ROOM                   },
		{ "room.close",                        Request::MethodId::ROOM_CLOSE                           },
		{ "room.dump",                         Request::MethodId::ROOM_DUMP                            },
		{ "room.createPeer",                   Request::MethodId::ROOM_CREATE_PEER                     },
		{ "peer.close",                        Request::MethodId::PEER_CLOSE                           },
		{ "peer.dump",                         Request::MethodId::PEER_DUMP                            },
		{ "peer.setCapabilities",              Request::MethodId::PEER_SET_CAPABILITIES                },
		{ "peer.createTransport",              Request::MethodId::PEER_CREATE_TRANSPORT                },
		{ "peer.createRtpReceiver",            Request::MethodId::PEER_CREATE_RTP_RECEIVER             },
		{ "transport.close",                   Request::MethodId::TRANSPORT_CLOSE                      },
		{ "transport.dump",                    Request::MethodId::TRANSPORT_DUMP                       },
		{ "transport.setRemoteDtlsParameters", Request::MethodId::TRANSPORT_SET_REMOTE_DTLS_PARAMETERS },
		{ "transport.setMaxBitrate",           Request::MethodId::TRANSPORT_SET_MAX_BITRATE            },
		{ "transport.changeUfragPwd",          Request::MethodId::TRANSPORT_CHANGE_UFRAG_PWD           },
		{ "rtpReceiver.close",                 Request::MethodId::RTP_RECEIVER_CLOSE                   },
		{ "rtpReceiver.dump",                  Request::MethodId::RTP_RECEIVER_DUMP                    },
		{ "rtpReceiver.receive",               Request::MethodId::RTP_RECEIVER_RECEIVE                 },
		{ "rtpReceiver.setTransport",          Request::MethodId::RTP_RECEIVER_SET_TRANSPORT           },
		{ "rtpReceiver.setRtpRawEvent",        Request::MethodId::RTP_RECEIVER_SET_RTP_RAW_EVENT       },
		{ "rtpReceiver.setRtpObjectEvent",     Request::MethodId::RTP_RECEIVER_SET_RTP_OBJECT_EVENT    },
		{ "rtpSender.dump",                    Request::MethodId::RTP_SENDER_DUMP                      },
		{ "rtpSender.setTransport",            Request::MethodId::RTP_SENDER_SET_TRANSPORT             },
		{ "rtpSender.disable",                 Request::MethodId::RTP_SENDER_DISABLE                   }
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
			MS_THROW_ERROR("json has no numeric .id field");

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
			Reject("method not allowed");

			MS_THROW_ERROR("unknown .method '%s'", this->method.c_str());
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
