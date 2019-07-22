#define MS_CLASS "RTC::RembClient"
// #define MS_LOG_DEV

#include "RTC/RembClient.hpp"
#include "DepLibUV.hpp"
#include "Logger.hpp"

namespace RTC
{
	/* Static. */

	static constexpr uint64_t AvailableBitrateEventInterval{ 2000u };       // In ms.
	static constexpr uint64_t MaxElapsedTime{ 5000u };                      // In ms.
	static constexpr uint64_t InitialAvailableBitrateDuration{ 8000u };     // in ms.
	static constexpr size_t RtpProbationPacketLen{ 1100u };                 // in bytes.
	static constexpr uint64_t RtpProbationScheduleSuccessTimeout{ 2000u };  // In ms.
	static constexpr uint64_t RtpProbationScheduleFailureTimeout{ 10000u }; // In ms.
	static constexpr uint8_t RtpProbationMaxFractionLost{ 10u };

	/* Instance methods. */

	RembClient::RembClient(RTC::RembClient::Listener* listener, uint32_t initialAvailableBitrate)
	  : listener(listener), initialAvailableBitrate(initialAvailableBitrate)
	{
		MS_TRACE();

		// Create a RTP probator.
		this->rtpProbator = new RTC::RtpProbator(this, RtpProbationPacketLen);

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

		this->rtpProbator->Stop();
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

		// Emit event if AvailableBitrateEventInterval elapsed.
		if (now - this->lastAvailableBitrateEventAt >= AvailableBitrateEventInterval)
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

			if (this->rtpProbator->IsActive())
			{
				this->rtpProbator->Stop();

				// Try again after RtpProbationScheduleFailureTimeout.
				this->rtpProbationScheduleTimer->Start(RtpProbationScheduleFailureTimeout, 0);
			}
		}

		if (notify)
		{
			this->lastAvailableBitrateEventAt = now;

			this->listener->OnRembClientAvailableBitrate(this, this->availableBitrate);
		}
	}

	void RembClient::ReceiveRtpProbatorReceiverReport(RTC::RTCP::ReceiverReport* report)
	{
		MS_TRACE();

		auto fractionLost = report->GetFractionLost();

		if (this->rtpProbator->IsActive() && fractionLost >= RtpProbationMaxFractionLost)
		{
			MS_DEBUG_TAG(bwe, "stopping RTP probator due to probation fraction lost:%" PRIu8, fractionLost);

			this->rtpProbator->Stop();

			// Try again after RtpProbationScheduleFailureTimeout.
			this->rtpProbationScheduleTimer->Start(RtpProbationScheduleFailureTimeout, 0);
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

		this->lastAvailableBitrateEventAt = DepLibUV::GetTime();
	}

	inline void RembClient::CheckStatus(uint64_t now)
	{
		MS_TRACE();

		if (now - this->lastAvailableBitrateEventAt > MaxElapsedTime)
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
			MS_DEBUG_TAG(bwe, "needed probation bitrate is now 0, stopping RTP probator");

			this->rtpProbator->Stop();

			// Try again after RtpProbationScheduleSuccessTimeout.
			this->rtpProbationScheduleTimer->Start(RtpProbationScheduleSuccessTimeout, 0);
		}
	}

	inline void RembClient::OnRtpProbatorEnded(RTC::RtpProbator* /*rtpProbator*/)
	{
		MS_TRACE();

		// Try again after RtpProbationScheduleSuccessTimeout.
		this->rtpProbationScheduleTimer->Start(RtpProbationScheduleSuccessTimeout, 0);
	}

	inline void RembClient::OnTimer(Timer* timer)
	{
		MS_TRACE();

		if (timer == this->rtpProbationScheduleTimer)
		{
			this->rtpProbator->Stop();

			uint32_t probationBitrate{ 0u };

			this->listener->OnRembClientNeedProbationBitrate(this, probationBitrate);

			if (probationBitrate == 0u)
			{
				// Try again after RtpProbationScheduleSuccessTimeout.
				this->rtpProbationScheduleTimer->Start(RtpProbationScheduleSuccessTimeout, 0);
			}
			else
			{
				this->rtpProbator->Start(probationBitrate);
			}
		}
	}
} // namespace RTC
