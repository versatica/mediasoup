#include "fuzzer/include/RTC/RTCP/packet/FuzzerCompoundPacket.hpp"
#include "RTC/RTCP/packet/AnyPayloadSpecificFeedbackPacket.hpp"
#include "RTC/RTCP/packet/AnyTransportLayerFeedbackPacket.hpp"
#include "RTC/RTCP/packet/ByePacket.hpp"
#include "RTC/RTCP/packet/CompoundPacket.hpp"
#include "RTC/RTCP/packet/FirPacket.hpp"
#include "RTC/RTCP/packet/NackPacket.hpp"
#include "RTC/RTCP/packet/Packet.hpp"
#include "RTC/RTCP/packet/PliPacket.hpp"
#include "RTC/RTCP/packet/ReceiverReportPacket.hpp"
#include "RTC/RTCP/packet/RembPacket.hpp"
#include "RTC/RTCP/packet/SdesPacket.hpp"
#include "RTC/RTCP/packet/SenderReportPacket.hpp"
#include "RTC/RTCP/packet/TransportWideCcPacket.hpp"
#include "RTC/RTCP/packet/UnknownPacket.hpp"
#include "RTC/RTCP/packet/UnknownPayloadSpecificFeedbackPacket.hpp"
#include "RTC/RTCP/packet/UnknownTransportLayerFeedbackPacket.hpp"
#include "RTC/RTCP/packet/XrPacket.hpp"
#include "fuzzer/include/RTC/RTCP/packet/FuzzerByePacket.hpp"
#include "fuzzer/include/RTC/RTCP/packet/FuzzerFirPacket.hpp"
#include "fuzzer/include/RTC/RTCP/packet/FuzzerNackPacket.hpp"
#include "fuzzer/include/RTC/RTCP/packet/FuzzerPliPacket.hpp"
#include "fuzzer/include/RTC/RTCP/packet/FuzzerReceiverReportPacket.hpp"
#include "fuzzer/include/RTC/RTCP/packet/FuzzerRembPacket.hpp"
#include "fuzzer/include/RTC/RTCP/packet/FuzzerSdesPacket.hpp"
#include "fuzzer/include/RTC/RTCP/packet/FuzzerSenderReportPacket.hpp"
#include "fuzzer/include/RTC/RTCP/packet/FuzzerTransportWideCcPacket.hpp"
#include "fuzzer/include/RTC/RTCP/packet/FuzzerUnknownPacket.hpp"
#include "fuzzer/include/RTC/RTCP/packet/FuzzerUnknownPayloadSpecificFeedbackPacket.hpp"
#include "fuzzer/include/RTC/RTCP/packet/FuzzerUnknownTransportLayerFeedbackPacket.hpp"
#include "fuzzer/include/RTC/RTCP/packet/FuzzerXrPacket.hpp"

namespace
{
	thread_local uint8_t CompoundPacketSerializeBuffer[65536];
	thread_local uint8_t CompoundPacketCloneBuffer[65536];
} // namespace

