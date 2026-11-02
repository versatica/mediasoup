#include "RTC/NEW_RTCP/packet/FuzzerCompoundPacket.hpp"
#include "RTC/NEW_RTCP/packet/AnyPayloadSpecificFeedbackPacket.hpp"
#include "RTC/NEW_RTCP/packet/AnyTransportLayerFeedbackPacket.hpp"
#include "RTC/NEW_RTCP/packet/ByePacket.hpp"
#include "RTC/NEW_RTCP/packet/CompoundPacket.hpp"
#include "RTC/NEW_RTCP/packet/FirPacket.hpp"
#include "RTC/NEW_RTCP/packet/FuzzerByePacket.hpp"
#include "RTC/NEW_RTCP/packet/FuzzerFirPacket.hpp"
#include "RTC/NEW_RTCP/packet/FuzzerNackPacket.hpp"
#include "RTC/NEW_RTCP/packet/FuzzerPliPacket.hpp"
#include "RTC/NEW_RTCP/packet/FuzzerReceiverReportPacket.hpp"
#include "RTC/NEW_RTCP/packet/FuzzerRembPacket.hpp"
#include "RTC/NEW_RTCP/packet/FuzzerSdesPacket.hpp"
#include "RTC/NEW_RTCP/packet/FuzzerSenderReportPacket.hpp"
#include "RTC/NEW_RTCP/packet/FuzzerUnknownPacket.hpp"
#include "RTC/NEW_RTCP/packet/FuzzerUnknownPayloadSpecificFeedbackPacket.hpp"
#include "RTC/NEW_RTCP/packet/FuzzerUnknownTransportLayerFeedbackPacket.hpp"
#include "RTC/NEW_RTCP/packet/NackPacket.hpp"
#include "RTC/NEW_RTCP/packet/Packet.hpp"
#include "RTC/NEW_RTCP/packet/PliPacket.hpp"
#include "RTC/NEW_RTCP/packet/ReceiverReportPacket.hpp"
#include "RTC/NEW_RTCP/packet/RembPacket.hpp"
#include "RTC/NEW_RTCP/packet/SdesPacket.hpp"
#include "RTC/NEW_RTCP/packet/SenderReportPacket.hpp"
#include "RTC/NEW_RTCP/packet/UnknownPacket.hpp"
#include "RTC/NEW_RTCP/packet/UnknownPayloadSpecificFeedbackPacket.hpp"
#include "RTC/NEW_RTCP/packet/UnknownTransportLayerFeedbackPacket.hpp"

namespace
{
	thread_local uint8_t CompoundPacketSerializeBuffer[65536];
	thread_local uint8_t CompoundPacketCloneBuffer[65536];
} // namespace

