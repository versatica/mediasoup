#ifndef MS_PAYLOAD_CHANNEL_NOTIFICATION_HPP
#define MS_PAYLOAD_CHANNEL_NOTIFICATION_HPP

#include "common.hpp"
#include "FBS/notification_generated.h"
#include <absl/container/flat_hash_map.h>
#include <string>

namespace PayloadChannel
{
	class PayloadChannelNotification
	{
	public:
		using Event = FBS::Notification::Event;

	public:
		static flatbuffers::FlatBufferBuilder bufferBuilder;

	private:
		static absl::flat_hash_map<FBS::Notification::Event, const char*> event2String;

	public:
		PayloadChannelNotification(const char* msg, size_t msgLen);
		PayloadChannelNotification(const FBS::Notification::Notification* notification);
		virtual ~PayloadChannelNotification();

	public:
		void SetPayload(const uint8_t* payload, size_t payloadLen);

	public:
		// Passed by argument.
		// TODO: Move it to const char*.
		std::string eventStr;
		Event event;
		std::string handlerId;
		const FBS::Notification::Notification* data{ nullptr };
	};
} // namespace PayloadChannel

#endif
