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
	  RTC::TransportCongestionControlServer::Listener* listener)
	  : listener(listener)
	{
		MS_TRACE();

		// Create a TransportCongestionControl feedback packet.
		// TODO
		// this->feedbackPacket = new RTC::RTCP::FeedbackRtpTransportPacket();
	}

	TransportCongestionControlServer::~TransportCongestionControlServer()
	{
		MS_TRACE();

		// TODO
		// delete this->feedbackPacket;
	}

	void TransportCongestionControlServer::IncomingPacket(int64_t arrivalTimeMs, uint16_t wideSeqNumber)
	{
		MS_TRACE();

		// MS_DEBUG_DEV("[arrivalTimeMs:%" PRIu64 ", wideSeqNumber:%" PRIu16 "]", arrivalTimeMs, wideSeqNumber);

		// If this RTP packet arrives after more than MaxElapsedTimeBetweenRtpPackets
		// reset our RTCP Transport Feedback packet.
		// clang-format off
		if (
			this->lastRtpPacketReceivedAt != 0u &&
			arrivalTimeMs - this->lastRtpPacketReceivedAt > MaxElapsedTimeBetweenRtpPackets
		)
		// clang-format on
		{
			MS_DEBUG_DEV("too much time between RTP packets, resetting feedback packet");

			ResetFeedback();
		}

		this->lastRtpPacketReceivedAt = arrivalTimeMs;

		// Provide the feedback packet with the RTP packet info. If it fails, send
		// current feedback and add the RTP packet to a new one.
		// TODO
		// if (!this->feedbackPacket->AddPacket(wideSeqNumber, timestamp))
		// {
		// 	MS_DEBUG_DEV("RTP packet cannot be added into the feedback packet, sending feedback now");

		// 	SendFeedback();
		// 	ResetFeedback();

		// 	this->feedbackPacket->AddPacket(wideSeqNumber, timestamp);
		// }

		// If the feedback packet is full, send it now.
		// TODO
		// if (this->feedbackPacket->IsFull())
		// {
		// 	MS_DEBUG_DEV("feedback packet is full, sending feedback now");

		// 	SendFeedback();
		// 	ResetFeedback();
		// }
	}

	inline void TransportCongestionControlServer::SendFeedback()
	{
		MS_TRACE();

		// Notify the listener.
		// TODO
		// this->listener->OnTransportCongestionControlServerSendFeedback(this, this->feedbackPacket);
	}

	inline void TransportCongestionControlServer::ResetFeedback()
	{
		MS_TRACE();

		// TODO
		// delete this->feedbackPacket;

		// TODO
		// this->feedbackPacket = new RTC::RTCP::FeedbackRtpTransportPacket();
	}
} // namespace RTC
