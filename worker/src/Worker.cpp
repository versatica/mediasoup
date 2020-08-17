#define MS_CLASS "Worker"
// #define MS_LOG_DEV_LEVEL 3

#include "Worker.hpp"
#include "DepLibUV.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "Settings.hpp"
#include "Channel/Notifier.hpp"

/* Instance methods. */

Worker::Worker(::Channel::UnixStreamSocket* channel, PayloadChannel::UnixStreamSocket* payloadChannel)
  : channel(channel), payloadChannel(payloadChannel)
{
	MS_TRACE();

	// Set us as Channel's listener.
	this->channel->SetListener(this);

	// Set us as PayloadChannel's listener.
	this->payloadChannel->SetListener(this);

	// Set the signals handler.
	this->signalsHandler = new SignalsHandler(this);

	// Add signals to handle.
	this->signalsHandler->AddSignal(SIGINT, "INT");
	this->signalsHandler->AddSignal(SIGTERM, "TERM");

	// Tell the Node process that we are running.
	Channel::Notifier::Emit(std::to_string(Logger::pid), "running");

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

	// Delete the SignalsHandler.
	delete this->signalsHandler;

	// Delete all Routers.
	for (auto& kv : this->mapRouters)
	{
		auto* router = kv.second;

		delete router;
	}
	this->mapRouters.clear();

	// Close the Channel.
	delete this->channel;

	// Close the PayloadChannel.
	delete this->payloadChannel;
}

void Worker::FillJson(json& jsonObject) const
{
	MS_TRACE();

	// Add pid.
	jsonObject["pid"] = Logger::pid;

	// Add routerIds.
	jsonObject["routerIds"] = json::array();
	auto jsonRouterIdsIt    = jsonObject.find("routerIds");

	for (auto& kv : this->mapRouters)
	{
		auto& routerId = kv.first;

		jsonRouterIdsIt->emplace_back(routerId);
	}
}

void Worker::FillJsonResourceUsage(json& jsonObject) const
{
	MS_TRACE();

	int err;
	uv_rusage_t uvRusage; // NOLINT(cppcoreguidelines-pro-type-member-init)

	err = uv_getrusage(std::addressof(uvRusage));

	if (err != 0)
		MS_THROW_ERROR("uv_getrusagerequest() failed: %s", uv_strerror(err));

	// Add ru_utime (uv_timeval_t, user CPU time used, converted to ms).
	jsonObject["ru_utime"] =
	  (uvRusage.ru_utime.tv_sec * static_cast<uint64_t>(1000)) + (uvRusage.ru_utime.tv_usec / 1000);

	// Add ru_stime (uv_timeval_t, system CPU time used, converted to ms).
	jsonObject["ru_stime"] =
	  (uvRusage.ru_stime.tv_sec * static_cast<uint64_t>(1000)) + (uvRusage.ru_stime.tv_usec / 1000);

	// Add ru_maxrss (uint64_t, maximum resident set size).
	jsonObject["ru_maxrss"] = uvRusage.ru_maxrss;

	// Add ru_ixrss (uint64_t, integral shared memory size).
	jsonObject["ru_ixrss"] = uvRusage.ru_ixrss;

	// Add ru_idrss (uint64_t, integral unshared data size).
	jsonObject["ru_idrss"] = uvRusage.ru_idrss;

	// Add ru_isrss (uint64_t, integral unshared stack size).
	jsonObject["ru_isrss"] = uvRusage.ru_isrss;

	// Add ru_minflt (uint64_t, page reclaims, soft page faults).
	jsonObject["ru_minflt"] = uvRusage.ru_minflt;

	// Add ru_majflt (uint64_t, page faults, hard page faults).
	jsonObject["ru_majflt"] = uvRusage.ru_majflt;

	// Add ru_nswap (uint64_t, swaps).
	jsonObject["ru_nswap"] = uvRusage.ru_nswap;

	// Add ru_inblock (uint64_t, block input operations).
	jsonObject["ru_inblock"] = uvRusage.ru_inblock;

	// Add ru_oublock (uint64_t, block output operations).
	jsonObject["ru_oublock"] = uvRusage.ru_oublock;

	// Add ru_msgsnd (uint64_t, IPC messages sent).
	jsonObject["ru_msgsnd"] = uvRusage.ru_msgsnd;

	// Add ru_msgrcv (uint64_t, IPC messages received).
	jsonObject["ru_msgrcv"] = uvRusage.ru_msgrcv;

	// Add ru_nsignals (uint64_t, signals received).
	jsonObject["ru_nsignals"] = uvRusage.ru_nsignals;

	// Add ru_nvcsw (uint64_t, voluntary context switches).
	jsonObject["ru_nvcsw"] = uvRusage.ru_nvcsw;

	// Add ru_nivcsw (uint64_t, involuntary context switches).
	jsonObject["ru_nivcsw"] = uvRusage.ru_nivcsw;
}

