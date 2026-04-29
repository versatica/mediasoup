#ifndef MS_CHANNEL_MESSAGE_REGISTRATOR_INTERFACE_HPP
#define MS_CHANNEL_MESSAGE_REGISTRATOR_INTERFACE_HPP

// TODO: We should have a ChannelSocketInterface class instead.
#include "Channel/ChannelSocket.hpp"
#include <string>

namespace Channel
{
	class ChannelMessageRegistratorInterface
	{
	public:
		virtual ~ChannelMessageRegistratorInterface() = default;

	public:
		virtual flatbuffers::Offset<FBS::Worker::ChannelMessageHandlers> FillBuffer(
		  flatbuffers::FlatBufferBuilder& builder) = 0;

		virtual void RegisterHandler(
		  const std::string& id,
		  ChannelSocket::RequestHandler* channelRequestHandler,
		  ChannelSocket::NotificationHandler* channelNotificationHandler) = 0;

		virtual void UnregisterHandler(const std::string& id) = 0;

		virtual ChannelSocket::RequestHandler* GetChannelRequestHandler(const std::string& id) = 0;

		virtual ChannelSocket::NotificationHandler* GetChannelNotificationHandler(const std::string& id) = 0;
	};
} // namespace Channel

#endif