void FuzzerRtcRtcpCompoundPacket::Fuzz(const uint8_t* data, size_t len)
{
	const auto fuzzPackets = [](const RTC::NEW_RTCP::CompoundPacket* compoundPacket) -> void
	{
		compoundPacket->HasPackets();
		compoundPacket->GetPacketsCount();
		compoundPacket->GetPacketAt(0);
		compoundPacket->GetPacketAt(1);
		compoundPacket->GetPacketAt(2);
		compoundPacket->GetPacketAt(666);
		compoundPacket->GetFirstPacketOfType<RTC::NEW_RTCP::SenderReportPacket>();
		compoundPacket->GetFirstPacketOfType<RTC::NEW_RTCP::ReceiverReportPacket>();
		compoundPacket->GetFirstPacketOfType<RTC::NEW_RTCP::SdesPacket>();
		compoundPacket->GetFirstPacketOfType<RTC::NEW_RTCP::ByePacket>();
		compoundPacket->GetFirstPacketOfType<RTC::NEW_RTCP::NackPacket>();
		compoundPacket->GetFirstPacketOfType<RTC::NEW_RTCP::UnknownTransportLayerFeedbackPacket>();
		compoundPacket->GetFirstPacketOfType<RTC::NEW_RTCP::PliPacket>();
		compoundPacket->GetFirstPacketOfType<RTC::NEW_RTCP::FirPacket>();
		compoundPacket->GetFirstPacketOfType<RTC::NEW_RTCP::RembPacket>();
		compoundPacket->GetFirstPacketOfType<RTC::NEW_RTCP::UnknownPayloadSpecificFeedbackPacket>();
		compoundPacket->GetFirstPacketOfType<RTC::NEW_RTCP::UnknownPacket>();

		for (auto it = compoundPacket->PacketsBegin(); it != compoundPacket->PacketsEnd(); ++it)
		{
			const RTC::NEW_RTCP::Packet* const packet = *it;

			packet->GetVersion();
			packet->GetType();
			packet->HasUnknownType();
			packet->HasPadding();

			switch (packet->GetType())
			{
				case RTC::NEW_RTCP::Packet::PacketType::SR:
				{
					const auto* const senderReportPacket =
					  static_cast<const RTC::NEW_RTCP::SenderReportPacket*>(packet);

					FuzzerRtcRtcpSenderReportPacket::Fuzz(senderReportPacket);

					break;
				}

				case RTC::NEW_RTCP::Packet::PacketType::RR:
				{
					const auto* const receiverReportPacket =
					  static_cast<const RTC::NEW_RTCP::ReceiverReportPacket*>(packet);

					FuzzerRtcRtcpReceiverReportPacket::Fuzz(receiverReportPacket);

					break;
				}

				case RTC::NEW_RTCP::Packet::PacketType::SDES:
				{
					const auto* const sdesPacket = static_cast<const RTC::NEW_RTCP::SdesPacket*>(packet);

					FuzzerRtcRtcpSdesPacket::Fuzz(sdesPacket);

					break;
				}

				case RTC::NEW_RTCP::Packet::PacketType::BYE:
				{
					const auto* const byePacket = static_cast<const RTC::NEW_RTCP::ByePacket*>(packet);

					FuzzerRtcRtcpByePacket::Fuzz(byePacket);

					break;
				}

				case RTC::NEW_RTCP::Packet::PacketType::RTPFB:
				{
					const auto* const transportLayerFeedbackPacket =
					  static_cast<const RTC::NEW_RTCP::AnyTransportLayerFeedbackPacket*>(packet);

					switch (transportLayerFeedbackPacket->GetMessageType())
					{
						case RTC::NEW_RTCP::AnyTransportLayerFeedbackPacket::MessageType::NACK:
						{
							const auto* const nackPacket =
							  static_cast<const RTC::NEW_RTCP::NackPacket*>(transportLayerFeedbackPacket);

							FuzzerRtcRtcpNackPacket::Fuzz(nackPacket);

							break;
						}

						default:
						{
							const auto* const unknownTransportLayerFeedbackPacket =
							  static_cast<const RTC::NEW_RTCP::UnknownTransportLayerFeedbackPacket*>(
							    transportLayerFeedbackPacket);

							FuzzerRtcRtcpUnknownTransportLayerFeedbackPacket::Fuzz(
							  unknownTransportLayerFeedbackPacket);
						}
					}

					break;
				}

				case RTC::NEW_RTCP::Packet::PacketType::PSFB:
				{
					const auto* const payloadSpecificFeedbackPacket =
					  static_cast<const RTC::NEW_RTCP::AnyPayloadSpecificFeedbackPacket*>(packet);

					switch (payloadSpecificFeedbackPacket->GetMessageType())
					{
						case RTC::NEW_RTCP::AnyPayloadSpecificFeedbackPacket::MessageType::PLI:
						{
							const auto* const pliPacket =
							  static_cast<const RTC::NEW_RTCP::PliPacket*>(payloadSpecificFeedbackPacket);

							FuzzerRtcRtcpPliPacket::Fuzz(pliPacket);

							break;
						}

						case RTC::NEW_RTCP::AnyPayloadSpecificFeedbackPacket::MessageType::FIR:
						{
							const auto* const firPacket =
							  static_cast<const RTC::NEW_RTCP::FirPacket*>(payloadSpecificFeedbackPacket);

							FuzzerRtcRtcpFirPacket::Fuzz(firPacket);

							break;
						}

						case RTC::NEW_RTCP::AnyPayloadSpecificFeedbackPacket::MessageType::AFB:
						{
							// An AFB packet whose application is not implemented is an
							// UnknownPayloadSpecificFeedbackPacket.
							if (payloadSpecificFeedbackPacket->HasUnknownMessageType())
							{
								const auto* const unknownPayloadSpecificFeedbackPacket =
								  static_cast<const RTC::NEW_RTCP::UnknownPayloadSpecificFeedbackPacket*>(
								    payloadSpecificFeedbackPacket);

								FuzzerRtcRtcpUnknownPayloadSpecificFeedbackPacket::Fuzz(
								  unknownPayloadSpecificFeedbackPacket);
							}
							else
							{
								const auto* const rembPacket =
								  static_cast<const RTC::NEW_RTCP::RembPacket*>(payloadSpecificFeedbackPacket);

								FuzzerRtcRtcpRembPacket::Fuzz(rembPacket);
							}

							break;
						}

						default:
						{
							const auto* const unknownPayloadSpecificFeedbackPacket =
							  static_cast<const RTC::NEW_RTCP::UnknownPayloadSpecificFeedbackPacket*>(
							    payloadSpecificFeedbackPacket);

							FuzzerRtcRtcpUnknownPayloadSpecificFeedbackPacket::Fuzz(
							  unknownPayloadSpecificFeedbackPacket);
						}
					}

					break;
				}

				case RTC::NEW_RTCP::Packet::PacketType::XR:
				{
					// TODO

					break;
				}

				default:
				{
					const auto* const unknownPacket = static_cast<const RTC::NEW_RTCP::UnknownPacket*>(packet);

					FuzzerRtcRtcpUnknownPacket::Fuzz(unknownPacket);
				}
			}
		}
	};

	RTC::NEW_RTCP::CompoundPacket* const compoundPacket =
	  RTC::NEW_RTCP::CompoundPacket::Parse(data, len);

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
