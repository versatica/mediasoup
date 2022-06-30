#define MS_CLASS "PayloadChannel::PayloadChannelRequest"
// #define MS_LOG_DEV_LEVEL 3

#include "PayloadChannel/PayloadChannelRequest.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "Utils.hpp"
#include "PayloadChannel/PayloadChannelSocket.hpp"

namespace PayloadChannel
{
	/* Class variables. */

	// clang-format off
	absl::flat_hash_map<std::string, PayloadChannelRequest::MethodId> PayloadChannelRequest::string2MethodId =
	{
		{ "dataConsumer.send", PayloadChannelRequest::MethodId::DATA_CONSUMER_SEND },
	};
	// clang-format on

	/* Class methods. */
	bool PayloadChannelRequest::IsRequest(json& jsonRequest)
	{
		MS_TRACE();

		auto jsonIdIt = jsonRequest.find("id");

		if (jsonIdIt == jsonRequest.end() || !Utils::Json::IsPositiveInteger(*jsonIdIt))
			return false;

		auto jsonMethodIt = jsonRequest.find("method");

		if (jsonMethodIt == jsonRequest.end() || !jsonMethodIt->is_string())
			return false;

		return true;
	}

	/* Instance methods. */

	PayloadChannelRequest::PayloadChannelRequest(
	  PayloadChannel::PayloadChannelSocket* channel, json& jsonRequest)
	  : channel(channel)
	{
		MS_TRACE();

		auto jsonIdIt = jsonRequest.find("id");

		if (jsonIdIt == jsonRequest.end() || !Utils::Json::IsPositiveInteger(*jsonIdIt))
			MS_THROW_ERROR("missing id");

		this->id = jsonIdIt->get<uint32_t>();

		auto jsonMethodIt = jsonRequest.find("method");

		if (jsonMethodIt == jsonRequest.end() || !jsonMethodIt->is_string())
			MS_THROW_ERROR("missing method");

		this->method = jsonMethodIt->get<std::string>();

		auto methodIdIt = PayloadChannelRequest::string2MethodId.find(this->method);

		if (methodIdIt == PayloadChannelRequest::string2MethodId.end())
		{
			Error("unknown method");

			MS_THROW_ERROR("unknown method '%s'", this->method.c_str());
		}

		this->methodId = methodIdIt->second;

		auto jsonInternalIt = jsonRequest.find("internal");

		if (jsonInternalIt != jsonRequest.end() && jsonInternalIt->is_object())
			this->internal = *jsonInternalIt;
		else
			this->internal = json::object();

		auto jsonDataIt = jsonRequest.find("data");

		if (jsonDataIt != jsonRequest.end() && jsonDataIt->is_object())
			this->data = *jsonDataIt;
		else
			this->data = json::object();
	}

	PayloadChannelRequest::~PayloadChannelRequest()
	{
		MS_TRACE();
	}

	void PayloadChannelRequest::Accept()
	{
		MS_TRACE();

		MS_ASSERT(!this->replied, "request already replied");

		this->replied = true;

		json jsonResponse = json::object();

		jsonResponse["id"]       = this->id;
		jsonResponse["accepted"] = true;

		this->channel->Send(jsonResponse);
	}

	void PayloadChannelRequest::Accept(json& data)
	{
		MS_TRACE();

		MS_ASSERT(!this->replied, "request already replied");

		this->replied = true;

		json jsonResponse = json::object();

		jsonResponse["id"]       = this->id;
		jsonResponse["accepted"] = true;

		if (data.is_structured())
			jsonResponse["data"] = data;

		this->channel->Send(jsonResponse);
	}

	void PayloadChannelRequest::Error(const char* reason)
	{
		MS_TRACE();

		MS_ASSERT(!this->replied, "request already replied");

		this->replied = true;

		json jsonResponse = json::object();

		jsonResponse["id"]    = this->id;
		jsonResponse["error"] = "Error";

		if (reason != nullptr)
			jsonResponse["reason"] = reason;

		this->channel->Send(jsonResponse);
	}

	void PayloadChannelRequest::TypeError(const char* reason)
	{
		MS_TRACE();

		MS_ASSERT(!this->replied, "request already replied");

		this->replied = true;

		json jsonResponse = json::object();

		jsonResponse["id"]    = this->id;
		jsonResponse["error"] = "TypeError";

		if (reason != nullptr)
			jsonResponse["reason"] = reason;

		this->channel->Send(jsonResponse);
	}

	void PayloadChannelRequest::SetPayload(const uint8_t* payload, size_t payloadLen)
	{
		MS_TRACE();

		this->payload    = payload;
		this->payloadLen = payloadLen;
	}
} // namespace PayloadChannel
