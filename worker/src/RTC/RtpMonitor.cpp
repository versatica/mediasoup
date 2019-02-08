#define MS_CLASS "RTC::RtpMonitor"
// #define MS_LOG_DEV

#include "RTC/RtpMonitor.hpp"
#include "Logger.hpp"
#include "RTC/RtpStream.hpp"
#include <cmath> // std::round()

namespace RTC
{
	/* Static. */

	static constexpr size_t HistogramLength{ 8 };
	static constexpr size_t MaxRepairedPacketRetransmission{ 2 };
	static constexpr size_t MaxRepairedPacketsLength{ 1000 };
	// Score constraints weight.
	static constexpr float LossPercentageWeight{ -1.0f };
	static constexpr float RepairedPercentageWeight{ 0.5f };

	/* Instance methods. */

	RtpMonitor::RtpMonitor(Listener* listener, RTC::RtpStream* rtpStream, uint8_t initialScore)
	  : rtpStream(rtpStream), listener(listener), score(initialScore)
	{
		MS_TRACE();
	}

	void RtpMonitor::Dump()
	{
		MS_TRACE();

		MS_DEBUG_DEV("<RtpMonitor>");
		MS_DEBUG_DEV("  score                : %" PRIu8, this->score);
		MS_DEBUG_DEV("  totalSourceLoss      : %" PRIi32, this->totalSourceLoss);
		MS_DEBUG_DEV("  totalReportedLoss    : %" PRIi32, this->totalReportedLoss);
		MS_DEBUG_DEV("  repairedPackets size : %zu", this->repairedPackets.size());
		MS_DEBUG_DEV("</RtpMonitor>");
	}

	// A RTCP receiver report triggers a score calculation.
	void RtpMonitor::ReceiveRtcpReceiverReport(RTC::RTCP::ReceiverReport* report)
	{
		MS_TRACE();

		// Calculate packet loss reported since last RR.
		auto previousTotalReportedLoss = this->totalReportedLoss;

		UpdateReportedLoss(report);

		auto reportedLoss = this->totalReportedLoss - previousTotalReportedLoss;

		if (reportedLoss < 0)
			reportedLoss = 0;

		// Calculate source packet loss since last RR.
		auto previousTotalSourceLoss = this->totalSourceLoss;

		UpdateSourceLoss();

		auto sourceLoss = this->totalSourceLoss - previousTotalSourceLoss;

		if (sourceLoss < 0)
			sourceLoss = 0;

		// Calculate effective loss since last report.
		size_t currentLoss;

		if (reportedLoss < sourceLoss)
			currentLoss = 0;
		else
			currentLoss = reportedLoss - sourceLoss;

		// Calculate repaired packets.
		auto repairedPacketCount = GetRepairedPacketCount();

		// Reset repaired packets map.
		this->repairedPackets.clear();

		// Calculate packets sent since last RR.
		auto previousTotalSentPackets = this->totalSentPackets;

		UpdateSentPackets();

		auto sentPackets = this->totalSentPackets - previousTotalSentPackets;

		// No packet was sent. Consider lost and repaired packets though.
		if (sentPackets == 0)
			sentPackets = (currentLoss > repairedPacketCount) ? currentLoss : repairedPacketCount;

		// Nothing to do.
		if (sentPackets == 0)
			return;

		// There cannot be more loss than sent packets.
		if (currentLoss > sentPackets)
			currentLoss = sentPackets;

		// There cannot be more repaired than sent packets.
		if (repairedPacketCount > sentPackets)
			repairedPacketCount = sentPackets;

		float lossPercentage     = currentLoss * 100 / sentPackets;
		float repairedPercentage = repairedPacketCount * 100 / sentPackets;

		/*
		 * Calculate score. Starting from a score of 100:
		 *
		 * - Each loss porcentual point has a weight of LossPercentageWeight.
		 * - Each repaired porcentual point has a  weight of RepairedPercentageWeight.
		 */

		float score{ 100 };

		score += lossPercentage * LossPercentageWeight;
		score += repairedPercentage * RepairedPercentageWeight;

		// Add score to histogram.
		AddScore(static_cast<uint8_t>(std::lround(score / 10)));

#ifdef MS_LOG_DEV
		MS_DEBUG_TAG(
		  rtp,
		  "sentPackets: %zu, currentLoss: %zu, repairedPacketCount: %zu, lossPercentage: %f, repairedPercentage: %f, score: %f",
		  sentPackets,
		  currentLoss,
		  repairedPacketCount,
		  lossPercentage,
		  repairedPercentage,
		  score);

		report->Dump();
		Dump();
#endif
	}

