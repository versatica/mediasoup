#define MS_CLASS "RTC::DataConsumer"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/DataConsumer.hpp"
#include "DepLibUV.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "Utils.hpp"
#include "Channel/ChannelNotifier.hpp"

namespace RTC
{
	/* Instance methods. */

	DataConsumer::DataConsumer(
	  const std::string& id,
	  const std::string& dataProducerId,
	  RTC::DataConsumer::Listener* listener,
	  json& data,
	  size_t maxMessageSize)
	  : id(id), dataProducerId(dataProducerId), listener(listener), maxMessageSize(maxMessageSize)
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
			this->type = DataConsumer::Type::SCTP;
		else if (this->typeString == "direct")
			this->type = DataConsumer::Type::DIRECT;
		else
			MS_THROW_TYPE_ERROR("invalid type");

		if (this->type == DataConsumer::Type::SCTP)
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

	DataConsumer::~DataConsumer()
	{
		MS_TRACE();
	}

	void DataConsumer::FillJson(json& jsonObject) const
	{
		MS_TRACE();

		// Add id.
		jsonObject["id"] = this->id;

		// Add type.
		jsonObject["type"] = this->typeString;

		// Add dataProducerId.
		jsonObject["dataProducerId"] = this->dataProducerId;

		// Add sctpStreamParameters.
		if (this->type == DataConsumer::Type::SCTP)
		{
			this->sctpStreamParameters.FillJson(jsonObject["sctpStreamParameters"]);
		}

		// Add label.
		jsonObject["label"] = this->label;

		// Add protocol.
		jsonObject["protocol"] = this->protocol;

		// Add bufferedAmountLowThreshold.
		jsonObject["bufferedAmountLowThreshold"] = this->bufferedAmountLowThreshold;
	}

	void DataConsumer::FillJsonStats(json& jsonArray) const
	{
		MS_TRACE();

		jsonArray.emplace_back(json::value_t::object);
		auto& jsonObject = jsonArray[0];

		// Add type.
		jsonObject["type"] = "data-consumer";

		// Add timestamp.
		jsonObject["timestamp"] = DepLibUV::GetTimeMs();

		// Add label.
		jsonObject["label"] = this->label;

		// Add protocol.
		jsonObject["protocol"] = this->protocol;

		// Add messagesSent.
		jsonObject["messagesSent"] = this->messagesSent;

		// Add bytesSent.
		jsonObject["bytesSent"] = this->bytesSent;

		// Add bufferedAmount.
		jsonObject["bufferedAmount"] = this->bufferedAmount;
	}

	void DataConsumer::HandleRequest(Channel::ChannelRequest* request)
	{
		MS_TRACE();

		switch (request->methodId)
		{
			case Channel::ChannelRequest::MethodId::DATA_CONSUMER_DUMP:
			{
				json data = json::object();

				FillJson(data);

				request->Accept(data);

				break;
			}

			case Channel::ChannelRequest::MethodId::DATA_CONSUMER_GET_STATS:
			{
				json data = json::array();

				FillJsonStats(data);

				request->Accept(data);

				break;
			}

			case Channel::ChannelRequest::MethodId::DATA_CONSUMER_SET_BUFFERED_AMOUNT_LOW_THRESHOLD:
			{
				auto jsonThresholdIt = request->data.find("threshold");

				if (jsonThresholdIt == request->data.end() || !jsonThresholdIt->is_number_unsigned())
					MS_THROW_TYPE_ERROR("wrong bufferedAmountThreshold (not an unsigned number)");

				this->bufferedAmountLowThreshold = jsonThresholdIt->get<uint32_t>();

				request->Accept();

				// There is less or same buffered data than the given threshold.
				// Trigger 'bufferedamountlow' now.
				if (this->bufferedAmount <= this->bufferedAmountLowThreshold)
				{
					// Notify the Node DataConsumer.
					json data = json::object();

					data["bufferedAmount"] = this->bufferedAmount;

					Channel::ChannelNotifier::Emit(this->id, "bufferedamountlow", data);
				}
				// Force the trigger of 'bufferedamountlow' once there is less or same
				// buffered data than the given threshold.
				else
				{
					this->forceTriggerBufferedAmountLow = true;
				}

				break;
			}

			default:
			{
				MS_THROW_ERROR("unknown method '%s'", request->method.c_str());
			}
		}
	}

