#define MS_CLASS "RTC::RtpProbationGenerator"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/RtpProbationGenerator.hpp"
#include "Logger.hpp"
#include "Utils.hpp"
#include "RTC/RtpDictionaries.hpp"
#include "RTC/RtpHeaderExtensionIds.hpp"
#include <cstring> // std::memcpy()
#include <vector>

namespace RTC
{
	/* Static. */

	static constexpr size_t ProbationPacketBufferSize{ 1400 };
	thread_local static uint8_t ProbationPacketBuffer[ProbationPacketBufferSize];
	static constexpr size_t ProbationPacketExtensionsBufferSize{ 200 };
	thread_local static uint8_t ProbationPacketExtensionsBuffer[ProbationPacketExtensionsBufferSize];
	// 8 bytes, same as RTC::Consts::MidRtpExtensionMaxLength.
	static const std::string MidValue{ "probator" };

	/* Instance methods. */

	RtpProbationGenerator::RtpProbationGenerator()
	{
		MS_TRACE();

		// Create the probation RTP Packet.
		this->probationPacket =
		  RTC::RTP::Packet::Factory(ProbationPacketBuffer, sizeof(ProbationPacketBufferSize));

		// Sex fixed codec payload type.
		this->probationPacket->SetPayloadType(RTC::RtpProbationGenerator::PayloadType);

		// Set fixed SSRC.
		this->probationPacket->SetSsrc(RTC::RtpProbationGenerator::Ssrc);

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

	RtpProbationGenerator::~RtpProbationGenerator()
	{
		MS_TRACE();

		// Delete the probation RTP Packet.
		delete this->probationPacket;
	}

	RTC::RTP::Packet* RtpProbationGenerator::GetNextPacket(size_t len)
	{
		MS_TRACE();

		// Make the Packet length fit into our available limits.
		if (len > ProbationPacketBufferSize)
		{
			MS_WARN_TAG(
			  rtp, "cannot generate a probation packet bigger than %zu bytes", ProbationPacketBufferSize);

			len = ProbationPacketBufferSize;
		}
		else if (len < this->probationPacketMinLength)
		{
			MS_WARN_TAG(
			  rtp,
			  "cannot generate a probation packet smaller than %zu bytes",
			  this->probationPacketMinLength);

			len = this->probationPacketMinLength;
		}

		// Just send up to StepNumPackets per step.
		// Increase RTP seq number and timestamp.
		const auto seq       = this->probationPacket->GetSequenceNumber() + 1;
		const auto timestamp = this->probationPacket->GetTimestamp() + 20;

		this->probationPacket->SetSequenceNumber(seq);
		this->probationPacket->SetTimestamp(timestamp);

		// Set padding.
		this->probationPacket->SetPaddingLength(len - this->probationPacketMinLength);

		MS_DUMP("TODO: (REMOVE) probation packet len should be %zu bytes", len);
		this->probationPacket->Dump();

		return this->probationPacket;
	}
} // namespace RTC
