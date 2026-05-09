#include "mocks/include/MockShared.hpp"
#include "mocks/include/handles/MockBackoffTimerHandle.hpp"
#include "mocks/include/handles/MockTimerHandle.hpp"

namespace mocks
{
	MockShared::MockShared(std::function<uint64_t()> getTimeMs)
	  : channelSocket(new ::Channel::ChannelSocket()),
	    channelMessageRegistrator(new mocks::Channel::MockChannelMessageRegistrator()),
	    channelNotifier(new ::Channel::ChannelNotifier(this->channelSocket.get())),
	    getTimeMs(std::move(getTimeMs))
	{
	}

	TimerHandleInterface* MockShared::CreateTimer(TimerHandleInterface::Listener* /*listener*/) const
	{
		return new MockTimerHandle();
	}

	BackoffTimerHandleInterface* MockShared::CreateBackoffTimer(
	  const BackoffTimerHandleInterface::BackoffTimerHandleOptions& options) const
	{
		return new MockBackoffTimerHandle(options, this->getTimeMs);
	}

	uint64_t MockShared::GetTimeMs() const
	{
		return this->getTimeMs();
	}

	uint64_t MockShared::GetTimeUs() const
	{
		return GetTimeMs() * 1000;
	}

	uint64_t MockShared::GetTimeNs() const
	{
		return GetTimeMs() * 1000 * 1000;
	}

	int64_t MockShared::GetTimeMsInt64() const
	{
		return static_cast<int64_t>(GetTimeMs());
	}

	int64_t MockShared::GetTimeUsInt64() const
	{
		return static_cast<int64_t>(GetTimeUs());
	}
} // namespace mocks
