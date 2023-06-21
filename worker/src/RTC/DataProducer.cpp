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
		  /*channelNotificationHandler*/ this);
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

	flatbuffers::Offset<FBS::DataProducer::GetStatsResponse> DataProducer::FillBufferStats(
	  flatbuffers::FlatBufferBuilder& builder) const
	{
		MS_TRACE();

		return FBS::DataProducer::CreateGetStatsResponseDirect(
		  builder,
		  // timestamp.
		  DepLibUV::GetTimeMs(),
		  // label.
		  this->label.c_str(),
		  // protocol.
		  this->protocol.c_str(),
		  // messagesReceived.
		  this->messagesReceived,
		  // bytesReceived.
		  this->bytesReceived);
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
				auto responseOffset = FillBufferStats(request->GetBufferBuilder());

				request->Accept(FBS::Response::Body::FBS_DataProducer_GetStatsResponse, responseOffset);

				break;
			}

			case Channel::ChannelRequest::Method::DATA_PRODUCER_PAUSE:
			{
				if (this->paused)
				{
					request->Accept();

					break;
				}

				this->paused = true;

				MS_DEBUG_DEV("DataProducer paused [dataProducerId:%s]", this->id.c_str());

				this->listener->OnDataProducerPaused(this);

				request->Accept();

				break;
			}

			case Channel::ChannelRequest::Method::DATA_PRODUCER_RESUME:
			{
				if (!this->paused)
				{
					request->Accept();

					break;
				}

				this->paused = false;

				MS_DEBUG_DEV("DataProducer resumed [dataProducerId:%s]", this->id.c_str());

				this->listener->OnDataProducerResumed(this);

				request->Accept();

				break;
			}

			default:
			{
				MS_THROW_ERROR("unknown method '%s'", request->methodCStr);
			}
		}
	}

	void DataProducer::HandleNotification(Channel::ChannelNotification* notification)
	{
		MS_TRACE();

		switch (notification->event)
		{
			case Channel::ChannelNotification::Event::DATA_PRODUCER_SEND:
			{
				const auto* body = notification->data->body_as<FBS::DataProducer::SendNotification>();
				const uint8_t* data{ nullptr };
				size_t len{ 0 };

				if (body->data_type() == FBS::DataProducer::Data::String)
				{
					data = body->data_as_String()->value()->Data();
					len  = body->data_as_String()->value()->size();
				}
				else
				{
					data = body->data_as_Binary()->value()->Data();
					len  = body->data_as_Binary()->value()->size();
				}

				if (len > this->maxMessageSize)
				{
					MS_THROW_TYPE_ERROR(
					  "given message exceeds maxMessageSize value [maxMessageSize:%zu, len:%zu]",
					  this->maxMessageSize,
					  len);
				}

				this->ReceiveMessage(body->ppid(), data, len);

				// Increase receive transmission.
				this->listener->OnDataProducerReceiveData(this, len);

				break;
			}

			default:
			{
				MS_ERROR("unknown event '%s'", notification->eventCStr);
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
