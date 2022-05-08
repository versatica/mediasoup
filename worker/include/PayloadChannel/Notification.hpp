#ifndef MS_PAYLOAD_CHANNEL_NOTIFICATION_HPP
#define MS_PAYLOAD_CHANNEL_NOTIFICATION_HPP

#include "common.hpp"
#include <absl/container/flat_hash_map.h>
#include <nlohmann/json.hpp>
#include <string>

using json = nlohmann::json;

namespace PayloadChannel
{
	class Notification
	{
	public:
		enum class EventId
		{
			TRANSPORT_SEND_RTCP = 1,
			PRODUCER_SEND,
			DATA_PRODUCER_SEND
		};

	public:
		static bool IsNotification(json& jsonNotification);

	private:
		static absl::flat_hash_map<std::string, EventId> string2EventId;

	public:
		explicit Notification(json& jsonNotification);
		virtual ~Notification();

	public:
		void SetPayload(const uint8_t* payload, size_t payloadLen);

	public:
		// Passed by argument.
		std::string event;
		EventId eventId;
		json internal;
		json data;
		const uint8_t* payload{ nullptr };
		size_t payloadLen{ 0u };
	};
} // namespace PayloadChannel

#endif
