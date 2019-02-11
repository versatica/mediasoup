#include "common.hpp"
#include "catch.hpp"
#include "RTC/RtpPacket.hpp"
#include "RTC/RtpStream.hpp"
#include "RTC/RtpStreamRecv.hpp"
#include <vector>

using namespace RTC;

SCENARIO("receive RTP packets and trigger NACK", "[rtp][rtpstream]")
{
	class RtpStreamRecvListener : public RtpStream::Listener
	{
	public:
		void OnRtpStreamSendRtcpPacket(RTC::RtpStream* rtpStream, RTC::RTCP::Packet* packet) override
		{
			switch (packet->GetType())
			{
				case RTC::RTCP::Type::PSFB:
				{
					switch (dynamic_cast<RTC::RTCP::FeedbackPsPacket*>(packet)->GetMessageType())
					{
						case RTC::RTCP::FeedbackPs::MessageType::PLI:
						{
							INFO("PLI required");

							REQUIRE(this->shouldTriggerPLI == true);

							this->shouldTriggerPLI = false;
							this->numNackedPackets = 0;

							break;
						}

						case RTC::RTCP::FeedbackPs::MessageType::FIR:
						{
							INFO("FIR required");

							REQUIRE(this->shouldTriggerFIR == true);

							this->shouldTriggerFIR = false;
							this->numNackedPackets = 0;

							break;
						}

						default:;
					}

					break;
				}

				case RTC::RTCP::Type::RTPFB:
				{
					switch (dynamic_cast<RTC::RTCP::FeedbackRtpPacket*>(packet)->GetMessageType())
					{
						case RTC::RTCP::FeedbackRtp::MessageType::NACK:
						{
							INFO("NACK required");

							REQUIRE(this->shouldTriggerNack == true);

							this->shouldTriggerNack = false;

							auto* nackPacket = dynamic_cast<RTC::RTCP::FeedbackRtpNackPacket*>(packet);

							for (auto it = nackPacket->Begin(); it != nackPacket->End(); ++it)
							{
								RTC::RTCP::FeedbackRtpNackItem* item = *it;

								this->numNackedPackets += item->CountRequestedPackets();
							}

							break;
						}

						default:;
					}

					break;
				}

				default:;
			}
		}

		void OnRtpStreamRetransmitRtpPacket(RTC::RtpStream* rtpStream, RTC::RtpPacket* packet) override
		{
		}

		void OnRtpStreamScore(RTC::RtpStream* /*rtpStream*/, uint8_t /*score*/) override
		{
		}

	public:
		bool shouldTriggerNack  = false;
		bool shouldTriggerPLI   = false;
		bool shouldTriggerFIR   = false;
		size_t numNackedPackets = 0;
	};

	// clang-format off
	uint8_t buffer[] =
	{
		0b10000000, 0b00000001, 0, 1,
		0, 0, 0, 4,
		0, 0, 0, 5
	};
	// clang-format on

	RtpPacket* packet = RtpPacket::Parse(buffer, sizeof(buffer));

	if (!packet)
		FAIL("not a RTP packet");

	RtpStream::Params params;

	params.ssrc      = packet->GetSsrc();
	params.clockRate = 90000;
	params.useNack   = true;
	params.usePli    = true;
	params.useFir    = false;

	SECTION("NACK one packet")
	{
		RtpStreamRecvListener listener;
		RtpStreamRecv rtpStream(&listener, params);

		packet->SetSequenceNumber(1);
		rtpStream.ReceivePacket(packet);

		packet->SetSequenceNumber(3);
		listener.shouldTriggerNack = true;
		listener.shouldTriggerPLI  = false;
		listener.shouldTriggerFIR  = false;
		rtpStream.ReceivePacket(packet);

		REQUIRE(listener.numNackedPackets == 1);
		listener.numNackedPackets = 0;

		packet->SetSequenceNumber(2);
		rtpStream.ReceivePacket(packet);

		REQUIRE(listener.numNackedPackets == 0);

		packet->SetSequenceNumber(4);
		rtpStream.ReceivePacket(packet);

		REQUIRE(listener.numNackedPackets == 0);
	}

	SECTION("wrapping sequence numbers")
	{
		RtpStreamRecvListener listener;
		RtpStreamRecv rtpStream(&listener, params);

		packet->SetSequenceNumber(0xfffe);
		rtpStream.ReceivePacket(packet);

		packet->SetSequenceNumber(1);
		listener.shouldTriggerNack = true;
		listener.shouldTriggerPLI  = false;
		listener.shouldTriggerFIR  = false;
		rtpStream.ReceivePacket(packet);

		REQUIRE(listener.numNackedPackets == 2);
		listener.numNackedPackets = 0;
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
		listener.shouldTriggerPLI = true;
		listener.shouldTriggerFIR = false;
		rtpStream.ReceivePacket(packet);
	}

	delete packet;
}
