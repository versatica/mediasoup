#ifndef MS_RTC_SENDER_BANDWIDTH_ESTIMATOR_HPP
#define MS_RTC_SENDER_BANDWIDTH_ESTIMATOR_HPP

#include "common.hpp"
#include "RTC/RTCP/FeedbackRtpTransport.hpp"
#include "RTC/RateCalculator.hpp"
#include "RTC/SeqManager.hpp"
#include "RTC/TrendCalculator.hpp"
#include "handles/Timer.hpp"
#include <map>
#include <vector>

namespace RTC
{
	class SenderBandwidthEstimator : public Timer::Listener
	{
	public:
		struct SentInfo
		{
			void Dump() const;
			uint16_t wideSeq{ 0u };
			size_t size{ 0u };
			bool isProbation{ false };
			uint64_t sendingAtMs{ 0u };
			uint64_t sentAtMs{ 0u };
			bool received{ false };
			int64_t receivedAtMs{ 0u };
			float rtt{ 0.f };
			int16_t dod{ 0 };
		};

		struct Bitrates
		{
			uint32_t availableBitrate{ 0u };
			uint32_t previousAvailableBitrate{ 0u };
			uint32_t sendBitrate{ 0u };
			uint32_t recvBitrate{ 0u };
		};

	public:
		class Listener
		{
		public:
			virtual void OnSenderBandwidthEstimatorAvailableBitrate(
			  RTC::SenderBandwidthEstimator* senderBwe, Bitrates& bitrates) = 0;

			virtual void OnSenderBandwidthEstimatorRtpFeedback(
			  RTC::SenderBandwidthEstimator* senderBwe, std::vector<SentInfo>& sentInfos) = 0;
		};

	public:
		struct SendRecvBitrates
		{
			uint32_t sendBitrate{ 0u };
			uint32_t recvBitrate{ 0u };
		};

		enum Trend
		{
			INCREASE = 0,
			DECREASE,
			HOLD,
		};

		std::string static TrendToString(Trend trend)
		{
			switch (trend)
			{
				case INCREASE:
					return "increase";
				case DECREASE:
					return "decrease";
				case HOLD:
					return "hold";
			}
		}

	public:
		SenderBandwidthEstimator(
		  RTC::SenderBandwidthEstimator::Listener* listener, uint32_t initialAvailableBitrate);
		virtual ~SenderBandwidthEstimator();

	public:
		void TransportConnected();
		void TransportDisconnected();
		void RtpPacketSent(SentInfo& sentInfo);
		void ReceiveRtcpTransportFeedback(const RTC::RTCP::FeedbackRtpTransportPacket* feedback);
		void EstimateAvailableBitrate();
		void UpdateRtt(float rtt);
		void SetDesiredBitrate(uint32_t desiredBitrate);
		uint32_t GetAvailableBitrate() const;
		void RescheduleNextAvailableBitrateEvent();

		/* Pure virtual methods inherited from Timer::Listener. */
	public:
		void OnTimer(Timer* timer) override;

	private:
		void RemoveOldInfos();
		SendRecvBitrates GetSendRecvBitrates();

	private:
		// Passed by argument.
		Listener* listener{ nullptr };
		// Others.
		uint32_t initialAvailableBitrate{ 0u };
		uint32_t availableBitrate{ 0u };
		uint32_t desiredBitrate{ 0u };
		uint64_t lastAvailableBitrateEventAtMs{ 0u };
		std::map<uint16_t, SentInfo, RTC::SeqManager<uint16_t>::SeqLowerThan> sentInfos;
		float rtt{ 0 }; // Round trip time in ms.
		RTC::RateCalculator sendTransmission;
		RTC::TrendCalculator sendTransmissionTrend;
		Timer* timer{ nullptr };
		uint16_t lastReceivedWideSeq{ 0u };
	};
} // namespace RTC

#endif