void FuzzerRtcRtcpCompoundPacket::Fuzz(const uint8_t* data, size_t len)
{
	const auto fuzzPackets = [](const RTC::RTCP::CompoundPacket* compoundPacket) -> void
	{
		compoundPacket->HasPackets();
		compoundPacket->GetPacketsCount();
		compoundPacket->GetPacketAt(0);
		compoundPacket->GetPacketAt(1);
		compoundPacket->GetPacketAt(2);
		compoundPacket->GetPacketAt(666);
		compoundPacket->GetFirstPacketOfType<RTC::RTCP::SenderReportPacket>();
		compoundPacket->GetFirstPacketOfType<RTC::RTCP::ReceiverReportPacket>();
		compoundPacket->GetFirstPacketOfType<RTC::RTCP::SdesPacket>();
		compoundPacket->GetFirstPacketOfType<RTC::RTCP::ByePacket>();
		compoundPacket->GetFirstPacketOfType<RTC::RTCP::NackPacket>();
		compoundPacket->GetFirstPacketOfType<RTC::RTCP::TransportWideCcPacket>();
		compoundPacket->GetFirstPacketOfType<RTC::RTCP::UnknownTransportLayerFeedbackPacket>();
		compoundPacket->GetFirstPacketOfType<RTC::RTCP::PliPacket>();
		compoundPacket->GetFirstPacketOfType<RTC::RTCP::FirPacket>();
		compoundPacket->GetFirstPacketOfType<RTC::RTCP::RembPacket>();
		compoundPacket->GetFirstPacketOfType<RTC::RTCP::UnknownPayloadSpecificFeedbackPacket>();
		compoundPacket->GetFirstPacketOfType<RTC::RTCP::XrPacket>();
		compoundPacket->GetFirstPacketOfType<RTC::RTCP::UnknownPacket>();

		for (auto it = compoundPacket->PacketsBegin(); it != compoundPacket->PacketsEnd(); ++it)
		{
			const RTC::RTCP::Packet* const packet = *it;

			packet->GetVersion();
			packet->GetType();
			packet->HasUnknownType();
			packet->HasPadding();
			packet->GetPaddingLength();

			switch (packet->GetType())
			{
				case RTC::RTCP::Packet::PacketType::SR:
				{
					const auto* const senderReportPacket =
					  static_cast<const RTC::RTCP::SenderReportPacket*>(packet);

					FuzzerRtcRtcpSenderReportPacket::Fuzz(senderReportPacket);

					break;
				}

				case RTC::RTCP::Packet::PacketType::RR:
				{
					const auto* const receiverReportPacket =
					  static_cast<const RTC::RTCP::ReceiverReportPacket*>(packet);

					FuzzerRtcRtcpReceiverReportPacket::Fuzz(receiverReportPacket);

					break;
				}

				case RTC::RTCP::Packet::PacketType::SDES:
				{
					const auto* const sdesPacket = static_cast<const RTC::RTCP::SdesPacket*>(packet);

					FuzzerRtcRtcpSdesPacket::Fuzz(sdesPacket);

					break;
				}

				case RTC::RTCP::Packet::PacketType::BYE:
				{
					const auto* const byePacket = static_cast<const RTC::RTCP::ByePacket*>(packet);

					FuzzerRtcRtcpByePacket::Fuzz(byePacket);

					break;
				}

				case RTC::RTCP::Packet::PacketType::RTPFB:
				{
					const auto* const transportLayerFeedbackPacket =
					  static_cast<const RTC::RTCP::AnyTransportLayerFeedbackPacket*>(packet);

					switch (transportLayerFeedbackPacket->GetMessageType())
					{
						case RTC::RTCP::AnyTransportLayerFeedbackPacket::MessageType::NACK:
						{
							const auto* const nackPacket =
							  static_cast<const RTC::RTCP::NackPacket*>(transportLayerFeedbackPacket);

							FuzzerRtcRtcpNackPacket::Fuzz(nackPacket);

							break;
						}

						case RTC::RTCP::AnyTransportLayerFeedbackPacket::MessageType::TCC:
						{
							const auto* const transportWideCcPacket =
							  static_cast<const RTC::RTCP::TransportWideCcPacket*>(transportLayerFeedbackPacket);

							FuzzerRtcRtcpTransportWideCcPacket::Fuzz(transportWideCcPacket);

							break;
						}

						default:
						{
							const auto* const unknownTransportLayerFeedbackPacket =
							  static_cast<const RTC::RTCP::UnknownTransportLayerFeedbackPacket*>(
							    transportLayerFeedbackPacket);

							FuzzerRtcRtcpUnknownTransportLayerFeedbackPacket::Fuzz(
							  unknownTransportLayerFeedbackPacket);
						}
					}

					break;
				}

				case RTC::RTCP::Packet::PacketType::PSFB:
				{
					const auto* const payloadSpecificFeedbackPacket =
					  static_cast<const RTC::RTCP::AnyPayloadSpecificFeedbackPacket*>(packet);

					switch (payloadSpecificFeedbackPacket->GetMessageType())
					{
						case RTC::RTCP::AnyPayloadSpecificFeedbackPacket::MessageType::PLI:
						{
							const auto* const pliPacket =
							  static_cast<const RTC::RTCP::PliPacket*>(payloadSpecificFeedbackPacket);

							FuzzerRtcRtcpPliPacket::Fuzz(pliPacket);

							break;
						}

						case RTC::RTCP::AnyPayloadSpecificFeedbackPacket::MessageType::FIR:
						{
							const auto* const firPacket =
							  static_cast<const RTC::RTCP::FirPacket*>(payloadSpecificFeedbackPacket);

							FuzzerRtcRtcpFirPacket::Fuzz(firPacket);

							break;
						}

						case RTC::RTCP::AnyPayloadSpecificFeedbackPacket::MessageType::AFB:
						{
							if (
							  RTC::RTCP::RembPacket::IsRemb(
							    payloadSpecificFeedbackPacket->GetBuffer(),
							    payloadSpecificFeedbackPacket->GetLength()))
							{
								const auto* const rembPacket =
								  static_cast<const RTC::RTCP::RembPacket*>(payloadSpecificFeedbackPacket);

								FuzzerRtcRtcpRembPacket::Fuzz(rembPacket);
							}
							else
							{
								const auto* const unknownPayloadSpecificFeedbackPacket =
								  static_cast<const RTC::RTCP::UnknownPayloadSpecificFeedbackPacket*>(
								    payloadSpecificFeedbackPacket);

								FuzzerRtcRtcpUnknownPayloadSpecificFeedbackPacket::Fuzz(
								  unknownPayloadSpecificFeedbackPacket);
							}

							break;
						}

						default:
						{
							const auto* const unknownPayloadSpecificFeedbackPacket =
							  static_cast<const RTC::RTCP::UnknownPayloadSpecificFeedbackPacket*>(
							    payloadSpecificFeedbackPacket);

							FuzzerRtcRtcpUnknownPayloadSpecificFeedbackPacket::Fuzz(
							  unknownPayloadSpecificFeedbackPacket);
						}
					}

					break;
				}

				case RTC::RTCP::Packet::PacketType::XR:
				{
					const auto* const xrPacket = static_cast<const RTC::RTCP::XrPacket*>(packet);

					FuzzerRtcRtcpXrPacket::Fuzz(xrPacket);

					break;
				}

				default:
				{
					const auto* const unknownPacket = static_cast<const RTC::RTCP::UnknownPacket*>(packet);

					FuzzerRtcRtcpUnknownPacket::Fuzz(unknownPacket);
				}
			}
		}
	};

	RTC::RTCP::RembPacket::IsRemb(data, len);

	RTC::RTCP::CompoundPacket* const compoundPacket = RTC::RTCP::CompoundPacket::Parse(data, len);

	if (!compoundPacket)
	{
		return;
	}

	fuzzPackets(compoundPacket);

	compoundPacket->Serialize(CompoundPacketSerializeBuffer, len);

	fuzzPackets(compoundPacket);

	auto* const clonedCompoundPacket = compoundPacket->Clone(CompoundPacketCloneBuffer, len);

	delete compoundPacket;

	fuzzPackets(clonedCompoundPacket);

	clonedCompoundPacket->Serialize(CompoundPacketSerializeBuffer, len);

	fuzzPackets(clonedCompoundPacket);

	delete clonedCompoundPacket;
}
