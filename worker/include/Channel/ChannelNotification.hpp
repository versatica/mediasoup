#ifndef MS_CHANNEL_NOTIFICATION_HPP
#define MS_CHANNEL_NOTIFICATION_HPP

#include "common.hpp"
#include "FBS/notification_generated.h"
#include <absl/container/flat_hash_map.h>
#include <string>

namespace Channel
{
	class ChannelNotification
	{
	public:
		using Event = FBS::Notification::Event;

	public:
		static flatbuffers::FlatBufferBuilder bufferBuilder;

	private:
		static absl::flat_hash_map<FBS::Notification::Event, const char*> event2String;

	public:
		ChannelNotification(const FBS::Notification::Notification* notification);
		virtual ~ChannelNotification();

	public:
		void Set(const uint8_t* payload, size_t payloadLen);

	public:
		// Passed by argument.
		Event event;
		const char* eventCStr;
		std::string handlerId;
		const FBS::Notification::Notification* data{ nullptr };
	};
} // namespace Channel

#endif
