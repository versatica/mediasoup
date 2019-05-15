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
	}

	RembClient::~RembClient()
	{
		MS_TRACE();
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
		// Otherwise if REMB reports less than minimumAvailableBitrate, honor it.
		else if (this->availableBitrate < this->minimumAvailableBitrate)
		{
			this->availableBitrate = this->minimumAvailableBitrate;
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
		}

		if (notify)
		{
			this->lastEventAt = now;

			this->listener->OnRembClientAvailableBitrate(this, this->availableBitrate);
		}

		CalculateProbationTargetBitrate();
	}

	void RembClient::SentRtpPacket(RTC::RtpPacket* packet, bool /*retransmitted*/)
	{
		MS_TRACE();

		// Increase transmission counter.
		this->transmissionCounter.Update(packet);
	}

	void RembClient::SentProbationRtpPacket(RTC::RtpPacket* packet)
	{
		MS_TRACE();

		MS_DEBUG_DEV("[seq:%" PRIu16 ", size:%zu]", packet->GetSequenceNumber(), packet->GetSize());

		// Increase probation transmission counter.
		this->probationTransmissionCounter.Update(packet);
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

	bool RembClient::IsProbationNeeded()
	{
		MS_TRACE();

		if (!this->probationTargetBitrate)
			return false;

		auto now                          = DepLibUV::GetTime();
		auto probationTransmissionBitrate = this->probationTransmissionCounter.GetBitrate(now);

		return (probationTransmissionBitrate <= this->probationTargetBitrate);
	}

	inline void RembClient::CheckStatus(uint64_t now)
	{
		MS_TRACE();

		if (now - this->lastEventAt > MaxElapsedTime)
		{
			MS_DEBUG_DEV(bwe, "resetting REMB client");

			this->initialAvailableBitrateAt = now;
			this->availableBitrate          = this->initialAvailableBitrate;

			CalculateProbationTargetBitrate();
		}
	}

	inline void RembClient::CalculateProbationTargetBitrate()
	{
		MS_TRACE();

		auto previousProbationTargetBitrate = this->probationTargetBitrate;

		this->probationTargetBitrate = 0;

		if (this->availableBitrate == 0)
			return;

		uint64_t now             = DepLibUV::GetTime();
		auto transmissionBitrate = this->transmissionCounter.GetBitrate(now);
		auto factor =
		  static_cast<float>(transmissionBitrate) / static_cast<float>(this->availableBitrate);

		// Just consider probation if transmission bitrate is close to available bitrate
		// (without exceeding it much).
		if (factor >= 0.8 && factor <= 1.2)
		{
			if (this->availableBitrate > transmissionBitrate)
				this->probationTargetBitrate = 2 * (this->availableBitrate - transmissionBitrate);
			else
				this->probationTargetBitrate = 0.5 * transmissionBitrate;
		}
		// If there is no bitrate, set available bitrate as probation target bitrate.
		else if (factor == 0)
		{
			this->probationTargetBitrate = this->availableBitrate;
		}

		if (this->probationTargetBitrate != previousProbationTargetBitrate)
		{
			MS_DEBUG_TAG(
			  bwe,
			  "probation %s [bitrate:%" PRIu32 ", availableBitrate:%" PRIu32
			  ", factor:%f, probationTargetBitrate:%" PRIu32 "]",
			  this->probationTargetBitrate ? "enabled" : "disabled",
			  transmissionBitrate,
			  this->availableBitrate,
			  factor,
			  this->probationTargetBitrate);
		}
	}
} // namespace RTC
