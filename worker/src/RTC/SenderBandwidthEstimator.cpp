#define MS_CLASS "RTC::SenderBandwidthEstimator"
#define MS_LOG_DEV_LEVEL 3

#include "RTC/SenderBandwidthEstimator.hpp"
#include "DepLibUV.hpp"
#include "Logger.hpp"
#include "Utils.hpp"
#include <limits>

namespace RTC
{
	/* Static. */

	// static constexpr uint64_t AvailableBitrateEventInterval{ 2000u }; // In ms.
	static constexpr float MaxBitrateIncrementFactor{ 1.35f };
	static constexpr float DefaultRtt{ 100 };
	static constexpr uint16_t TimerInterval{ 500u };

	/* Instance methods. */

	SenderBandwidthEstimator::SenderBandwidthEstimator(
	  RTC::SenderBandwidthEstimator::Listener* listener, uint32_t initialAvailableBitrate)
	  : listener(listener), initialAvailableBitrate(initialAvailableBitrate),
	    availableBitrate(initialAvailableBitrate), rtt(DefaultRtt), sendTransmission(1000u),
	    sendTransmissionTrend(0.15f)
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

			sentInfo.recvInfo.dod = (result.receivedAtMs - previousSentInfo.recvInfo.receivedAtMs) -
			                        (sentInfo.sentAtMs - previousSentInfo.sentAtMs);

			// Create and store the DeltaOfDelta.
			DeltaOfDelta deltaOfDelta;

			deltaOfDelta.wideSeq  = wideSeq;
			deltaOfDelta.sentAtMs = sentInfo.sentAtMs;
			deltaOfDelta.dod      = sentInfo.recvInfo.dod;

			deltaOfDeltas.push_back(deltaOfDelta);

