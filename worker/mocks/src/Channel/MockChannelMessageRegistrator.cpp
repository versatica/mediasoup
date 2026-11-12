#define MS_CLASS "mocks::Channel::MockChannelMessageRegistrator"
// #define MS_LOG_DEV_LEVEL 3

#include "mocks/include/Channel/MockChannelMessageRegistrator.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include <vector>

namespace mocks
{
	namespace Channel
	{
		MockChannelMessageRegistrator::MockChannelMessageRegistrator()
		{
			MS_TRACE();
		}

		MockChannelMessageRegistrator::~MockChannelMessageRegistrator()
		{
			MS_TRACE();

			this->mapChannelRequestHandlers.clear();
			this->mapChannelNotificationHandlers.clear();
		}

		flatbuffers::Offset<FBS::Worker::ChannelMessageHandlers> MockChannelMessageRegistrator::FillBuffer(
		  flatbuffers::FlatBufferBuilder& builder)
		{
			MS_TRACE();

			// Add channelRequestHandlerIds.
			std::vector<flatbuffers::Offset<flatbuffers::String>> channelRequestHandlerIds;

			for (const auto& kv : this->mapChannelRequestHandlers)
			{
				const std::string& handlerId = kv.first;

				channelRequestHandlerIds.push_back(builder.CreateString(handlerId));
			}

			// Add channelNotificationHandlerIds.
			std::vector<flatbuffers::Offset<flatbuffers::String>> channelNotificationHandlerIds;

			for (const auto& kv : this->mapChannelNotificationHandlers)
			{
				const std::string& handlerId = kv.first;

				channelNotificationHandlerIds.push_back(builder.CreateString(handlerId));
			}

			return FBS::Worker::CreateChannelMessageHandlersDirect(
			  builder,
			  std::addressof(channelRequestHandlerIds),
			  std::addressof(channelNotificationHandlerIds));
		}

		void MockChannelMessageRegistrator::RegisterHandler(
		  const std::string& id,
		  ::Channel::ChannelSocket::RequestHandler* channelRequestHandler,
		  ::Channel::ChannelSocket::NotificationHandler* channelNotificationHandler)
		{
			MS_TRACE();

			if (channelRequestHandler)
			{
				if (this->mapChannelRequestHandlers.contains(id))
				{
					MS_THROW_ERROR("Channel request handler with ID %s already exists", id.c_str());
				}

				this->mapChannelRequestHandlers[id] = channelRequestHandler;
			}

			if (channelNotificationHandler)
			{
				if (this->mapChannelNotificationHandlers.contains(id))
				{
					if (channelRequestHandler)
					{
						this->mapChannelRequestHandlers.erase(id);
					}

					MS_THROW_ERROR("Channel notification handler with ID %s already exists", id.c_str());
				}

				this->mapChannelNotificationHandlers[id] = channelNotificationHandler;
			}
		}

		void MockChannelMessageRegistrator::UnregisterHandler(const std::string& id)
		{
			MS_TRACE();

			this->mapChannelRequestHandlers.erase(id);
			this->mapChannelNotificationHandlers.erase(id);
		}

		::Channel::ChannelSocket::RequestHandler* MockChannelMessageRegistrator::GetChannelRequestHandler(
		  const std::string& id)
		{
			MS_TRACE();

			const auto it = this->mapChannelRequestHandlers.find(id);

			if (it != this->mapChannelRequestHandlers.end())
			{
				::Channel::ChannelSocket::RequestHandler* const channelRequestHandler = it->second;

				return channelRequestHandler;
			}
			else
			{
				return nullptr;
			}
		}

		::Channel::ChannelSocket::NotificationHandler* MockChannelMessageRegistrator::GetChannelNotificationHandler(
		  const std::string& id)
		{
			MS_TRACE();

			const auto it = this->mapChannelNotificationHandlers.find(id);

			if (it != this->mapChannelNotificationHandlers.end())
			{
				::Channel::ChannelSocket::NotificationHandler* const channelNotificationHandler = it->second;

				return channelNotificationHandler;
			}
			else
			{
				return nullptr;
			}
		}
	} // namespace Channel
} // namespace mocks
