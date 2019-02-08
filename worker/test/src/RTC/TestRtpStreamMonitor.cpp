#include "common.hpp"
#include "catch.hpp"
#include "RTC/RTCP/ReceiverReport.hpp"
#include "RTC/RtpStreamMonitor.hpp"
#include "RTC/RtpStreamSend.hpp"
#include <cmath>

using namespace RTC;

static constexpr size_t ScoreTriggerCount{ 8 };

SCENARIO("RTP Monitor", "[rtp][monitor]")
{
	class TestRtpStreamMonitorListener : public RtpStreamMonitor::Listener
	{
	public:
		virtual void OnRtpStreamMonitorScore(RtpStreamMonitor* /*rtpMonitor*/, uint8_t /*score*/) override
		{
			this->scoreTriggered = true;
		}

		void Check(bool shouldHaveTriggeredScore)
		{
			REQUIRE(shouldHaveTriggeredScore == this->scoreTriggered);

			this->scoreTriggered = false;
		}

	public:
		bool scoreTriggered = false;
	};

	class TestRtpStreamListener : public RtpStreamSend::Listener
	{
	public:
		virtual void OnRtpStreamScore(RtpStream* /*rtpMonitor*/, uint8_t /*score*/) override
		{
		}
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
	TestRtpStreamListener testRtpStreamListener;

	params.ssrc      = report->GetSsrc();
	params.clockRate = 90000;
	params.useNack   = true;

	// Create a RtpStreamSend.
	RtpStreamSend* rtpStream = new RtpStreamSend(&testRtpStreamListener, params, 200);

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

	TestRtpStreamMonitorListener listener;
	RtpStreamMonitor rtpMonitor(&listener, rtpStream, 5);
	auto sequenceNumber = packet->GetSequenceNumber();

	SECTION("initial score matches the given one (if any)")
	{
		REQUIRE(rtpMonitor.GetScore() == 5);
	}

	SECTION("the eighth report triggers the score")
	{
		for (size_t counter = 0; counter < ScoreTriggerCount; counter++)
		{
			packet->SetSequenceNumber(sequenceNumber++);
			rtpStream->ReceivePacket(packet);
			rtpMonitor.ReceiveRtcpReceiverReport(report);

			if (counter < ScoreTriggerCount - 1)
				listener.Check(false);
		}

		listener.Check(true);
	}

	SECTION("next eighth consecutive reports trigger the score")
	{
		for (size_t counter = 0; counter < ScoreTriggerCount; counter++)
		{
			packet->SetSequenceNumber(sequenceNumber++);
			rtpStream->ReceivePacket(packet);
			rtpMonitor.ReceiveRtcpReceiverReport(report);

			if (counter < ScoreTriggerCount - 1)
				listener.Check(false);
		}

		listener.Check(true);
	}

	SECTION("Reset() triggers score 0 unless it was already 0")
	{
		rtpMonitor.Reset();
		listener.Check(true);

		rtpMonitor.Reset();
		listener.Check(false);
	}

	delete report;
	delete rtpStream;
	delete packet;
}
