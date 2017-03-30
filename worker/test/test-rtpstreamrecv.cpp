#include "include/catch.hpp"
#include "include/helpers.hpp"
#include "common.hpp"
#include "RTC/RtpPacket.hpp"
#include "RTC/RtpStream.hpp"
#include "RTC/RtpStreamRecv.hpp"
#include "Logger.hpp"
#include <vector>

using namespace RTC;

SCENARIO("receive RTP packets and trigger NACK", "[rtp][rtpstream]")
{
	class RtpStreamRecvListener :
		public RtpStreamRecv::Listener
	{
	public:
		virtual void onNackRequired(RTC::RtpStreamRecv* rtpStream, const std::vector<uint16_t>& seq_numbers) override
		{
			INFO("NACK required");

			REQUIRE(this->should_trigger_nack == true);

			this->should_trigger_nack = false;
			this->seq_numbers = seq_numbers;
		}

		virtual void onPliRequired(RtpStreamRecv* rtpStream) override
		{
			INFO("PLI required");

			REQUIRE(this->should_trigger_pli == true);

			this->should_trigger_pli = false;
			this->seq_numbers.clear();
		}

	public:
		bool should_trigger_nack = false;
		bool should_trigger_pli = false;
		std::vector<uint16_t> seq_numbers;
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

	params.ssrc = packet->GetSsrc();
	params.clockRate = 90000;
	params.useNack = true;
	params.usePli = true;

	SECTION("nack one packet")
	{
		RtpStreamRecvListener listener;
		RtpStreamRecv rtpStream(&listener, params);

		packet->SetSequenceNumber(1);
		rtpStream.ReceivePacket(packet);

		packet->SetSequenceNumber(3);
		listener.should_trigger_nack = true;
		rtpStream.ReceivePacket(packet);

		REQUIRE(listener.seq_numbers.size() == 1);
		REQUIRE(listener.seq_numbers[0] == 2);
		listener.seq_numbers.clear();

		packet->SetSequenceNumber(2);
		rtpStream.ReceivePacket(packet);

		REQUIRE(listener.seq_numbers.size() == 0);

		packet->SetSequenceNumber(4);
		rtpStream.ReceivePacket(packet);

		REQUIRE(listener.seq_numbers.size() == 0);
	}

	SECTION("wrapping sequence numbers")
	{
		RtpStreamRecvListener listener;
		RtpStreamRecv rtpStream(&listener, params);

		packet->SetSequenceNumber(0xfffe);
		rtpStream.ReceivePacket(packet);

		packet->SetSequenceNumber(1);
		listener.should_trigger_nack = true;
		rtpStream.ReceivePacket(packet);

		REQUIRE(listener.seq_numbers.size() == 2);
		REQUIRE(listener.seq_numbers[0] == 0xffff);
		REQUIRE(listener.seq_numbers[1] == 0);
		listener.seq_numbers.clear();
	}

	SECTION("require PLI")
	{
		RtpStreamRecvListener listener;
		RtpStreamRecv rtpStream(&listener, params);

		packet->SetSequenceNumber(1);
		rtpStream.ReceivePacket(packet);

		packet->SetSequenceNumber(510);
		listener.should_trigger_pli = true;
		rtpStream.ReceivePacket(packet);
	}

	delete packet;
}
