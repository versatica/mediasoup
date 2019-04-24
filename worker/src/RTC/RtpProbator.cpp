#define MS_CLASS "RTC::RtpProbator"
// #define MS_LOG_DEV

#include "RTC/RtpProbator.hpp"
#include "DepLibUV.hpp"
#include "Logger.hpp"

namespace RTC
{
	/* Instance methods. */

	RtpProbator::RtpProbator(RTC::RtpProbator::Listener* listener) : listener(listener)
	{
		MS_TRACE();
	}

	RtpProbator::~RtpProbator()
	{
		MS_TRACE();
	}

	void RtpProbator::ForceStart(uint32_t probationTargetBitrate)
	{
		MS_TRACE();

		// This method enables probing while there is no info about available
		// bitrate.

		this->enabled                = true;
		this->probationTargetBitrate = probationTargetBitrate;
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

		// If transmission bitrate is 0 (or available bitrate is 0, rare) abort.
		if (transmissionBitrate == 0 || availableBitrate == 0)
		{
			if (wasEnabled)
				MS_DEBUG_DEV("probation disabled due no transmission");

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

			MS_DEBUG_DEV(
			  "probation enabled [bitrate:%" PRIu32 ", availableBitrate:%" PRIu32
			  ", factor:%f, probationTargetBitrate:%" PRIu32 "]",
			  transmissionBitrate,
			  availableBitrate,
			  factor,
			  this->probationTargetBitrate);
		}

		if (wasEnabled && !this->enabled)
		{
			MS_DEBUG_DEV(
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

		// Check whether we have to use this packet in probation.
		if (probationTransmissionBitrate <= this->probationTargetBitrate)
		{
			MS_DEBUG_DEV(
			  "sending probation RTP packet [probationTargetBitrate:%" PRIu32
			  ", probationTransmissionBitrate:%" PRIu32 "]",
			  this->probationTargetBitrate,
			  probationTransmissionBitrate);

			// Increase probation transmission counter.
			this->probationTransmissionCounter.Update(packet);

			// Send the probation packet.
			this->listener->OnRtpProbatorSendRtpPacket(this, packet);

			// May send it twice.
			probationTransmissionBitrate = this->probationTransmissionCounter.GetBitrate(now);

			if (probationTransmissionBitrate <= this->probationTargetBitrate)
			{
				MS_DEBUG_DEV(
				  "sending duplicated probation RTP packet [probationTargetBitrate:%" PRIu32
				  ", probationTransmissionBitrate:%" PRIu32 "]",
				  this->probationTargetBitrate,
				  probationTransmissionBitrate);

				// Increase probation transmission counter.
				this->probationTransmissionCounter.Update(packet);

				// Send the probation packet.
				this->listener->OnRtpProbatorSendRtpPacket(this, packet);
			}
		}
	}
} // namespace RTC
