#define MS_CLASS "Channel::ChannelRequest"
// #define MS_LOG_DEV_LEVEL 3

#include "Channel/ChannelRequest.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "Utils.hpp"

namespace Channel
{
	/* Class variables. */

	// clang-format off
	absl::flat_hash_map<std::string, ChannelRequest::MethodId> ChannelRequest::string2MethodId =
	{
		{ "worker.close",                                ChannelRequest::MethodId::WORKER_CLOSE                                     },
		{ "worker.dump",                                 ChannelRequest::MethodId::WORKER_DUMP                                      },
		{ "worker.getResourceUsage",                     ChannelRequest::MethodId::WORKER_GET_RESOURCE_USAGE                        },
		{ "worker.updateSettings",                       ChannelRequest::MethodId::WORKER_UPDATE_SETTINGS                           },
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
		{ "transport.setMinOutgoingBitrate",             ChannelRequest::MethodId::TRANSPORT_SET_MIN_OUTGOING_BITRATE               },
		{ "transport.restartIce",                        ChannelRequest::MethodId::TRANSPORT_RESTART_ICE                            },
		{ "transport.produce",                           ChannelRequest::MethodId::TRANSPORT_PRODUCE                                },
		{ "transport.consume",                           ChannelRequest::MethodId::TRANSPORT_CONSUME                                },
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

	ChannelRequest::~ChannelRequest()
	{
		MS_TRACE();
	}

	void ChannelRequest::Accept()
	{
		MS_TRACE();

		MS_ASSERT(!this->replied, "request already replied");

		this->replied = true;

		std::string response("{\"id\":");

		response.append(std::to_string(this->id));
		response.append(",\"accepted\":true}");

		this->channel->Send(response);
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
