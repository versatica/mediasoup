#ifndef MS_PAYLOAD_CHANNEL_NOTIFIER_HPP
#define MS_PAYLOAD_CHANNEL_NOTIFIER_HPP

#include "common.hpp"
#include "PayloadChannel/PayloadChannelSocket.hpp"
#include <string>

namespace PayloadChannel
{
	class PayloadChannelNotifier
	{
	public:
		explicit PayloadChannelNotifier(PayloadChannel::PayloadChannelSocket* payloadChannel);

	public:
		static flatbuffers::FlatBufferBuilder bufferBuilder;

	public:
		flatbuffers::FlatBufferBuilder& GetBufferBuilder() const
		{
			return PayloadChannelNotifier::bufferBuilder;
		}

		template<class Body>
		void Emit(
		  const std::string& targetId,
		  FBS::Notification::Event event,
		  FBS::Notification::Body type,
		  flatbuffers::Offset<Body>& body)
		{
			auto& builder     = PayloadChannelNotifier::bufferBuilder;
			auto notification = FBS::Notification::CreateNotificationDirect(
			  builder, targetId.c_str(), event, type, body.Union());

			auto message = FBS::Message::CreateMessage(
			  builder,
			  FBS::Message::Type::NOTIFICATION,
			  FBS::Message::Body::FBS_Notification_Notification,
			  notification.Union());

			builder.Finish(message);
			this->payloadChannel->Send(builder.GetBufferPointer(), builder.GetSize());
			builder.Reset();
		}

		void Emit(const std::string& targetId, FBS::Notification::Event event);

	private:
		// Passed by argument.
		PayloadChannel::PayloadChannelSocket* payloadChannel{ nullptr };
	};
} // namespace PayloadChannel

#endif
