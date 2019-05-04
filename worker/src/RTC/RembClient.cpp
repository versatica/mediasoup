#define MS_CLASS "RTC::RembClient"
// #define MS_LOG_DEV

#include "RTC/RembClient.hpp"
#include "DepLibUV.hpp"
#include "Logger.hpp"

namespace RTC
{
	/* Static. */

	static constexpr uint64_t EventInterval{ 2000 };                   // In ms.
	static constexpr uint64_t MaxElapsedTime{ 5000 };                  // In ms.
	static constexpr uint64_t InitialAvailableBitrateDuration{ 8000 }; // in ms.

	/* Instance methods. */

	RembClient::RembClient(
	  RTC::RembClient::Listener* listener,
	  uint32_t initialAvailableBitrate,
	  uint32_t minimumAvailableBitrate)
	  : listener(listener), initialAvailableBitrate(initialAvailableBitrate),
	    minimumAvailableBitrate(minimumAvailableBitrate)
	{
		MS_TRACE();

		this->rtpProbator = new RTC::RtpProbator(this);
	}

	RembClient::~RembClient()
	{
		MS_TRACE();

		delete this->rtpProbator;
	}

	void RembClient::ReceiveRembFeedback(RTC::RTCP::FeedbackPsRembPacket* remb)
	{
		MS_TRACE();

		uint64_t now = DepLibUV::GetTime();

		CheckStatus(now);

		// Update availableBitrate.
		this->availableBitrate = static_cast<uint32_t>(remb->GetBitrate());

		// If REMB reports less than initialAvailableBitrate during
		// InitialAvailableBitrateDuration, honor initialAvailableBitrate.
		// clang-format off
		if (
			this->availableBitrate < this->initialAvailableBitrate &&
			now - this->initialAvailableBitrateAt <= InitialAvailableBitrateDuration
		)
		// clang-format on
		{
			this->availableBitrate = this->initialAvailableBitrate;
		}
		// Otherwise if REMB reports less than minimumAvailableBitrate, honor it.
		else if (this->availableBitrate < this->minimumAvailableBitrate)
		{
			this->availableBitrate = this->minimumAvailableBitrate;
		}

		// Emit event if EventInterval elapsed.
		if (now - this->lastEventAt >= EventInterval)
		{
			this->lastEventAt = now;

			this->listener->OnRembClientAvailableBitrate(this, this->availableBitrate);
		}

		// Pass available bitrate to the RtpProbator.
		this->rtpProbator->UpdateAvailableBitrate(this->availableBitrate);
	}

	void RembClient::SentRtpPacket(RTC::RtpPacket* packet, bool retransmitted)
	{
		MS_TRACE();

		// Pass the packet to the RtpProbator.
		this->rtpProbator->ReceiveRtpPacket(packet, retransmitted);
	}

	uint32_t RembClient::GetAvailableBitrate()
	{
		MS_TRACE();

		uint64_t now = DepLibUV::GetTime();

		CheckStatus(now);

		return this->availableBitrate;
	}

	void RembClient::ResecheduleNextEvent()
	{
		MS_TRACE();

		this->lastEventAt = DepLibUV::GetTime();
	}

	inline void RembClient::CheckStatus(uint64_t now)
	{
		MS_TRACE();

		if (now - this->lastEventAt > MaxElapsedTime)
		{
			MS_DEBUG_DEV(bwe, "resetting REMB client");

			this->initialAvailableBitrateAt = now;
			this->availableBitrate          = this->initialAvailableBitrate;

			// Tell the RTP probator to start probing even before receiving REMB
			// feedbacks.
			this->rtpProbator->UpdateAvailableBitrate(this->initialAvailableBitrate);
		}
	}

	inline void RembClient::OnRtpProbatorSendRtpPacket(
	  RTC::RtpProbator* /*rtpProbator*/, RTC::RtpPacket* packet)
	{
		MS_TRACE();

		this->listener->OnRembClientSendProbationRtpPacket(this, packet);
	}
} // namespace RTC
