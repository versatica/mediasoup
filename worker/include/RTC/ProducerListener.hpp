#ifndef MS_RTC_PRODUCER_LISTENER_HPP
#define MS_RTC_PRODUCER_LISTENER_HPP

#include "common.hpp"
#include "RTC/RtpPacket.hpp"
#include "RTC/RtpStream.hpp"

namespace RTC
{
	// Avoid cyclic #include problem by declaring classes instead of including
	// the corresponding header files.
	class Producer;

	class ProducerListener
	{
	public:
		virtual void OnProducerClosed(RTC::Producer* producer)  = 0;
		virtual void OnProducerPaused(RTC::Producer* producer)  = 0;
		virtual void OnProducerResumed(RTC::Producer* producer) = 0;
		virtual void OnProducerRtpPacket(RTC::Producer* producer, RTC::RtpPacket* packet) = 0;
		virtual void OnProducerStreamEnabled(
		  RTC::Producer* producer,
		  const RTC::RtpStream* rtpStream,
		  uint32_t translatedSsrc) = 0;
		virtual void OnProducerStreamDisabled(
		  RTC::Producer* producer,
		  const RTC::RtpStream* rtpStream,
		  uint32_t translatedSsrc) = 0;
	};
} // namespace RTC

#endif
