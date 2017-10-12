#include "common.hpp"
#include "include/catch.hpp"
#include "include/helpers.hpp"
#include "RTC/RtpPacket.hpp"
#include "RTC/RtpStream.hpp"
#include "RTC/RtpStreamRecv.hpp"
#include <vector>

using namespace RTC;

SCENARIO("receive RTP packets and trigger NACK", "[rtp][rtpstream]")
{
	class RtpStreamRecvListener : public RtpStreamRecv::Listener
	{
	public:
		virtual void OnRtpStreamActive(RTC::RtpStream* /*rtpStream*/) override
		{
		}

		virtual void OnRtpStreamInactive(RTC::RtpStream* /*rtpStream*/) override
		{
		}

		virtual void OnRtpStreamRecvNackRequired(
		  RTC::RtpStreamRecv* /*rtpStream*/, const std::vector<uint16_t>& seqNumbers) override
		{
			INFO("NACK required");

			REQUIRE(this->shouldTriggerNack == true);

			this->shouldTriggerNack = false;
			this->seqNumbers        = seqNumbers;
		}

		virtual void OnRtpStreamRecvPliRequired(RtpStreamRecv* /*rtpStream*/) override
		{
			INFO("PLI required");

			REQUIRE(this->shouldTriggerKeyFrame == true);

			this->shouldTriggerKeyFrame = false;
			this->seqNumbers.clear();
		}

	public:
		bool shouldTriggerNack     = false;
		bool shouldTriggerKeyFrame = false;
		std::vector<uint16_t> seqNumbers;
	};

	uint8_t buffer[] =
	{
		0b10000000, 0b00000001, 0, 1,
		0, 0, 0, 4,
		0, 0, 0, 5
	};
	RtpPacket* packet = RtpPacket::Parse(buffer, sizeof(buffer));

	if (!packet)
		FAIL("not a RTP packet");

	RtpStream::Params params;

	params.ssrc      = packet->GetSsrc();
	params.clockRate = 90000;
	params.useNack   = true;
	params.usePli    = true;

	SECTION("NACK one packet")
	{
		RtpStreamRecvListener listener;
		RtpStreamRecv rtpStream(&listener, params);

		packet->SetSequenceNumber(1);
		rtpStream.ReceivePacket(packet);

		packet->SetSequenceNumber(3);
		listener.shouldTriggerNack = true;
		rtpStream.ReceivePacket(packet);

		REQUIRE(listener.seqNumbers.size() == 1);
		REQUIRE(listener.seqNumbers[0] == 2);
		listener.seqNumbers.clear();

		packet->SetSequenceNumber(2);
		rtpStream.ReceivePacket(packet);

		REQUIRE(listener.seqNumbers.size() == 0);

		packet->SetSequenceNumber(4);
		rtpStream.ReceivePacket(packet);

		REQUIRE(listener.seqNumbers.size() == 0);
	}

	SECTION("wrapping sequence numbers")
	{
		RtpStreamRecvListener listener;
		RtpStreamRecv rtpStream(&listener, params);

		packet->SetSequenceNumber(0xfffe);
		rtpStream.ReceivePacket(packet);

		packet->SetSequenceNumber(1);
		listener.shouldTriggerNack = true;
		rtpStream.ReceivePacket(packet);

		REQUIRE(listener.seqNumbers.size() == 2);
		REQUIRE(listener.seqNumbers[0] == 0xffff);
		REQUIRE(listener.seqNumbers[1] == 0);
		listener.seqNumbers.clear();
	}

	SECTION("require key frame")
	{
		RtpStreamRecvListener listener;
		RtpStreamRecv rtpStream(&listener, params);

		packet->SetSequenceNumber(1);
		rtpStream.ReceivePacket(packet);

		// Seq different is bigger than MaxNackPackets in RTC::NackGenerator, so it
		// triggers a key frame.
		packet->SetSequenceNumber(1003);
		listener.shouldTriggerKeyFrame = true;
		rtpStream.ReceivePacket(packet);
	}

	delete packet;
}
