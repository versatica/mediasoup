#define MS_CLASS "RTC::RembClient"
// #define MS_LOG_DEV

#include "RTC/RembClient.hpp"
#include "DepLibUV.hpp"
#include "Logger.hpp"

namespace RTC
{
	/* Instance methods. */

	RembClient::RembClient(RTC::RembClient::Listener* listener) : listener(listener)
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

		// Update available bitrate.
		this->availableBitrate = static_cast<uint32_t>(remb->GetBitrate());

		// TODO: Testing.

		uint32_t rembBitrate = static_cast<uint32_t>(remb->GetBitrate());
		uint64_t now         = DepLibUV::GetTime();
		uint32_t bitrate     = this->transmissionCounter.GetRate(now);
		int32_t diff         = rembBitrate - bitrate;

		MS_ERROR(
		  "REMB received [sending bitrate:%" PRIu32 ", REMB available bitrate:%" PRIu32
		  ", diff:%" PRIi32 "]",
		  bitrate,
		  rembBitrate,
		  diff);
	}
} // namespace RTC
