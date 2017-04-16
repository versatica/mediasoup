#define MS_CLASS "Channel::Notifier"
// #define MS_LOG_DEV

#include "Channel/Notifier.hpp"
#include "Logger.hpp"

namespace Channel
{
	/* Instance methods. */

	Notifier::Notifier(Channel::UnixStreamSocket* channel) : channel(channel)
	{
		MS_TRACE();
	}

	void Notifier::Emit(uint32_t targetId, const std::string& event)
	{
		MS_TRACE();

		static const Json::StaticString JsonString_targetId("targetId");
		static const Json::StaticString JsonString_event("event");

		Json::Value json(Json::objectValue);

		json[JsonString_targetId] = (Json::UInt)targetId;
		json[JsonString_event]    = event;

		this->channel->Send(json);
	}

	void Notifier::Emit(uint32_t targetId, const std::string& event, Json::Value& data)
	{
		MS_TRACE();

		static const Json::StaticString JsonString_targetId("targetId");
		static const Json::StaticString JsonString_event("event");
		static const Json::StaticString JsonString_data("data");

		Json::Value json(Json::objectValue);

		json[JsonString_targetId] = (Json::UInt)targetId;
		json[JsonString_event]    = event;
		json[JsonString_data]     = data;

		this->channel->Send(json);
	}

	void Notifier::EmitWithBinary(
	    uint32_t targetId,
	    const std::string& event,
	    Json::Value& data,
	    const uint8_t* binaryData,
	    size_t binaryLen)
	{
		MS_TRACE();

		static const Json::StaticString JsonString_targetId("targetId");
		static const Json::StaticString JsonString_event("event");
		static const Json::StaticString JsonString_data("data");
		static const Json::StaticString JsonString_binary("binary");

		Json::Value json(Json::objectValue);

		json[JsonString_targetId] = (Json::UInt)targetId;
		json[JsonString_event]    = event;
		json[JsonString_data]     = data;
		json[JsonString_binary]   = true;

		this->channel->Send(json);
		this->channel->SendBinary(binaryData, binaryLen);
	}
} // namespace Channel