	void RtpMonitor::RtpPacketRepaired(RTC::RtpPacket* packet)
	{
		MS_TRACE();

		if (this->repairedPackets.size() == MaxRepairedPacketsLength)
			this->repairedPackets.erase(this->repairedPackets.begin());

		this->repairedPackets[packet->GetSequenceNumber()]++;
	}

	void RtpMonitor::Reset()
	{
		MS_TRACE();

		this->totalSourceLoss   = 0;
		this->totalReportedLoss = 0;
		this->totalSentPackets  = 0;

		this->repairedPackets.clear();
		this->scores.clear();

		if (this->score != 0)
		{
			this->score = 0;
			this->listener->OnRtpMonitorScore(this, 0);
		}
	}

	void RtpMonitor::AddScore(uint8_t score)
	{
		MS_TRACE();

		if (this->scores.size() == HistogramLength)
			this->scores.erase(this->scores.begin());

		this->scores.push_back(score);

		auto previousScore = this->score;

		ComputeScore();

		if (this->score != previousScore)
		{
			MS_DEBUG_TAG(
			  score,
			  "[added score:%" PRIu8 ", previous computed score:%" PRIu8 ", new computed score:%" PRIu8
			  "] (calling listener)",
			  score,
			  previousScore,
			  this->score);

			this->listener->OnRtpMonitorScore(this, this->score);
		}
		else
		{
			MS_DEBUG_TAG(
			  score, "[added score:%" PRIu8 ", computed score:%" PRIu8 "] (no change)", score, this->score);
		}
	}

	/*
	 * Scoring mechanism is a weighted average.
	 *
	 * The more recent the score is, the more weight it has.
	 * The oldest score has a weight of 1 and subsequent scores weight is
	 * increased by one sequentially.
	 *
	 * Ie:
	 * - scores: [1,2,3,4]
	 * - this->scores = ((1) + (2+2) + (3+3+3) + (4+4+4+4)) / 10 = 2.8 => 3
	 */
	inline void RtpMonitor::ComputeScore()
	{
		MS_TRACE();

		if (this->scores.empty())
		{
			this->score = 0;

			return;
		}

		size_t weight{ 0 };
		size_t samples{ 0 };
		size_t totalScore{ 0 };

		for (auto score : this->scores)
		{
			weight++;
			samples += weight;
			totalScore += weight * score;
		}

		this->score = static_cast<uint8_t>(std::round(totalScore / samples));
	}

	/*
	 * A RTP packet is considered to be repaired if it was resent no more than
	 * MaxRepairedPacketRetransmission times.
	 */
	inline size_t RtpMonitor::GetRepairedPacketCount() const
	{
		MS_TRACE();

		size_t count{ 0 };

		for (auto& kv : this->repairedPackets)
		{
			if (kv.second <= MaxRepairedPacketRetransmission)
				count++;
		}

		return count;
	}

	inline void RtpMonitor::UpdateReportedLoss(RTC::RTCP::ReceiverReport* report)
	{
		MS_TRACE();

		this->totalReportedLoss = report->GetTotalLost();
	}

	inline void RtpMonitor::UpdateSourceLoss()
	{
		MS_TRACE();

		this->totalSourceLoss = static_cast<int32_t>(
		  this->rtpStream->GetExpectedPackets() - this->rtpStream->transmissionCounter.GetPacketCount());
	}

	inline void RtpMonitor::UpdateSentPackets()
	{
		MS_TRACE();

		this->totalSentPackets = this->rtpStream->transmissionCounter.GetPacketCount();
	}
} // namespace RTC
