#define MS_CLASS "RTC::SenderBandwidthEstimator"
#define MS_LOG_DEV_LEVEL 3

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

	void SenderBandwidthEstimator::EstimateAvailableBitrate()
	{
		MS_TRACE();

		auto previousAvailableBitrate = this->availableBitrate;
		auto bitrates                 = GetBitrates();

		if (bitrates.sentBitrate == 0)
		{
			return;
		}

		double ratio =
		  static_cast<double>(bitrates.sentBitrate) / static_cast<double>(bitrates.recvBitrate);

		// RTP is being received properly.
		if (0.75f <= ratio && ratio <= 1.25f)
		{
			if (bitrates.recvBitrate > this->availableBitrate)
			{
				this->availableBitrate = bitrates.recvBitrate;

				MS_DEBUG_DEV("BWE UP [ratio:%f, availableBitrate:%" PRIu32 "]", ratio, this->availableBitrate);
			}
		}
		// RTP is being received worst than expected.
		else
		{
			if (bitrates.recvBitrate < this->availableBitrate)
			{
				// TODO: This 0.8 must be set acording to other values: dod, jitter, etc.
				this->availableBitrate *= 0.8;

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
			RemoveOldInfos();

			// TODO. Remove.
			auto bitrates = GetBitrates();

			MS_ERROR(
			  "sendBitrate:%" PRIu32 ", recvBitrate:%" PRIu32, bitrates.sentBitrate, bitrates.recvBitrate);

			EstimateAvailableBitrate();

			this->timer->Start(static_cast<uint64_t>(TimerInterval));
		}
	}

	void SenderBandwidthEstimator::RemoveOldInfos()
	{
		if (this->sentInfos.empty())
		{
			return;
		}

		const auto nowMs = DepLibUV::GetTimeMs();

		// A value of -1 indicates there is no old sendInfo.
		int32_t oldestWideSeq = -1;
		for (auto& kv : this->sentInfos)
		{
			auto& sentInfo = kv.second;

			if (sentInfo.sentAtMs < nowMs - MaxSentInfoAge)
			{
				oldestWideSeq = kv.first;

				continue;
			}
			// Following sentInfo's are newer.
			else
			{
				break;
			}
		}

		if (oldestWideSeq != -1)
		{
			auto sentInfosIt = this->sentInfos.lower_bound(oldestWideSeq);

			this->sentInfos.erase(this->sentInfos.begin(), sentInfosIt);
		}
	}

	SenderBandwidthEstimator::Bitrates SenderBandwidthEstimator::GetBitrates()
	{
		uint32_t totalBytes    = 0;
		uint64_t firstSentAtMs = 0;
		uint64_t lastSentAtMs  = 0;
		uint64_t firstRecvAtMs = 0;
		uint64_t lastRecvAtMs  = 0;
		SenderBandwidthEstimator::Bitrates bitrates;

		for (auto& kv : this->sentInfos)
		{
			auto& sentInfo = kv.second;

			// RTP packet feedback not received.
			if (!sentInfo.received)
			{
				continue;
			}

			if (!firstSentAtMs)
			{
				firstSentAtMs = sentInfo.sentAtMs;
				firstRecvAtMs = sentInfo.recvInfo.receivedAtMs;
				lastSentAtMs  = sentInfo.sentAtMs;
				lastRecvAtMs  = sentInfo.recvInfo.receivedAtMs;
			}

			// Handle disorder on sending.
			if (RTC::SeqManager<uint64_t>::IsSeqLowerThan(sentInfo.sentAtMs, firstSentAtMs))
			{
				firstSentAtMs = sentInfo.sentAtMs;
			}
			else if (RTC::SeqManager<uint64_t>::IsSeqHigherThan(sentInfo.sentAtMs, lastSentAtMs))
			{
				lastSentAtMs = sentInfo.sentAtMs;
			}

			// Handle disorder on receiving.
			if (RTC::SeqManager<uint64_t>::IsSeqLowerThan(sentInfo.recvInfo.receivedAtMs, firstRecvAtMs))
			{
				firstRecvAtMs = sentInfo.recvInfo.receivedAtMs;
			}
			else if (RTC::SeqManager<uint64_t>::IsSeqHigherThan(
			           sentInfo.recvInfo.receivedAtMs, lastRecvAtMs))
			{
				lastRecvAtMs = sentInfo.recvInfo.receivedAtMs;
			}

			// Increase total bytes.
			totalBytes += sentInfo.size;
		}

		// Zero bytes processed.
		if (totalBytes == 0)
		{
			bitrates.sentBitrate = 0;
			bitrates.sentBitrate = 0;

			return bitrates;
		}

		uint64_t sentTimeWindowMs = lastSentAtMs - firstSentAtMs;
		uint64_t recvTimeWindowMs = lastRecvAtMs - firstRecvAtMs;

		// All packets sent in the same millisecond.
		if (sentTimeWindowMs == 0)
		{
			sentTimeWindowMs = 1000;
		}

		// All packets received in the same millisecond.
		if (recvTimeWindowMs == 0)
		{
			recvTimeWindowMs = 1000;
		}

		// TODO: Remove.
		MS_DEBUG_DEV(
		  "totalBytes:%" PRIu32 ", sentTimeWindowMs:%" PRIu64 ",recvTimeWindowMs:%" PRIu64,
		  totalBytes,
		  sentTimeWindowMs,
		  recvTimeWindowMs);

		bitrates.sentBitrate =
		  static_cast<double>(totalBytes * 8) / (static_cast<double>(sentTimeWindowMs) / 1000.0f);
		bitrates.recvBitrate =
		  static_cast<double>(totalBytes * 8) / (static_cast<double>(recvTimeWindowMs) / 1000.0f);

		return bitrates;
	}
} // namespace RTC
