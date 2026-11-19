#ifndef MS_CHANNEL_REQUEST_HPP
#define MS_CHANNEL_REQUEST_HPP

#include "common.hpp"
#include <FBS/request.h>
#include <FBS/response.h>
#include <flatbuffers/flatbuffer_builder.h>
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
		static thread_local flatbuffers::FlatBufferBuilder bufferBuilder;

	public:
		ChannelRequest(Channel::ChannelSocket* channel, const FBS::Request::Request* request);

		~ChannelRequest() = default;

		flatbuffers::FlatBufferBuilder& GetBufferBuilder() const
		{
			return ChannelRequest::bufferBuilder;
		}

		void Accept();

		template<class Body>
		void Accept(FBS::Response::Body type, const flatbuffers::Offset<Body>& body)
		{
			Accept(type, body.Union());
		}

		void Accept(FBS::Response::Body type, flatbuffers::Offset<void> body);

		void Error(const char* reason = nullptr);

		void TypeError(const char* reason = nullptr);

		void NotFoundError(const char* reason = nullptr);

	private:
		void Send(const uint8_t* buffer, size_t size) const;

		void SendResponse(const flatbuffers::Offset<FBS::Response::Response>& response) const;

	public:
		// Passed by argument.
		Channel::ChannelSocket* const channel;
		const FBS::Request::Request* const data;
		// Others.
		const uint32_t id;
		const Method method;
		const char* methodCStr;
		const std::string handlerId;
		bool replied{ false };
	};
} // namespace Channel

#endif
