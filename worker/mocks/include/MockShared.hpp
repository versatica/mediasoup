#ifndef MS_MOCKS_MOCK_SHARED_HPP
#define MS_MOCKS_MOCK_SHARED_HPP

#include "SharedInterface.hpp"
#include "mocks/include/Channel/MockChannelMessageRegistrator.hpp"
#include "mocks/include/handles/MockBackoffTimerHandle.hpp"
#include "Channel/ChannelNotifier.hpp"
#include "Channel/ChannelSocket.hpp"
#include <map>
#include <string>
#include <string_view>

namespace mocks
{
	class MockShared : public SharedInterface
	{
	public:
		explicit MockShared(std::function<uint64_t()> getTimeMs);

		~MockShared() override = default;

	public:
		::Channel::ChannelMessageRegistratorInterface* GetChannelMessageRegistrator() override
		{
			return this->channelMessageRegistrator.get();
		}

		::Channel::ChannelNotifier* GetChannelNotifier() override
		{
			return this->channelNotifier.get();
		}

		TimerHandleInterface* CreateTimer(TimerHandleInterface::Listener* listener) override;

		BackoffTimerHandleInterface* CreateBackoffTimer(
		  const BackoffTimerHandleInterface::BackoffTimerHandleOptions& options) override;

		uint64_t GetTimeMs() override
		{
			return this->getTimeMs();
		}

		uint64_t GetTimeUs() override
		{
			return GetTimeMs() * 1000;
		}

		uint64_t GetTimeNs() override
		{
			return GetTimeMs() * 1000 * 1000;
		}

		int64_t GetTimeMsInt64() override
		{
			return static_cast<int64_t>(GetTimeMs());
		}

		int64_t GetTimeUsInt64() override
		{
			return static_cast<int64_t>(GetTimeUs());
		}

		// For tests.

		MockBackoffTimerHandle* GetBackoffTimer(const std::string_view label) const
		{
			const auto it = this->backoffTimers.find(std::string(label));

			if (it != this->backoffTimers.end())
			{
				return it->second;
			}
			else
			{
				return nullptr;
			}
		}

	private:
		// Given by argument.
		const std::function<uint64_t()> getTimeMs;
		// Others.
		std::unique_ptr<::Channel::ChannelSocket> channelSocket;
		std::unique_ptr<mocks::Channel::MockChannelMessageRegistrator> channelMessageRegistrator;
		std::unique_ptr<::Channel::ChannelNotifier> channelNotifier;
		std::map<std::string /*label*/, MockBackoffTimerHandle* /*backoffTimer*/> backoffTimers;
	};
} // namespace mocks

#endif
