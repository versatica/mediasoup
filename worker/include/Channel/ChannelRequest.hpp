#ifndef MS_CHANNEL_REQUEST_HPP
#define MS_CHANNEL_REQUEST_HPP

#include "common.hpp"
#include "FBS/message_generated.h"
#include "FBS/request_generated.h"
#include "FBS/response_generated.h"
#include <absl/container/flat_hash_map.h>
#include <flatbuffers/minireflect.h>
#include <string>

namespace Channel
{
	// Avoid cyclic #include problem by declaring classes instead of including
	// the corresponding header files.
	class ChannelSocket;

	class ChannelRequest
	{
	public:
		using Method = FBS::Request::Method;

	public:
		static absl::flat_hash_map<FBS::Request::Method, const char*> method2String;

	public:
		static flatbuffers::FlatBufferBuilder bufferBuilder;

	public:
		ChannelRequest(Channel::ChannelSocket* channel, const FBS::Request::Request* request);
		virtual ~ChannelRequest();

		flatbuffers::FlatBufferBuilder& GetBufferBuilder()
		{
			return bufferBuilder;
		}
		void Accept();
		template<class Body>
		void Accept(FBS::Response::Body type, flatbuffers::Offset<Body>& body)
		{
			// TODO: Assert the request is not already replied.

			this->replied = true;

			auto& builder = ChannelRequest::bufferBuilder;
			auto response = FBS::Response::CreateResponse(builder, this->id, true, type, body.Union());

			auto message = FBS::Message::CreateMessage(
			  builder,
			  FBS::Message::Type::RESPONSE,
			  FBS::Message::Body::FBS_Response_Response,
			  response.Union());

			builder.FinishSizePrefixed(message);
			this->Send(builder.GetBufferPointer(), builder.GetSize());
			builder.Reset();
		}
		void Error(const char* reason = nullptr);
		void TypeError(const char* reason = nullptr);

	private:
		void Send(uint8_t* buffer, size_t size);
		void Send(const flatbuffers::Offset<FBS::Response::Response>& reponse);

	public:
		// Passed by argument.
		Channel::ChannelSocket* channel{ nullptr };
		uint32_t id{ 0u };
		Method method;
		const char* methodCStr;
		std::string handlerId;
		const FBS::Request::Request* data{ nullptr };
		// Others.
		bool replied{ false };
	};
} // namespace Channel

#endif
