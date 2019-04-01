#ifndef MS_RTC_REMB_CLIENT_HPP
#define MS_RTC_REMB_CLIENT_HPP

#include "common.hpp"
#include "RTC/RTCP/FeedbackPsRemb.hpp"
#include "RTC/RtpDataCounter.hpp"
#include "RTC/RtpPacket.hpp"

namespace RTC
{
	class RembClient
	{
	public:
		class Listener
		{
		public:
			virtual void OnRembClientBitrateEstimation(
			  RTC::RembClient* rembClient, int32_t availableBitrate) = 0;
		};

	public:
		RembClient(RTC::RembClient::Listener* listener);
		virtual ~RembClient();

	public:
		uint32_t GetAvailableBitrate() const;
		void ReceiveRtpPacket(RTC::RtpPacket* packet);
		void ReceiveRembFeedback(RTC::RTCP::FeedbackPsRembPacket* remb);

	private:
		Listener* listener;
		RTC::RtpDataCounter transmissionCounter;
		uint32_t availableBitrate{ 0 };
	};

	/* Inline methods. */

	inline uint32_t RembClient::GetAvailableBitrate() const
	{
		return this->availableBitrate;
	}
} // namespace RTC

#endif
