#include "common.hpp"
#include "RTC/RTCP/FeedbackPs.hpp"
#include "RTC/RTCP/FeedbackRtpNack.hpp"
#include "RTC/RTCP/XrDelaySinceLastRr.hpp"
#include "RTC/RTP/Packet.hpp"
#include "RTC/RTP/RtpStream.hpp"
#include "RTC/RTP/RtpStreamRecv.hpp"
#include "mocks/include/MockShared.hpp"
#include <catch2/catch_test_macros.hpp>
#include <vector>

// 17: 16 bit mask + the initial sequence number.
static constexpr size_t MaxRequestedPackets{ 17 };
static constexpr uint32_t SendNackDelay{ 0u }; // In ms.
static const bool UseRtpInactivityCheck{ false };

SCENARIO("RtpStreamRecv", "[rtp][rtpstream][rtpstreamrecv]")
{
	class RtpStreamRecvListener : public RTC::RTP::RtpStreamRecv::Listener
	{
	public:
		void OnRtpStreamScore(
		  RTC::RTP::RtpStream* /*rtpStream*/, uint8_t /*score*/, uint8_t /*previousScore*/) override
		{
		}

		void OnRtpStreamSendRtcpPacket(RTC::RTP::RtpStreamRecv* /*rtpStream*/, RTC::RTCP::Packet* packet) override
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
							this->nackedSeqNumbers.clear();

							break;
						}

						case RTC::RTCP::FeedbackPs::MessageType::FIR:
						{
							INFO("FIR required");

							REQUIRE(this->shouldTriggerFIR == true);

							this->shouldTriggerFIR = false;
							this->nackedSeqNumbers.clear();

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
								const RTC::RTCP::FeedbackRtpNackItem* item = *it;

								const uint16_t firstSeq = item->GetPacketId();
								uint16_t bitmask        = item->GetLostPacketBitmask();

								this->nackedSeqNumbers.push_back(firstSeq);

								for (size_t i{ 1 }; i < MaxRequestedPackets; ++i)
								{
									if ((bitmask & 1) != 0)
									{
										this->nackedSeqNumbers.push_back(firstSeq + i);
									}

									bitmask >>= 1;
								}
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

		uint8_t OnRtpStreamNeedWorstRemoteFractionLost(RTC::RTP::RtpStreamRecv* /*rtpStream*/) override
		{
			return 0;
		}

		void OnRtpStreamSpatialLayerActivityChanged(
		  RTC::RTP::RtpStreamRecv* /*rtpStream*/, uint8_t /*spatialLayer*/, bool /*isActive*/) override
		{
		}

	public:
		bool shouldTriggerNack = false;
		bool shouldTriggerPLI  = false;
		bool shouldTriggerFIR  = false;
		std::vector<uint16_t> nackedSeqNumbers;
	};

	mocks::MockShared shared(/*getTimeUs*/
	                         []() -> int64_t
	                         {
		                         return 1000 * 1000;
	                         });

	// clang-format off
	alignas(4) uint8_t buffer[] =
	{
		0x80, 0x01, 0x00, 0x01,
		0x00, 0x00, 0x00, 0x04,
		0x00, 0x00, 0x00, 0x05,
		0x00, 0x00, 0x00, 0x00 // Extra space for RTX encoding.
	};
	// clang-format on

	std::unique_ptr<RTC::RTP::Packet> packet{ RTC::RTP::Packet::Parse(buffer, 12, 12 + 4) };

	if (!packet)
	{
		FAIL("not a RTP packet");
	}

	RTC::RTP::RtpStream::Params params;

	params.ssrc           = packet->GetSsrc();
	params.rtxSsrc        = 1234;
	params.rtxPayloadType = 96;
	params.clockRate      = 90000;
	params.useNack        = true;
	params.usePli         = true;
	params.useFir         = false;

	SECTION("NACK one packet")
	{
		RtpStreamRecvListener listener;
		RTC::RTP::RtpStreamRecv rtpStream(
		  std::addressof(listener), std::addressof(shared), params, SendNackDelay, UseRtpInactivityCheck);

		packet->SetSequenceNumber(1);
		rtpStream.ReceivePacket(packet.get(), shared.GetTimeUs());

		packet->SetSequenceNumber(3);
		listener.shouldTriggerNack = true;
		listener.shouldTriggerPLI  = false;
		listener.shouldTriggerFIR  = false;
		rtpStream.ReceivePacket(packet.get(), shared.GetTimeUs());

		REQUIRE(listener.nackedSeqNumbers.size() == 1);
		REQUIRE(listener.nackedSeqNumbers[0] == 2);
		listener.nackedSeqNumbers.clear();

		packet->SetSequenceNumber(2);
		rtpStream.ReceivePacket(packet.get(), shared.GetTimeUs());

		REQUIRE(listener.nackedSeqNumbers.empty());

		packet->SetSequenceNumber(4);
		rtpStream.ReceivePacket(packet.get(), shared.GetTimeUs());

		REQUIRE(listener.nackedSeqNumbers.empty());
	}

	SECTION("receive RTX before corresponding RTP")
	{
		RtpStreamRecvListener listener;
		RTC::RTP::RtpStreamRecv rtpStream(
		  std::addressof(listener), std::addressof(shared), params, SendNackDelay, UseRtpInactivityCheck);

		packet->SetSequenceNumber(1);
		rtpStream.ReceivePacket(packet.get(), shared.GetTimeUs());

		packet->SetSequenceNumber(2);
		rtpStream.ReceivePacket(packet.get(), shared.GetTimeUs());

		packet->SetSequenceNumber(3);
		rtpStream.ReceivePacket(packet.get(), shared.GetTimeUs());

		packet->SetSequenceNumber(4);
		rtpStream.ReceivePacket(packet.get(), shared.GetTimeUs());

		packet->SetSequenceNumber(5);
		rtpStream.ReceivePacket(packet.get(), shared.GetTimeUs());

		// Sequence number 6 arrives via RTX before the original RTP packet.

		auto originalSsrc        = packet->GetSsrc();
		auto originalPayloadType = packet->GetPayloadType();

		packet->SetSequenceNumber(6);
		packet->RtxEncode(params.rtxPayloadType, params.rtxSsrc, 1000 /*seq=*/);

		REQUIRE(rtpStream.ReceiveRtxPacket(packet.get()));

		packet->RtxDecode(originalPayloadType, originalSsrc);
	}

	SECTION("wrapping sequence numbers")
	{
		RtpStreamRecvListener listener;
		RTC::RTP::RtpStreamRecv rtpStream(
		  std::addressof(listener), std::addressof(shared), params, SendNackDelay, UseRtpInactivityCheck);

		packet->SetSequenceNumber(0xfffe);
		rtpStream.ReceivePacket(packet.get(), shared.GetTimeUs());

		packet->SetSequenceNumber(1);
		listener.shouldTriggerNack = true;
		listener.shouldTriggerPLI  = false;
		listener.shouldTriggerFIR  = false;
		rtpStream.ReceivePacket(packet.get(), shared.GetTimeUs());

		REQUIRE(listener.nackedSeqNumbers.size() == 2);
		REQUIRE(listener.nackedSeqNumbers[0] == 0xffff);
		REQUIRE(listener.nackedSeqNumbers[1] == 0);
		listener.nackedSeqNumbers.clear();
	}

	SECTION("RTT is computed from Extended Reports")
	{
		RtpStreamRecvListener listener;
		RTC::RTP::RtpStreamRecv rtpStream(
		  std::addressof(listener), std::addressof(shared), params, SendNackDelay, UseRtpInactivityCheck);

		const auto receiveDelaySinceLastRr = [&](int64_t receivedAtUs, uint32_t lastRr, uint32_t dlrr)
		{
			RTC::RTCP::DelaySinceLastRr::SsrcInfo ssrcInfo;

			ssrcInfo.SetSsrc(params.ssrc);
			ssrcInfo.SetLastReceiverReport(lastRr);
			ssrcInfo.SetDelaySinceLastReceiverReport(dlrr);

			rtpStream.ReceiveRtcpXrDelaySinceLastRr(std::addressof(ssrcInfo), receivedAtUs);
		};

		// The compact NTP representation has 16 bits of seconds, so it wraps around
		// every 65536 seconds.
		constexpr int64_t WrapUs{ 65536 * 1000000LL };

		// The Receiver Reference Time is sent 1 second before the wrap, the remote
		// endpoint holds it for half a second, and its answer arrives right at the wrap.
		receiveDelaySinceLastRr(WrapUs, 0xFFFF0000, 0x8000);

		REQUIRE(rtpStream.GetRttMs() == 500.0f);

		// No Receiver Reference Time was received by the remote endpoint yet, so the
		// last RTT is kept.
		receiveDelaySinceLastRr(WrapUs, 0, 0);

		REQUIRE(rtpStream.GetRttMs() == 500.0f);

		// The remote endpoint answers the Receiver Reference Time right away.
		receiveDelaySinceLastRr((10 * 1000000) + 500000, 0x000A0000, 0);

		REQUIRE(rtpStream.GetRttMs() == 500.0f);

		// A negative RTT yields 1 millisecond.
		receiveDelaySinceLastRr(20 * 1000000, 0x00140000, 0x8000);

		REQUIRE(rtpStream.GetRttMs() == 1.0f);

		// So does a RTT too small to be true.
		receiveDelaySinceLastRr(20 * 1000000, 0x0013FFFF, 0);

		REQUIRE(rtpStream.GetRttMs() == 1.0f);
	}

	SECTION("require key frame")
	{
		RtpStreamRecvListener listener;
		RTC::RTP::RtpStreamRecv rtpStream(
		  std::addressof(listener), std::addressof(shared), params, SendNackDelay, UseRtpInactivityCheck);

		packet->SetSequenceNumber(1);
		rtpStream.ReceivePacket(packet.get(), shared.GetTimeUs());

		// Seq different is bigger than MaxNackPackets in NackGenerator, so it
		// triggers a key frame.
		packet->SetSequenceNumber(1003);
		listener.shouldTriggerPLI = true;
		listener.shouldTriggerFIR = false;
		rtpStream.ReceivePacket(packet.get(), shared.GetTimeUs());
	}
}
