#ifndef MS_CHANNEL_MESSAGE_REGISTRATOR_HPP
#define MS_CHANNEL_MESSAGE_REGISTRATOR_HPP

#include "Channel/ChannelSocket.hpp"
#include <absl/container/flat_hash_map.h>
#include <string>

namespace Channel
{
	class ChannelMessageRegistrator
	{
	public:
		explicit ChannelMessageRegistrator();
		~ChannelMessageRegistrator();

	public:
		flatbuffers::Offset<FBS::Worker::ChannelMessageHandlers> FillBuffer(
		  flatbuffers::FlatBufferBuilder& builder);
		void RegisterHandler(
		  const std::string& id,
		  ChannelSocket::RequestHandler* channelRequestHandler,
		  ChannelSocket::NotificationHandler* channelNotificationHandler);
		void UnregisterHandler(const std::string& id);
		ChannelSocket::RequestHandler* GetChannelRequestHandler(const std::string& id);
		ChannelSocket::NotificationHandler* GetChannelNotificationHandler(const std::string& id);

	private:
		absl::flat_hash_map<std::string, ChannelSocket::RequestHandler*> mapChannelRequestHandlers;
		absl::flat_hash_map<std::string, ChannelSocket::NotificationHandler*> mapChannelNotificationHandlers;
	};
} // namespace Channel

#endif
