#define MS_CLASS "RTC::SendTransportController"
// #define MS_LOG_DEV

#include "RTC/SendTransportController/SendTransportController.hpp"
#include "Logger.hpp"

namespace RTC
{
	/* Instance methods. */

	SendTransportController::SendTransportController(RTC::SendTransportController::Listener* listener)
	  : listener(listener)
	{
		MS_TRACE();

		// TODO
	}

	SendTransportController::~SendTransportController()
	{
		MS_TRACE();

		// TODO
	}
} // namespace RTC