			// TODO: Remove.
			// MS_DEBUG_DEV(
			//   "received delta for packet [wideSeq:%" PRIu16 ", send delta:%" PRIi32
			//   ", recv delta:%" PRIu64 "]",
			//   wideSeq,
			//   sentInfo.sentAtMs - previousSentInfo.sentAtMs,
			//   result.delta / 4);
		}

		if (deltaOfDeltas.empty())
		{
			return;
		}

		auto previousDeltaOfDelta = this->currentDeltaOfDelta;

		for (const auto deltaOfDelta : deltaOfDeltas)
		{
			this->currentDeltaOfDelta =
			  Utils::ComputeEWMA(this->currentDeltaOfDelta, static_cast<double>(deltaOfDelta.dod), 0.6f);
		}

		if (this->currentDeltaOfDelta > previousDeltaOfDelta)
		{
			this->deltaOfdeltaTrend = INCREASE;
		}
		else if (this->currentDeltaOfDelta < previousDeltaOfDelta)
		{
			this->deltaOfdeltaTrend = DECREASE;
		}
		else
		{
			this->deltaOfdeltaTrend = HOLD;
		}

		// TODO: Remove.
		MS_DEBUG_DEV(
		  "Delta of Delta. Previous:%f, Current:%f, Trend: %s",
		  previousDeltaOfDelta,
		  this->currentDeltaOfDelta,
		  TrendToString(this->deltaOfdeltaTrend).c_str());

		// for (const auto& deltaOfDelta : deltaOfDeltas)
		// {
		// 	MS_DEBUG_DEV("wideSeq:%" PRIu16 ", dod:%" PRIi16,
		// 			deltaOfDelta.wideSeq,
		// 			deltaOfDelta.dod);
		// }

		// Notify listener.
		this->listener->OnSenderBandwidthEstimatorDeltaOfDelta(this, deltaOfDeltas);
	}

	void SenderBandwidthEstimator::EstimateAvailableBitrate()
	{
		MS_TRACE();

		auto previousAvailableBitrate = this->availableBitrate;
		auto sendRecvBitrates         = GetSendRecvBitrates();

		if (sendRecvBitrates.sendBitrate == 0)
		{
			return;
		}

		double ratio = static_cast<double>(sendRecvBitrates.sendBitrate) /
		               static_cast<double>(sendRecvBitrates.recvBitrate);

		// RTP is being received properly.
		if (0.75f <= ratio && ratio <= 1.25f)
		{
			if (sendRecvBitrates.recvBitrate > this->availableBitrate)
			{
				this->availableBitrate = sendRecvBitrates.recvBitrate;

				MS_DEBUG_DEV("BWE UP [ratio:%f, availableBitrate:%" PRIu32 "]", ratio, this->availableBitrate);
			}
		}
		// RTP is being received worst than expected.
		else
		{
			if (sendRecvBitrates.recvBitrate < this->availableBitrate)
			{
				// TODO: This 0.8 must be set acording to other values: dod, jitter, etc.
				this->availableBitrate *= 0.8;

				MS_DEBUG_DEV(
				  "BWE DOWN [ratio:%f, availableBitrate:%" PRIu32 "]", ratio, this->availableBitrate);
			}
		}

		auto maxBitrate = std::max<uint32_t>(
		  this->initialAvailableBitrate, this->desiredBitrate * MaxBitrateIncrementFactor);

		// Limit maximum available bitrate to desired one.
		if (this->availableBitrate > maxBitrate)
		{
			this->availableBitrate = maxBitrate;
		}

		Bitrates bitrates;

		bitrates.availableBitrate         = this->availableBitrate;
		bitrates.previousAvailableBitrate = previousAvailableBitrate;
		bitrates.sendBitrate              = sendRecvBitrates.sendBitrate;
		bitrates.recvBitrate              = sendRecvBitrates.recvBitrate;

		this->listener->OnSenderBandwidthEstimatorAvailableBitrate(this, bitrates);
	}

	void SenderBandwidthEstimator::UpdateRtt(float rtt)
	{
		MS_TRACE();

		this->rtt = rtt;
	}

	void SenderBandwidthEstimator::SetDesiredBitrate(uint32_t desiredBitrate)
	{
		MS_TRACE();

		this->desiredBitrate = desiredBitrate;
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

		if (timer == this->timer)
		{
			RemoveOldInfos();
			EstimateAvailableBitrate();
			RemoveProcessedInfos();

			this->timer->Start(static_cast<uint64_t>(TimerInterval));
		}
	}

	void SenderBandwidthEstimator::RemoveOldInfos()
	{
		const auto nowMs = DepLibUV::GetTimeMs();

		// Remove all SentInfo's that are older than RTT.
		for (auto it = this->sentInfos.begin(); it != this->sentInfos.end();)
		{
			if (it->second.sentAtMs < nowMs - this->rtt)
			{
				it = this->sentInfos.erase(it);
			}
			else
			{
				++it;
			}
		}
	}

	void SenderBandwidthEstimator::RemoveProcessedInfos()
	{
		// Remove all SentInfo's that have been processed.
		for (auto it = this->sentInfos.begin(); it != this->sentInfos.end();)
		{
			if (it->second.received)
			{
				it = this->sentInfos.erase(it);
			}
			else
			{
				++it;
			}
		}
	}

	SenderBandwidthEstimator::SendRecvBitrates SenderBandwidthEstimator::GetSendRecvBitrates()
	{
		uint32_t totalBytes    = 0;
		uint64_t firstSentAtMs = 0;
		uint64_t lastSentAtMs  = 0;
		uint64_t firstRecvAtMs = 0;
		uint64_t lastRecvAtMs  = 0;
		SendRecvBitrates sendRecvBitrates;

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
			sendRecvBitrates.sendBitrate = 0;
			sendRecvBitrates.sendBitrate = 0;

			return sendRecvBitrates;
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

		sendRecvBitrates.sendBitrate =
		  static_cast<double>(totalBytes * 8) / (static_cast<double>(sentTimeWindowMs) / 1000.0f);
		sendRecvBitrates.recvBitrate =
		  static_cast<double>(totalBytes * 8) / (static_cast<double>(recvTimeWindowMs) / 1000.0f);

		// TODO: Remove.
		MS_DEBUG_DEV(
		  "totalBytes:%" PRIu32 ", sentTimeWindowMs:%" PRIu64 ",recvTimeWindowMs:%" PRIu64
		  ",sendBitrate:%" PRIu32 ", recvBitrate:%" PRIu32,
		  totalBytes,
		  sentTimeWindowMs,
		  recvTimeWindowMs,
		  sendRecvBitrates.sendBitrate,
		  sendRecvBitrates.recvBitrate);

		return sendRecvBitrates;
	}
} // namespace RTC
