#define MS_CLASS "RTC::TransportCongestionControlServer"
// #define MS_LOG_DEV

#include "RTC/TransportCongestionControlServer.hpp"
#include "Logger.hpp"

namespace RTC
{
	/* Static. */

	static constexpr uint64_t MaxElapsedTimeBetweenRtpPackets{ 5000u }; // In ms.

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
	}

	TransportCongestionControlServer::~TransportCongestionControlServer()
	{
		MS_TRACE();
	}

	void TransportCongestionControlServer::IncomingPacket(int64_t arrivalTimeMs, uint16_t wideSeqNumber)
	{
		MS_TRACE();

		// MS_DEBUG_DEV("[arrivalTimeMs:%" PRIu64 ", wideSeqNumber:%" PRIu16 "]", arrivalTimeMs, wideSeqNumber);

		// If this RTP packet arrives after more than MaxElapsedTimeBetweenRtpPackets
		// reset our feedback packet.
		// clang-format off
		if (
			this->lastRtpPacketReceivedAt != 0u &&
			arrivalTimeMs - this->lastRtpPacketReceivedAt > MaxElapsedTimeBetweenRtpPackets
		)
		// clang-format on
		{
			MS_DEBUG_DEV("too much time between RTP packets, resetting feedback packet");

			// Create a new feedback packet.
			this->feedbackPacket.reset(new RTC::RTCP::FeedbackRtpTransportPacket(0, 0));

			// Keep the previous packet count.
			this->feedbackPacket->SetFeedbackPacketCount(this->feedbackPacketCount);
		}

		this->lastRtpPacketReceivedAt = arrivalTimeMs;

		// Provide the feedback packet with the RTP packet info. If it fails, send
		// current feedback and add the RTP packet to a new one.
		if (!this->feedbackPacket->AddPacket(wideSeqNumber, arrivalTimeMs, this->maxRtcpPacketLen))
		{
			MS_DEBUG_DEV("RTP packet cannot be added into the feedback packet, sending feedback now");

			// Notify the listener.
			this->listener->OnTransportCongestionControlServerSendRtcpPacket(this, this->feedbackPacket.get());

			// Create a new feedback packet.
			this->feedbackPacket.reset(new RTC::RTCP::FeedbackRtpTransportPacket(0, 0));

			// Increment packet count.
			this->feedbackPacket->SetFeedbackPacketCount(++this->feedbackPacketCount);
		}

		// If the feedback packet is full, send it now.
		if (this->feedbackPacket->IsFull())
		{
			MS_DEBUG_DEV("feedback packet is full, sending feedback now");

			// Notify the listener.
			this->listener->OnTransportCongestionControlServerSendRtcpPacket(this, this->feedbackPacket.get());

			// Create a new feedback packet.
			this->feedbackPacket.reset(new RTC::RTCP::FeedbackRtpTransportPacket(0, 0));

			// Increment packet count.
			this->feedbackPacket->SetFeedbackPacketCount(++this->feedbackPacketCount);
		}
	}
} // namespace RTC
