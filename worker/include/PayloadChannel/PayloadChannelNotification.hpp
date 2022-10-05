#ifndef MS_PAYLOAD_CHANNEL_NOTIFICATION_HPP
#define MS_PAYLOAD_CHANNEL_NOTIFICATION_HPP

#include "common.hpp"
#include <absl/container/flat_hash_map.h>
#include <string>

namespace PayloadChannel
{
	class PayloadChannelNotification
	{
	public:
		enum class EventId
		{
			TRANSPORT_SEND_RTCP = 1,
			PRODUCER_SEND,
			DATA_PRODUCER_SEND
		};

	public:
		static bool IsNotification(const char* msg, size_t msgLen);

	private:
		static absl::flat_hash_map<std::string, EventId> string2EventId;

	public:
		PayloadChannelNotification(const char* msg, size_t msgLen);
		virtual ~PayloadChannelNotification();

	public:
		void SetPayload(const uint8_t* payload, size_t payloadLen);

	public:
		// Passed by argument.
		std::string event;
		EventId eventId;
		std::string handlerId;
		std::string data;
		const uint8_t* payload{ nullptr };
		size_t payloadLen{ 0u };
	};
} // namespace PayloadChannel

#endif
