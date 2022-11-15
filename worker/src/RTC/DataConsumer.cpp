#define MS_CLASS "RTC::DataConsumer"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/DataConsumer.hpp"
#include "DepLibUV.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "Utils.hpp"
#include "RTC/SctpAssociation.hpp"
#include <stdexcept>

namespace RTC
{
	/* Instance methods. */

	DataConsumer::DataConsumer(
	  RTC::Shared* shared,
	  const std::string& id,
	  const std::string& dataProducerId,
	  RTC::SctpAssociation* sctpAssociation,
	  RTC::DataConsumer::Listener* listener,
	  json& data,
	  size_t maxMessageSize)
	  : id(id), dataProducerId(dataProducerId), shared(shared), sctpAssociation(sctpAssociation),
	    listener(listener), maxMessageSize(maxMessageSize)
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

		// NOTE: This may throw.
		this->shared->channelMessageRegistrator->RegisterHandler(
		  this->id,
		  /*channelRequestHandler*/ this,
		  /*payloadChannelRequestHandler*/ this,
		  /*payloadChannelNotificationHandler*/ nullptr);
	}

	DataConsumer::~DataConsumer()
	{
		MS_TRACE();

		this->shared->channelMessageRegistrator->UnregisterHandler(this->id);
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

			case Channel::ChannelRequest::MethodId::DATA_CONSUMER_GET_BUFFERED_AMOUNT:
			{
				if (this->GetType() != RTC::DataConsumer::Type::SCTP)
				{
					MS_THROW_TYPE_ERROR("invalid DataConsumer type");
				}

				if (!this->sctpAssociation)
				{
					MS_THROW_ERROR("no SCTP association present");
				}

				// Create status response.
				json data = json::object();

				data["bufferedAmount"] = this->sctpAssociation->GetSctpBufferedAmount();

				request->Accept(data);

				break;
			}

			case Channel::ChannelRequest::MethodId::DATA_CONSUMER_SET_BUFFERED_AMOUNT_LOW_THRESHOLD:
			{
				if (this->GetType() != DataConsumer::Type::SCTP)
				{
					MS_THROW_TYPE_ERROR("invalid DataConsumer type");
				}

				auto jsonThresholdIt = request->data.find("threshold");

				if (jsonThresholdIt == request->data.end() || !jsonThresholdIt->is_number_unsigned())
					MS_THROW_TYPE_ERROR("wrong bufferedAmountThreshold (not an unsigned number)");

				this->bufferedAmountLowThreshold = jsonThresholdIt->get<uint32_t>();

				request->Accept();

				// There is less or same buffered data than the given threshold.
				// Trigger 'bufferedamountlow' now.
				if (this->bufferedAmount <= this->bufferedAmountLowThreshold)
				{
					std::string data(R"({"bufferedAmount":)");

					data.append(std::to_string(this->bufferedAmount));
					data.append("}");
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
				if (this->GetType() != RTC::DataConsumer::Type::SCTP)
				{
					MS_THROW_TYPE_ERROR("invalid DataConsumer type");
				}

				if (!this->sctpAssociation)
				{
					MS_THROW_ERROR("no SCTP association present");
				}

				int ppid;

				// This may throw.
				// NOTE: If this throws we have to catch the error and throw a MediaSoupError
				// intead, otherwise the process would crash.
				try
				{
					ppid = std::stoi(request->data);
				}
				catch (const std::exception& error)
				{
					MS_THROW_TYPE_ERROR("invalid PPID value: %s", error.what());
				}

				const auto* msg = request->payload;
				auto len        = request->payloadLen;

				if (len > this->maxMessageSize)
				{
					MS_THROW_TYPE_ERROR(
					  "given message exceeds maxMessageSize value [maxMessageSize:%zu, len:%zu]",
					  len,
					  this->maxMessageSize);
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
			std::string data(R"({"bufferedAmount":)");

			data.append(std::to_string(this->bufferedAmount));
			data.append("}");

			this->shared->channelNotifier->Emit(this->id, "bufferedamountlow", data);
		}
	}

	void DataConsumer::SctpAssociationSendBufferFull()
	{
		MS_TRACE();

		this->shared->channelNotifier->Emit(this->id, "sctpsendbufferfull");
	}

	// The caller (Router) is supposed to proceed with the deletion of this DataConsumer
	// right after calling this method. Otherwise ugly things may happen.
	void DataConsumer::DataProducerClosed()
	{
		MS_TRACE();

		this->dataProducerClosed = true;

		MS_DEBUG_DEV("DataProducer closed [dataConsumerId:%s]", this->id.c_str());

		this->shared->channelNotifier->Emit(this->id, "dataproducerclose");

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
