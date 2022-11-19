#define MS_CLASS "Channel::ChannelRequest"
// #define MS_LOG_DEV_LEVEL 3

#include "Channel/ChannelRequest.hpp"
#include "FBS/request_generated.h"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "Utils.hpp"
#include <flatbuffers/minireflect.h>

namespace Channel
{
	/* Class variables. */

	// TODO: Remove once JSON is removed.
	// clang-format off
	absl::flat_hash_map<std::string, FBS::Request::Method> ChannelRequest::string2Method =
	{
		{ "transport.getStats",                          FBS::Request::Method::TRANSPORT_GET_STATS                              },
		{ "transport.connect",                           FBS::Request::Method::TRANSPORT_CONNECT                                },
		{ "transport.restartIce",                        FBS::Request::Method::TRANSPORT_RESTART_ICE                            },
		{ "producer.getStats",                           FBS::Request::Method::PRODUCER_GET_STATS                               },
		{ "consumer.getStats",                           FBS::Request::Method::CONSUMER_GET_STATS                               },
		{ "dataProducer.getStats",                       FBS::Request::Method::DATA_PRODUCER_GET_STATS                          },
		{ "dataConsumer.getStats",                       FBS::Request::Method::DATA_CONSUMER_GET_STATS                          },
	};
	// clang-format on

