#ifndef MS_RTC_REMB_CLIENT_HPP
#define MS_RTC_REMB_CLIENT_HPP

#include "common.hpp"
#include "RTC/RTCP/FeedbackPsRemb.hpp"
#include "RTC/RtpDataCounter.hpp"
#include "RTC/RtpPacket.hpp"
#include <limits>

namespace RTC
{
	class RembClient
	{
	public:
		class Listener
		{
		public:
			virtual void OnRembClientIncreaseBitrate(RTC::RembClient* rembClient, uint32_t bitrate) = 0;
			virtual void OnRembClientDecreaseBitrate(RTC::RembClient* rembClient, uint32_t bitrate) = 0;
		};

	public:
		RembClient(RTC::RembClient::Listener* listener, uint32_t initialAvailableBitrate);
		virtual ~RembClient();

	public:
		void ReceiveRtpPacket(RTC::RtpPacket* packet);
		void ReceiveRembFeedback(RTC::RTCP::FeedbackPsRembPacket* remb);
		uint32_t GetAvailableBitrate();

	private:
		bool CheckStatus();

	private:
		Listener* listener;
		RTC::RtpDataCounter transmissionCounter;
		uint32_t initialAvailableBitrate{ 0 };
		uint32_t availableBitrate{ 0 };
		uint64_t lastEventAt{ 0 };
	};
} // namespace RTC

#endif
