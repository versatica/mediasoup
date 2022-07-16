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

	/*
	 * msg request starts with "r:"
	 */
	bool PayloadChannelRequest::IsRequest(const char* msg)
	{
		MS_TRACE();

		return (msg[0] == 'r' && msg[1] == ':');
	}

	/* Instance methods. */

	/**
	 * msg represents:
	 *   * id:method:internal:data
	 * internal represents the internal routing, ie:
	 *   * routerId,transportId,producerId
	 * data representation depends on the target
	 */
	PayloadChannelRequest::PayloadChannelRequest(
	  PayloadChannel::PayloadChannelSocket* channel, char* msg, size_t msgLen)
	  : channel(channel)
	{
		MS_TRACE();

		auto info = Utils::String::Split(std::string(msg, msgLen), ':');

		if (info.size() < 2)
			MS_THROW_ERROR("too few arguments");

		this->id     = std::stoul(info[0]);
		this->method = info[1];

		auto methodIdIt = PayloadChannelRequest::string2MethodId.find(this->method);

		if (methodIdIt == PayloadChannelRequest::string2MethodId.end())
		{
			Error("unknown method");

			MS_THROW_ERROR("unknown method '%s'", this->method.c_str());
		}

		this->methodId = methodIdIt->second;

		if (info.size() > 2)
		{
			auto internal = info[2];

			if (internal != "undefined")
				this->internal = Utils::String::Split(internal, ',');
		}

		if (info.size() > 3)
		{
			auto data = info[3];

			if (data != "undefined")
				this->data = data;
		}
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

		std::string response("{\"id\":");

		response.append(std::to_string(this->id));
		response.append(",\"accepted\":true}");

		this->channel->Send(response);
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

	const std::string& PayloadChannelRequest::GetNextInternalRoutingId()
	{
		MS_TRACE();

		if (this->internal.size() < this->nextRoutingLevel + 1)
		{
			MS_THROW_ERROR("routing id not found for level %" PRIu8, this->nextRoutingLevel);
		}

		return this->internal[this->nextRoutingLevel++];
	}
} // namespace PayloadChannel
