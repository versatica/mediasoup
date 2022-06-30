#ifndef MS_PAYLOAD_CHANNEL_REQUEST_HPP
#define MS_PAYLOAD_CHANNEL_REQUEST_HPP

#include "common.hpp"
#include <absl/container/flat_hash_map.h>
#include <nlohmann/json.hpp>
#include <string>

using json = nlohmann::json;

namespace PayloadChannel
{
	// Avoid cyclic #include problem by declaring classes instead of including
	// the corresponding header files.
	class PayloadChannelSocket;

	class PayloadChannelRequest
	{
	public:
		enum class MethodId
		{
			DATA_CONSUMER_SEND
		};

	public:
		static bool IsRequest(json& jsonRequest);

	private:
		static absl::flat_hash_map<std::string, MethodId> string2MethodId;

	public:
		PayloadChannelRequest(PayloadChannel::PayloadChannelSocket* channel, json& jsonRequest);
		virtual ~PayloadChannelRequest();

	public:
		void Accept();
		void Accept(json& data);
		void Error(const char* reason = nullptr);
		void TypeError(const char* reason = nullptr);
		void SetPayload(const uint8_t* payload, size_t payloadLen);

	public:
		// Passed by argument.
		PayloadChannel::PayloadChannelSocket* channel{ nullptr };
		uint32_t id{ 0u };
		std::string method;
		MethodId methodId;
		json internal;
		json data;
		const uint8_t* payload{ nullptr };
		size_t payloadLen{ 0u };
		// Others.
		bool replied{ false };
	};
} // namespace PayloadChannel

#endif
