#include "common.hpp"
#include "RTC/Consts.hpp"
#include "RTC/NEW_RTCP/packet/AnyPayloadSpecificFeedbackPacket.hpp"
#include "RTC/NEW_RTCP/packet/AnyReportPacket.hpp"
#include "RTC/NEW_RTCP/packet/AnyTransportLayerFeedbackPacket.hpp"
#include "RTC/NEW_RTCP/packet/NackPacket.hpp"
#include "RTC/NEW_RTCP/packet/Packet.hpp"
#include "RTC/NEW_RTCP/packet/ReceiverReportPacket.hpp"
#include "RTC/NEW_RTCP/packet/SenderReportPacket.hpp"
#include "RTC/NEW_RTCP/packet/extendedReports/DlrrExtendedReport.hpp"
#include "RTC/NackGenerator.hpp"
#include "RTC/RTP/Packet.hpp"
#include "RTC/RTP/RtpStream.hpp"
#include "RTC/RTP/RtpStreamRecv.hpp"
#include "RTC/RtpDictionaries.hpp"
#include "test/include/RTC/NEW_RTCP/rtcpCommon.hpp"
#include "mocks/include/MockShared.hpp"
#include <catch2/catch_test_macros.hpp>
#include <vector>

// 17: 16 bit mask + the initial sequence number.
static constexpr size_t MaxRequestedPackets{ 17 };
static constexpr uint32_t SendNackDelay{ 0 }; // In ms.
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

		void OnRtpStreamSendRtcpPacket(
		  RTC::RTP::RtpStreamRecv* /*rtpStream*/, const RTC::NEW_RTCP::Packet* packet) override
		{
			switch (packet->GetType())
			{
				case RTC::NEW_RTCP::Packet::PacketType::PSFB:
				{
					switch (dynamic_cast<const RTC::NEW_RTCP::AnyPayloadSpecificFeedbackPacket*>(packet)
					          ->GetMessageType())
					{
						case RTC::NEW_RTCP::AnyPayloadSpecificFeedbackPacket::MessageType::PLI:
						{
							INFO("PLI required");

							REQUIRE(this->shouldTriggerPLI == true);

							this->shouldTriggerPLI = false;
							this->nackedSeqNumbers.clear();

							break;
						}

						case RTC::NEW_RTCP::AnyPayloadSpecificFeedbackPacket::MessageType::FIR:
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

				case RTC::NEW_RTCP::Packet::PacketType::RTPFB:
				{
					switch (dynamic_cast<const RTC::NEW_RTCP::AnyTransportLayerFeedbackPacket*>(packet)
					          ->GetMessageType())
					{
						case RTC::NEW_RTCP::AnyTransportLayerFeedbackPacket::MessageType::NACK:
						{
							INFO("NACK required");

							REQUIRE(this->shouldTriggerNack == true);

							this->shouldTriggerNack = false;

							const auto* const nackPacket = dynamic_cast<const RTC::NEW_RTCP::NackPacket*>(packet);

							for (const RTC::NEW_RTCP::NackPacket::Item& item : nackPacket->GetItems())
							{
								const uint16_t firstSeq = item.packetId;
								uint16_t bitmask        = item.lostPacketBitmask;

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

	SECTION("NACK that doesn't fit in a RTCP packet is split into several")
	{
		class NackItemsListener : public RTC::RTP::RtpStreamRecv::Listener
		{
		public:
			void OnRtpStreamScore(
			  RTC::RTP::RtpStream* /*rtpStream*/, uint8_t /*score*/, uint8_t /*previousScore*/) override
			{
			}

			void OnRtpStreamSendRtcpPacket(
			  RTC::RTP::RtpStreamRecv* /*rtpStream*/, const RTC::NEW_RTCP::Packet* packet) override
			{
				const auto* const nackPacket = dynamic_cast<const RTC::NEW_RTCP::NackPacket*>(packet);

				REQUIRE(nackPacket);
				REQUIRE(nackPacket->GetLength() <= RTC::Consts::OutgoingRtcpPacketMaxSize);

				this->nackPacketItemCounts.push_back(nackPacket->GetNumberOfItems());
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
			std::vector<size_t> nackPacketItemCounts;
		};

		NackItemsListener listener;
		RTC::RTP::RtpStreamRecv rtpStream(
		  std::addressof(listener), std::addressof(shared), params, SendNackDelay, UseRtpInactivityCheck);

		// Sequence numbers 17 apart, so each one takes a NACK item of its own.
		std::vector<uint16_t> seqNumbers;

		for (uint16_t i{ 0 }; i < 400; ++i)
		{
			seqNumbers.push_back(static_cast<uint16_t>(i * 17));
		}

		static_cast<RTC::NackGenerator::Listener&>(rtpStream).OnNackGeneratorNackRequired(seqNumbers);

		// A NACK packet takes 12 bytes of headers plus 4 bytes per item, so at most
		// 297 items fit in 1200 bytes.
		REQUIRE(listener.nackPacketItemCounts.size() == 2);
		REQUIRE(listener.nackPacketItemCounts[0] == 297);
		REQUIRE(listener.nackPacketItemCounts[1] == 103);
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

		REQUIRE(rtpStream.ReceiveRtxPacket(packet.get(), shared.GetTimeUs()));

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

		const auto receiveDelaySinceLastRr =
		  [&](int64_t receivedAtUs, uint32_t lastRr, uint32_t dlrr) -> std::optional<float>
		{
			const RTC::NEW_RTCP::DlrrExtendedReport::SubBlock subBlock{
				.ssrc                         = params.ssrc,
				.lastReceiverReport           = lastRr,
				.delaySinceLastReceiverReport = dlrr,
			};

			return rtpStream.ReceiveRtcpDlrrSubBlock(subBlock, receivedAtUs);
		};

		// The compact NTP representation has 16 bits of seconds, so it wraps around
		// every 65536 seconds.
		constexpr int64_t WrapUs{ 65536 * 1000000LL };

		// Nothing has been measured yet.
		REQUIRE(!rtpStream.GetRttMs().has_value());

		// The Receiver Reference Time is sent 1 second before the wrap, the remote
		// endpoint holds it for half a second, and its answer arrives right at the wrap.
		REQUIRE(receiveDelaySinceLastRr(WrapUs, 0xFFFF0000, 0x8000) == 500.0f);

		REQUIRE(rtpStream.GetRttMs() == 500.0f);

		// No Receiver Reference Time was received by the remote endpoint yet, so
		// nothing is measured and the last RTT is kept.
		REQUIRE(receiveDelaySinceLastRr(WrapUs, 0, 0) == std::nullopt);

		REQUIRE(rtpStream.GetRttMs() == 500.0f);

		// The remote endpoint answers the Receiver Reference Time right away.
		REQUIRE(receiveDelaySinceLastRr((10 * 1000000) + 500000, 0x000A0000, 0) == 500.0f);

		REQUIRE(rtpStream.GetRttMs() == 500.0f);

		// A negative RTT yields 1 millisecond.
		REQUIRE(receiveDelaySinceLastRr(20 * 1000000, 0x00140000, 0x8000) == 1.0f);

		REQUIRE(rtpStream.GetRttMs() == 1.0f);

		// So does a RTT too small to be true.
		REQUIRE(receiveDelaySinceLastRr(20 * 1000000, 0x0013FFFF, 0) == 1.0f);

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

	SECTION("ReceiveRtcpSenderReportPacket() ignores a zeroed NTP timestamp")
	{
		RtpStreamRecvListener listener;
		RTC::RTP::RtpStreamRecv rtpStream(
		  std::addressof(listener), std::addressof(shared), params, SendNackDelay, UseRtpInactivityCheck);

		const std::unique_ptr<RTC::NEW_RTCP::SenderReportPacket> senderReportPacket{
			RTC::NEW_RTCP::SenderReportPacket::Factory(
			  rtcpCommon::FactoryBuffer, sizeof(rtcpCommon::FactoryBuffer))
		};

		senderReportPacket->SetSsrc(params.ssrc);
		senderReportPacket->SetNtp({ .seconds = 0, .fractions = 0 });
		senderReportPacket->SetRtpTimestamp(1000);

		rtpStream.ReceiveRtcpSenderReportPacket(senderReportPacket.get(), shared.GetTimeUs());

		REQUIRE(!rtpStream.GetSenderReportReceivedAtUs().has_value());
		REQUIRE(!rtpStream.GetSenderReportMapping().has_value());

		senderReportPacket->SetNtp({ .seconds = 1000, .fractions = 0 });

		rtpStream.ReceiveRtcpSenderReportPacket(senderReportPacket.get(), shared.GetTimeUs());

		REQUIRE(rtpStream.GetSenderReportReceivedAtUs() == shared.GetTimeUs());

		const std::optional<RTC::RTP::RtpStream::SenderReportMapping> senderReportMapping =
		  rtpStream.GetSenderReportMapping();

		REQUIRE(senderReportMapping.has_value());

		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		const RTC::RTP::RtpStream::SenderReportMapping& senderReportMappingValue =
		  senderReportMapping.value();

		REQUIRE(senderReportMappingValue.ntpUs == 1000 * 1000000);
		REQUIRE(senderReportMappingValue.ts == 1000);
	}

	SECTION("AddRtcpReportBlock() reports the losses and the last Sender Report")
	{
		RtpStreamRecvListener listener;
		RTC::RTP::RtpStreamRecv rtpStream(
		  std::addressof(listener), std::addressof(shared), params, SendNackDelay, UseRtpInactivityCheck);

		packet->SetSequenceNumber(1);
		rtpStream.ReceivePacket(packet.get(), shared.GetTimeUs());

		packet->SetSequenceNumber(2);
		rtpStream.ReceivePacket(packet.get(), shared.GetTimeUs());

		// Sequence number 3 is lost.
		packet->SetSequenceNumber(4);
		listener.shouldTriggerNack = true;
		rtpStream.ReceivePacket(packet.get(), shared.GetTimeUs());

		// The Sender Report arrived half a second before the report block is built.
		const std::unique_ptr<RTC::NEW_RTCP::SenderReportPacket> senderReportPacket{
			RTC::NEW_RTCP::SenderReportPacket::Factory(rtcpCommon::FactoryBuffer, 1000)
		};

		senderReportPacket->SetSsrc(params.ssrc);
		senderReportPacket->SetNtp({ .seconds = 0x00010002, .fractions = 0x00030000 });
		senderReportPacket->SetRtpTimestamp(4);

		rtpStream.ReceiveRtcpSenderReportPacket(senderReportPacket.get(), shared.GetTimeUs() - 500000);

		const std::unique_ptr<RTC::NEW_RTCP::ReceiverReportPacket> receiverReportPacket{
			RTC::NEW_RTCP::ReceiverReportPacket::Factory(rtcpCommon::FactoryBuffer + 1000, 1000)
		};

		REQUIRE(rtpStream.AddRtcpReportBlock(receiverReportPacket.get()));
		REQUIRE(receiverReportPacket->GetNumberOfReportBlocks() == 1);

		const std::vector<RTC::NEW_RTCP::AnyReportPacket::ReportBlock> reportBlocks =
		  receiverReportPacket->GetReportBlocks();
		const RTC::NEW_RTCP::AnyReportPacket::ReportBlock& reportBlock = reportBlocks.at(0);

		REQUIRE(reportBlock.ssrc == params.ssrc);
		// 1 packet lost out of 4 expected, as a fixed point number with 8 bits of
		// fraction.
		REQUIRE(reportBlock.fractionLost == 64);
		REQUIRE(reportBlock.totalLost == 1);
		REQUIRE(reportBlock.extendedHighestSeq == 4);
		REQUIRE(reportBlock.jitter == 0);
		// Middle 32 bits out of 64 in the NTP timestamp of the Sender Report.
		REQUIRE(reportBlock.lastSenderReport == 0x00020003);
		// Half a second in units of 1/65536 seconds.
		REQUIRE(reportBlock.delaySinceLastSenderReport == 0x8000);
	}

	SECTION("AddRtcpReportBlock() touches nothing if the report block does not fit")
	{
		RtpStreamRecvListener listener;
		RTC::RTP::RtpStreamRecv rtpStream(
		  std::addressof(listener), std::addressof(shared), params, SendNackDelay, UseRtpInactivityCheck);

		packet->SetSequenceNumber(1);
		rtpStream.ReceivePacket(packet.get(), shared.GetTimeUs());

		packet->SetSequenceNumber(2);
		rtpStream.ReceivePacket(packet.get(), shared.GetTimeUs());

		// Sequence number 3 is lost.
		packet->SetSequenceNumber(4);
		listener.shouldTriggerNack = true;
		rtpStream.ReceivePacket(packet.get(), shared.GetTimeUs());

		// Room for the header of the packet alone.
		const std::unique_ptr<RTC::NEW_RTCP::ReceiverReportPacket> smallReceiverReportPacket{
			RTC::NEW_RTCP::ReceiverReportPacket::Factory(
			  rtcpCommon::FactoryBuffer, RTC::NEW_RTCP::ReceiverReportPacket::ReceiverReportHeaderLength)
		};

		REQUIRE(!rtpStream.AddRtcpReportBlock(smallReceiverReportPacket.get()));
		REQUIRE(smallReceiverReportPacket->GetNumberOfReportBlocks() == 0);
		REQUIRE(
		  smallReceiverReportPacket->GetLength() ==
		  RTC::NEW_RTCP::ReceiverReportPacket::ReceiverReportHeaderLength);

		// Room for as many report blocks as the packet can count, all of them taken.
		const std::unique_ptr<RTC::NEW_RTCP::ReceiverReportPacket> fullReceiverReportPacket{
			RTC::NEW_RTCP::ReceiverReportPacket::Factory(rtcpCommon::FactoryBuffer + 1000, 1000)
		};

		for (size_t idx{ 0 }; idx < RTC::NEW_RTCP::AnyReportPacket::MaxReportBlocks; ++idx)
		{
			fullReceiverReportPacket->AddReportBlock({ .ssrc = static_cast<uint32_t>(idx) });
		}

		REQUIRE(!rtpStream.AddRtcpReportBlock(fullReceiverReportPacket.get()));
		REQUIRE(
		  fullReceiverReportPacket->GetNumberOfReportBlocks() ==
		  RTC::NEW_RTCP::AnyReportPacket::MaxReportBlocks);

		// Had any of the failed attempts closed the reporting interval, this one
		// would report no loss.
		const std::unique_ptr<RTC::NEW_RTCP::ReceiverReportPacket> receiverReportPacket{
			RTC::NEW_RTCP::ReceiverReportPacket::Factory(rtcpCommon::FactoryBuffer + 2000, 1000)
		};

		REQUIRE(rtpStream.AddRtcpReportBlock(receiverReportPacket.get()));

		const std::vector<RTC::NEW_RTCP::AnyReportPacket::ReportBlock> reportBlocks =
		  receiverReportPacket->GetReportBlocks();
		const RTC::NEW_RTCP::AnyReportPacket::ReportBlock& reportBlock = reportBlocks.at(0);

		REQUIRE(reportBlock.fractionLost == 64);
		REQUIRE(reportBlock.totalLost == 1);
	}

	SECTION("AddRtxRtcpReportBlock() reports about the RTX stream once it receives packets")
	{
		// Needed by the RTX stream to tell its MIME type, and by the codec
		// processing of the packets it carries.
		params.mimeType.type    = RTC::RtpCodecMimeType::Type::VIDEO;
		params.mimeType.subtype = RTC::RtpCodecMimeType::Subtype::VP8;

		RtpStreamRecvListener listener;
		RTC::RTP::RtpStreamRecv rtpStream(
		  std::addressof(listener), std::addressof(shared), params, SendNackDelay, UseRtpInactivityCheck);

		const std::unique_ptr<RTC::NEW_RTCP::ReceiverReportPacket> receiverReportPacket{
			RTC::NEW_RTCP::ReceiverReportPacket::Factory(rtcpCommon::FactoryBuffer, 1000)
		};

		REQUIRE(!rtpStream.AddRtxRtcpReportBlock(receiverReportPacket.get()));
		REQUIRE(receiverReportPacket->GetNumberOfReportBlocks() == 0);

		rtpStream.SetRtx(params.rtxPayloadType, params.rtxSsrc);

		// Neither while the RTX stream has received no packet.
		REQUIRE(!rtpStream.AddRtxRtcpReportBlock(receiverReportPacket.get()));
		REQUIRE(receiverReportPacket->GetNumberOfReportBlocks() == 0);

		packet->SetSequenceNumber(1);
		packet->RtxEncode(params.rtxPayloadType, params.rtxSsrc, /*seq*/ 1000);

		rtpStream.ReceiveRtxPacket(packet.get(), shared.GetTimeUs());

		// A Sender Report with a zeroed NTP timestamp leaves nothing to report back.
		const std::unique_ptr<RTC::NEW_RTCP::SenderReportPacket> senderReportPacket{
			RTC::NEW_RTCP::SenderReportPacket::Factory(rtcpCommon::FactoryBuffer + 1000, 1000)
		};

		senderReportPacket->SetSsrc(params.rtxSsrc);
		senderReportPacket->SetNtp({ .seconds = 0, .fractions = 0 });

		rtpStream.ReceiveRtxRtcpSenderReportPacket(senderReportPacket.get(), shared.GetTimeUs() - 500000);

		REQUIRE(rtpStream.AddRtxRtcpReportBlock(receiverReportPacket.get()));

		// The Sender Report arrived half a second before the report block is built.
		senderReportPacket->SetNtp({ .seconds = 0x00010002, .fractions = 0x00030000 });

		rtpStream.ReceiveRtxRtcpSenderReportPacket(senderReportPacket.get(), shared.GetTimeUs() - 500000);

		REQUIRE(rtpStream.AddRtxRtcpReportBlock(receiverReportPacket.get()));
		REQUIRE(receiverReportPacket->GetNumberOfReportBlocks() == 2);

		const std::vector<RTC::NEW_RTCP::AnyReportPacket::ReportBlock> reportBlocks =
		  receiverReportPacket->GetReportBlocks();
		const RTC::NEW_RTCP::AnyReportPacket::ReportBlock& reportBlock0 = reportBlocks.at(0);

		REQUIRE(reportBlock0.ssrc == params.rtxSsrc);
		REQUIRE(reportBlock0.lastSenderReport == 0);
		REQUIRE(reportBlock0.delaySinceLastSenderReport == 0);

		const RTC::NEW_RTCP::AnyReportPacket::ReportBlock& reportBlock1 = reportBlocks.at(1);

		REQUIRE(reportBlock1.ssrc == params.rtxSsrc);
		// Middle 32 bits out of 64 in the NTP timestamp of the Sender Report.
		REQUIRE(reportBlock1.lastSenderReport == 0x00020003);
		// Half a second in units of 1/65536 seconds.
		REQUIRE(reportBlock1.delaySinceLastSenderReport == 0x8000);
	}

	SECTION(
	  "AddRtcpReportBlock() and AddRtxRtcpReportBlock() stop reporting about streams that receive nothing for too long")
	{
		int64_t nowUs{ 1000 * 1000 };

		mocks::MockShared sharedWithClock(/*getTimeUs*/
		                                  [&nowUs]() -> int64_t
		                                  {
			                                  return nowUs;
		                                  });

		// Needed by the RTX stream to tell its MIME type, and by the codec
		// processing of the packets it carries.
		params.mimeType.type    = RTC::RtpCodecMimeType::Type::VIDEO;
		params.mimeType.subtype = RTC::RtpCodecMimeType::Subtype::VP8;

		RtpStreamRecvListener listener;
		RTC::RTP::RtpStreamRecv rtpStream(
		  std::addressof(listener),
		  std::addressof(sharedWithClock),
		  params,
		  SendNackDelay,
		  UseRtpInactivityCheck);

		rtpStream.SetRtx(params.rtxPayloadType, params.rtxSsrc);

		packet->SetSequenceNumber(1);
		rtpStream.ReceivePacket(packet.get(), sharedWithClock.GetTimeUs());

		packet->SetSequenceNumber(2);
		packet->RtxEncode(params.rtxPayloadType, params.rtxSsrc, /*seq*/ 1000);
		rtpStream.ReceiveRtxPacket(packet.get(), sharedWithClock.GetTimeUs());

		const std::unique_ptr<RTC::NEW_RTCP::ReceiverReportPacket> receiverReportPacket{
			RTC::NEW_RTCP::ReceiverReportPacket::Factory(
			  rtcpCommon::FactoryBuffer, sizeof(rtcpCommon::FactoryBuffer))
		};

		// Right before the timeout both streams are still reported about.
		nowUs += (RTC::RTP::RtpStream::ReportBlockTimeoutMs * 1000) - 1;

		REQUIRE(rtpStream.AddRtcpReportBlock(receiverReportPacket.get()));
		REQUIRE(rtpStream.AddRtxRtcpReportBlock(receiverReportPacket.get()));
		REQUIRE(receiverReportPacket->GetNumberOfReportBlocks() == 2);

		// Once the timeout is reached neither of them is.
		nowUs += 1;

		REQUIRE(!rtpStream.AddRtcpReportBlock(receiverReportPacket.get()));
		REQUIRE(!rtpStream.AddRtxRtcpReportBlock(receiverReportPacket.get()));
		REQUIRE(receiverReportPacket->GetNumberOfReportBlocks() == 2);

		// A new packet makes the stream that receives it be reported about again.
		packet->SetSequenceNumber(3);
		rtpStream.ReceivePacket(packet.get(), sharedWithClock.GetTimeUs());

		REQUIRE(rtpStream.AddRtcpReportBlock(receiverReportPacket.get()));
		REQUIRE(!rtpStream.AddRtxRtcpReportBlock(receiverReportPacket.get()));
		REQUIRE(receiverReportPacket->GetNumberOfReportBlocks() == 3);
	}
}
