#define MS_CLASS "RTC::RtpMonitor"
// #define MS_LOG_DEV

#include "RTC/RtpMonitor.hpp"
#include "Logger.hpp"

namespace RTC
{
	/* Static. */

	static constexpr size_t LossPercentageScoreRatio{ 2 };
	static constexpr size_t MaxScore{ 10 };
	static constexpr size_t HistogramLength{ 8 };

	/* Instance methods. */

	RtpMonitor::RtpMonitor(Listener* listener) : listener(listener)
	{
		MS_TRACE();
	}

	void RtpMonitor::ReceiveRtcpReceiverReport(RTC::RTCP::ReceiverReport* report)
	{
		MS_TRACE();

		auto fractionLost   = report->GetFractionLost();
		auto lossPercentage = static_cast<uint8_t>(fractionLost * 100 / 255);

		auto score = MaxScore - static_cast<int8_t>(lossPercentage / LossPercentageScoreRatio);

		this->AddScore(score);
	}

	void RtpMonitor::AddScore(int8_t currentScore)
	{
		MS_TRACE();

		auto it        = this->scores.crbegin();
		auto lastScore = MaxScore;

		if (it != this->scores.crend())
		{
			lastScore = *it;
		}

		// Transitioning to unhealthy...
		if (IsHealthyScore(lastScore) && !IsHealthyScore(currentScore))
		{
			this->scores.clear();
			this->scores.push_back(currentScore);
		}

		// Transitioning to healthy...
		else if (!IsHealthyScore(lastScore) && IsHealthyScore(currentScore))
		{
			this->scores.clear();
			this->scores.push_back(currentScore);
		}

		// HistogramLength healthy or unhealthy scores achieved. Notify listener.
		else if (this->scores.size() == HistogramLength - 1)
		{
			this->scores.clear();
			this->scores.push_back(currentScore);

			if (IsHealthyScore(currentScore))
			{
				this->healthy = true;
				this->listener->OnRtpMonitorHealthy();
			}
			else
			{
				this->healthy = false;
				this->listener->OnRtpMonitorUnhealthy();
			}
		}

		else
		{
			this->scores.push_back(currentScore);
		}
	}
} // namespace RTC
