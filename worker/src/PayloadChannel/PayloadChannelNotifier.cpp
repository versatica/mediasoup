#define MS_CLASS "PayloadChannel::Notifier"
// #define MS_LOG_DEV_LEVEL 3

#include "PayloadChannel/PayloadChannelNotifier.hpp"
#include "Logger.hpp"

namespace PayloadChannel
{
	/* Class variables. */
	flatbuffers::FlatBufferBuilder PayloadChannelNotifier::bufferBuilder;

	PayloadChannelNotifier::PayloadChannelNotifier(PayloadChannel::PayloadChannelSocket* payloadChannel)
	  : payloadChannel(payloadChannel)
	{
		MS_TRACE();
	}

	void PayloadChannelNotifier::Emit(const std::string& targetId, FBS::Notification::Event event)
	{
		auto& builder     = PayloadChannelNotifier::bufferBuilder;
		auto notification = FBS::Notification::CreateNotificationDirect(builder, targetId.c_str(), event);

		auto message = FBS::Message::CreateMessage(
		  builder,
		  FBS::Message::Type::NOTIFICATION,
		  FBS::Message::Body::FBS_Notification_Notification,
		  notification.Union());

		builder.Finish(message);
		this->payloadChannel->Send(builder.GetBufferPointer(), builder.GetSize());
		builder.Reset();
	}
} // namespace PayloadChannel
