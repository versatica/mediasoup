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
		virtual void OnRtpMonitorScore(uint8_t /*score*/) override
		{
			this->scoreTriggered = true;
		}

		void Check(bool shouldHaveTriggeredScore)
		{
			REQUIRE(shouldHaveTriggeredScore == this->scoreTriggered);

			this->scoreTriggered = false;
		}

	public:
		bool scoreTriggered   = false;
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

	RtpStream::Params params;

	params.ssrc      = report->GetSsrc();
	params.clockRate = 90000;
	params.useNack   = true;

	// Create a RtpStreamSend.
	RtpStreamSend* rtpStream = new RtpStreamSend(params, 200);

	// clang-format off
	uint8_t rtpBuffer[] =
	{
		0b10000000, 0b01111011, 0b01010010, 0b00001110,
		0b01011011, 0b01101011, 0b11001010, 0b10110101,
		0, 0, 0, 2
	};
	// clang-format on

	// packet1 [pt:123, seq:21006, timestamp:1533790901]
	RtpPacket* packet = RtpPacket::Parse(rtpBuffer, sizeof(rtpBuffer));

	REQUIRE(packet);

	SECTION("The eighth report triggers the score")
	{
		TestRtpMonitorListener listener;
		RtpMonitor rtpMonitor(&listener, rtpStream);

		auto sequenceNumber = packet->GetSequenceNumber();

		for (size_t counter = 0; counter < RtpMonitor::ScoreTriggerCount; counter++)
		{
			packet->SetSequenceNumber(sequenceNumber++);
			rtpStream->ReceivePacket(packet);
			rtpMonitor.ReceiveRtcpReceiverReport(report);
		}

		listener.Check(true);

		SECTION("Next eighth consecutive reports trigger the score")
		{

			for (size_t counter = 0; counter < RtpMonitor::ScoreTriggerCount; counter++)
			{
				packet->SetSequenceNumber(sequenceNumber++);
				rtpStream->ReceivePacket(packet);
				rtpMonitor.ReceiveRtcpReceiverReport(report);
			}

			listener.Check(true);
		}
	}

	delete report;
}
