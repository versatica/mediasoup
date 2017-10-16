#ifndef MS_RTC_PRODUCER_LISTENER_HPP
#define MS_RTC_PRODUCER_LISTENER_HPP

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
		virtual void OnProducerClosed(RTC::Producer* producer)               = 0;
		virtual void OnProducerRtpParametersUpdated(RTC::Producer* producer) = 0;
		virtual void OnProducerPaused(RTC::Producer* producer)               = 0;
		virtual void OnProducerResumed(RTC::Producer* producer)              = 0;
		virtual void OnProducerRtpPacket(
		  RTC::Producer* producer, RTC::RtpPacket* packet, RTC::RtpEncodingParameters::Profile profile) = 0;
		virtual void OnProducerProfileEnabled(
		  RTC::Producer* producer,
		  RTC::RtpEncodingParameters::Profile profile,
		  const RTC::RtpStream* rtpStream) = 0;
		virtual void OnProducerProfileDisabled(
		  RTC::Producer* producer, RTC::RtpEncodingParameters::Profile profile) = 0;
	};
} // namespace RTC

#endif
