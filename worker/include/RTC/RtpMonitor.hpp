#ifndef MS_RTC_RTP_MONITOR_HPP
#define MS_RTC_RTP_MONITOR_HPP

#include "common.hpp"
#include "RTC/RTCP/ReceiverReport.hpp"
#include <vector>

namespace RTC
{
	class RtpMonitor
	{
	public:
		class Listener
		{
		public:
			virtual void OnRtpMonitorHealthy()   = 0;
			virtual void OnRtpMonitorUnhealthy() = 0;
		};

		explicit RtpMonitor(Listener* listener);
		void ReceiveRtcpReceiverReport(RTC::RTCP::ReceiverReport* report);
		bool IsHealthy() const;
		void Reset();

	private:
		void AddScore(int8_t score);
		bool IsHealthyScore(int8_t score);

	private:
		Listener* listener{ nullptr };
		std::vector<int8_t> scores;
		bool healthy{ true };
	};

	inline bool RtpMonitor::IsHealthy() const
	{
		return this->healthy;
	}

	inline void RtpMonitor::Reset()
	{
		this->scores.clear();
		this->healthy = true;
	}

	inline bool RtpMonitor::IsHealthyScore(int8_t score)
	{
		return score >= 0;
	}
} // namespace RTC

#endif
