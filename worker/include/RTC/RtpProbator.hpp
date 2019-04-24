#ifndef MS_RTC_RTP_PROBATOR_HPP
#define MS_RTC_RTP_PROBATOR_HPP

#include "common.hpp"
#include "RTC/RtpDataCounter.hpp"
#include "RTC/RtpPacket.hpp"

namespace RTC
{
	class RtpProbator
	{
	public:
		class Listener
		{
		public:
			virtual void OnRtpProbatorSendRtpPacket(RTC::RtpProbator* rtpProbator, RTC::RtpPacket* packet) = 0;
		};

	public:
		RtpProbator(RTC::RtpProbator::Listener* listener);
		virtual ~RtpProbator();

	public:
		void UpdateAvailableBitrate(uint32_t availableBitrate);
		void ReceiveRtpPacket(RTC::RtpPacket* packet, bool retransmitted);

	private:
		// Passed by argument.
		Listener* listener{ nullptr };
		// Others.
		bool enabled{ false };
		uint32_t probationTargetBitrate{ 0 };
		RTC::RtpDataCounter transmissionCounter;
		RTC::RtpDataCounter probationTransmissionCounter;
	};
} // namespace RTC

#endif
