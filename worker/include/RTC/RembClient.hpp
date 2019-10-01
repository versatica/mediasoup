#ifndef MS_RTC_REMB_CLIENT_HPP
#define MS_RTC_REMB_CLIENT_HPP

#include "common.hpp"
#include "RTC/RTCP/FeedbackPsRemb.hpp"

namespace RTC
{
	class RembClient
	{
	public:
		class Listener
		{
		public:
			virtual void OnRembClientAvailableBitrate(
			  RTC::RembClient* rembClient, uint32_t availableBitrate, uint32_t previousAvailableBitrate) = 0;
		};

	public:
		RembClient(RTC::RembClient::Listener* listener, uint32_t initialAvailableBitrate);
		~RembClient();

	public:
		void ReceiveRembFeedback(RTC::RTCP::FeedbackPsRembPacket* remb);
		uint32_t GetAvailableBitrate();
		void RescheduleNextAvailableBitrateEvent();

	private:
		void CheckStatus(uint64_t now);

	private:
		// Passed by argument.
		Listener* listener{ nullptr };
		// Others.
		uint32_t initialAvailableBitrate{ 0 };
		uint64_t initialAvailableBitrateAt{ 0 };
		uint32_t availableBitrate{ 0 };
		uint64_t lastAvailableBitrateEventAt{ 0 };
	};
} // namespace RTC

#endif
