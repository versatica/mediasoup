#ifndef MS_PAYLOAD_CHANNEL_NOTIFICATION_HPP
#define MS_PAYLOAD_CHANNEL_NOTIFICATION_HPP

#include "common.hpp"
#include <absl/container/flat_hash_map.h>
#include <string>

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
		static bool IsNotification(const char* jsonNotification);

	private:
		static absl::flat_hash_map<std::string, EventId> string2EventId;

	public:
		Notification(const char* msg, size_t msgLen);

		virtual ~Notification();

	public:
		void SetPayload(const uint8_t* payload, size_t payloadLen);
		const std::string& GetNextInternalRoutingId();

	public:
		// Passed by argument.
		std::string event;
		EventId eventId;
		std::vector<std::string> internal;
		std::string data;
		const uint8_t* payload{ nullptr };
		size_t payloadLen{ 0u };

	private:
		uint8_t nextRoutingLevel{ 0 };
	};
} // namespace PayloadChannel

#endif
