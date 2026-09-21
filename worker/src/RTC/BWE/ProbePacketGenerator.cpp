#define MS_CLASS "RTC::BWE::ProbePacketGenerator"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/BWE/ProbePacketGenerator.hpp"
#include "Logger.hpp"
#include "RTC/Consts.hpp"
#include "RTC/RtpDictionaries.hpp"
#include "Utils.hpp"
#include <cstring> // std::memcpy()
#include <vector>

namespace RTC
{
	namespace BWE
	{
		/* Static. */

		static constexpr size_t ExtensionsBufferLength{ 200 };
		alignas(4) static thread_local uint8_t ExtensionsBuffer[ExtensionsBufferLength];
		// How much the RTP timestamp moves from one packet to the next.
		// NOTE: It stands for no lapse of media, since there is none to play out.
		// It just has to move so that the packets don't all claim to belong to the
		// same instant.
		static constexpr uint32_t TimestampStep{ 20 };

		/* Instance methods. */

		ProbePacketGenerator::ProbePacketGenerator(Listener* listener) : listener(listener)
		{
			MS_TRACE();

			this->packet.reset(
			  RTC::RTP::Packet::Factory(this->packetBuffer.data(), this->packetBuffer.size()));

			this->packet->SetPayloadType(RTC::Consts::BweProbeRtpPayloadType);
			this->packet->SetSsrc(RTC::Consts::BweProbeRtpSsrc);
			this->packet->SetSequenceNumber(Utils::Crypto::GetRandomUInt<uint16_t>(0, 65535));
			this->packet->SetTimestamp(Utils::Crypto::GetRandomUInt<uint32_t>(0, 4294967295));

			// The extensions that make the packets reportable, so that the receiver
			// tells when each of them arrived.
			std::vector<RTC::RTP::Packet::Extension> extensions;
			uint8_t extenLen;
			uint8_t* bufferPtr{ ExtensionsBuffer };

			// urn:ietf:params:rtp-hdrext:sdes:mid.
			{
				extenLen = RTC::Consts::BweProbeRtpMid.size();

				extensions.emplace_back(
				  /*type*/ RTC::RtpHeaderExtensionUri::Type::MID,
				  /*id*/ static_cast<uint8_t>(RTC::RtpHeaderExtensionUri::Type::MID),
				  /*len*/ extenLen,
				  /*value*/ bufferPtr);

				std::memcpy(bufferPtr, RTC::Consts::BweProbeRtpMid.data(), extenLen);

				bufferPtr += extenLen;
			}

			// http://www.webrtc.org/experiments/rtp-hdrext/abs-send-time.
			//
			// NOTE: Just the id and the room for its value, which whoever sends the
			// packet fills in.
			{
				extenLen = 3;

				extensions.emplace_back(
				  /*type*/ RTC::RtpHeaderExtensionUri::Type::ABS_SEND_TIME,
				  /*id*/ static_cast<uint8_t>(RTC::RtpHeaderExtensionUri::Type::ABS_SEND_TIME),
				  /*len*/ extenLen,
				  /*value*/ bufferPtr);

				bufferPtr += extenLen;
			}

			// http://www.ietf.org/id/draft-holmer-rmcat-transport-wide-cc-extensions-01.
			//
			// NOTE: The same, and it's the one the arrival times come back in.
			{
				extenLen = 2;

				extensions.emplace_back(
				  /*type*/ RTC::RtpHeaderExtensionUri::Type::TRANSPORT_WIDE_CC_01,
				  /*id*/ static_cast<uint8_t>(RTC::RtpHeaderExtensionUri::Type::TRANSPORT_WIDE_CC_01),
				  /*len*/ extenLen,
				  /*value*/ bufferPtr);
			}

			this->packet->SetExtensions(RTC::RTP::Packet::ExtensionsType::OneByte, extensions);

			this->minPacketLength = this->packet->GetLength();
		}

		void ProbePacketGenerator::GeneratePackets(size_t size)
		{
			MS_TRACE();

			if (size > ProbePacketGenerator::MaxGeneratePacketsSize)
			{
				MS_WARN_TAG(
				  bwe,
				  "burst of more bytes than any real one could be, sending fewer [asked:%zu, sending:%zu]",
				  size,
				  ProbePacketGenerator::MaxGeneratePacketsSize);

				size = ProbePacketGenerator::MaxGeneratePacketsSize;
			}

			size_t remaining{ size };

			while (remaining > 0)
			{
				// A packet cannot be shorter than its own header, so the last one of a
				// burst overshoots rather than falling short.
				size_t length = std::clamp(
				  Utils::Byte::PadTo4Bytes(remaining),
				  this->minPacketLength,
				  ProbePacketGenerator::MaxPacketLength);

				this->packet->SetSequenceNumber(this->packet->GetSequenceNumber() + 1);
				this->packet->SetTimestamp(this->packet->GetTimestamp() + TimestampStep);
				this->packet->SetPayloadLength(length - this->minPacketLength);

				// NOTE: The length is read back rather than assumed, since padding the
				// payload to 4 bytes may have made the packet longer than asked for.
				length = this->packet->GetLength();

				remaining -= std::min(remaining, length);

				if (!this->listener->OnProbePacketGeneratorSendRtpPacket(this, this->packet.get()))
				{
					MS_DEBUG_DEV("burst given up on, %zu bytes were left", remaining);

					break;
				}
			}
		}
	} // namespace BWE
} // namespace RTC
