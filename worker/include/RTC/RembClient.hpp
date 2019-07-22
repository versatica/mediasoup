#ifndef MS_RTC_REMB_CLIENT_HPP
#define MS_RTC_REMB_CLIENT_HPP

#include "common.hpp"
#include "RTC/RTCP/FeedbackPsRemb.hpp"
#include "RTC/RateCalculator.hpp"
#include "RTC/RtpPacket.hpp"
#include "RTC/RtpProbator.hpp"
#include "handles/Timer.hpp"

namespace RTC
{
	class RembClient : public RTC::RtpProbator::Listener, public Timer::Listener
	{
	public:
		class Listener
		{
		public:
			virtual void OnRembClientAvailableBitrate(
			  RTC::RembClient* rembClient, uint32_t availableBitrate) = 0;
			virtual void OnRembClientNeedProbationBitrate(
			  RTC::RembClient* rembClient, uint32_t& probationBitrate) = 0;
			virtual void OnRembClientSendProbationRtpPacket(
			  RTC::RembClient* rembClient, RTC::RtpPacket* packet) = 0;
		};

	public:
		RembClient(RTC::RembClient::Listener* listener, uint32_t initialAvailableBitrate);
		virtual ~RembClient();

	public:
		void TransportConnected();
		void TransportDisconnected();
		void ReceiveRembFeedback(RTC::RTCP::FeedbackPsRembPacket* remb);
		uint32_t GetAvailableBitrate();
		void RescheduleNextAvailableBitrateEvent();

	private:
		void CheckStatus(uint64_t now);
		void CalculateProbationTargetBitrate(); // TODO

		/* Pure virtual methods inherited from RTC::RtpProbator. */
	public:
		void OnRtpProbatorSendRtpPacket(RTC::RtpProbator* rtpProbator, RTC::RtpPacket* packet) override;

		/* Pure virtual methods inherited from Timer::Listener. */
	public:
		void OnTimer(Timer* timer) override;

	private:
		// Passed by argument.
		Listener* listener{ nullptr };
		// Allocated by this.
		RTC::RtpProbator* rtpProbator{ nullptr };
		Timer* rtpProbationTimer{ nullptr };
		// Others.
		uint32_t initialAvailableBitrate{ 0 };
		uint64_t initialAvailableBitrateAt{ 0 };
		uint32_t availableBitrate{ 0 };
		uint64_t lastEventAt{ 0 };
		uint32_t probationTargetBitrate{ 0 };
		RTC::RtpDataCounter transmissionCounter;
		RTC::RtpDataCounter probationTransmissionCounter;
	};
} // namespace RTC

#endif
