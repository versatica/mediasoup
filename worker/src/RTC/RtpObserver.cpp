#define MS_CLASS "RTC::RtpObserver"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/RtpObserver.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"

namespace RTC
{
	/* Instance methods. */

	RtpObserver::RtpObserver(RTC::Shared* shared, const std::string& id, RTC::RtpObserver::Listener* listener)
	  : id(id), shared(shared), listener(listener)
	{
		MS_TRACE();
	}

	RtpObserver::~RtpObserver()
	{
		MS_TRACE();
	}

	void RtpObserver::HandleRequest(Channel::ChannelRequest* request)
	{
		MS_TRACE();

		switch (request->methodId)
		{
			case Channel::ChannelRequest::MethodId::RTP_OBSERVER_PAUSE:
			{
				this->Pause();

				request->Accept();

				break;
			}

			case Channel::ChannelRequest::MethodId::RTP_OBSERVER_RESUME:
			{
				this->Resume();

				request->Accept();

				break;
			}

			case Channel::ChannelRequest::MethodId::RTP_OBSERVER_ADD_PRODUCER:
			{
				// This may throw.
				auto producerId         = GetProducerIdFromData(request->data);
				RTC::Producer* producer = this->listener->RtpObserverGetProducer(this, producerId);

				this->AddProducer(producer);

				this->listener->OnRtpObserverAddProducer(this, producer);

				request->Accept();

				break;
			}

			case Channel::ChannelRequest::MethodId::RTP_OBSERVER_REMOVE_PRODUCER:
			{
				// This may throw.
				auto producerId         = GetProducerIdFromData(request->data);
				RTC::Producer* producer = this->listener->RtpObserverGetProducer(this, producerId);

				this->RemoveProducer(producer);

				// Remove from the map.
				this->listener->OnRtpObserverRemoveProducer(this, producer);

				request->Accept();

				break;
			}

			default:
			{
				MS_THROW_ERROR("unknown method '%s'", request->method.c_str());
			}
		}
	}

	void RtpObserver::Pause()
	{
		MS_TRACE();

		if (this->paused)
			return;

		this->paused = true;

		Paused();
	}

	void RtpObserver::Resume()
	{
		MS_TRACE();

		if (!this->paused)
			return;

		this->paused = false;

		Resumed();
	}

	std::string RtpObserver::GetProducerIdFromData(json& data) const
	{
		MS_TRACE();

		auto jsonRouterIdIt = data.find("producerId");

		if (jsonRouterIdIt == data.end() || !jsonRouterIdIt->is_string())
			MS_THROW_ERROR("missing data.producerId");

		return jsonRouterIdIt->get<std::string>();
	}
} // namespace RTC
