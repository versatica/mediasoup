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
	virtual Channel::ChannelMessageRegistratorInterface* GetChannelMessageRegistrator() const = 0;

	/**
	 * @todo We should have a ChannelNotifierInterface class instead.
	 */
	virtual Channel::ChannelNotifier* GetChannelNotifier() const = 0;

	/**
	 * Creates a TimerHandle timer.
	 *
	 * @remarks
	 * - The caller is responsible for freeing it.
	 */
	virtual TimerHandleInterface* CreateTimer(TimerHandleInterface::Listener* listener) const = 0;

	/**
	 * Creates a BackoffTimerHandle timer.
	 *
	 * @remarks
	 * - The caller is responsible for freeing it.
	 */
	virtual BackoffTimerHandleInterface* CreateBackoffTimer(
	  const BackoffTimerHandleInterface::BackoffTimerHandleOptions& options) const = 0;
};

#endif
