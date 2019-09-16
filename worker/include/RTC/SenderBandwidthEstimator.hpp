#ifndef MS_RTC_SENDER_BANDWIDTH_ESTIMATOR_HPP
#define MS_RTC_SENDER_BANDWIDTH_ESTIMATOR_HPP

#include "common.hpp"
#include "RTC/RTCP/FeedbackRtpTransport.hpp"
#include "RTC/RtpPacket.hpp"

namespace RTC
{
	class SenderBandwidthEstimator
	{
	public:
		class Listener
		{
		public:
			virtual void OnSenderBandwidthEstimatorAvailableBitrate(
			  RTC::SenderBandwidthEstimator* senderBwe,
			  uint32_t availableBitrate,
			  uint32_t previousAvailableBitrate) = 0;
		};

	public:
		SenderBandwidthEstimator(
		  RTC::SenderBandwidthEstimator::Listener* listener, uint32_t initialAvailableBitrate);
		virtual ~SenderBandwidthEstimator();

	public:
		void TransportConnected();
		void TransportDisconnected();
		void RtpPacketToBeSent(RTC::RtpPacket* packet, uint64_t now);
		void RtpPacketSent(uint16_t wideSeqNumber, uint64_t now);
		void ReceiveRtcpTransportFeedback(const RTC::RTCP::FeedbackRtpTransportPacket* feedback);
		uint32_t GetAvailableBitrate() const;
		void RescheduleNextAvailableBitrateEvent();

	private:
		// Passed by argument.
		Listener* listener{ nullptr };
		// Others.
		uint32_t initialAvailableBitrate{ 0u };
		uint32_t availableBitrate{ 0u };
		uint64_t lastAvailableBitrateEventAt{ 0u };
	};
} // namespace RTC

#endif
