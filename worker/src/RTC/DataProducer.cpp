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
	  const FBS::Transport::ProduceDataRequest* data)
	  : id(id), shared(shared), maxMessageSize(maxMessageSize), listener(listener)
	{
		MS_TRACE();

		if (!flatbuffers::IsFieldPresent(data, FBS::Transport::ProduceDataRequest::VT_TYPE))
			MS_THROW_TYPE_ERROR("missing type");

		this->typeString = data->type()->str();

		if (this->typeString == "sctp")
			this->type = DataProducer::Type::SCTP;
		else if (this->typeString == "direct")
			this->type = DataProducer::Type::DIRECT;
		else
			MS_THROW_TYPE_ERROR("invalid type");

		if (this->type == DataProducer::Type::SCTP)
		{
			if (!flatbuffers::IsFieldPresent(
			      data, FBS::Transport::ProduceDataRequest::VT_SCTPSTREAMPARAMETERS))
			{
				MS_THROW_TYPE_ERROR("missing sctpStreamParameters");
			}

			// This may throw.
			this->sctpStreamParameters = RTC::SctpStreamParameters(data->sctpStreamParameters());
		}

		if (flatbuffers::IsFieldPresent(data, FBS::Transport::ProduceDataRequest::VT_LABEL))
			this->label = data->label()->str();

		if (flatbuffers::IsFieldPresent(data, FBS::Transport::ProduceDataRequest::VT_PROTOCOL))
			this->protocol = data->protocol()->str();

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

	flatbuffers::Offset<FBS::DataProducer::DumpResponse> DataProducer::FillBuffer(
	  flatbuffers::FlatBufferBuilder& builder) const
	{
		MS_TRACE();

		flatbuffers::Offset<FBS::SctpParameters::SctpStreamParameters> sctpStreamParametersOffset;

		// Add sctpStreamParameters.
		if (this->type == DataProducer::Type::SCTP)
		{
			sctpStreamParametersOffset = this->sctpStreamParameters.FillBuffer(builder);
		}

		return FBS::DataProducer::CreateDumpResponseDirect(
		  builder,
		  this->id.c_str(),
		  this->typeString.c_str(),
		  sctpStreamParametersOffset,
		  this->label.c_str(),
		  this->protocol.c_str());
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

		switch (request->method)
		{
			case Channel::ChannelRequest::Method::DATA_PRODUCER_DUMP:
			{
				auto dumpOffset = FillBuffer(request->GetBufferBuilder());

				request->Accept(FBS::Response::Body::FBS_DataProducer_DumpResponse, dumpOffset);

				break;
			}

			case Channel::ChannelRequest::Method::DATA_PRODUCER_GET_STATS:
			{
				// TMP: Replace JSON by flatbuffers.

				json data = json::array();

				FillJsonStats(data);

				auto responseOffset = FBS::DataProducer::CreateGetStatsResponseDirect(
				  request->GetBufferBuilder(), data.dump().c_str());

				request->Accept(FBS::Response::Body::FBS_DataProducer_GetStatsResponse, responseOffset);

				break;
			}

			default:
			{
				MS_THROW_ERROR("unknown method '%s'", request->methodStr.c_str());
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
