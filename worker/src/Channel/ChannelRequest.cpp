#define MS_CLASS "Channel::ChannelRequest"
// #define MS_LOG_DEV_LEVEL 3

#include "Channel/ChannelRequest.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "Utils.hpp"
#include "FBS/message_generated.h"
#include "FBS/request_generated.h"
#include <flatbuffers/minireflect.h>

namespace Channel
{
	/* Class variables. */

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
		{ FBS::Request::Method::WORKER_CLOSE_ROUTER,                             "worker.closeRouter"                         },
		{ FBS::Request::Method::WEBRTC_SERVER_DUMP,                              "webRtcServer.dump"                          },
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
		{ FBS::Request::Method::PLAIN_TRANSPORT_CONNECT,                         "plainTransport.connect"                     },
		{ FBS::Request::Method::PIPE_TRANSPORT_CONNECT,                          "pipeTransport.connect"                      },
		{ FBS::Request::Method::WEBRTC_TRANSPORT_CONNECT,                        "webRtcTransport.connect"                    },
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
		{ FBS::Request::Method::DATA_CONSUMER_SEND,                              "dataConsumer.send"                          },
		{ FBS::Request::Method::RTP_OBSERVER_PAUSE,                              "rtpObserver.pause"                          },
		{ FBS::Request::Method::RTP_OBSERVER_RESUME,                             "rtpObserver.resume"                         },
		{ FBS::Request::Method::RTP_OBSERVER_ADD_PRODUCER,                       "rtpObserver.addProducer"                    },
		{ FBS::Request::Method::RTP_OBSERVER_REMOVE_PRODUCER,                    "rtpObserver.removeProducer"                 },
	};
	// clang-format on

	/* Class methods. */
	flatbuffers::FlatBufferBuilder ChannelRequest::bufferBuilder;

	/* Instance methods. */

	/**
	 * msg contains the request flatbuffer.
	 */
	ChannelRequest::ChannelRequest(Channel::ChannelSocket* channel, const FBS::Request::Request* request)
	  : channel(channel)
	{
		MS_TRACE();

		this->data       = request;
		this->id         = request->id();
		this->method     = request->method();

		auto methodCStrIt = ChannelRequest::method2String.find(this->method);

		if (methodCStrIt == ChannelRequest::method2String.end())
		{
			Error("unknown method");

			MS_THROW_ERROR("unknown method '%" PRIu8 "'", this->method);
		}

		this->methodCStr = methodCStrIt->second;

		// Handler ID is optional.
		if (flatbuffers::IsFieldPresent(this->data, FBS::Request::Request::VT_HANDLERID))
			this->handlerId = this->data->handlerId()->str();
	}

	void ChannelRequest::Accept()
	{
		MS_TRACE();

		MS_ASSERT(!this->replied, "request already replied");

		this->replied = true;

		auto& builder = ChannelRequest::bufferBuilder;
		auto response =
		  FBS::Response::CreateResponse(builder, this->id, true, FBS::Response::Body::NONE, 0);

		Send(response);
	}

	void ChannelRequest::Error(const char* reason)
	{
		MS_TRACE();

		MS_ASSERT(!this->replied, "request already replied");

		this->replied = true;

		auto& builder = ChannelRequest::bufferBuilder;
		auto response = FBS::Response::CreateResponseDirect(
		  builder, this->id, false /*accepted*/, FBS::Response::Body::NONE, 0, "Error" /*Error*/, reason);

		Send(response);
	}

	void ChannelRequest::TypeError(const char* reason)
	{
		MS_TRACE();

		MS_ASSERT(!this->replied, "request already replied");

		this->replied = true;

		auto& builder = ChannelRequest::bufferBuilder;
		auto response = FBS::Response::CreateResponseDirect(
		  builder, this->id, false /*accepted*/, FBS::Response::Body::NONE, 0, "TypeError" /*Error*/, reason);

		Send(response);
	}

	void ChannelRequest::Send(uint8_t* buffer, size_t size)
	{
		this->channel->Send(buffer, size);
	}

	void ChannelRequest::Send(const flatbuffers::Offset<FBS::Response::Response>& response)
	{
		auto& builder = ChannelRequest::bufferBuilder;
		auto message  = FBS::Message::CreateMessage(
      builder,
      FBS::Message::Type::RESPONSE,
      FBS::Message::Body::FBS_Response_Response,
      response.Union());

		builder.FinishSizePrefixed(message);
		this->Send(builder.GetBufferPointer(), builder.GetSize());
		builder.Reset();
	}
} // namespace Channel
