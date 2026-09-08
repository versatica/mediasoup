#ifndef MS_MOCKS_MOCK_SHARED_HPP
#define MS_MOCKS_MOCK_SHARED_HPP

#include "Channel/ChannelNotifier.hpp"
#include "Channel/ChannelSocket.hpp"
#include "SharedInterface.hpp"
#include "mocks/include/Channel/MockChannelMessageRegistrator.hpp"
#include "mocks/include/handles/MockBackoffTimerHandle.hpp"
#include "mocks/include/handles/MockTimerHandle.hpp"
#include <map>
#include <string>
#include <string_view>

namespace mocks
{
	class MockShared : public SharedInterface
	{
	public:
		explicit MockShared(std::function<int64_t()> getTimeUsInt64);

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

		TimerHandleInterface* CreateTimer(TimerHandleInterface::Listener* listener, std::string label) override;

		BackoffTimerHandleInterface* CreateBackoffTimer(
		  const BackoffTimerHandleInterface::BackoffTimerHandleOptions& options) override;

		int64_t GetTimeMsInt64() override
		{
			return GetTimeUsInt64() / 1000;
		}

		int64_t GetTimeUsInt64() override
		{
			return this->getTimeUsInt64();
		}

		// NOTE: The NTP epoch is made to be the very clock given by argument, so that tests
		// can reason about a single set of values.
		int64_t GetNtpOffsetUs() override
		{
			return 0;
		}

		// Methods for testing.
	public:
		MockTimerHandle* GetTimer(const std::string_view label) const
		{
			const auto it = this->timers.find(std::string(label));

			if (it != this->timers.end())
			{
				return it->second;
			}
			else
			{
				return nullptr;
			}
		}

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
		const std::function<int64_t()> getTimeUsInt64;
		// Others.
		std::unique_ptr<::Channel::ChannelSocket> channelSocket;
		std::unique_ptr<mocks::Channel::MockChannelMessageRegistrator> channelMessageRegistrator;
		std::unique_ptr<::Channel::ChannelNotifier> channelNotifier;
		std::map<std::string /*label*/, MockTimerHandle* /*timer*/> timers;
		std::map<std::string /*label*/, MockBackoffTimerHandle* /*backoffTimer*/> backoffTimers;
	};
} // namespace mocks

#endif
