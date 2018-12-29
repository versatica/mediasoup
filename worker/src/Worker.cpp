#define MS_CLASS "Worker"
// #define MS_LOG_DEV

#include "Worker.hpp"
#include "DepLibUV.hpp"
#include "Logger.hpp"
#include "MediaSoupError.hpp"
#include "Settings.hpp"
#include <json/json.h>
#include <cerrno>
#include <iostream> // std::cout, std::cerr
#include <string>
#include <utility> // std::pair()

/* Instance methods. */

Worker::Worker(Channel::UnixStreamSocket* channel) : channel(channel)
{
	MS_TRACE();

	// Set us as Channel's listener.
	this->channel->SetListener(this);

	// Create the Notifier instance.
	this->notifier = new Channel::Notifier(this->channel);

	// Set the signals handler.
	this->signalsHandler = new SignalsHandler(this);

	// Add signals to handle.
	this->signalsHandler->AddSignal(SIGINT, "INT");
	this->signalsHandler->AddSignal(SIGTERM, "TERM");

	MS_DEBUG_DEV("starting libuv loop");
	DepLibUV::RunLoop();
	MS_DEBUG_DEV("libuv loop ended");
}

Worker::~Worker()
{
	MS_TRACE();

	if (!this->closed)
		Close();
}

void Worker::Close()
{
	MS_TRACE();

	if (this->closed)
		return;

	this->closed = true;

	// Close the SignalsHandler.
	delete this->signalsHandler;

	// Close all the Routers.
	// NOTE: Upon Router closure the onRouterClosed() method is called, which
	// removes it from the map, so this is the safe way to iterate the map
	// and remove elements.
	for (auto it = this->routers.begin(); it != this->routers.end();)
	{
		RTC::Router* router = it->second;

		it = this->routers.erase(it);
		delete router;
	}

	// Delete the Notifier.
	delete this->notifier;

	// Close the Channel socket.
	delete this->channel;
}

RTC::Router* Worker::GetRouterFromRequest(Channel::Request* request, uint32_t* routerId)
{
	MS_TRACE();

	static const Json::StaticString JsonStringRouterId{ "routerId" };

	auto jsonRouterId = request->internal[JsonStringRouterId];

	if (!jsonRouterId.isUInt())
		MS_THROW_ERROR("Request has not numeric internal.routerId");

	// If given, fill routerId.
	if (routerId != nullptr)
		*routerId = jsonRouterId.asUInt();

	auto it = this->routers.find(jsonRouterId.asUInt());
	if (it != this->routers.end())
	{
		RTC::Router* router = it->second;

		return router;
	}

	return nullptr;
}

void Worker::OnSignal(SignalsHandler* /*signalsHandler*/, int signum)
{
	MS_TRACE();

	switch (signum)
	{
		case SIGINT:
		{
			MS_DEBUG_DEV("signal INT received, exiting");

			Close();

			break;
		}

		case SIGTERM:
		{
			MS_DEBUG_DEV("signal TERM received, exiting");

			Close();

			break;
		}

		default:
		{
			MS_WARN_DEV("received a signal (with signum %d) for which there is no handling code", signum);
		}
	}
}

