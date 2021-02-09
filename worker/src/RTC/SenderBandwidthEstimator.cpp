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

		std::vector<SentInfo> sentInfos;

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

			// Fill the received info.
			sentInfo.received     = true;
			sentInfo.receivedAtMs = result.receivedAtMs;

			// TODO: Properly retrieve RTT.
			sentInfo.rtt = this->rtt;

			// Retrieve the received info of the previously received RTP packet in order to calculate
			// the delta.
			sentInfosIt = this->sentInfos.find(this->lastReceivedWideSeq);
			if (sentInfosIt == this->sentInfos.end())
			{
				this->lastReceivedWideSeq = wideSeq;

				continue;
			}

			this->lastReceivedWideSeq = wideSeq;

			auto& previousSentInfo = sentInfosIt->second;

			sentInfo.dod = (result.receivedAtMs - previousSentInfo.receivedAtMs) -
			               (sentInfo.sentAtMs - previousSentInfo.sentAtMs);

			// TODO: Remove.
			// sentInfo.Dump();

			sentInfos.push_back(sentInfo);

			// TODO: Remove.
			// MS_DEBUG_DEV(
			//   "received delta for packet [wideSeq:%" PRIu16 ", send delta:%" PRIi32
			//   ", recv delta:%" PRIu64 "]",
			//   wideSeq,
			//   sentInfo.sentAtMs - previousSentInfo.sentAtMs,
			//   result.delta / 4);
		}

		if (sentInfos.empty())
		{
			return;
		}

		auto previousDeltaOfDelta = this->currentDeltaOfDelta;

		for (const auto sentInfo : sentInfos)
		{
			this->currentDeltaOfDelta =
			  Utils::ComputeEWMA(this->currentDeltaOfDelta, static_cast<double>(sentInfo.dod), 0.6f);
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
		// MS_DEBUG_DEV(
		//   "Delta of Delta. Previous:%f, Current:%f, Trend: %s",
		//   previousDeltaOfDelta,
		//   this->currentDeltaOfDelta,
		//   TrendToString(this->deltaOfdeltaTrend).c_str());

		// for (const auto& deltaOfDelta : deltaOfDeltas)
		// {
		// 	MS_DEBUG_DEV("wideSeq:%" PRIu16 ", dod:%" PRIi16,
		// 			deltaOfDelta.wideSeq,
		// 			deltaOfDelta.dod);
		// }

		// Notify listener.
		this->listener->OnSenderBandwidthEstimatorRtpFeedback(this, sentInfos);
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
		if (0.85f <= ratio && ratio <= 1.25f)
		{
			if (sendRecvBitrates.recvBitrate > this->availableBitrate)
			{
				this->availableBitrate =
				  Utils::ComputeEWMA(this->availableBitrate, sendRecvBitrates.recvBitrate, 0.8f);

				MS_DEBUG_DEV("BWE UP [ratio:%f, availableBitrate:%" PRIu32 "]", ratio, this->availableBitrate);
			}
		}
		// RTP is being received worst than expected.
		else if (ratio >= 1.35f)
		{
			if (sendRecvBitrates.recvBitrate < this->availableBitrate)
			{
				// TODO: This 0.8 must be set acording to other values: dod, jitter, etc.
				this->availableBitrate = Utils::ComputeEWMA(
				  this->availableBitrate, sendRecvBitrates.recvBitrate, static_cast<float>(ratio - 1));

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

			MS_DEBUG_DEV("BWE CAP [ratio:%f, availableBitrate:%" PRIu32 "]", ratio, this->availableBitrate);
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
				firstRecvAtMs = sentInfo.receivedAtMs;
				lastSentAtMs  = sentInfo.sentAtMs;
				lastRecvAtMs  = sentInfo.receivedAtMs;
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
			if (RTC::SeqManager<uint64_t>::IsSeqLowerThan(sentInfo.receivedAtMs, firstRecvAtMs))
			{
				firstRecvAtMs = sentInfo.receivedAtMs;
			}
			else if (RTC::SeqManager<uint64_t>::IsSeqHigherThan(sentInfo.receivedAtMs, lastRecvAtMs))
			{
				lastRecvAtMs = sentInfo.receivedAtMs;
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
		  "lastReceivedWideSeq:%" PRIu16 ", totalBytes:%" PRIu32 ", sentTimeWindowMs:%" PRIu64
		  ",recvTimeWindowMs:%" PRIu64 ",sendBitrate:%" PRIu32 ", recvBitrate:%" PRIu32,
		  this->lastReceivedWideSeq,
		  totalBytes,
		  sentTimeWindowMs,
		  recvTimeWindowMs,
		  sendRecvBitrates.sendBitrate,
		  sendRecvBitrates.recvBitrate);

		return sendRecvBitrates;
	}

	void SenderBandwidthEstimator::SentInfo::Dump() const
	{
		MS_TRACE();

		MS_DUMP("<SentInfo>");
		MS_DUMP("  wideSeq     : %" PRIu16, this->wideSeq);
		MS_DUMP("  size        : %zu", this->size);
		MS_DUMP("  isProbation : %s", this->isProbation ? "true" : "false");
		MS_DUMP("  sendingAt   : %" PRIu64, this->sendingAtMs);
		MS_DUMP("  sentAt      : %" PRIu64, this->sentAtMs);
		MS_DUMP("  received    : %s", this->received ? "true" : "false");

		if (this->received)
		{
			MS_DUMP("  receivedAt : %" PRIu64, this->receivedAtMs);
			MS_DUMP("  dod        : %" PRIi16, this->dod);
		}

		MS_DUMP("</SentInfo>");
	}
} // namespace RTC
