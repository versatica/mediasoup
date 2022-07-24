#define MS_CLASS "RTC::RtpObserver"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/RtpObserver.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"

namespace RTC
{
	/* Instance methods. */

	RtpObserver::RtpObserver(const std::string& id) : id(id)
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
} // namespace RTC
