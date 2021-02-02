#define MS_CLASS "RTC::SenderBandwidthEstimator"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SenderBandwidthEstimator.hpp"
#include "DepLibUV.hpp"
#include "Logger.hpp"
#include <limits>

namespace RTC
{
	/* Static. */

	// static constexpr uint64_t AvailableBitrateEventInterval{ 2000u }; // In ms.
	static constexpr uint16_t MaxSentInfoAge{ 2000u }; // TODO: Let's see.
	static constexpr float DefaultRtt{ 100 };
	static constexpr uint16_t TimerInterval{ 1000u };

	/* Instance methods. */

	SenderBandwidthEstimator::SenderBandwidthEstimator(
	  RTC::SenderBandwidthEstimator::Listener* listener, uint32_t initialAvailableBitrate)
	  : listener(listener), initialAvailableBitrate(initialAvailableBitrate), rtt(DefaultRtt),
	    sendTransmission(1000u), sendTransmissionTrend(0.15f)
	{
		MS_TRACE();

		// Create the timer.
		this->timer = new Timer(this);
	}

	SenderBandwidthEstimator::~SenderBandwidthEstimator()
	{
		MS_TRACE();

		// Delete the timer.
		delete this->timer;
		this->timer = nullptr;
	}

	void SenderBandwidthEstimator::TransportConnected()
	{
		MS_TRACE();

		this->availableBitrate              = this->initialAvailableBitrate;
		this->lastAvailableBitrateEventAtMs = DepLibUV::GetTimeMs();

		// Start the timer.
		this->timer->Start(static_cast<uint64_t>(TimerInterval));
	}

	void SenderBandwidthEstimator::TransportDisconnected()
	{
		MS_TRACE();

		this->availableBitrate = 0u;

		this->sentInfos.clear();

		// Stop the timer.
		this->timer->Stop();
	}

	void SenderBandwidthEstimator::RtpPacketSent(SentInfo& sentInfo)
	{
		MS_TRACE();

		auto nowMs = sentInfo.sentAtMs;

		this->RemoveOldInfos();

		// Insert the sent info into the map.
		this->sentInfos[sentInfo.wideSeq] = sentInfo;

		// Fill the send transmission counter.
		this->sendTransmission.Update(sentInfo.size, nowMs);

		// Update the send transmission trend.
		auto sendBitrate = this->sendTransmission.GetRate(nowMs);

		this->sendTransmissionTrend.Update(sendBitrate, nowMs);

		// TODO: Remove.
		// MS_DEBUG_DEV(
		// 	"[wideSeq:%" PRIu16 ", size:%zu, isProbation:%s, bitrate:%" PRIu32 "]",
		// 	sentInfo.wideSeq,
		// 	sentInfo.size,
		// 	sentInfo.isProbation ? "true" : "false",
		// 	sendBitrate);
	}

	void SenderBandwidthEstimator::ReceiveRtcpTransportFeedback(
	  const RTC::RTCP::FeedbackRtpTransportPacket* feedback)
	{
		MS_TRACE();

		const auto packetResults = feedback->GetPacketResults();

		// TODO: Remove.
		// feedback->Dump();

		// For calling OnSenderBandwidthEstimatorDeltaOfDelta.
		std::vector<DeltaOfDelta> deltaOfDeltas;

		for (const auto& result : packetResults)
		{
			if (!result.received)
				continue;

			uint16_t wideSeq = result.sequenceNumber;
			auto sentInfosIt = this->sentInfos.find(wideSeq);

			if (sentInfosIt == this->sentInfos.end())
			{
				MS_WARN_DEV("received packet not present in sent infos [wideSeq:%" PRIu16 "]", wideSeq);

				continue;
			}

			auto& sentInfo = sentInfosIt->second;

			// Fill the RecvInfo.
			sentInfo.received              = true;
			sentInfo.recvInfo.receivedAtMs = result.receivedAtMs;
			sentInfo.recvInfo.delta        = result.delta;

			// Retrieve the RecvInfo of the previously received RTP packet in order to calculate
			// the delta.
			sentInfosIt = this->sentInfos.find(this->lastReceivedWideSeq);
			if (sentInfosIt == this->sentInfos.end())
			{
				this->lastReceivedWideSeq = wideSeq;

				continue;
			}

			this->lastReceivedWideSeq = wideSeq;

			auto& previousSentInfo = sentInfosIt->second;

			sentInfo.recvInfo.dod = (result.delta / 4) - (sentInfo.sentAtMs - previousSentInfo.sentAtMs);

			// Create and store the DeltaOfDelta.
			DeltaOfDelta deltaOfDelta;

			deltaOfDelta.wideSeq  = wideSeq;
			deltaOfDelta.sentAtMs = sentInfo.sentAtMs;
			deltaOfDelta.dod      = (result.delta / 4) - (sentInfo.sentAtMs - previousSentInfo.sentAtMs);

			deltaOfDeltas.push_back(deltaOfDelta);

			// TODO: Remove.
			// MS_DEBUG_DEV(
			//   "received delta for packet [wideSeq:%" PRIu16 ", send delta:%" PRIi32
			//   ", recv delta:%" PRIu64 "]",
			//   wideSeq,
			//   sentInfo.sentAtMs - previousSentInfo.sentAtMs,
			//   result.delta / 4);
		}

		// Notify listener.
		this->listener->OnSenderBandwidthEstimatorDeltaOfDelta(this, deltaOfDeltas);
	}

