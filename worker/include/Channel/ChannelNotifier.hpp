#ifndef MS_CHANNEL_NOTIFIER_HPP
#define MS_CHANNEL_NOTIFIER_HPP

#include "common.hpp"
#include "Channel/ChannelSocket.hpp"
#include <nlohmann/json.hpp>
#include <string>

using json = nlohmann::json;

namespace Channel
{
	class ChannelNotifier
	{
	public:
		explicit ChannelNotifier(Channel::ChannelSocket* channel);

	public:
		static flatbuffers::FlatBufferBuilder bufferBuilder;

	public:
		flatbuffers::FlatBufferBuilder& GetBufferBuilder() const
		{
			return ChannelNotifier::bufferBuilder;
		}

		template<class Body>
		void Emit(
		  const std::string& targetId,
		  FBS::Notification::Event event,
		  FBS::Notification::Body type,
		  flatbuffers::Offset<Body>& body)
		{
			auto& builder     = ChannelNotifier::bufferBuilder;
			auto notification = FBS::Notification::CreateNotificationDirect(
			  builder, targetId.c_str(), event, type, body.Union());

			auto message = FBS::Message::CreateMessage(
			  builder,
			  FBS::Message::Type::NOTIFICATION,
			  FBS::Message::Body::FBS_Notification_Notification,
			  notification.Union());

			builder.Finish(message);
			this->channel->Send(builder.GetBufferPointer(), builder.GetSize());
			builder.Reset();
		}

		void Emit(const std::string& targetId, FBS::Notification::Event event)
		{
			auto& builder = ChannelNotifier::bufferBuilder;
			auto notification =
			  FBS::Notification::CreateNotificationDirect(builder, targetId.c_str(), event);

			auto message = FBS::Message::CreateMessage(
			  builder,
			  FBS::Message::Type::NOTIFICATION,
			  FBS::Message::Body::FBS_Notification_Notification,
			  notification.Union());

			builder.Finish(message);
			this->channel->Send(builder.GetBufferPointer(), builder.GetSize());
			builder.Reset();
		}
		void Emit(const FBS::Notification::Notification& notification);

	private:
		// Passed by argument.
		Channel::ChannelSocket* channel{ nullptr };
	};
} // namespace Channel

#endif
