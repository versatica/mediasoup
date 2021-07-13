#ifndef MS_PAYLOAD_CHANNEL_NOTIFIER_HPP
#define MS_PAYLOAD_CHANNEL_NOTIFIER_HPP

#include "common.hpp"
#include "PayloadChannel/PayloadChannelSocket.hpp"
#include <nlohmann/json.hpp>
#include <string>

namespace PayloadChannel
{
	class PayloadChannelNotifier
	{
	public:
		static void ClassInit(PayloadChannel::PayloadChannelSocket* payloadChannel);
		static void Emit(
		  const std::string& targetId, const char* event, const uint8_t* payload, size_t payloadLen);
		static void Emit(
		  const std::string& targetId,
		  const char* event,
		  json& data,
		  const uint8_t* payload,
		  size_t payloadLen);

	public:
		// Passed by argument.
		thread_local static PayloadChannel::PayloadChannelSocket* payloadChannel;
	};
} // namespace PayloadChannel

#endif
