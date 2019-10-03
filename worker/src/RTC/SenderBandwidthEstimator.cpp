#define MS_CLASS "RTC::SenderBandwidthEstimator"
// #define MS_LOG_DEV // TODO

#include "RTC/SenderBandwidthEstimator.hpp"
#include "DepLibUV.hpp"
#include "Logger.hpp"
#include <limits>

namespace RTC
{
	/* Static. */

	static constexpr uint64_t AvailableBitrateEventInterval{ 2000u }; // In ms.

	/* Instance methods. */

	SenderBandwidthEstimator::SenderBandwidthEstimator(
	  RTC::SenderBandwidthEstimator::Listener* listener, uint32_t initialAvailableBitrate)
	  : listener(listener), initialAvailableBitrate(initialAvailableBitrate)
	{
		MS_TRACE();
	}

	SenderBandwidthEstimator::~SenderBandwidthEstimator()
	{
		MS_TRACE();
	}

	void SenderBandwidthEstimator::TransportConnected()
	{
		MS_TRACE();

		this->availableBitrate            = this->initialAvailableBitrate;
		this->lastAvailableBitrateEventAt = DepLibUV::GetTimeMs();
	}

	void SenderBandwidthEstimator::TransportDisconnected()
	{
		MS_TRACE();

		this->availableBitrate = 0u;

		// TODO: Reset all status (maps, etc).
	}

	void SenderBandwidthEstimator::RtpPacketToBeSent(RTC::RtpPacket* packet, uint64_t now)
	{
		MS_TRACE();

		uint16_t wideSeqNumber;

		// Ignore packet if transport-wide-cc extension is not present.
		if (!packet->ReadTransportWideCc01(wideSeqNumber))
		{
			MS_WARN_DEV("ignoring RTP packet without transport-wide-cc extension");

			return;
		}

		// TODO: more

		// TODO: Remove.
		MS_DEBUG_DEV("[wideSeqNumber:%" PRIu16 ", size:%zu]", wideSeqNumber, packet->GetSize());
	}

	void SenderBandwidthEstimator::RtpPacketSent(uint16_t wideSeqNumber, uint64_t now)
	{
		MS_TRACE();

		// TODO: more

		// TODO: Remove.
		MS_DEBUG_DEV("[wideSeqNumber:%" PRIu16 "]", wideSeqNumber);
	}

	void SenderBandwidthEstimator::ReceiveRtcpTransportFeedback(
	  const RTC::RTCP::FeedbackRtpTransportPacket* feedback)
	{
		MS_TRACE();

		// TODO: more

		// TODO Remove.
#ifdef MS_LOG_DEV
		feedback->Dump();
#endif
	}

	uint32_t SenderBandwidthEstimator::GetAvailableBitrate() const
	{
		MS_TRACE();

		return this->availableBitrate;
	}

	void SenderBandwidthEstimator::RescheduleNextAvailableBitrateEvent()
	{
		MS_TRACE();

		this->lastAvailableBitrateEventAt = DepLibUV::GetTimeMs();
	}
} // namespace RTC
