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
	absl::flat_hash_map<FBS::Request::Method, const char*> PayloadChannelRequest::method2String =
	{
		{ FBS::Request::Method::DATA_CONSUMER_SEND, "dataConsumer.send" },
	};
	// clang-format on

	/* Class methods. */
	flatbuffers::FlatBufferBuilder PayloadChannelRequest::bufferBuilder;

	/* Instance methods. */

	PayloadChannelRequest::PayloadChannelRequest(
	  PayloadChannel::PayloadChannelSocket* channel, const FBS::Request::Request* request)
	  : channel(channel)
	{
		MS_TRACE();

		this->data      = request;
		this->id        = this->data->id();
		this->method    = this->data->method();
		this->methodStr = method2String[this->method];

		// Handler ID is optional.
		if (flatbuffers::IsFieldPresent(this->data, FBS::Request::Request::VT_HANDLERID))
			this->handlerId = this->data->handlerId()->str();
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

		auto& builder = PayloadChannelRequest::bufferBuilder;
		auto response =
		  FBS::Response::CreateResponse(builder, this->id, true, FBS::Response::Body::NONE, 0);

		Send(response);
	}

	void PayloadChannelRequest::Error(const char* reason)
	{
		MS_TRACE();

		MS_ASSERT(!this->replied, "request already replied");

		this->replied = true;

		auto& builder = PayloadChannelRequest::bufferBuilder;
		auto response = FBS::Response::CreateResponseDirect(
		  builder, this->id, false /*accepted*/, FBS::Response::Body::NONE, 0, "Error" /*Error*/, reason);

		Send(response);
	}

	void PayloadChannelRequest::TypeError(const char* reason)
	{
		MS_TRACE();

		MS_ASSERT(!this->replied, "request already replied");

		this->replied = true;

		auto& builder = PayloadChannelRequest::bufferBuilder;
		auto response = FBS::Response::CreateResponseDirect(
		  builder, this->id, false /*accepted*/, FBS::Response::Body::NONE, 0, "TypeError" /*Error*/, reason);

		Send(response);
	}

	void PayloadChannelRequest::Send(const flatbuffers::Offset<FBS::Response::Response>& response)
	{
		auto& builder = PayloadChannelRequest::bufferBuilder;
		auto message  = FBS::Message::CreateMessage(
      builder,
      FBS::Message::Type::RESPONSE,
      FBS::Message::Body::FBS_Response_Response,
      response.Union());

		builder.Finish(message);
		this->channel->Send(builder.GetBufferPointer(), builder.GetSize());
		builder.Reset();
	}
} // namespace PayloadChannel
