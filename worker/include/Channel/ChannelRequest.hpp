#ifndef MS_CHANNEL_REQUEST_HPP
#define MS_CHANNEL_REQUEST_HPP

#include "common.hpp"
#include "FBS/request_generated.h"
#include "FBS/response_generated.h"
#include <absl/container/flat_hash_map.h>
#include <flatbuffers/minireflect.h>
#include <nlohmann/json.hpp>
#include <string>

using json = nlohmann::json;

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

	private:
		// TODO: Remove once JSON is removed.
		static absl::flat_hash_map<std::string, FBS::Request::Method> string2Method;

	public:
		static flatbuffers::FlatBufferBuilder bufferBuilder;

	public:
		ChannelRequest(Channel::ChannelSocket* channel, const char* msg, size_t msgLen);
		ChannelRequest(Channel::ChannelSocket* channel, const uint8_t* msg);
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

			builder.Finish(response);

			this->Send(builder.GetBufferPointer(), builder.GetSize());

			builder.Reset();
		}
		void Accept(json& data);
		void Error(const char* reason = nullptr);
		void TypeError(const char* reason = nullptr);

	private:
		void Send(uint8_t* buffer, size_t size);

	public:
		// Passed by argument.
		Channel::ChannelSocket* channel{ nullptr };
		uint32_t id{ 0u };
		std::string methodStr;
		Method method;
		std::string handlerId;
		json data;
		// TODO: Rename to `data` once JSON is removed.
		const FBS::Request::Request* _data{ nullptr };
		// Others.
		bool replied{ false };
	};
} // namespace Channel

#endif
