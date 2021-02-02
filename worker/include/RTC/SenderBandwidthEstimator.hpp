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
		struct DeltaOfDelta
		{
			uint16_t wideSeq{ 0u };
			uint64_t sentAtMs{ 0u };
			int16_t dod{ 0 };
		};

	public:
		class Listener
		{
		public:
			virtual void OnSenderBandwidthEstimatorAvailableBitrate(
			  RTC::SenderBandwidthEstimator* senderBwe,
			  uint32_t availableBitrate,
			  uint32_t previousAvailableBitrate,
			  uint32_t sendBirate,
			  uint32_t recvBirate) = 0;

			virtual void OnSenderBandwidthEstimatorDeltaOfDelta(
			  RTC::SenderBandwidthEstimator* senderBwe, std::vector<DeltaOfDelta>& deltaOfDeltas) = 0;
		};

	public:
		struct RecvInfo
		{
			uint64_t receivedAtMs{ 0u };
			int16_t delta{ 0 };
			int16_t dod{ 0 };
		};

		struct SentInfo
		{
			uint16_t wideSeq{ 0u };
			size_t size{ 0u };
			bool isProbation{ false };
			uint64_t sendingAtMs{ 0u };
			uint64_t sentAtMs{ 0u };
			bool received{ false };
			RecvInfo recvInfo;
		};

		struct Bitrates
		{
			uint32_t sendBitrate{ 0u };
			uint32_t recvBitrate{ 0u };
		};

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
		uint32_t GetAvailableBitrate() const;
		uint32_t GetSendBitrate() const;
		uint32_t GetRecvBitrate() const;
		void RescheduleNextAvailableBitrateEvent();

		/* Pure virtual methods inherited from Timer::Listener. */
	public:
		void OnTimer(Timer* timer) override;

	private:
		void RemoveOldInfos();
		void RemoveProcessedInfos();
		Bitrates GetBitrates();

	private:
		// Passed by argument.
		Listener* listener{ nullptr };
		// Others.
		uint32_t initialAvailableBitrate{ 0u };
		uint32_t availableBitrate{ 0u };
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
