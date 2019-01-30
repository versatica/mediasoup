#ifndef MS_RTC_RTP_STREAM_MONITOR_HPP
#define MS_RTC_RTP_STREAM_MONITOR_HPP

#include "common.hpp"
#include "RTC/RTCP/ReceiverReport.hpp"
#include "RTC/RtpPacket.hpp"
#include <vector>

namespace RTC
{
	class RtpStream;

	class RtpStreamMonitor
	{
	public:
		static constexpr size_t ScoreTriggerCount{ 8 };

	public:
		class Listener
		{
		public:
			virtual void OnRtpStreamMonitorScore(const RtpStreamMonitor* rtpMonitor, uint8_t score) = 0;
		};

	public:
		RtpStreamMonitor(Listener* listener, RTC::RtpStream* rtpStream);

	public:
		void ReceiveRtcpReceiverReport(RTC::RTCP::ReceiverReport* report);
		void RtpPacketRepaired(RTC::RtpPacket* packet);
		void AddScore(uint8_t score);
		uint8_t GetScore() const;
		void Reset();
		void Dump();

	private:
		size_t GetRepairedPacketCount() const;
		void UpdateReportedLoss(RTC::RTCP::ReceiverReport* report);
		void UpdateSourceLoss();
		void UpdateSentPackets();

	protected:
		RTC::RtpStream* rtpStream{ nullptr };

	private:
		// Passed by argument.
		Listener* listener{ nullptr };
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

	inline void RtpStreamMonitor::Reset()
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
