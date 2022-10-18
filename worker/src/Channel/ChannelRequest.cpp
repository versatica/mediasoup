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

	// clang-format off
	absl::flat_hash_map<std::string, ChannelRequest::MethodId> ChannelRequest::string2MethodId =
	{
		{ "worker.createWebRtcServer",                   ChannelRequest::MethodId::WORKER_CREATE_WEBRTC_SERVER                      },
		{ "worker.createRouter",                         ChannelRequest::MethodId::WORKER_CREATE_ROUTER                             },
		{ "worker.closeWebRtcServer",                    ChannelRequest::MethodId::WORKER_WEBRTC_SERVER_CLOSE                       },
		{ "webRtcServer.dump",                           ChannelRequest::MethodId::WEBRTC_SERVER_DUMP                               },
		{ "worker.closeRouter",                          ChannelRequest::MethodId::WORKER_CLOSE_ROUTER                              },
		{ "router.dump",                                 ChannelRequest::MethodId::ROUTER_DUMP                                      },
		{ "router.createWebRtcTransport",                ChannelRequest::MethodId::ROUTER_CREATE_WEBRTC_TRANSPORT                   },
		{ "router.createWebRtcTransportWithServer",      ChannelRequest::MethodId::ROUTER_CREATE_WEBRTC_TRANSPORT_WITH_SERVER       },
		{ "router.createPlainTransport",                 ChannelRequest::MethodId::ROUTER_CREATE_PLAIN_TRANSPORT                    },
		{ "router.createPipeTransport",                  ChannelRequest::MethodId::ROUTER_CREATE_PIPE_TRANSPORT                     },
		{ "router.createDirectTransport",                ChannelRequest::MethodId::ROUTER_CREATE_DIRECT_TRANSPORT                   },
		{ "router.closeTransport",                       ChannelRequest::MethodId::ROUTER_CLOSE_TRANSPORT                           },
		{ "router.createActiveSpeakerObserver",          ChannelRequest::MethodId::ROUTER_CREATE_ACTIVE_SPEAKER_OBSERVER            },
		{ "router.createAudioLevelObserver",             ChannelRequest::MethodId::ROUTER_CREATE_AUDIO_LEVEL_OBSERVER               },
		{ "router.closeRtpObserver",                     ChannelRequest::MethodId::ROUTER_CLOSE_RTP_OBSERVER                        },
		{ "transport.dump",                              ChannelRequest::MethodId::TRANSPORT_DUMP                                   },
		{ "transport.getStats",                          ChannelRequest::MethodId::TRANSPORT_GET_STATS                              },
		{ "transport.connect",                           ChannelRequest::MethodId::TRANSPORT_CONNECT                                },
		{ "transport.setMaxIncomingBitrate",             ChannelRequest::MethodId::TRANSPORT_SET_MAX_INCOMING_BITRATE               },
		{ "transport.setMaxOutgoingBitrate",             ChannelRequest::MethodId::TRANSPORT_SET_MAX_OUTGOING_BITRATE               },
		{ "transport.restartIce",                        ChannelRequest::MethodId::TRANSPORT_RESTART_ICE                            },
		{ "transport.produce",                           ChannelRequest::MethodId::TRANSPORT_PRODUCE                                },
		{ "transport.produceData",                       ChannelRequest::MethodId::TRANSPORT_PRODUCE_DATA                           },
		{ "transport.consumeData",                       ChannelRequest::MethodId::TRANSPORT_CONSUME_DATA                           },
		{ "transport.enableTraceEvent",                  ChannelRequest::MethodId::TRANSPORT_ENABLE_TRACE_EVENT                     },
		{ "transport.closeProducer",                     ChannelRequest::MethodId::TRANSPORT_CLOSE_PRODUCER                         },
		{ "transport.closeConsumer",                     ChannelRequest::MethodId::TRANSPORT_CLOSE_CONSUMER                         },
		{ "transport.closeDataProducer",                 ChannelRequest::MethodId::TRANSPORT_CLOSE_DATA_PRODUCER                    },
		{ "transport.closeDataConsumer",                 ChannelRequest::MethodId::TRANSPORT_CLOSE_DATA_CONSUMER                    },
		{ "producer.dump",                               ChannelRequest::MethodId::PRODUCER_DUMP                                    },
		{ "producer.getStats",                           ChannelRequest::MethodId::PRODUCER_GET_STATS                               },
		{ "producer.pause",                              ChannelRequest::MethodId::PRODUCER_PAUSE                                   },
		{ "producer.resume" ,                            ChannelRequest::MethodId::PRODUCER_RESUME                                  },
		{ "producer.enableTraceEvent",                   ChannelRequest::MethodId::PRODUCER_ENABLE_TRACE_EVENT                      },
		{ "consumer.dump",                               ChannelRequest::MethodId::CONSUMER_DUMP                                    },
		{ "consumer.getStats",                           ChannelRequest::MethodId::CONSUMER_GET_STATS                               },
		{ "consumer.pause",                              ChannelRequest::MethodId::CONSUMER_PAUSE                                   },
		{ "consumer.resume",                             ChannelRequest::MethodId::CONSUMER_RESUME                                  },
		{ "consumer.setPreferredLayers",                 ChannelRequest::MethodId::CONSUMER_SET_PREFERRED_LAYERS                    },
		{ "consumer.setPriority",                        ChannelRequest::MethodId::CONSUMER_SET_PRIORITY                            },
		{ "consumer.requestKeyFrame",                    ChannelRequest::MethodId::CONSUMER_REQUEST_KEY_FRAME                       },
		{ "consumer.enableTraceEvent",                   ChannelRequest::MethodId::CONSUMER_ENABLE_TRACE_EVENT                      },
		{ "dataProducer.dump",                           ChannelRequest::MethodId::DATA_PRODUCER_DUMP                               },
		{ "dataProducer.getStats",                       ChannelRequest::MethodId::DATA_PRODUCER_GET_STATS                          },
		{ "dataConsumer.dump",                           ChannelRequest::MethodId::DATA_CONSUMER_DUMP                               },
		{ "dataConsumer.getStats",                       ChannelRequest::MethodId::DATA_CONSUMER_GET_STATS                          },
		{ "dataConsumer.getBufferedAmount",              ChannelRequest::MethodId::DATA_CONSUMER_GET_BUFFERED_AMOUNT                },
		{ "dataConsumer.setBufferedAmountLowThreshold",  ChannelRequest::MethodId::DATA_CONSUMER_SET_BUFFERED_AMOUNT_LOW_THRESHOLD  },
		{ "rtpObserver.pause",                           ChannelRequest::MethodId::RTP_OBSERVER_PAUSE                               },
		{ "rtpObserver.resume",                          ChannelRequest::MethodId::RTP_OBSERVER_RESUME                              },
		{ "rtpObserver.addProducer",                     ChannelRequest::MethodId::RTP_OBSERVER_ADD_PRODUCER                        },
		{ "rtpObserver.removeProducer",                  ChannelRequest::MethodId::RTP_OBSERVER_REMOVE_PRODUCER                     }
	};
	// clang-format on

	// clang-format off
	absl::flat_hash_map<FBS::Request::Method, const char*> ChannelRequest::method2String =
	{
		{ FBS::Request::Method::WORKER_CLOSE,                                    "worker.close"                               },
		{ FBS::Request::Method::WORKER_DUMP,                                     "worker.dump"                                },
		{ FBS::Request::Method::WORKER_GET_RESOURCE_USAGE,                       "worker.getResourceUsage"                    },
		{ FBS::Request::Method::WORKER_UPDATE_SETTINGS,                           "worker.updateSettings"                      },
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

		this->id     = std::stoul(info[0]);
		this->method = info[1];

		auto methodIdIt = ChannelRequest::string2MethodId.find(this->method);

		if (methodIdIt == ChannelRequest::string2MethodId.end())
		{
			Error("unknown method");

			MS_THROW_ERROR("unknown method '%s'", this->method.c_str());
		}

		this->methodId = methodIdIt->second;

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

		// TMP.
		auto s = flatbuffers::FlatBufferToString(msg, FBS::Request::RequestTypeTable());
		MS_ERROR("%s", s.c_str());

		this->_data = FBS::Request::GetRequest(msg);

		this->id = this->_data->id();
		this->_method = this->_data->method();
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

		auto response = FBS::Response::CreateResponse(builder, this->id, true, FBS::Response::Body::NONE, 0);

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
