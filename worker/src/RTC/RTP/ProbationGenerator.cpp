#define MS_CLASS "ProbationGenerator"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/RTP/ProbationGenerator.hpp"
#include "Logger.hpp"
#include "Utils.hpp"
#include "RTC/RtpDictionaries.hpp"
#include "RTC/RtpHeaderExtensionIds.hpp"
#include <cstring> // std::memcpy(), std::memset()
#include <vector>

namespace RTC
{
	namespace RTP
	{
		/* Static. */

		thread_local static uint8_t ProbationPacketBuffer[ProbationGenerator::ProbationPacketMaxLength];
		static constexpr size_t ProbationPacketExtensionsBufferLength{ 200 };
		thread_local static uint8_t ProbationPacketExtensionsBuffer[ProbationPacketExtensionsBufferLength];
		// 8 bytes, same as RTC::Consts::MidRtpExtensionMaxLength.
		static const std::string MidValue{ "probator" };

		/* Instance methods. */

		ProbationGenerator::ProbationGenerator()
		{
			MS_TRACE();

			// Trick to only fill the padding with zeroes once.
			static bool mustInitializePayload{ true };

			if (mustInitializePayload)
			{
				std::memset(ProbationPacketBuffer, 0x00, sizeof(ProbationPacketBuffer));

				mustInitializePayload = false;
			}

			// Create the probation RTP Packet.
			this->probationPacket =
			  RTC::RTP::Packet::Factory(ProbationPacketBuffer, sizeof(ProbationPacketBuffer));

			// Sex fixed codec payload type.
			this->probationPacket->SetPayloadType(ProbationGenerator::PayloadType);

			// Set fixed SSRC.
			this->probationPacket->SetSsrc(ProbationGenerator::Ssrc);

			// Set random initial RTP seq number.
			this->probationPacket->SetSequenceNumber(
			  static_cast<uint16_t>(Utils::Crypto::GetRandomUInt(0, 65535)));

			// Set random initial RTP timestamp.
			this->probationPacket->SetTimestamp(Utils::Crypto::GetRandomUInt(0, 4294967295));

			// Add BWE related RTP header extensions.
			std::vector<RTC::RTP::Packet::Extension> extensions;
			uint8_t extenLen;
			uint8_t* bufferPtr{ ProbationPacketExtensionsBuffer };

			// Add urn:ietf:params:rtp-hdrext:sdes:mid.
			{
				extenLen = MidValue.size();

				extensions.emplace_back(
				  /*type*/ RTC::RtpHeaderExtensionUri::Type::MID,
				  /*id*/ static_cast<uint8_t>(RTC::RtpHeaderExtensionUri::Type::MID),
				  /*len*/ extenLen,
				  /*value*/ bufferPtr);

				std::memcpy(bufferPtr, MidValue.c_str(), extenLen);

				bufferPtr += extenLen;
			}

			// Add http://www.webrtc.org/experiments/rtp-hdrext/abs-send-time.
			// NOTE: Just the corresponding id and space for its value.
			{
				extenLen = 3u;

				extensions.emplace_back(
				  /*type*/ RTC::RtpHeaderExtensionUri::Type::ABS_SEND_TIME,
				  /*id*/ static_cast<uint8_t>(RTC::RtpHeaderExtensionUri::Type::ABS_SEND_TIME),
				  /*len*/ extenLen,
				  /*value*/ bufferPtr);

				bufferPtr += extenLen;
			}

			// Add http://www.ietf.org/id/draft-holmer-rmcat-transport-wide-cc-extensions-01.
			// NOTE: Just the corresponding id and space for its value.
			{
				extenLen = 2u;

				extensions.emplace_back(
				  /*type*/ RTC::RtpHeaderExtensionUri::Type::TRANSPORT_WIDE_CC_01,
				  /*id*/ static_cast<uint8_t>(RTC::RtpHeaderExtensionUri::Type::TRANSPORT_WIDE_CC_01),
				  /*len*/ extenLen,
				  /*value*/ bufferPtr);

				// Not needed since this is the latest added extension.
				// bufferPtr += extenLen;
			}

			// Set the extensions into the Packet using One-Byte format.
			this->probationPacket->SetExtensions(RTC::RTP::Packet::ExtensionsType::OneByte, extensions);

			this->probationPacketMinLength = this->probationPacket->GetLength();
		}

		ProbationGenerator::~ProbationGenerator()
		{
			MS_TRACE();

			// Delete the probation RTP Packet.
			delete this->probationPacket;
		}

		/**
		 * This method maybe called with desired `len` higher than typical RTP
		 * packet mas length. That's ok since the caller will iterate and call
		 * this method again until it satisfies the total desired `len`.
		 */
		RTC::RTP::Packet* ProbationGenerator::GetNextPacket(size_t len)
		{
			MS_TRACE();

			// Pad given length to 4 bytes.
			len = Utils::Byte::PadTo4Bytes(len);

			// Make the Packet length fit into our available limits.
			if (len > ProbationGenerator::ProbationPacketMaxLength)
			{
				len = ProbationGenerator::ProbationPacketMaxLength;
			}
			else if (len < this->probationPacketMinLength)
			{
				len = this->probationPacketMinLength;
			}

			// Just send up to StepNumPackets per step.
			// Increase RTP seq number and timestamp.
			const auto seq             = this->probationPacket->GetSequenceNumber() + 1;
			const auto timestamp       = this->probationPacket->GetTimestamp() + 20;
			const size_t payloadLength = len - this->probationPacketMinLength;

			this->probationPacket->SetSequenceNumber(seq);
			this->probationPacket->SetTimestamp(timestamp);

			// Set payload length.
			this->probationPacket->SetPayloadLength(payloadLength);

			return this->probationPacket;
		}
	} // namespace RTP
} // namespace RTC
