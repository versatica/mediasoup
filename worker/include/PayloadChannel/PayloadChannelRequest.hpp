#ifndef MS_PAYLOAD_CHANNEL_REQUEST_HPP
#define MS_PAYLOAD_CHANNEL_REQUEST_HPP

#include "common.hpp"
#include "FBS/message_generated.h"
#include "FBS/request_generated.h"
#include "FBS/response_generated.h"
#include <absl/container/flat_hash_map.h>
#include <flatbuffers/flatbuffers.h>
#include <string>

namespace PayloadChannel
{
	// Avoid cyclic #include problem by declaring classes instead of including
	// the corresponding header files.
	class PayloadChannelSocket;

	class PayloadChannelRequest
	{
	public:
		using Method = FBS::Request::Method;

	public:
		static flatbuffers::FlatBufferBuilder bufferBuilder;

	private:
		static absl::flat_hash_map<FBS::Request::Method, const char*> method2String;

	public:
		PayloadChannelRequest(
		  PayloadChannel::PayloadChannelSocket* channel, const FBS::Request::Request* request);
		virtual ~PayloadChannelRequest();

	public:
		void Accept();
		void Error(const char* reason = nullptr);
		void TypeError(const char* reason = nullptr);

	private:
		void Send(const flatbuffers::Offset<FBS::Response::Response>& reponse);

	public:
		// Passed by argument.
		PayloadChannel::PayloadChannelSocket* channel{ nullptr };
		uint32_t id{ 0u };
		// TODO: Move it to const char*.
		std::string methodStr;
		Method method;
		std::string handlerId;
		const FBS::Request::Request* data{ nullptr };
		// Others.
		bool replied{ false };
	};
} // namespace PayloadChannel

#endif
