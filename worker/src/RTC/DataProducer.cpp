#define MS_CLASS "RTC::DataProducer"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/DataProducer.hpp"
#include "DepLibUV.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "Utils.hpp"

namespace RTC
{
	/* Instance methods. */

	DataProducer::DataProducer(const std::string& id, RTC::DataProducer::Listener* listener, json& data)
	  : id(id), listener(listener)
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
	}

	DataProducer::~DataProducer()
	{
		MS_TRACE();
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

	void DataProducer::HandleRequest(Channel::ChannelRequest* request) const
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

	void DataProducer::ReceiveMessage(uint32_t ppid, const uint8_t* msg, size_t len)
	{
		MS_TRACE();

		this->messagesReceived++;
		this->bytesReceived += len;

		this->listener->OnDataProducerMessageReceived(this, ppid, msg, len);
	}
} // namespace RTC
