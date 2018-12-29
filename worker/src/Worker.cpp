#define MS_CLASS "Worker"
// #define MS_LOG_DEV

#include "Worker.hpp"
#include "DepLibUV.hpp"
#include "Logger.hpp"
#include "MediaSoupError.hpp"
#include "Settings.hpp"

/* Instance methods. */

Worker::Worker(Channel::UnixStreamSocket* channel) : channel(channel)
{
	MS_TRACE();

	// Set us as Channel's listener.
	this->channel->SetListener(this);

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

	// Close the Channel socket.
	delete this->channel;
}

void Worker::FillJson(json& jsonObject) const
{
	MS_TRACE();

	jsonObject["routers"] = json::array();
	auto jsonRoutersIt = jsonObject.find("routers");

	for (auto& kv : this->routers)
	{
		auto& routerId = kv.first;

		jsonRoutersIt->emplace_back(routerId);
	}
}

void Worker::SetNewRouterIdFromRequest(Channel::Request* request, std::string& routerId) const
{
	MS_TRACE();

	auto jsonRouterIdIt = request->internal.find("routerId");

	if (jsonRouterIdIt == request->internal.end() || !jsonRouterIdIt->is_string())
		MS_THROW_ERROR("request has no internal.routerId");

	routerId.assign(jsonRouterIdIt->get<std::string>());

	if (this->routers.find(routerId) != this->routers.end())
		MS_THROW_ERROR("a Router with same routerId already exists");
}

RTC::Router* Worker::GetRouterFromRequest(Channel::Request* request) const
{
	MS_TRACE();

	auto jsonRouterIdIt = request->internal.find("routerId");

	if (jsonRouterIdIt == request->internal.end() || !jsonRouterIdIt->is_string())
		MS_THROW_ERROR("request has no internal.routerId");

	auto it = this->routers.find(jsonRouterIdIt->get<std::string>());

	if (it == this->routers.end())
		MS_THROW_ERROR("Router not found");

	RTC::Router* router = it->second;

	return router;
}

void Worker::OnChannelRequest(Channel::UnixStreamSocket* /*channel*/, Channel::Request* request)
{
	MS_TRACE();

	MS_DEBUG_DEV("'%s' request", request->method.c_str());

	switch (request->methodId)
	{
		case Channel::Request::MethodId::WORKER_DUMP:
		{
			json data = json::object();

			FillJson(data);

			request->Accept(data);

			break;
		}

		case Channel::Request::MethodId::WORKER_UPDATE_SETTINGS:
		{
			Settings::HandleRequest(request);

			break;
		}

		case Channel::Request::MethodId::WORKER_CREATE_ROUTER:
		{
			std::string routerId;

			try
			{
				SetNewRouterIdFromRequest(request, routerId);
			}
			catch (const MediaSoupError& error)
			{
				request->Reject(error.what());

				break;
			}

			auto* router = new RTC::Router(routerId);

			this->routers[routerId] = router;

			MS_DEBUG_DEV("Router created [routerId:%s]", routerId.c_str());

			request->Accept();

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

				break;
			}

			// Remove it from the map and delete it.
			this->routers.erase(router->routerId);
			delete router;

			request->Accept();

			break;
		}

		default:
		{
			RTC::Router* router;

			try
			{
				router = GetRouterFromRequest(request);
			}
			catch (const MediaSoupError& error)
			{
				request->Reject(error.what());

				break;
			}

			router->HandleRequest(request);

			break;
		}
	}
}

void Worker::OnChannelUnixStreamSocketRemotelyClosed(Channel::UnixStreamSocket* /*socket*/)
{
	MS_TRACE_STD();

	// When mediasoup Node process ends it sends a SIGTERM to us so we close this
	// pipe and then exit.
	// If the pipe is remotely closed it means that mediasoup Node process
	// abruptly died (SIGKILL?) so we must die.
	MS_ERROR_STD("channel remotely closed, closing myself");

	Close();
}

void Worker::OnSignal(SignalsHandler* /*signalsHandler*/, int signum)
{
	MS_TRACE();

	switch (signum)
	{
		case SIGINT:
		{
			MS_DEBUG_DEV("INT signal received, closing myself");

			Close();

			break;
		}

		case SIGTERM:
		{
			MS_DEBUG_DEV("TERM signal received, closing myself");

			Close();

			break;
		}

		default:
		{
			MS_WARN_DEV("received a non handled signal [signum:%d]", signum);
		}
	}
}