void Worker::OnChannelRequest(Channel::UnixStreamSocket* /*channel*/, Channel::Request* request)
{
	MS_TRACE();

	MS_DEBUG_DEV("'%s' request", request->method.c_str());

	switch (request->methodId)
	{
		case Channel::Request::MethodId::WORKER_DUMP:
		{
			static const Json::StaticString JsonStringWorkerId{ "workerId" };
			static const Json::StaticString JsonStringRouters{ "routers" };

			Json::Value json(Json::objectValue);
			Json::Value jsonRouters(Json::arrayValue);

			json[JsonStringWorkerId] = Logger::id;

			for (auto& kv : this->routers)
			{
				auto router = kv.second;

				jsonRouters.append(router->ToJson());
			}

			json[JsonStringRouters] = jsonRouters;

			request->Accept(json);

			break;
		}

		case Channel::Request::MethodId::WORKER_UPDATE_SETTINGS:
		{
			Settings::HandleRequest(request);

			break;
		}

		case Channel::Request::MethodId::WORKER_CREATE_ROUTER:
		{
			RTC::Router* router;
			uint32_t routerId;

			try
			{
				router = GetRouterFromRequest(request, &routerId);
			}
			catch (const MediaSoupError& error)
			{
				request->Reject(error.what());

				return;
			}

			if (router != nullptr)
			{
				request->Reject("Router already exists");

				return;
			}

			try
			{
				router = new RTC::Router(this, this->notifier, routerId);
			}
			catch (const MediaSoupError& error)
			{
				request->Reject(error.what());

				return;
			}

			this->routers[routerId] = router;

			MS_DEBUG_DEV("Router created [routerId:%" PRIu32 "]", routerId);

			Json::Value data(Json::objectValue);

			request->Accept(data);

			break;
		}

		case Channel::Request::MethodId::ROUTER_CLOSE:
		{
			RTC::Router* router;

			try
			{
				router = GetRouterFromRequest(request);
			}
			catch (const MediaSoupError& error)
			{
				request->Reject(error.what());

				return;
			}

			if (router == nullptr)
			{
				request->Reject("Router does not exist");

				return;
			}

			delete router;

			request->Accept();

			break;
		}

		case Channel::Request::MethodId::ROUTER_DUMP:
		case Channel::Request::MethodId::ROUTER_CREATE_WEBRTC_TRANSPORT:
		case Channel::Request::MethodId::ROUTER_CREATE_PLAIN_RTP_TRANSPORT:
		case Channel::Request::MethodId::ROUTER_CREATE_PRODUCER:
		case Channel::Request::MethodId::ROUTER_CREATE_CONSUMER:
		case Channel::Request::MethodId::ROUTER_SET_AUDIO_LEVELS_EVENT:
		case Channel::Request::MethodId::TRANSPORT_CLOSE:
		case Channel::Request::MethodId::TRANSPORT_DUMP:
		case Channel::Request::MethodId::TRANSPORT_GET_STATS:
		case Channel::Request::MethodId::TRANSPORT_SET_REMOTE_DTLS_PARAMETERS:
		case Channel::Request::MethodId::TRANSPORT_SET_REMOTE_PARAMETERS:
		case Channel::Request::MethodId::TRANSPORT_SET_MAX_BITRATE:
		case Channel::Request::MethodId::TRANSPORT_CHANGE_UFRAG_PWD:
		case Channel::Request::MethodId::TRANSPORT_START_MIRRORING:
		case Channel::Request::MethodId::TRANSPORT_STOP_MIRRORING:
		case Channel::Request::MethodId::PRODUCER_CLOSE:
		case Channel::Request::MethodId::PRODUCER_DUMP:
		case Channel::Request::MethodId::PRODUCER_GET_STATS:
		case Channel::Request::MethodId::PRODUCER_PAUSE:
		case Channel::Request::MethodId::PRODUCER_RESUME:
		case Channel::Request::MethodId::PRODUCER_SET_PREFERRED_PROFILE:
		case Channel::Request::MethodId::CONSUMER_CLOSE:
		case Channel::Request::MethodId::CONSUMER_DUMP:
		case Channel::Request::MethodId::CONSUMER_GET_STATS:
		case Channel::Request::MethodId::CONSUMER_ENABLE:
		case Channel::Request::MethodId::CONSUMER_PAUSE:
		case Channel::Request::MethodId::CONSUMER_RESUME:
		case Channel::Request::MethodId::CONSUMER_SET_PREFERRED_PROFILE:
		case Channel::Request::MethodId::CONSUMER_SET_ENCODING_PREFERENCES:
		case Channel::Request::MethodId::CONSUMER_REQUEST_KEY_FRAME:
		{
			RTC::Router* router;

			try
			{
				router = GetRouterFromRequest(request);
			}
			catch (const MediaSoupError& error)
			{
				request->Reject(error.what());

				return;
			}

			if (router == nullptr)
			{
				request->Reject("Router does not exist");

				return;
			}

			router->HandleRequest(request);

			break;
		}

		default:
		{
			MS_ERROR("unknown method");

			request->Reject("unknown method");
		}
	}
}

void Worker::OnChannelUnixStreamSocketRemotelyClosed(Channel::UnixStreamSocket* /*socket*/)
{
	MS_TRACE_STD();

	// When mediasoup Node process ends, it sends a SIGTERM to us so we close this
	// pipe and then exit.
	// If the pipe is remotely closed it means that mediasoup Node process
	// abruptly died (SIGKILL?) so we must die.
	MS_ERROR_STD("Channel remotely closed, killing myself");

	Close();
}

void Worker::OnRouterClosed(RTC::Router* router)
{
	MS_TRACE();

	this->routers.erase(router->routerId);
}
