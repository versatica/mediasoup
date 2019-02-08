#ifndef MS_RTC_RTP_MONITOR_HPP
#define MS_RTC_RTP_MONITOR_HPP

#include "common.hpp"
#include "RTC/RTCP/ReceiverReport.hpp"
#include "RTC/RtpPacket.hpp"
#include <vector>

namespace RTC
{
	class RtpStream;

	class RtpMonitor
	{
	public:
		class Listener
		{
		public:
			virtual void OnRtpMonitorScore(RTC::RtpMonitor* rtpMonitor, uint8_t score) = 0;
		};

	public:
		RtpMonitor(Listener* listener, RTC::RtpStream* rtpStream, uint8_t initialScore = 0);

	public:
		void Dump();
		void ReceiveRtcpReceiverReport(RTC::RTCP::ReceiverReport* report);
		void RtpPacketRepaired(RTC::RtpPacket* packet);
		uint8_t GetScore() const;
		void Reset();

	private:
		void AddScore(uint8_t score);
		void ComputeScore();
		size_t GetRepairedPacketCount() const;
		void UpdateReportedLoss(RTC::RTCP::ReceiverReport* report);
		void UpdateSourceLoss();
		void UpdateSentPackets();

	protected:
		RTC::RtpStream* rtpStream{ nullptr };

	private:
		// Passed by argument.
		Listener* listener{ nullptr };
		// Scores histogram and last computed score.
		std::vector<uint8_t> scores;
		uint8_t score{ 0 };
		// Rapaired RTP packet map.
		std::map<uint16_t, size_t> repairedPackets;
		// RTP stream data information for score calculation.
		int32_t totalSourceLoss{ 0 };
		int32_t totalReportedLoss{ 0 };
		size_t totalSentPackets{ 0 };
	};

	inline uint8_t RtpMonitor::GetScore() const
	{
		return this->score;
	}
} // namespace RTC

#endif