	void DataConsumer::HandleRequest(PayloadChannel::PayloadChannelRequest* request)
	{
		MS_TRACE();

		switch (request->methodId)
		{
			case PayloadChannel::PayloadChannelRequest::MethodId::DATA_CONSUMER_SEND:
			{
				auto jsonPpidIt = request->data.find("ppid");

				if (jsonPpidIt == request->data.end() || !Utils::Json::IsPositiveInteger(*jsonPpidIt))
				{
					MS_THROW_TYPE_ERROR("invalid ppid");
				}

				auto ppid       = jsonPpidIt->get<uint32_t>();
				const auto* msg = request->payload;
				auto len        = request->payloadLen;

				if (len > this->maxMessageSize)
				{
					MS_WARN_TAG(
					  message,
					  "given message exceeds maxMessageSize value [maxMessageSize:%zu, len:%zu]",
					  len,
					  this->maxMessageSize);

					return;
				}

				const auto* cb = new onQueuedCallback(
				  [&request](bool queued, bool sctpSendBufferFull)
				  {
					  if (queued)
						  request->Accept();
					  else
						  request->Error(
						    sctpSendBufferFull == true ? "sctpsendbufferfull" : "message send failed");
				  });

				SendMessage(ppid, msg, len, cb);

				break;
			}

			default:
			{
				MS_THROW_ERROR("unknown method '%s'", request->method.c_str());
			}
		}
	}

	void DataConsumer::TransportConnected()
	{
		MS_TRACE();

		this->transportConnected = true;

		MS_DEBUG_DEV("Transport connected [dataConsumerId:%s]", this->id.c_str());
	}

	void DataConsumer::TransportDisconnected()
	{
		MS_TRACE();

		this->transportConnected = false;

		MS_DEBUG_DEV("Transport disconnected [dataConsumerId:%s]", this->id.c_str());
	}

	void DataConsumer::SctpAssociationConnected()
	{
		MS_TRACE();

		this->sctpAssociationConnected = true;

		MS_DEBUG_DEV("SctpAssociation connected [dataConsumerId:%s]", this->id.c_str());
	}

	void DataConsumer::SctpAssociationClosed()
	{
		MS_TRACE();

		this->sctpAssociationConnected = false;

		MS_DEBUG_DEV("SctpAssociation closed [dataConsumerId:%s]", this->id.c_str());
	}

	void DataConsumer::SctpAssociationBufferedAmount(uint32_t bufferedAmount)
	{
		MS_TRACE();

		auto previousBufferedAmount = this->bufferedAmount;

		this->bufferedAmount = bufferedAmount;

		// clang-format off
		if (
				(this->forceTriggerBufferedAmountLow || previousBufferedAmount > this->bufferedAmountLowThreshold) &&
				this->bufferedAmount <= this->bufferedAmountLowThreshold
		)
		// clang-format on
		{
			this->forceTriggerBufferedAmountLow = false;

			// Notify the Node DataConsumer.
			json data = json::object();

			data["bufferedAmount"] = this->bufferedAmount;

			Channel::ChannelNotifier::Emit(this->id, "bufferedamountlow", data);
		}
	}

	// The caller (Router) is supposed to proceed with the deletion of this DataConsumer
	// right after calling this method. Otherwise ugly things may happen.
	void DataConsumer::DataProducerClosed()
	{
		MS_TRACE();

		this->dataProducerClosed = true;

		MS_DEBUG_DEV("DataProducer closed [dataConsumerId:%s]", this->id.c_str());

		Channel::ChannelNotifier::Emit(this->id, "dataproducerclose");

		this->listener->OnDataConsumerDataProducerClosed(this);
	}

	void DataConsumer::SendMessage(uint32_t ppid, const uint8_t* msg, size_t len, onQueuedCallback* cb)
	{
		MS_TRACE();

		if (!IsActive())
			return;

		if (len > this->maxMessageSize)
		{
			MS_WARN_TAG(
			  message,
			  "given message exceeds maxMessageSize value [maxMessageSize:%zu, len:%zu]",
			  len,
			  this->maxMessageSize);

			return;
		}

		this->messagesSent++;
		this->bytesSent += len;

		this->listener->OnDataConsumerSendMessage(this, ppid, msg, len, cb);
	}
} // namespace RTC