	// clang-format off
	absl::flat_hash_map<FBS::Request::Method, const char*> ChannelRequest::method2String =
	{
		{ FBS::Request::Method::WORKER_CLOSE,                                    "worker.close"                               },
		{ FBS::Request::Method::WORKER_DUMP,                                     "worker.dump"                                },
		{ FBS::Request::Method::WORKER_GET_RESOURCE_USAGE,                       "worker.getResourceUsage"                    },
		{ FBS::Request::Method::WORKER_UPDATE_SETTINGS,                          "worker.updateSettings"                      },
		{ FBS::Request::Method::WORKER_CREATE_WEBRTC_SERVER,                     "worker.createWebRtcServer"                  },
		{ FBS::Request::Method::WORKER_CREATE_ROUTER,                            "worker.createRouter"                        },
		{ FBS::Request::Method::WORKER_WEBRTC_SERVER_CLOSE,                      "worker.closeWebRtcServer"                   },
		{ FBS::Request::Method::WEBRTC_SERVER_DUMP,                              "webRtcServer.dump"                          },
		{ FBS::Request::Method::WORKER_CLOSE_ROUTER,                             "worker.closeRouter"                         },
		{ FBS::Request::Method::ROUTER_DUMP,                                     "router.dump"                                },
		{ FBS::Request::Method::ROUTER_CREATE_WEBRTC_TRANSPORT,                  "router.createWebRtcTransport"               },
		{ FBS::Request::Method::ROUTER_CREATE_WEBRTC_TRANSPORT_WITH_SERVER,      "router.createWebRtcTransportWithServer"     },
		{ FBS::Request::Method::ROUTER_CREATE_PLAIN_TRANSPORT,                   "router.createPlainTransport"                },
		{ FBS::Request::Method::ROUTER_CREATE_PIPE_TRANSPORT,                    "router.createPipeTransport"                 },
		{ FBS::Request::Method::ROUTER_CREATE_DIRECT_TRANSPORT,                  "router.createDirectTransport"               },
		{ FBS::Request::Method::ROUTER_CLOSE_TRANSPORT,                          "router.closeTransport"                      },
		{ FBS::Request::Method::ROUTER_CREATE_ACTIVE_SPEAKER_OBSERVER,           "router.createActiveSpeakerObserver"         },
		{ FBS::Request::Method::ROUTER_CREATE_AUDIO_LEVEL_OBSERVER,              "router.createAudioLevelObserver"            },
		{ FBS::Request::Method::ROUTER_CLOSE_RTP_OBSERVER,                       "router.closeRtpObserver"                    },
		{ FBS::Request::Method::TRANSPORT_DUMP,                                  "transport.dump"                             },
		{ FBS::Request::Method::TRANSPORT_GET_STATS,                             "transport.getStats"                         },
		{ FBS::Request::Method::TRANSPORT_CONNECT,                               "transport.connect"                          },
		{ FBS::Request::Method::TRANSPORT_SET_MAX_INCOMING_BITRATE,              "transport.setMaxIncomingBitrate"            },
		{ FBS::Request::Method::TRANSPORT_SET_MAX_OUTGOING_BITRATE,              "transport.setMaxOutgoingBitrate"            },
		{ FBS::Request::Method::TRANSPORT_RESTART_ICE,                           "transport.restartIce"                       },
		{ FBS::Request::Method::TRANSPORT_PRODUCE,                               "transport.produce"                          },
		{ FBS::Request::Method::TRANSPORT_PRODUCE_DATA,                          "transport.produceData"                      },
		{ FBS::Request::Method::TRANSPORT_CONSUME,                               "transport.consume"                          },
		{ FBS::Request::Method::TRANSPORT_CONSUME_DATA,                          "transport.consumeData"                      },
		{ FBS::Request::Method::TRANSPORT_ENABLE_TRACE_EVENT,                    "transport.enableTraceEvent"                 },
		{ FBS::Request::Method::TRANSPORT_CLOSE_PRODUCER,                        "transport.closeProducer"                    },
		{ FBS::Request::Method::TRANSPORT_CLOSE_CONSUMER,                        "transport.closeConsumer"                    },
		{ FBS::Request::Method::TRANSPORT_CLOSE_DATA_PRODUCER,                   "transport.closeDataProducer"                },
		{ FBS::Request::Method::TRANSPORT_CLOSE_DATA_CONSUMER,                   "transport.closeDataConsumer"                },
		{ FBS::Request::Method::PRODUCER_DUMP,                                   "producer.dump"                              },
		{ FBS::Request::Method::PRODUCER_GET_STATS,                              "producer.getStats"                          },
		{ FBS::Request::Method::PRODUCER_PAUSE,                                  "producer.pause"                             },
		{ FBS::Request::Method::PRODUCER_RESUME,                                 "producer.resume"                            },
		{ FBS::Request::Method::PRODUCER_ENABLE_TRACE_EVENT,                     "producer.enableTraceEvent"                  },
		{ FBS::Request::Method::CONSUMER_DUMP,                                   "consumer.dump"                              },
		{ FBS::Request::Method::CONSUMER_GET_STATS,                              "consumer.getStats"                          },
		{ FBS::Request::Method::CONSUMER_PAUSE,                                  "consumer.pause"                             },
		{ FBS::Request::Method::CONSUMER_RESUME,                                 "consumer.resume"                            },
		{ FBS::Request::Method::CONSUMER_SET_PREFERRED_LAYERS,                   "consumer.setPreferredLayers"                },
		{ FBS::Request::Method::CONSUMER_SET_PRIORITY,                           "consumer.setPriority"                       },
		{ FBS::Request::Method::CONSUMER_REQUEST_KEY_FRAME,                      "consumer.requestKeyFrame"                   },
		{ FBS::Request::Method::CONSUMER_ENABLE_TRACE_EVENT,                     "consumer.enableTraceEvent"                  },
		{ FBS::Request::Method::DATA_PRODUCER_DUMP,                              "dataProducer.dump"                          },
		{ FBS::Request::Method::DATA_PRODUCER_GET_STATS,                         "dataProducer.getStats"                      },
		{ FBS::Request::Method::DATA_CONSUMER_DUMP,                              "dataConsumer.dump"                          },
		{ FBS::Request::Method::DATA_CONSUMER_GET_STATS,                         "dataConsumer.getStats"                      },
		{ FBS::Request::Method::DATA_CONSUMER_GET_BUFFERED_AMOUNT,               "dataConsumer.getBufferedAmount"             },
		{ FBS::Request::Method::DATA_CONSUMER_SET_BUFFERED_AMOUNT_LOW_THRESHOLD, "dataConsumer.setBufferedAmountLowThreshold" },
		{ FBS::Request::Method::RTP_OBSERVER_PAUSE,                              "rtpObserver.pause"                          },
		{ FBS::Request::Method::RTP_OBSERVER_RESUME,                             "rtpObserver.resume"                         },
		{ FBS::Request::Method::RTP_OBSERVER_ADD_PRODUCER,                       "rtpObserver.addProducer"                    },
		{ FBS::Request::Method::RTP_OBSERVER_REMOVE_PRODUCER,                    "rtpObserver.removeProducer"                 },
	};

	// clang-format on
	flatbuffers::FlatBufferBuilder ChannelRequest::bufferBuilder;

