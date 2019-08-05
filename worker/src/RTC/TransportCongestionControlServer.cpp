#define MS_CLASS "RTC::TransportCongestionControlServer"
// #define MS_LOG_DEV

#include "RTC/TransportCongestionControlServer.hpp"
#include "Logger.hpp"

namespace RTC
{
	/* Static. */

	static constexpr uint64_t FeedbackSendInterval{ 100u }; // In ms.

	/* Instance methods. */

	TransportCongestionControlServer::TransportCongestionControlServer(
	  RTC::TransportCongestionControlServer::Listener* listener, size_t maxRtcpPacketLen)
	  : listener(listener), maxRtcpPacketLen(maxRtcpPacketLen)
	{
		MS_TRACE();

		// Create a feedback packet.
		this->feedbackPacket.reset(new RTC::RTCP::FeedbackRtpTransportPacket(0, 0));

		// Set initial packet count.
		this->feedbackPacket->SetFeedbackPacketCount(this->feedbackPacketCount);

		// Create the feedback send periodic timer.
		this->feedbackSendPeriodicTimer = new Timer(this);
	}

	TransportCongestionControlServer::~TransportCongestionControlServer()
	{
		MS_TRACE();

		delete this->feedbackSendPeriodicTimer;
	}

	void TransportCongestionControlServer::TransportConnected()
	{
		MS_TRACE();

		this->feedbackSendPeriodicTimer->Start(FeedbackSendInterval, FeedbackSendInterval);
	}

	void TransportCongestionControlServer::TransportDisconnected()
	{
		MS_TRACE();

		this->feedbackSendPeriodicTimer->Stop();
	}

	void TransportCongestionControlServer::IncomingPacket(int64_t arrivalTimeMs, uint16_t wideSeqNumber)
	{
		MS_TRACE();

		// MS_DEBUG_DEV("[arrivalTimeMs:%" PRIu64 ", wideSeqNumber:%" PRIu16 "]", arrivalTimeMs, wideSeqNumber);

		// Provide the feedback packet with the RTP packet info. If it fails, send
		// current feedback and add the RTP packet to a new one.
		if (!this->feedbackPacket->AddPacket(wideSeqNumber, arrivalTimeMs, this->maxRtcpPacketLen))
		{
			MS_DEBUG_DEV("RTP packet cannot be added into the feedback packet, sending feedback now");

			// Notify the listener.
			this->listener->OnTransportCongestionControlServerSendRtcpPacket(
			  this, this->feedbackPacket.get());

			// Create a new feedback packet.
			this->feedbackPacket.reset(new RTC::RTCP::FeedbackRtpTransportPacket(0, 0));

			// Increment packet count.
			this->feedbackPacket->SetFeedbackPacketCount(++this->feedbackPacketCount);

			// Pass the packet info to the new feedback packet.
			this->feedbackPacket->AddPacket(wideSeqNumber, arrivalTimeMs, this->maxRtcpPacketLen);
		}

		// If the feedback packet is full, send it now.
		if (this->feedbackPacket->IsFull())
		{
			MS_DEBUG_DEV("feedback packet is full, sending feedback now");

			// Notify the listener.
			this->listener->OnTransportCongestionControlServerSendRtcpPacket(
			  this, this->feedbackPacket.get());

			// Create a new feedback packet.
			this->feedbackPacket.reset(new RTC::RTCP::FeedbackRtpTransportPacket(0, 0));

			// Increment packet count.
			this->feedbackPacket->SetFeedbackPacketCount(++this->feedbackPacketCount);
		}
	}

	inline void TransportCongestionControlServer::OnTimer(Timer* timer)
	{
		MS_TRACE();

		if (timer == this->feedbackSendPeriodicTimer)
		{
			if (feedbackPacket->GetPacketStatusCount() <= 1u)
				return;

			// Notify the listener.
			this->listener->OnTransportCongestionControlServerSendRtcpPacket(
			  this, this->feedbackPacket.get());

			// Create a new feedback packet.
			this->feedbackPacket.reset(new RTC::RTCP::FeedbackRtpTransportPacket(0, 0));

			// Increment packet count.
			this->feedbackPacket->SetFeedbackPacketCount(++this->feedbackPacketCount);
		}
	}
} // namespace RTC
