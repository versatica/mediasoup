#define MS_CLASS "RTC::RtpProbator"
// #define MS_LOG_DEV

#include "RTC/RtpProbator.hpp"
#include "DepLibUV.hpp"
#include "Logger.hpp"
#include "Utils.hpp"

namespace RTC
{
	/* Instance methods. */

	RtpProbator::RtpProbator(RTC::RtpProbator::Listener* listener)
	  : listener(listener), ssrc(Utils::Crypto::GetRandomUInt(1000, 1999))
	{
		MS_TRACE();
	}

	RtpProbator::~RtpProbator()
	{
		MS_TRACE();
	}

	void RtpProbator::UpdateAvailableBitrate(uint32_t availableBitrate)
	{
		MS_TRACE();

		auto wasEnabled = this->enabled;

		// Let's do this stateless.
		this->enabled                = false;
		this->probationTargetBitrate = 0;

		auto now                 = DepLibUV::GetTime();
		auto transmissionBitrate = this->transmissionCounter.GetBitrate(now);

		if (availableBitrate == 0)
		{
			if (wasEnabled)
				MS_DEBUG_TAG(bwe, "probation disabled due zero available bitrate");

			return;
		}

		// Just enable probation if transmission bitrate is close to available bitrate
		// (without exceeding it).
		auto factor = static_cast<float>(transmissionBitrate) / static_cast<float>(availableBitrate);

		if (factor >= 0.8 && factor <= 1.2)
		{
			this->enabled = true;

			if (availableBitrate > transmissionBitrate)
				this->probationTargetBitrate = 2 * (availableBitrate - transmissionBitrate);
			else
				this->probationTargetBitrate = 0.5 * transmissionBitrate;
		}
		// If there is no bitrate, set available bitrate as probation target bitrate.
		else if (factor == 0)
		{
			this->enabled                = true;
			this->probationTargetBitrate = availableBitrate;
		}

		if (!wasEnabled && this->enabled)
		{
			MS_DEBUG_TAG(
			  bwe,
			  "probation enabled [bitrate:%" PRIu32 ", availableBitrate:%" PRIu32
			  ", factor:%f, probationTargetBitrate:%" PRIu32 "]",
			  transmissionBitrate,
			  availableBitrate,
			  factor,
			  this->probationTargetBitrate);
		}
		else if (wasEnabled && this->enabled)
		{
			MS_DEBUG_DEV(
			  "probation updated [bitrate:%" PRIu32 ", availableBitrate:%" PRIu32
			  ", factor:%f, probationTargetBitrate:%" PRIu32 "]",
			  transmissionBitrate,
			  availableBitrate,
			  factor,
			  this->probationTargetBitrate);
		}
		else if (wasEnabled && !this->enabled)
		{
			MS_DEBUG_TAG(
			  bwe,
			  "probation disabled [bitrate:%" PRIu32 ", availableBitrate:%" PRIu32 ", factor:%f]",
			  transmissionBitrate,
			  availableBitrate,
			  factor);
		}
	}

	void RtpProbator::ReceiveRtpPacket(RTC::RtpPacket* packet, bool retransmitted)
	{
		MS_TRACE();

		// Increase transmission counter.
		this->transmissionCounter.Update(packet);

		// If probation disabled or a retransmitted packet, exit here.
		if (!this->enabled || retransmitted)
			return;

		auto now                          = DepLibUV::GetTime();
		auto probationTransmissionBitrate = this->probationTransmissionCounter.GetBitrate(now);
		auto origSsrc                     = packet->GetSsrc();
		auto origSeq                      = packet->GetSequenceNumber();

		packet->SetSsrc(this->ssrc);

		for (int count{ 1 }; count <= 4; ++count)
		{
			if (probationTransmissionBitrate >= this->probationTargetBitrate)
				break;

			packet->SetSequenceNumber(this->seq++);

			MS_DEBUG_DEV(
			  "sending probation RTP packet [count:%d, probationTargetBitrate:%" PRIu32
			  ", probationTransmissionBitrate:%" PRIu32 ", ssrc:%" PRIu32 ", seq:%" PRIu16 "]",
			  count,
			  this->probationTargetBitrate,
			  probationTransmissionBitrate,
			  this->ssrc,
			  this->seq);

			// Increase probation transmission counter.
			this->probationTransmissionCounter.Update(packet);

			// Send the probation packet.
			this->listener->OnRtpProbatorSendRtpPacket(this, packet);

			// Update probation bitrate.
			probationTransmissionBitrate = this->probationTransmissionCounter.GetBitrate(now);
		}

		// Restore packet fields.
		packet->SetSsrc(origSsrc);
		packet->SetSequenceNumber(origSeq);
	}
} // namespace RTC
