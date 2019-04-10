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

	RembClient::RembClient(RTC::RembClient::Listener* listener)
	  : listener(listener), lastEventAt(DepLibUV::GetTime())
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

		// Check if we should fire the event.
		uint64_t now = DepLibUV::GetTime();

		if (!HasRecentData())
		{
			MS_DEBUG_DEV(bwe, "ignoring first REMB after inactivity");

			// Update last event time.
			this->lastEventAt = now;

			// Set available bitrate to the maximum value.
			this->availableBitrate = std::numeric_limits<uint32_t>::max();

			// Update REMB bitrate.
			this->rembBitrate = static_cast<uint32_t>(remb->GetBitrate());

			return;
		}
		else if (now - this->lastEventAt < EventInterval)
		{
			return;
		}

		// Update last event time.
		this->lastEventAt = now;

		uint32_t newRembBitrate = static_cast<uint32_t>(remb->GetBitrate());
		int64_t rembTrend =
		  static_cast<int64_t>(newRembBitrate) - static_cast<int64_t>(this->rembBitrate);
		uint32_t bitrateInUse = this->transmissionCounter.GetRate(now);

		// Update REMB bitrate.
		this->rembBitrate = newRembBitrate;

		if (this->rembBitrate >= bitrateInUse)
		{
			uint32_t availableBitrate = this->rembBitrate - bitrateInUse;

			MS_DEBUG_TAG(
			  bwe,
			  "available bitrate [REMB:%" PRIu32 " >= bitrateInUse:%" PRIu32 ", availableBitrate:%" PRIu32
			  "]",
			  this->rembBitrate,
			  bitrateInUse,
			  availableBitrate);

			this->availableBitrate = availableBitrate;

			this->listener->OnRembClientAvailableBitrate(this, availableBitrate);
		}
		else if (rembTrend > 0)
		{
			MS_DEBUG_TAG(
			  bwe,
			  "positive REMB trend [REMB:%" PRIu32 " < bitrateInUse:%" PRIu32 ", trend:%" PRIi64 "]",
			  this->rembBitrate,
			  bitrateInUse,
			  rembTrend);

			this->availableBitrate = 0;
		}
		else
		{
			uint32_t exceedingBitrate = bitrateInUse - this->rembBitrate;

			MS_WARN_TAG(
			  bwe,
			  "exceeding bitrate [REMB:%" PRIu32 " < bitrateInUse:%" PRIu32 ", exceedingBitrate:%" PRIu32
			  "]",
			  this->rembBitrate,
			  bitrateInUse,
			  exceedingBitrate);

			this->availableBitrate = 0;

			this->listener->OnRembClientExceedingBitrate(this, exceedingBitrate);
		}
	}

	uint32_t RembClient::GetAvailableBitrate()
	{
		MS_TRACE();

		if (!HasRecentData())
			this->availableBitrate = std::numeric_limits<uint32_t>::max();

		  // TODO
		  MS_ERROR("--- availableBitrate:%" PRIu32, this->availableBitrate);

		return this->availableBitrate;
	}

	void RembClient::AddExtraBitrate(uint32_t extraBitrate)
	{
		MS_TRACE();

		// Update last event time.
		this->lastEventAt = DepLibUV::GetTime();

		if (!HasRecentData())
			this->availableBitrate = std::numeric_limits<uint32_t>::max();
		else if (this->availableBitrate > extraBitrate)
			this->availableBitrate -= extraBitrate;
		else
			this->availableBitrate = 0;
	}

	void RembClient::RemoveExtraBitrate(uint32_t extraBitrate)
	{
		MS_TRACE();

		// Update last event time.
		this->lastEventAt = DepLibUV::GetTime();

		if (!HasRecentData())
			this->availableBitrate = std::numeric_limits<uint32_t>::max();
		else if (std::numeric_limits<uint32_t>::max() - this->availableBitrate > extraBitrate)
			this->availableBitrate += extraBitrate;
		else
			this->availableBitrate = std::numeric_limits<uint32_t>::max();
	}

	inline bool RembClient::HasRecentData() const
	{
		MS_TRACE();

		return (DepLibUV::GetTime() - this->lastEventAt < MaxEventInterval);
	}
} // namespace RTC
