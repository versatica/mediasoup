#define MS_CLASS "RTC::RembClient"
#define MS_LOG_DEV // TODO

#include "RTC/RembClient.hpp"
#include "DepLibUV.hpp"
#include "Logger.hpp"
#include <limits>

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

		if (now - this->lastEventAt >= MaxEventInterval)
		{
			MS_DEBUG_TAG(bwe, "ignoring first REMB after inactivity");

			// Update last event time.
			this->lastEventAt = now;

			return;
		}
		else if (now - this->lastEventAt < EventInterval)
		{
			return;
		}

		uint32_t newRembBitrate = static_cast<uint32_t>(remb->GetBitrate());
		auto rembTrend = static_cast<int64_t>(newRembBitrate) - static_cast<int64_t>(this->rembBitrate);

		// Update REMB bitrate.
		this->rembBitrate = newRembBitrate;

		// Update bitrate in use.
		this->bitrateInUse = this->transmissionCounter.GetRate(now);

		// Update last event time.
		this->lastEventAt = now;

		if (this->rembBitrate >= this->bitrateInUse)
		{
			uint32_t availableBitrate = this->rembBitrate - this->bitrateInUse;

			MS_DEBUG_TAG(
			  bwe,
			  "available bitrate [REMB:%" PRIu32 " >= bitrateInUse:%" PRIu32 ", availableBitrate:%" PRIu32
			  "]",
			  this->rembBitrate,
			  this->bitrateInUse,
			  availableBitrate);
		}
		else if (rembTrend > 0)
		{
			MS_DEBUG_TAG(
			  bwe,
			  "positive REMT trend [REMB:%" PRIu32 " < bitrateInUse:%" PRIu32 ", trend:%" PRIi64 "]",
			  this->rembBitrate,
			  this->bitrateInUse,
			  rembTrend);
		}
		else
		{
			uint32_t exceedingBitrate = this->bitrateInUse - this->rembBitrate;

			MS_WARN_TAG(
			  bwe,
			  "exceeding bitrate [REMB:%" PRIu32 " < bitrateInUse:%" PRIu32 ", exceedingBitrate:%" PRIu32
			  "]",
			  this->rembBitrate,
			  this->bitrateInUse,
			  exceedingBitrate);
		}
	}

	uint32_t RembClient::GetAvailableBitrate() const
	{
		MS_TRACE();

		// if (this->bitrateInUse == 0)
		// 	return std::numeric_limits<uint32_t>::max();
		// else if (this->availableBitrate >= this->extraBitrateInUse)
		// 	return this->availableBitrate - this->extraBitrateInUse;
		// else
		// 	return 0;

		return 1234;
	}

	void RembClient::AddExtraBitrate(uint32_t extraBitrate)
	{
		MS_TRACE();

		// Update last event time.
		this->lastEventAt = DepLibUV::GetTime();

		// Increase extra bitrate in use.
		this->extraBitrateInUse += extraBitrate;
	}

	void RembClient::RemoveExtraBitrate(uint32_t extraBitrate)
	{
		MS_TRACE();

		// Update last event time.
		this->lastEventAt = DepLibUV::GetTime();

		// Decrease extra bitrate in use.
		if (this->extraBitrateInUse >= extraBitrate)
			this->extraBitrateInUse -= extraBitrate;
		else
			this->extraBitrateInUse = 0;
	}
} // namespace RTC