void Worker::SetNewRouterIdFromInternal(json& internal, std::string& routerId) const
{
	MS_TRACE();

	auto jsonRouterIdIt = internal.find("routerId");

	if (jsonRouterIdIt == internal.end() || !jsonRouterIdIt->is_string())
		MS_THROW_ERROR("missing internal.routerId");

	routerId.assign(jsonRouterIdIt->get<std::string>());

	if (this->mapRouters.find(routerId) != this->mapRouters.end())
		MS_THROW_ERROR("a Router with same routerId already exists");
}

RTC::Router* Worker::GetRouterFromInternal(json& internal) const
{
	MS_TRACE();

	auto jsonRouterIdIt = internal.find("routerId");

	if (jsonRouterIdIt == internal.end() || !jsonRouterIdIt->is_string())
		MS_THROW_ERROR("missing internal.routerId");

	auto it = this->mapRouters.find(jsonRouterIdIt->get<std::string>());

	if (it == this->mapRouters.end())
		MS_THROW_ERROR("Router not found");

	RTC::Router* router = it->second;

	return router;
}

inline void Worker::OnChannelRequest(Channel::UnixStreamSocket* /*channel*/, Channel::Request* request)
{
	MS_TRACE();

	MS_DEBUG_DEV(
	  "Channel request received [method:%s, id:%" PRIu32 "]", request->method.c_str(), request->id);

	switch (request->methodId)
	{
		case Channel::Request::MethodId::WORKER_DUMP:
		{
			json data = json::object();

			FillJson(data);

			request->Accept(data);

			break;
		}

		case Channel::Request::MethodId::WORKER_GET_RESOURCE_USAGE:
		{
			json data = json::object();

			FillJsonResourceUsage(data);

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

			// This may throw.
			SetNewRouterIdFromInternal(request->internal, routerId);

			auto* router = new RTC::Router(routerId);

			this->mapRouters[routerId] = router;

			MS_DEBUG_DEV("Router created [routerId:%s]", routerId.c_str());

			request->Accept();

			break;
		}

		case Channel::Request::MethodId::ROUTER_CLOSE:
		{
			// This may throw.
			RTC::Router* router = GetRouterFromInternal(request->internal);

			// Remove it from the map and delete it.
			this->mapRouters.erase(router->id);
			delete router;

			MS_DEBUG_DEV("Router closed [id:%s]", router->id.c_str());

			request->Accept();

			break;
		}

		// Any other request must be delivered to the corresponding Router.
		default:
		{
			// This may throw.
			RTC::Router* router = GetRouterFromInternal(request->internal);

			router->HandleRequest(request);

			break;
		}
	}
}

inline void Worker::OnChannelClosed(Channel::UnixStreamSocket* /*socket*/)
{
	MS_TRACE_STD();

	// If the pipe is remotely closed it may mean that mediasoup Node process
	// abruptly died (SIGKILL?) so we must die.
	MS_ERROR_STD("channel remotely closed, closing myself");

	Close();
}

inline void Worker::OnPayloadChannelNotification(
  PayloadChannel::UnixStreamSocket* /*payloadChannel*/, PayloadChannel::Notification* notification)
{
	MS_TRACE();

	MS_DEBUG_DEV("PayloadChannel notification received [event:%s]", notification->event.c_str());

	// This may throw.
	RTC::Router* router = GetRouterFromInternal(notification->internal);

	router->HandleNotification(notification);
}

inline void Worker::OnPayloadChannelRequest(
  PayloadChannel::UnixStreamSocket* /*payloadChannel*/, PayloadChannel::Request* request)
{
	MS_TRACE();

	MS_DEBUG_DEV(
	  "PayloadChannel request received [method:%s, id:%" PRIu32 "]",
	  request->method.c_str(),
	  request->id);

	// This may throw.
	RTC::Router* router = GetRouterFromInternal(request->internal);

	router->HandleRequest(request);
}

inline void Worker::OnPayloadChannelClosed(PayloadChannel::UnixStreamSocket* /*payloadChannel*/)
{
	MS_TRACE();

	// If the pipe is remotely closed it may mean that mediasoup Node process
	// abruptly died (SIGKILL?) so we must die.
	MS_ERROR_STD("payloadChannel remotely closed, closing myself");

	Close();
}

inline void Worker::OnSignal(SignalsHandler* /*signalsHandler*/, int signum)
{
	MS_TRACE();

	if (this->closed)
		return;

	switch (signum)
	{
		case SIGINT:
		{
			if (this->closed)
				return;

			MS_DEBUG_DEV("INT signal received, closing myself");

			Close();

			break;
		}

		case SIGTERM:
		{
			if (this->closed)
				return;

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
