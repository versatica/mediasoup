#define MS_CLASS "RTC::DataProducer"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/DataProducer.hpp"
#include "DepLibUV.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "Utils.hpp"
#include <stdexcept>

namespace RTC
{
	/* Instance methods. */

	DataProducer::DataProducer(
	  RTC::Shared* shared,
	  const std::string& id,
	  size_t maxMessageSize,
	  RTC::DataProducer::Listener* listener,
	  json& data)
	  : id(id), shared(shared), maxMessageSize(maxMessageSize), listener(listener)
	{
		MS_TRACE();

		auto jsonTypeIt                 = data.find("type");
		auto jsonSctpStreamParametersIt = data.find("sctpStreamParameters");
		auto jsonLabelIt                = data.find("label");
		auto jsonProtocolIt             = data.find("protocol");

		if (jsonTypeIt == data.end() || !jsonTypeIt->is_string())
			MS_THROW_TYPE_ERROR("missing type");

		this->typeString = jsonTypeIt->get<std::string>();

		if (this->typeString == "sctp")
			this->type = DataProducer::Type::SCTP;
		else if (this->typeString == "direct")
			this->type = DataProducer::Type::DIRECT;
		else
			MS_THROW_TYPE_ERROR("invalid type");

		if (this->type == DataProducer::Type::SCTP)
		{
			// clang-format off
			if (
				jsonSctpStreamParametersIt == data.end() ||
				!jsonSctpStreamParametersIt->is_object()
			)
			// clang-format on
			{
				MS_THROW_TYPE_ERROR("missing sctpStreamParameters");
			}

			// This may throw.
			this->sctpStreamParameters = RTC::SctpStreamParameters(*jsonSctpStreamParametersIt);
		}

		if (jsonLabelIt != data.end() && jsonLabelIt->is_string())
			this->label = jsonLabelIt->get<std::string>();

		if (jsonProtocolIt != data.end() && jsonProtocolIt->is_string())
			this->protocol = jsonProtocolIt->get<std::string>();

		// NOTE: This may throw.
		this->shared->channelMessageRegistrator->RegisterHandler(
		  this->id,
		  /*channelRequestHandler*/ this,
		  /*payloadChannelRequestHandler*/ nullptr,
		  /*payloadChannelNotificationHandler*/ this);
	}

	DataProducer::~DataProducer()
	{
		MS_TRACE();

		this->shared->channelMessageRegistrator->UnregisterHandler(this->id);
	}

	void DataProducer::FillJson(json& jsonObject) const
	{
		MS_TRACE();

		// Add id.
		jsonObject["id"] = this->id;

		// Add type.
		jsonObject["type"] = this->typeString;

		// Add sctpStreamParameters.
		if (this->type == DataProducer::Type::SCTP)
		{
			this->sctpStreamParameters.FillJson(jsonObject["sctpStreamParameters"]);
		}

		// Add label.
		jsonObject["label"] = this->label;

		// Add protocol.
		jsonObject["protocol"] = this->protocol;
	}

	void DataProducer::FillJsonStats(json& jsonArray) const
	{
		MS_TRACE();

		jsonArray.emplace_back(json::value_t::object);
		auto& jsonObject = jsonArray[0];

		// Add type.
		jsonObject["type"] = "data-producer";

		// Add timestamp.
		jsonObject["timestamp"] = DepLibUV::GetTimeMs();

		// Add label.
		jsonObject["label"] = this->label;

		// Add protocol.
		jsonObject["protocol"] = this->protocol;

		// Add messagesReceived.
		jsonObject["messagesReceived"] = this->messagesReceived;

		// Add bytesReceived.
		jsonObject["bytesReceived"] = this->bytesReceived;
	}

	void DataProducer::HandleRequest(Channel::ChannelRequest* request)
	{
		MS_TRACE();

		switch (request->methodId)
		{
			case Channel::ChannelRequest::MethodId::DATA_PRODUCER_DUMP:
			{
				json data = json::object();

				FillJson(data);

				request->Accept(data);

				break;
			}

			case Channel::ChannelRequest::MethodId::DATA_PRODUCER_GET_STATS:
			{
				json data = json::array();

				FillJsonStats(data);

				request->Accept(data);

				break;
			}

			default:
			{
				MS_THROW_ERROR("unknown method '%s'", request->method.c_str());
			}
		}
	}

	void DataProducer::HandleNotification(PayloadChannel::PayloadChannelNotification* notification)
	{
		MS_TRACE();

		switch (notification->eventId)
		{
			case PayloadChannel::PayloadChannelNotification::EventId::DATA_PRODUCER_SEND:
			{
				int ppid;

				// This may throw.
				// NOTE: If this throws we have to catch the error and throw a MediaSoupError
				// intead, otherwise the process would crash.
				try
				{
					ppid = std::stoi(notification->data);
				}
				catch (const std::exception& error)
				{
					MS_THROW_TYPE_ERROR("invalid PPID value: %s", error.what());
				}

				const auto* msg = notification->payload;
				auto len        = notification->payloadLen;

				if (len > this->maxMessageSize)
				{
					MS_THROW_TYPE_ERROR(
					  "given message exceeds maxMessageSize value [maxMessageSize:%zu, len:%zu]",
					  len,
					  this->maxMessageSize);
				}

				this->ReceiveMessage(ppid, msg, len);

				// Increase receive transmission.
				this->listener->OnDataProducerReceiveData(this, len);

				break;
			}

			default:
			{
				MS_ERROR("unknown event '%s'", notification->event.c_str());
			}
		}
	}

	void DataProducer::ReceiveMessage(uint32_t ppid, const uint8_t* msg, size_t len)
	{
		MS_TRACE();

		this->messagesReceived++;
		this->bytesReceived += len;

		this->listener->OnDataProducerMessageReceived(this, ppid, msg, len);
	}
} // namespace RTC
