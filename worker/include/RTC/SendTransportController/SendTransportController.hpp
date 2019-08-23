#ifndef MS_RTC_SEND_TRANSPORT_CONTROLLER_HPP
#define MS_RTC_SEND_TRANSPORT_CONTROLLER_HPP

#include "common.hpp"

namespace RTC
{
	class SendTransportController
	{
	public:
		class Listener
		{
		};

	public:
		SendTransportController(RTC::SendTransportController::Listener* listener);
		virtual ~SendTransportController();

	private:
		// Passed by argument.
		RTC::SendTransportController::Listener* listener{ nullptr };
	};
} // namespace RTC

#endif
