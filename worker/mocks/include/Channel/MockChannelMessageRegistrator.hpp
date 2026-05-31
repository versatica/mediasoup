#ifndef MS_MOCKS_MOCK_CHANNEL_MESSAGE_REGISTRATOR_HPP
#define MS_MOCKS_MOCK_CHANNEL_MESSAGE_REGISTRATOR_HPP

#include "Channel/ChannelMessageRegistratorInterface.hpp"
// TODO: We should have a ChannelSocketInterface class instead.
#include "Channel/ChannelSocket.hpp"
#include <ankerl/unordered_dense.h>
#include <string>

namespace mocks
{
	namespace Channel
	{
		class MockChannelMessageRegistrator : public ::Channel::ChannelMessageRegistratorInterface
		{
		public:
			explicit MockChannelMessageRegistrator();

			~MockChannelMessageRegistrator() override;

		public:
			flatbuffers::Offset<FBS::Worker::ChannelMessageHandlers> FillBuffer(
			  flatbuffers::FlatBufferBuilder& builder) override;

			void RegisterHandler(
			  const std::string& id,
			  ::Channel::ChannelSocket::RequestHandler* channelRequestHandler,
			  ::Channel::ChannelSocket::NotificationHandler* channelNotificationHandler) override;

			void UnregisterHandler(const std::string& id) override;

			::Channel::ChannelSocket::RequestHandler* GetChannelRequestHandler(const std::string& id) override;

			::Channel::ChannelSocket::NotificationHandler* GetChannelNotificationHandler(
			  const std::string& id) override;

		private:
			ankerl::unordered_dense::map<std::string, ::Channel::ChannelSocket::RequestHandler*>
			  mapChannelRequestHandlers;
			ankerl::unordered_dense::map<std::string, ::Channel::ChannelSocket::NotificationHandler*>
			  mapChannelNotificationHandlers;
		};
	} // namespace Channel
} // namespace mocks

#endif
