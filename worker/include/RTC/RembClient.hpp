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
		uint32_t averageBitrate{ 0 };
		uint32_t extraBitrate{ 0 };
		uint32_t availableBitrate{ 0 };
	};

	/* Inline methods. */

	inline uint32_t RembClient::GetAvailableBitrate() const
	{
		if (this->availableBitrate >= this->extraBitrate)
			return this->availableBitrate - this->extraBitrate;
		else
			return 0;
	}

	inline void RembClient::AddExtraBitrate(uint32_t extraBitrate)
	{
		this->extraBitrate += extraBitrate;
	}

	inline void RembClient::RemoveExtraBitrate(uint32_t extraBitrate)
	{
		if (this->extraBitrate >= extraBitrate)
			this->extraBitrate -= extraBitrate;
		else
			this->extraBitrate = 0;
	}
} // namespace RTC

#endif
