#define MS_CLASS "RTC::RembClient"
// #define MS_LOG_DEV

#include "RTC/RembClient.hpp"
#include "DepLibUV.hpp"
#include "Logger.hpp"

namespace RTC
{
	/* Static. */

	static constexpr uint64_t EventInterval{ 1500 };    // In ms.
	static constexpr uint64_t MaxEventInterval{ 3000 }; // In ms.

	/* Instance methods. */

	RembClient::RembClient(RTC::RembClient::Listener* listener, uint32_t initialAvailableBitrate)
	  : listener(listener), initialAvailableBitrate(initialAvailableBitrate),
	    availableBitrate(initialAvailableBitrate)
	{
		MS_TRACE();
	}

	RembClient::~RembClient()
	{
		MS_TRACE();
	}

	void RembClient::ReceiveRtpPacket(RTC::RtpPacket* packet)
	{
		MS_TRACE();

		this->transmissionCounter.Update(packet);
	}

	void RembClient::ReceiveRembFeedback(RTC::RTCP::FeedbackPsRembPacket* remb)
	{
		MS_TRACE();

		uint64_t now = DepLibUV::GetTime();

		// If we don't have recent data yet, start from here.
		if (!CheckStatus())
		{
			// Update last event time.
			this->lastEventAt = now;

			return;
		}
		// Otherwise ensure EventInterval has happened.
		else if (this->lastEventAt != 0 && (now - this->lastEventAt) < EventInterval)
		{
			return;
		}

		// Update last event time.
		this->lastEventAt = now;

		auto previousAvailableBitrate = this->availableBitrate;

		// Update available bitrate.
		this->availableBitrate = static_cast<uint32_t>(remb->GetBitrate());

		int64_t trend =
		  static_cast<int64_t>(this->availableBitrate) - static_cast<int64_t>(previousAvailableBitrate);
		uint32_t usedBitrate = this->transmissionCounter.GetBitrate(now);

		if (this->availableBitrate >= usedBitrate)
		{
			uint32_t usableBitrate = this->availableBitrate - usedBitrate;

			MS_DEBUG_TAG(
			  bwe,
			  "usable bitrate [availableBitrate:%" PRIu32 " >= usedBitrate:%" PRIu32
			  ", usableBitrate:%" PRIu32 "]",
			  this->availableBitrate,
			  usedBitrate,
			  usableBitrate);

			this->listener->OnRembClientIncreaseBitrate(this, usableBitrate);
		}
		else if (trend > 0)
		{
			MS_DEBUG_TAG(
			  bwe,
			  "positive REMB trend [availableBitrate:%" PRIu32 " < usedBitrate:%" PRIu32
			  ", trend:%" PRIi64 "]",
			  this->availableBitrate,
			  usedBitrate,
			  trend);
		}
		else
		{
			uint32_t exceedingBitrate = usedBitrate - this->availableBitrate;

			MS_WARN_TAG(
			  bwe,
			  "exceeding bitrate [availableBitrate:%" PRIu32 " < usedBitrate:%" PRIu32
			  ", exceedingBitrate:%" PRIu32 "]",
			  this->availableBitrate,
			  usedBitrate,
			  exceedingBitrate);

			this->listener->OnRembClientDecreaseBitrate(this, exceedingBitrate);
		}
	}

	uint32_t RembClient::GetAvailableBitrate()
	{
		MS_TRACE();

		CheckStatus();

		return this->availableBitrate;
	}

	inline bool RembClient::CheckStatus()
	{
		MS_TRACE();

		uint64_t now = DepLibUV::GetTime();

		if (this->lastEventAt != 0 && (now - this->lastEventAt) < MaxEventInterval)
		{
			return true;
		}
		else
		{
			this->availableBitrate = this->initialAvailableBitrate;

			return false;
		}
	}
} // namespace RTC
