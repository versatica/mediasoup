#ifndef MS_PAYLOAD_CHANNEL_NOTIFIER_HPP
#define MS_PAYLOAD_CHANNEL_NOTIFIER_HPP

#include "common.hpp"
#include "PayloadChannel/PayloadChannelSocket.hpp"
#include <nlohmann/json.hpp>
#include <string>

using json = nlohmann::json;

namespace PayloadChannel
{
	class PayloadChannelNotifier
	{
	public:
		explicit PayloadChannelNotifier(PayloadChannel::PayloadChannelSocket* payloadChannel);

	public:
		void Emit(const std::string& targetId, const char* event, const uint8_t* payload, size_t payloadLen);
		void Emit(
		  const std::string& targetId,
		  const char* event,
		  json& data,
		  const uint8_t* payload,
		  size_t payloadLen);
		void Emit(
		  const std::string& targetId,
		  const char* event,
		  const std::string& data,
		  const uint8_t* payload,
		  size_t payloadLen);

	private:
		// Passed by argument.
		PayloadChannel::PayloadChannelSocket* payloadChannel{ nullptr };
	};
} // namespace PayloadChannel

#endif
