#include "common.hpp"
#include "catch.hpp"
#include "RTC/RTCP/ReceiverReport.hpp"
#include "RTC/RtpMonitor.hpp"
#include <cmath>

using namespace RTC;

SCENARIO("RTP Monitor", "[rtp][monitor]")
{
	class TestRtpMonitorListener : public RtpMonitor::Listener
	{
	public:
		virtual void OnHealthy(RTC::RtpMonitor* /*rtpMonitor*/) override
		{
			INFO("OnHealthy");

			this->healthyTriggered = true;
		}

		virtual void OnUnhealthy(RTC::RtpMonitor* /*rtpMonitor*/) override
		{
			INFO("OnUnhealthy");

			this->unhealthyTriggered = true;
		}

		void Check(bool shouldHaveTriggeredHealthy, bool shouldHaveTriggeredUnhealthy)
		{
			REQUIRE(shouldHaveTriggeredHealthy == this->healthyTriggered);
			REQUIRE(shouldHaveTriggeredUnhealthy == this->unhealthyTriggered);

			this->healthyTriggered = this->unhealthyTriggered = false;
		}

	public:
		bool healthyTriggered   = false;
		bool unhealthyTriggered = false;
	};

	// RTCP Receiver Report Packet.

	// clang-format off
	uint8_t buffer[] =
	{
		// Receiver Report
		0x01, 0x93, 0x2d, 0xb4, // SSRC. 0x01932db4
		0x00, 0x00, 0x00, 0x01, // Fraction lost: 0, Total lost: 1
		0x00, 0x00, 0x00, 0x00, // Extended highest sequence number: 0
		0x00, 0x00, 0x00, 0x00, // Jitter: 0
		0x00, 0x00, 0x00, 0x00, // Last SR: 0
		0x00, 0x00, 0x00, 0x05  // DLSR: 0
	};
	// clang-format on

	RTCP::ReceiverReport* report = RTCP::ReceiverReport::Parse(buffer, sizeof(buffer));

	if (!report)
		FAIL("failed parsing RTCP::ReceiverReport");

	SECTION("Four consecutive bad reports trigger unhealthy state")
	{
		TestRtpMonitorListener listener;
		RtpMonitor rtpMonitor(&listener);

		report->SetFractionLost(255); /* 100% */
		rtpMonitor.ReceiveRtcpReceiverReport(report);

		report->SetFractionLost(255); /* 100% */
		rtpMonitor.ReceiveRtcpReceiverReport(report);

		report->SetFractionLost(255); /* 100% */
		rtpMonitor.ReceiveRtcpReceiverReport(report);

		report->SetFractionLost(255); /* 100% */
		rtpMonitor.ReceiveRtcpReceiverReport(report);

		report->SetFractionLost(255); /* 100% */
		rtpMonitor.ReceiveRtcpReceiverReport(report);

		report->SetFractionLost(255); /* 100% */
		rtpMonitor.ReceiveRtcpReceiverReport(report);

		report->SetFractionLost(255); /* 100% */
		rtpMonitor.ReceiveRtcpReceiverReport(report);

		report->SetFractionLost(255); /* 100% */
		rtpMonitor.ReceiveRtcpReceiverReport(report);
		listener.Check(false, true);

		SECTION("Good report after unhealth triggers healty state")
		{
			report->SetFractionLost(0);
			rtpMonitor.ReceiveRtcpReceiverReport(report);

			report->SetFractionLost(0);
			rtpMonitor.ReceiveRtcpReceiverReport(report);

			report->SetFractionLost(0);
			rtpMonitor.ReceiveRtcpReceiverReport(report);

			report->SetFractionLost(0);
			rtpMonitor.ReceiveRtcpReceiverReport(report);

			report->SetFractionLost(0);
			rtpMonitor.ReceiveRtcpReceiverReport(report);

			report->SetFractionLost(0);
			rtpMonitor.ReceiveRtcpReceiverReport(report);

			report->SetFractionLost(0);
			rtpMonitor.ReceiveRtcpReceiverReport(report);

			report->SetFractionLost(0);
			rtpMonitor.ReceiveRtcpReceiverReport(report);

			listener.Check(true, false);
		}
	}

	delete report;
}
