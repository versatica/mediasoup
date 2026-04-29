#ifndef MS_CHANNEL_MESSAGE_REGISTRATOR_HPP
#define MS_CHANNEL_MESSAGE_REGISTRATOR_HPP

#include "Channel/ChannelMessageRegistratorInterface.hpp"
#include "Channel/ChannelSocket.hpp"
#include <string>
#include <unordered_map>

namespace Channel
{
	class ChannelMessageRegistrator : public Channel::ChannelMessageRegistratorInterface
	{
	public:
		explicit ChannelMessageRegistrator();

		~ChannelMessageRegistrator() override;

	public:
		flatbuffers::Offset<FBS::Worker::ChannelMessageHandlers> FillBuffer(
		  flatbuffers::FlatBufferBuilder& builder) override;

		void RegisterHandler(
		  const std::string& id,
		  ChannelSocket::RequestHandler* channelRequestHandler,
		  ChannelSocket::NotificationHandler* channelNotificationHandler) override;

		void UnregisterHandler(const std::string& id) override;

		ChannelSocket::RequestHandler* GetChannelRequestHandler(const std::string& id) override;

		ChannelSocket::NotificationHandler* GetChannelNotificationHandler(const std::string& id) override;

	private:
		std::unordered_map<std::string, ChannelSocket::RequestHandler*> mapChannelRequestHandlers;
		std::unordered_map<std::string, ChannelSocket::NotificationHandler*> mapChannelNotificationHandlers;
	};
} // namespace Channel

#endif
