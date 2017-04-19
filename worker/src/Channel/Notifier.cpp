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

		static const Json::StaticString JsonStringTargetId{ "targetId" };
		static const Json::StaticString JsonStringEvent{ "event" };

		Json::Value json(Json::objectValue);

		json[JsonStringTargetId] = Json::UInt{ targetId };
		json[JsonStringEvent]    = event;

		this->channel->Send(json);
	}

	void Notifier::Emit(uint32_t targetId, const std::string& event, Json::Value& data)
	{
		MS_TRACE();

		static const Json::StaticString JsonStringTargetId{ "targetId" };
		static const Json::StaticString JsonStringEvent{ "event" };
		static const Json::StaticString JsonStringData{ "data" };

		Json::Value json(Json::objectValue);

		json[JsonStringTargetId] = Json::UInt{ targetId };
		json[JsonStringEvent]    = event;
		json[JsonStringData]     = data;

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

		static const Json::StaticString JsonStringTargetId{ "targetId" };
		static const Json::StaticString JsonStringEvent{ "event" };
		static const Json::StaticString JsonStringData{ "data" };
		static const Json::StaticString JsonStringBinary{ "binary" };

		Json::Value json(Json::objectValue);

		json[JsonStringTargetId] = Json::UInt{ targetId };
		json[JsonStringEvent]    = event;
		json[JsonStringData]     = data;
		json[JsonStringBinary]   = true;

		this->channel->Send(json);
		this->channel->SendBinary(binaryData, binaryLen);
	}
} // namespace Channel