	void SenderBandwidthEstimator::EstimateAvailableBitrate(CummulativeResult& cummulativeResult)
	{
		MS_TRACE();

		auto previousAvailableBitrate = this->availableBitrate;

		double ratio = static_cast<double>(cummulativeResult.GetReceiveBitrate()) /
		               static_cast<double>(cummulativeResult.GetSendBitrate());
		auto bitrate =
		  std::min<uint32_t>(cummulativeResult.GetReceiveBitrate(), cummulativeResult.GetSendBitrate());

		if (0.75f <= ratio && ratio <= 1.25f)
		{
			if (bitrate > this->availableBitrate)
			{
				this->availableBitrate = bitrate;

				MS_DEBUG_DEV("BWE UP [ratio:%f, availableBitrate:%" PRIu32 "]", ratio, this->availableBitrate);
			}
		}
		else
		{
			if (bitrate < this->availableBitrate)
			{
				this->availableBitrate = bitrate;

				MS_DEBUG_DEV(
				  "BWE DOWN [ratio:%f, availableBitrate:%" PRIu32 "]", ratio, this->availableBitrate);
			}
		}

		// TODO: No, should wait for AvailableBitrateEventInterval and so on.
		this->listener->OnSenderBandwidthEstimatorAvailableBitrate(
		  this, this->availableBitrate, previousAvailableBitrate);
	}

	void SenderBandwidthEstimator::UpdateRtt(float rtt)
	{
		MS_TRACE();

		this->rtt = rtt;
	}

	uint32_t SenderBandwidthEstimator::GetAvailableBitrate() const
	{
		MS_TRACE();

		return this->availableBitrate;
	}

	void SenderBandwidthEstimator::RescheduleNextAvailableBitrateEvent()
	{
		MS_TRACE();

		this->lastAvailableBitrateEventAtMs = DepLibUV::GetTimeMs();
	}

	void SenderBandwidthEstimator::OnTimer(Timer* timer)
	{
		MS_TRACE();

		// RTCP timer.
		if (timer == this->timer)
		{
			// TODO.
			// RemoveOldInfos();
			// EstimateAvailableBitrate();

			this->timer->Start(static_cast<uint64_t>(TimerInterval));
		}
	}

	void SenderBandwidthEstimator::RemoveOldInfos()
	{
		if (this->sentInfos.empty())
		{
			return;
		}

		// TODO: Do it based on time. Remove items older than X MS.

		// Retrieve last sent wide seq.
		auto lastSentWideSeq = this->sentInfos.rbegin()->first;

		// Remove old sent infos.
		auto sentInfosIt = this->sentInfos.lower_bound(lastSentWideSeq - MaxSentInfoAge + 1);

		this->sentInfos.erase(this->sentInfos.begin(), sentInfosIt);
	}

	void SenderBandwidthEstimator::CummulativeResult::AddPacket(
	  size_t size, int64_t sentAtMs, int64_t receivedAtMs)
	{
		MS_TRACE();

		if (this->numPackets == 0u)
		{
			this->firstPacketSentAtMs     = sentAtMs;
			this->firstPacketReceivedAtMs = receivedAtMs;
			this->lastPacketSentAtMs      = sentAtMs;
			this->lastPacketReceivedAtMs  = receivedAtMs;
		}
		else
		{
			if (sentAtMs < this->firstPacketSentAtMs)
				this->firstPacketSentAtMs = sentAtMs;

			if (receivedAtMs < this->firstPacketReceivedAtMs)
				this->firstPacketReceivedAtMs = receivedAtMs;

			if (sentAtMs > this->lastPacketSentAtMs)
				this->lastPacketSentAtMs = sentAtMs;

			if (receivedAtMs > this->lastPacketReceivedAtMs)
				this->lastPacketReceivedAtMs = receivedAtMs;
		}

		this->numPackets++;
		this->totalSize += size;
	}

	void SenderBandwidthEstimator::CummulativeResult::Reset()
	{
		MS_TRACE();

		this->numPackets              = 0u;
		this->totalSize               = 0u;
		this->firstPacketSentAtMs     = 0u;
		this->lastPacketSentAtMs      = 0u;
		this->firstPacketReceivedAtMs = 0u;
		this->lastPacketReceivedAtMs  = 0u;
	}
} // namespace RTC