	/* Instance methods. */

	/**
	 * msg contains "id:method:handlerId:data" where:
	 * - id: The ID of the request.
	 * - handlerId: The ID of the target entity
	 * - data: JSON object.
	 */
	ChannelRequest::ChannelRequest(Channel::ChannelSocket* channel, const char* msg, size_t msgLen)
	  : channel(channel)
	{
		MS_TRACE();

		auto info = Utils::String::Split(std::string(msg, msgLen), ':', 3);

		if (info.size() < 2)
			MS_THROW_ERROR("too few arguments");

		this->id = std::stoul(info[0]);

		auto method = info[1];

		auto methodIdIt = ChannelRequest::string2Method.find(method);

		if (methodIdIt == ChannelRequest::string2Method.end())
		{
			Error("unknown method");

			MS_THROW_ERROR("unknown method '%s'", methodStr.c_str());
		}

		this->methodStr = method;
		this->method    = methodIdIt->second;

		if (info.size() > 2)
		{
			auto& handlerId = info[2];

			if (handlerId != "undefined")
				this->handlerId = handlerId;
		}

		if (info.size() > 3)
		{
			auto& data = info[3];

			if (data != "undefined")
			{
				try
				{
					this->data = json::parse(data);

					if (!this->data.is_object())
						this->data = json::object();
				}
				catch (const json::parse_error& error)
				{
					MS_THROW_TYPE_ERROR("JSON parsing error: %s", error.what());
				}
			}
		}
	}

	/**
	 * msg contains the request flatbuffer.
	 */
	ChannelRequest::ChannelRequest(Channel::ChannelSocket* channel, const uint8_t* msg)
	  : channel(channel)
	{
		MS_TRACE();

		// TMP: For debugging.
		auto s = flatbuffers::FlatBufferToString(msg, FBS::Request::RequestTypeTable());
		// TODO: Fails when consuming from a PIPE transport.
		MS_ERROR("%s", s.c_str());

		this->_data = FBS::Request::GetRequest(msg);

		this->id        = this->_data->id();
		this->method    = this->_data->method();
		this->methodStr = method2String[this->method];

		// Handler ID is optional.
		if (flatbuffers::IsFieldPresent(this->_data, FBS::Request::Request::VT_HANDLERID))
			this->handlerId = this->_data->handlerId()->str();
	}

	ChannelRequest::~ChannelRequest()
	{
		MS_TRACE();
	}

	void ChannelRequest::Accept()
	{
		MS_TRACE();

		MS_ASSERT(!this->replied, "request already replied");

		this->replied = true;

		auto& builder = ChannelRequest::bufferBuilder;
		auto response =
		  FBS::Response::CreateResponse(builder, this->id, true, FBS::Response::Body::NONE, 0);

		builder.Finish(response);

		this->Send(builder.GetBufferPointer(), builder.GetSize());

		builder.Reset();
	}

	void ChannelRequest::Send(uint8_t* buffer, size_t size)
	{
		this->channel->Send(buffer, size);
	}

	void ChannelRequest::Accept(json& data)
	{
		MS_TRACE();

		MS_ASSERT(!this->replied, "request already replied");

		this->replied = true;

		json jsonResponse = json::object();

		jsonResponse["id"]       = this->id;
		jsonResponse["accepted"] = true;

		if (data.is_structured())
			jsonResponse["data"] = data;

		this->channel->Send(jsonResponse);
	}

	void ChannelRequest::Error(const char* reason)
	{
		MS_TRACE();

		MS_ASSERT(!this->replied, "request already replied");

		this->replied = true;

		json jsonResponse = json::object();

		jsonResponse["id"]    = this->id;
		jsonResponse["error"] = "Error";

		if (reason != nullptr)
			jsonResponse["reason"] = reason;

		this->channel->Send(jsonResponse);
	}

	void ChannelRequest::TypeError(const char* reason)
	{
		MS_TRACE();

		MS_ASSERT(!this->replied, "request already replied");

		this->replied = true;

		json jsonResponse = json::object();

		jsonResponse["id"]    = this->id;
		jsonResponse["error"] = "TypeError";

		if (reason != nullptr)
			jsonResponse["reason"] = reason;

		this->channel->Send(jsonResponse);
	}
} // namespace Channel
