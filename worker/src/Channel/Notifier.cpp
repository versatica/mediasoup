#define MS_CLASS "Channel::Notifier"
// #define MS_LOG_DEV

#include "Channel/Notifier.hpp"
#include "Logger.hpp"

namespace Channel
{
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

	void Notifier::Emit(uint32_t targetId, std::string event)
	{
		MS_TRACE();

		static const Json::StaticString k_targetId("targetId");
		static const Json::StaticString k_event("event");

		Json::Value json(Json::objectValue);

		json[k_targetId] = (Json::UInt)targetId;
		json[k_event] = event;

		this->channel->Send(json);
	}

	void Notifier::Emit(uint32_t targetId, std::string event, Json::Value &data)
	{
		MS_TRACE();

		static const Json::StaticString k_targetId("targetId");
		static const Json::StaticString k_event("event");
		static const Json::StaticString k_data("data");

		Json::Value json(Json::objectValue);

		json[k_targetId] = (Json::UInt)targetId;
		json[k_event] = event;
		json[k_data] = data;

		this->channel->Send(json);
	}

	void Notifier::EmitWithBinary(uint32_t targetId, std::string event, Json::Value& data, const uint8_t* binary_data, size_t binary_len)
	{
		MS_TRACE();

		static const Json::StaticString k_targetId("targetId");
		static const Json::StaticString k_event("event");
		static const Json::StaticString k_data("data");
		static const Json::StaticString k_binary("binary");

		Json::Value json(Json::objectValue);

		json[k_targetId] = (Json::UInt)targetId;
		json[k_event] = event;
		json[k_data] = data;
		json[k_binary] = true;

		this->channel->Send(json);
		this->channel->SendBinary(binary_data, binary_len);
	}
}
