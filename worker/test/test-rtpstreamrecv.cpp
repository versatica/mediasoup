#include "include/catch.hpp"
#include "include/helpers.hpp"
#include "common.hpp"
#include "RTC/RtpPacket.hpp"
#include "RTC/RtpStream.hpp"
#include "RTC/RtpStreamRecv.hpp"
#include "Logger.hpp"
#include <bitset> // std::bitset()

using namespace RTC;

SCENARIO("receive RTP packets and trigger NACK", "[rtp][rtpstream]")
{
	class RtpStreamRecvListener :
		public RtpStreamRecv::Listener
	{
	public:
		virtual void onNackRequired(RtpStreamRecv* rtpStream, uint16_t seq, uint16_t bitmask) override
		{
			std::bitset<16> nack_bitset(bitmask);

			INFO("NACK required [seq:" << seq << ", bitmask:" << nack_bitset << "]");

			REQUIRE(this->should_trigger == true);
			REQUIRE(seq == this->expected_nack_seq);
			REQUIRE(bitmask == this->expected_nack_bitmask);

			this->should_trigger = false;
		}

	public:
		bool     should_trigger = false;
		uint16_t expected_nack_seq = 0;
		uint16_t expected_nack_bitmask = 0;
	};

	SECTION("loose packets newer than 16 seq units")
	{
		uint8_t buffer[] =
		{
			0b10000000, 0b00000001, 0, 100,
			0, 0, 0, 4,
			0, 0, 0, 5
		};

		RtpPacket* packet = RtpPacket::Parse(buffer, sizeof(buffer));

		if (!packet)
			FAIL("not a RTP packet");

		REQUIRE(packet->GetSequenceNumber() == 100);

		RtpStream::Params params;

		params.ssrc = packet->GetSsrc();
		params.clockRate = 90000;
		params.useNack = true;

		RtpStreamRecvListener listener;
		RtpStreamRecv rtpStream(&listener, params);

		rtpStream.ReceivePacket(packet);

		packet->SetSequenceNumber(101);
		rtpStream.ReceivePacket(packet);

		packet->SetSequenceNumber(98);
		rtpStream.ReceivePacket(packet);

		packet->SetSequenceNumber(104);
		listener.should_trigger = true;
		listener.expected_nack_seq = 102;
		listener.expected_nack_bitmask = 0b0000000000000001;
		rtpStream.ReceivePacket(packet);
		REQUIRE(listener.should_trigger == false);

		packet->SetSequenceNumber(104);
		rtpStream.ReceivePacket(packet);

		packet->SetSequenceNumber(102);
		rtpStream.ReceivePacket(packet);

		packet->SetSequenceNumber(103);
		rtpStream.ReceivePacket(packet);

		packet->SetSequenceNumber(108);
		listener.should_trigger = true;
		listener.expected_nack_seq = 105;
		listener.expected_nack_bitmask = 0b0000000000000011;
		rtpStream.ReceivePacket(packet);
		REQUIRE(listener.should_trigger == false);

		delete packet;
	}

	SECTION("loose packets older than 16 seq units")
	{
		uint8_t buffer[] =
		{
			0b10000000, 0b00000001, 0, 100,
			0, 0, 0, 4,
			0, 0, 0, 5
		};

		RtpPacket* packet = RtpPacket::Parse(buffer, sizeof(buffer));

		if (!packet)
			FAIL("not a RTP packet");

		REQUIRE(packet->GetSequenceNumber() == 100);

		RtpStream::Params params;

		params.ssrc = packet->GetSsrc();
		params.clockRate = 90000;
		params.useNack = true;

		RtpStreamRecvListener listener;
		RtpStreamRecv rtpStream(&listener, params);

		rtpStream.ReceivePacket(packet);

		packet->SetSequenceNumber(120);
		listener.should_trigger = true;
		listener.expected_nack_seq = 103;
		listener.expected_nack_bitmask = 0b1111111111111111;
		rtpStream.ReceivePacket(packet);
		REQUIRE(listener.should_trigger == false);

		delete packet;
	}

	SECTION("increase seq cycles and loose packets older than 16 seq units")
	{
		uint8_t buffer[] =
		{
			0b10000000, 0b00000001, 0b11111111, 0b11111111,
			0, 0, 0, 4,
			0, 0, 0, 5
		};

		RtpPacket* packet = RtpPacket::Parse(buffer, sizeof(buffer));

		if (!packet)
			FAIL("not a RTP packet");

		REQUIRE(packet->GetSequenceNumber() == 65535);

		RtpStream::Params params;

		params.ssrc = packet->GetSsrc();
		params.clockRate = 90000;
		params.useNack = true;

		RtpStreamRecvListener listener;
		RtpStreamRecv rtpStream(&listener, params);

		rtpStream.ReceivePacket(packet);

		packet->SetSequenceNumber(20);
		listener.should_trigger = true;
		listener.expected_nack_seq = 3;
		listener.expected_nack_bitmask = 0b1111111111111111;
		rtpStream.ReceivePacket(packet);
		REQUIRE(listener.should_trigger == false);

		delete packet;
	}
}
