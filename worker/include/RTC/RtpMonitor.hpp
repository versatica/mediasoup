#ifndef MS_RTC_RTP_MONITOR_HPP
#define MS_RTC_RTP_MONITOR_HPP

#include "common.hpp"
#include "RTC/RTCP/ReceiverReport.hpp"
#include "RTC/RtpPacket.hpp"
#include "RTC/RtpStreamSend.hpp"
#include <vector>

namespace RTC
{
	class RtpMonitor
	{
		static constexpr uint8_t MaxScore{ 10 };
		static constexpr uint8_t HealthScoreThreshold{ 7 };

	public:
		static constexpr size_t ScoreTriggerCount{ 8 };

	public:
		class Listener
		{
		public:
			virtual void OnRtpMonitorScore(uint8_t score) = 0;
		};

		explicit RtpMonitor(Listener* listener, const RTC::RtpStreamSend* rtpStream);
		void ReceiveRtcpReceiverReport(RTC::RTCP::ReceiverReport* report);
		void RtpPacketRepaired(RTC::RtpPacket* packet);
		bool IsHealthy() const;
		uint8_t GetScore() const;
		void Reset();
		void Dump();

	private:
		void AddScore(uint8_t score);
		size_t GetRepairedPacketCount() const;
		void UpdateReportedLoss(RTC::RTCP::ReceiverReport* report);
		void UpdateSourceLoss();
		void UpdateSentPackets();

	private:
		// Passed by argument.
		Listener* listener{ nullptr };
		const RTC::RtpStream* rtpStream{ nullptr };
		// Counter for event notification.
		size_t scoreTriggerCounter{ ScoreTriggerCount };
		// Scores histogram.
		std::vector<uint8_t> scores;
		// Rapaired RTP packet map.
		std::map<uint16_t, size_t> repairedPackets;
		// RTP stream data information for score calculation.
		int32_t totalSourceLoss{ 0 };
		int32_t totalReportedLoss{ 0 };
		size_t totalSentPackets{ 0 };
	};

	inline bool RtpMonitor::IsHealthy() const
	{
		// Consider it healthy if no score is present.
		if (this->scores.empty())
			return true;

		return GetScore() >= HealthScoreThreshold;
	}

	inline void RtpMonitor::Reset()
	{
		this->scoreTriggerCounter = ScoreTriggerCount;
		this->totalSourceLoss     = 0;
		this->totalReportedLoss   = 0;
		this->totalSentPackets    = 0;

		this->repairedPackets.clear();
		this->scores.clear();
	}
} // namespace RTC

#endif
