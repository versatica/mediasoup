#ifndef MS_CHANNEL_NOTIFIER_HPP
#define MS_CHANNEL_NOTIFIER_HPP

#include "common.hpp"
#include "Channel/ChannelSocket.hpp"
#include "FBS/message.h"
#include "FBS/notification.h"
#include <flatbuffers/flatbuffer_builder.h>
#include <string>

namespace Channel
{
	class ChannelNotifier
	{
	public:
		explicit ChannelNotifier(Channel::ChannelSocket* channel);

	public:
		flatbuffers::FlatBufferBuilder& GetBufferBuilder()
		{
			return this->bufferBuilder;
		}

		template<class Body>
		void Emit(
		  const std::string& targetId,
		  FBS::Notification::Event event,
		  FBS::Notification::Body type,
		  const flatbuffers::Offset<Body>& body)
		{
			flatbuffers::FlatBufferBuilder& builder = this->bufferBuilder;

			const auto notification = FBS::Notification::CreateNotificationDirect(
			  builder, targetId.c_str(), event, type, body.Union());
			const auto message =
			  FBS::Message::CreateMessage(builder, FBS::Message::Body::Notification, notification.Union());

			builder.FinishSizePrefixed(message);

			this->channel->Send(builder.GetBufferPointer(), builder.GetSize());

			builder.Clear();
		}

		void Emit(const std::string& targetId, FBS::Notification::Event event);

	private:
		// Passed by argument.
		Channel::ChannelSocket* const channel;
		// Others.
		flatbuffers::FlatBufferBuilder bufferBuilder;
	};
} // namespace Channel

#endif
