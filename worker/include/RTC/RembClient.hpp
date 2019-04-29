#ifndef MS_RTC_REMB_CLIENT_HPP
#define MS_RTC_REMB_CLIENT_HPP

#include "common.hpp"
#include "RTC/RTCP/FeedbackPsRemb.hpp"
#include "RTC/RtpPacket.hpp"
#include "RTC/RtpProbator.hpp"

namespace RTC
{
	class RembClient : public RTC::RtpProbator::Listener
	{
	public:
		class Listener
		{
		public:
			virtual void OnRembClientAvailableBitrate(
			  RTC::RembClient* rembClient, uint32_t availableBitrate) = 0;
			virtual void OnRembClientSendProbationRtpPacket(
			  RTC::RembClient* rembClient, RTC::RtpPacket* packet) = 0;
		};

	public:
		RembClient(RTC::RembClient::Listener* listener, uint32_t initialAvailableBitrate);
		virtual ~RembClient();

	public:
		void ReceiveRembFeedback(RTC::RTCP::FeedbackPsRembPacket* remb);
		void SentRtpPacket(RTC::RtpPacket* packet, bool retransmitted);
		uint32_t GetAvailableBitrate();
		void ResecheduleNextEvent();

	private:
		void CheckStatus(uint64_t now);

		/* Pure virtual methods inherited from RTC::RtpProbator::Listener. */
	public:
		void OnRtpProbatorSendRtpPacket(RTC::RtpProbator* rtpProbator, RTC::RtpPacket* packet) override;

	private:
		// Passed by argument.
		Listener* listener{ nullptr };
		// Allocated by this.
		RTC::RtpProbator* rtpProbator{ nullptr };
		// Others.
		uint32_t initialAvailableBitrate{ 0 };
		uint64_t initialAvailableBitrateAt{ 0 };
		uint32_t availableBitrate{ 0 };
		uint64_t lastEventAt{ 0 };
	};
} // namespace RTC

#endif
