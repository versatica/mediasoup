#ifndef MS_RTC_REMB_CLIENT_HPP
#define MS_RTC_REMB_CLIENT_HPP

#include "common.hpp"
#include "RTC/RTCP/FeedbackPsRemb.hpp"
#include "RTC/RateCalculator.hpp"
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
			  RTC::RembClient* rembClient, uint32_t availableBitrate) = 0;
		};

	public:
		RembClient(
		  RTC::RembClient::Listener* listener,
		  uint32_t initialAvailableBitrate,
		  uint32_t minimumAvailableBitrate);
		virtual ~RembClient();

	public:
		void ReceiveRembFeedback(RTC::RTCP::FeedbackPsRembPacket* remb);
		void SentRtpPacket(RTC::RtpPacket* packet, bool retransmitted);
		void SentProbationRtpPacket(RTC::RtpPacket* packet);
		uint32_t GetAvailableBitrate();
		void ResecheduleNextEvent();
		bool IsProbationNeeded();

	private:
		void CheckStatus(uint64_t now);
		void CalculateProbationTargetBitrate();

	private:
		// Passed by argument.
		Listener* listener{ nullptr };
		// Others.
		uint32_t initialAvailableBitrate{ 0 };
		uint32_t minimumAvailableBitrate{ 0 };
		uint64_t initialAvailableBitrateAt{ 0 };
		uint32_t availableBitrate{ 0 };
		uint64_t lastEventAt{ 0 };
		uint32_t probationTargetBitrate{ 0 };
		RTC::RtpDataCounter transmissionCounter;
		RTC::RtpDataCounter probationTransmissionCounter;
	};
} // namespace RTC

#endif
