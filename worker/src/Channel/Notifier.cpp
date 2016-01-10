#define MS_CLASS "Channel::Notifier"

#include "Channel/Notifier.h"
#include "Logger.h"

namespace Channel
{
	/* Static variables. */

	static Json::Value empty_data(Json::objectValue);

	/* Instance methods. */

	Notifier::Notifier(Channel::UnixStreamSocket* channel) :
		channel(channel)
	{
		MS_TRACE();
	}

	Notifier::~Notifier()
	{
		MS_TRACE();
	}

	void Notifier::Close()
	{
		MS_TRACE();

		delete this;
	}

	void Notifier::Emit(uint32_t targetId, std::string eventName)
	{
		MS_TRACE();

		Emit(targetId, eventName, empty_data);
	}

	void Notifier::Emit(uint32_t targetId, std::string eventName, Json::Value &eventData)
	{
		MS_TRACE();

		static const Json::StaticString k_targetId("targetId");
		static const Json::StaticString k_eventName("eventName");
		static const Json::StaticString k_eventData("eventData");

		Json::Value json;

		json[k_targetId] = targetId;
		json[k_eventName] = eventName;

		if (eventData.isObject())
			json[k_eventData] = eventData;
		else
			json[k_eventData] = empty_data;

		this->channel->Send(json);
	}
}
