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
			virtual void OnRembClientAvailableBitrate(
			  RTC::RembClient* rembClient, int32_t availableBitrate) = 0;
		};

	public:
		RembClient(RTC::RembClient::Listener* listener);
		virtual ~RembClient();

	public:
		void ReceiveRtpPacket(RTC::RtpPacket* packet);
		void ReceiveRembFeedback(RTC::RTCP::FeedbackPsRembPacket* remb);
		uint32_t GetAvailableBitrate() const;
		void AddExtraBitrate(uint32_t extraBitrate);
		void RemoveExtraBitrate(uint32_t extraBitrate);

	private:
		Listener* listener;
		RTC::RtpDataCounter transmissionCounter;
		uint32_t bitrateInUse{ 0 };
		uint32_t extraBitrateInUse{ 0 };
		uint32_t rembBitrate{ 0 };
		uint32_t availableBitrate{ 0 };
		uint64_t lastEventAt{ 0 };
	};
} // namespace RTC

#endif
