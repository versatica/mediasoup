#define MS_CLASS "RTC::DataConsumer"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/DataConsumer.hpp"
#include "ChannelMessageHandlers.hpp"
#include "DepLibUV.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "Utils.hpp"
#include "Channel/ChannelNotifier.hpp"
#include "RTC/SctpAssociation.hpp"
#include <stdexcept>

namespace RTC
{
	/* Instance methods. */

	DataConsumer::DataConsumer(
	  const std::string& id,
	  const std::string& dataProducerId,
	  RTC::SctpAssociation* sctpAssociation,
	  RTC::DataConsumer::Listener* listener,
	  const FBS::Transport::ConsumeDataRequest* data,
	  size_t maxMessageSize)
	  : id(id), dataProducerId(dataProducerId), sctpAssociation(sctpAssociation), listener(listener),
	    maxMessageSize(maxMessageSize)
	{
		MS_TRACE();

		if (!flatbuffers::IsFieldPresent(data, FBS::Transport::ConsumeDataRequest::VT_TYPE))
			MS_THROW_TYPE_ERROR("missing type");

		this->typeString = data->type()->str();

		if (this->typeString == "sctp")
			this->type = DataConsumer::Type::SCTP;
		else if (this->typeString == "direct")
			this->type = DataConsumer::Type::DIRECT;
		else
			MS_THROW_TYPE_ERROR("invalid type");

		if (this->type == DataConsumer::Type::SCTP)
		{
			if (!flatbuffers::IsFieldPresent(
			      data, FBS::Transport::ConsumeDataRequest::VT_SCTPSTREAMPARAMETERS))
			{
				MS_THROW_TYPE_ERROR("missing sctpStreamParameters");
			}

			// This may throw.
			this->sctpStreamParameters = RTC::SctpStreamParameters(data->sctpStreamParameters());
		}

		if (flatbuffers::IsFieldPresent(data, FBS::Transport::ConsumeDataRequest::VT_LABEL))
			this->label = data->label()->str();

		if (flatbuffers::IsFieldPresent(data, FBS::Transport::ConsumeDataRequest::VT_PROTOCOL))
			this->protocol = data->protocol()->str();

		// NOTE: This may throw.
		ChannelMessageHandlers::RegisterHandler(
		  this->id,
		  /*channelRequestHandler*/ this,
		  /*payloadChannelRequestHandler*/ this,
		  /*payloadChannelNotificationHandler*/ nullptr);
	}

	DataConsumer::~DataConsumer()
	{
		MS_TRACE();

		ChannelMessageHandlers::UnregisterHandler(this->id);
	}

	flatbuffers::Offset<FBS::DataConsumer::DumpResponse> DataConsumer::FillBuffer(
	  flatbuffers::FlatBufferBuilder& builder) const
	{
		MS_TRACE();

		flatbuffers::Offset<FBS::SctpParameters::SctpStreamParameters> sctpStreamParametersOffset;

		// Add sctpStreamParameters.
		if (this->type == DataConsumer::Type::SCTP)
		{
			sctpStreamParametersOffset = this->sctpStreamParameters.FillBuffer(builder);
		}

		return FBS::DataConsumer::CreateDumpResponseDirect(
		  builder,
		  this->id.c_str(),
		  this->dataProducerId.c_str(),
		  this->typeString.c_str(),
		  sctpStreamParametersOffset,
		  this->label.c_str(),
		  this->protocol.c_str());
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

		switch (request->method)
		{
			case Channel::ChannelRequest::Method::DATA_CONSUMER_DUMP:
			{
				auto dumpOffset = FillBuffer(request->GetBufferBuilder());

				request->Accept(FBS::Response::Body::FBS_DataConsumer_DumpResponse, dumpOffset);

				break;
			}

			case Channel::ChannelRequest::Method::DATA_CONSUMER_GET_STATS:
			{
				json data = json::array();

				FillJsonStats(data);

				request->Accept(data);

				break;
			}

			case Channel::ChannelRequest::Method::DATA_CONSUMER_GET_BUFFERED_AMOUNT:
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
				auto responseOffset = FBS::DataConsumer::CreateGetBufferedAmountResponse(
				  request->GetBufferBuilder(), this->sctpAssociation->GetSctpBufferedAmount());

				request->Accept(
				  FBS::Response::Body::FBS_DataConsumer_GetBufferedAmountResponse, responseOffset);

				break;
			}

			case Channel::ChannelRequest::Method::DATA_CONSUMER_SET_BUFFERED_AMOUNT_LOW_THRESHOLD:
			{
				if (this->GetType() != DataConsumer::Type::SCTP)
				{
					MS_THROW_TYPE_ERROR("invalid DataConsumer type");
				}

				auto body =
				  request->_data->body_as<FBS::DataConsumer::SetBufferedAmountLowThresholdRequest>();

				this->bufferedAmountLowThreshold = body->threshold();

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
				MS_THROW_ERROR("unknown method '%s'", request->methodStr.c_str());
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
