#ifndef MS_SHARED_INTERFACE_HPP
#define MS_SHARED_INTERFACE_HPP

#include "Channel/ChannelMessageRegistratorInterface.hpp"
// TODO: We should have a ChannelNotifierInterface class instead.
#include "Channel/ChannelNotifier.hpp"
#include "handles/BackoffTimerHandleInterface.hpp"
#include "handles/TimerHandleInterface.hpp"

class SharedInterface
{
public:
	virtual ~SharedInterface() = default;

public:
	/**
	 * @todo We should have a ChannelMessageRegistratorInterface class instead.
	 */
	virtual Channel::ChannelMessageRegistratorInterface* GetChannelMessageRegistrator() = 0;

	/**
	 * @todo We should have a ChannelNotifierInterface class instead.
	 */
	virtual Channel::ChannelNotifier* GetChannelNotifier() = 0;

	/**
	 * Creates a TimerHandle timer.
	 *
	 * @remarks
	 * - The caller is responsible for freeing it.
	 */
	virtual TimerHandleInterface* CreateTimer(TimerHandleInterface::Listener* listener) = 0;

	/**
	 * Creates a BackoffTimerHandle timer.
	 *
	 * @remarks
	 * - The caller is responsible for freeing it.
	 */
	virtual BackoffTimerHandleInterface* CreateBackoffTimer(
	  const BackoffTimerHandleInterface::BackoffTimerHandleOptions& options) = 0;

	/**
	 * Get current time in milliseconds.
	 */
	virtual uint64_t GetTimeMs() = 0;

	/**
	 * Get current time in microseconds.
	 */
	virtual uint64_t GetTimeUs() = 0;

	/**
	 * Get current time in nanoseconds.
	 */
	virtual uint64_t GetTimeNs() = 0;

	/**
	 * @remarks
	 * - Used within libwebrtc dependency which uses int64_t values for time
	 *   representation.
	 *
	 * @todo Remove once not needed.
	 */
	virtual int64_t GetTimeMsInt64() = 0;

	/**
	 * @remarks
	 * - Used within libwebrtc dependency which uses int64_t values for time
	 *   representation.
	 *
	 * @todo Remove once not needed.
	 */
	virtual int64_t GetTimeUsInt64() = 0;
};

#endif
