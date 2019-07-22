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

	RembClient::RembClient(RTC::RembClient::Listener* listener, uint32_t initialAvailableBitrate)
	  : listener(listener), initialAvailableBitrate(initialAvailableBitrate)
	{
		MS_TRACE();

		// Create a RTP probator.
		// TODO: Set proper provation packet size.
		this->rtpProbator = new RTC::RtpProbator(this, 1100);

		// Create the RTP probation timer.
		this->rtpProbationScheduleTimer = new Timer(this);
	}

	RembClient::~RembClient()
	{
		MS_TRACE();

		// Delete the RTP probator.
		delete this->rtpProbator;

		// Delete the RTP probation timer.
		delete this->rtpProbationScheduleTimer;
	}

	void RembClient::TransportConnected()
	{
		MS_TRACE();

		this->rtpProbationScheduleTimer->Start(0, 0);
	}

	void RembClient::TransportDisconnected()
	{
		MS_TRACE();

		this->rtpProbationScheduleTimer->Stop();
	}

	void RembClient::ReceiveRembFeedback(RTC::RTCP::FeedbackPsRembPacket* remb)
	{
		MS_TRACE();

		auto previousAvailableBitrate = this->availableBitrate;
		uint64_t now                  = DepLibUV::GetTime();
		bool notify{ false };

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

		// Emit event if EventInterval elapsed.
		if (now - this->lastEventAt >= EventInterval)
		{
			notify = true;
		}
		// Also emit the event fast if we detect a high REMB value decrease.
		else if (this->availableBitrate < previousAvailableBitrate * 0.75)
		{
			MS_WARN_TAG(
			  bwe,
			  "high REMB value decrease detected, notifying the listener [before:%" PRIu32
			  ", now:%" PRIu32 "]",
			  previousAvailableBitrate,
			  this->availableBitrate);

			notify = true;

			// TODO: Let's see.
			if (this->rtpProbator->IsActive())
			{
				this->rtpProbator->Stop();
				this->rtpProbationScheduleTimer->Start(10000, 0);
			}
		}

		if (notify)
		{
			this->lastEventAt = now;

			this->listener->OnRembClientAvailableBitrate(this, this->availableBitrate);
		}
	}

	uint32_t RembClient::GetAvailableBitrate()
	{
		MS_TRACE();

		uint64_t now = DepLibUV::GetTime();

		CheckStatus(now);

		return this->availableBitrate;
	}

	void RembClient::RescheduleNextAvailableBitrateEvent()
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
		}
	}

	inline void RembClient::OnRtpProbatorSendRtpPacket(
	  RTC::RtpProbator* /*rtpProbator*/, RTC::RtpPacket* packet)
	{
		MS_TRACE();

		// Notify the listener.
		this->listener->OnRembClientSendProbationRtpPacket(this, packet);
	}

	inline void RembClient::OnRtpProbatorStep(RTC::RtpProbator* /*rtpProbator*/)
	{
		MS_TRACE();

		uint32_t probationBitrate{ 0u };

		this->listener->OnRembClientNeedProbationBitrate(this, probationBitrate);

		if (probationBitrate == 0u)
		{
			// TODO
			MS_DUMP("probation bitrate is now 0, stopping RTP probator!!!");

			this->rtpProbator->Stop();

			// TODO: Let's see.
			this->rtpProbationScheduleTimer->Start(2000, 0);
		}
	}

	inline void RembClient::OnRtpProbatorEnded(RTC::RtpProbator* /*rtpProbator*/)
	{
		MS_TRACE();

		// TODO: Let's see.
		this->rtpProbationScheduleTimer->Start(5000, 0);
	}

	inline void RembClient::OnTimer(Timer* timer)
	{
		MS_TRACE();

		if (timer == this->rtpProbationScheduleTimer)
		{
			// TODO: TMP

			this->rtpProbator->Stop();

			uint32_t probationBitrate{ 0u };

			this->listener->OnRembClientNeedProbationBitrate(this, probationBitrate);

			if (probationBitrate != 0u)
			{
				this->rtpProbator->Start(probationBitrate);
			}
			else
			{
				// TODO: Let's see.
				this->rtpProbationScheduleTimer->Start(2000, 0);
			}
		}
	}
} // namespace RTC
