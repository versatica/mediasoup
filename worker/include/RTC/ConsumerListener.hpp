#ifndef MS_RTC_CONSUMER_LISTENER_HPP
#define MS_RTC_CONSUMER_LISTENER_HPP

#include "RTC/RtpPacket.hpp"

namespace RTC
{
	// Avoid cyclic #include problem by declaring classes instead of including
	// the corresponding header files.
	class Consumer;

	class ConsumerListener
	{
	public:
		virtual void OnConsumerClosed(RTC::Consumer* consumer)            = 0;
		virtual void OnConsumerFullFrameRequired(RTC::Consumer* consumer) = 0;
	};
} // namespace RTC

#endif
