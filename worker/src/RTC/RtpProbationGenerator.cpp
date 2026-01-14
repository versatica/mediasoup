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

	// clang-format off
	// Probation RTP Fixed Header.
	// Caution: This must have an exact size for the RTP extensions to be added
	// and must align extensions to 4 bytes.
	static const uint8_t ProbationPacketFixedHeader[] =
	{
		0b10010000, 0b01111111, 0, 0, // PayloadType: 127, Sequence Number: 0
		0, 0, 0, 0,                   // Timestamp: 0
		0, 0, 0, 0,                   // SSRC: 0
		0xBE, 0xDE, 0, 4,             // Header Extension (One-Byte Extensions)
		0, 0, 0, 0,                   // Space for MID extension
		0, 0, 0, 0,
		0,
		0, 0, 0, 0,                   // Space for abs-send-time extension
		0, 0, 0                       // Space for transport-wide-cc-01 extension
	};
	// clang-format on

	static constexpr size_t ProbationPacketFixedHeaderSize{ 32 };
	static constexpr size_t MaxProbationPacketSize{ 1400 };
	static const std::string MidValue{ "probator" }; // 8 bytes, same as RTC::MidMaxLength.

	/* Instance methods. */

	RtpProbationGenerator::RtpProbationGenerator()
	{
		MS_TRACE();

		// Allocate the probation RTP packet buffer.
		this->probationPacketBuffer = new uint8_t[MaxProbationPacketSize];

		// Copy the generic probation RTP Packet Fixed Header into the buffer.
		std::memcpy(this->probationPacketBuffer, ProbationPacketFixedHeader, ProbationPacketFixedHeaderSize);

		// Create the probation RTP Packet.
		// NOTE: Let's use Packet::ParseFromApplicationBuffer() since we own the
		// buffer.
		this->probationPacket =
		  RTC::RTP::Packet::ParseFromApplicationBuffer(this->probationPacketBuffer, MaxProbationPacketSize);

		// Sex fixed codec payload type.
		this->probationPacket->SetPayloadType(RTC::RtpProbationCodecPayloadType);

		// Set fixed SSRC.
		this->probationPacket->SetSsrc(RTC::RtpProbationSsrc);

		// Set random initial RTP seq number and timestamp.
		this->probationPacket->SetSequenceNumber(
		  static_cast<uint16_t>(Utils::Crypto::GetRandomUInt(0, 65535)));
		this->probationPacket->SetTimestamp(Utils::Crypto::GetRandomUInt(0, 4294967295));

		// Add BWE related RTP header extensions.
		thread_local static uint8_t buffer[4096];

		std::vector<RTC::RTP::Packet::Extension> extensions;
		uint8_t extenLen;
		uint8_t* bufferPtr{ buffer };

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
	}

	RtpProbationGenerator::~RtpProbationGenerator()
	{
		MS_TRACE();

		// Delete the probation Packet buffer.
		delete[] this->probationPacketBuffer;

		// Delete the probation RTP Packet.
		delete this->probationPacket;
	}

	RTC::RTP::Packet* RtpProbationGenerator::GetNextPacket(size_t len)
	{
		MS_TRACE();

		// Make the Packet length fit into our available limits.
		if (len > MaxProbationPacketSize)
		{
			len = MaxProbationPacketSize;
		}
		else if (len < ProbationPacketFixedHeaderSize)
		{
			len = ProbationPacketFixedHeaderSize;
		}

		// Just send up to StepNumPackets per step.
		// Increase RTP seq number and timestamp.
		auto seq       = this->probationPacket->GetSequenceNumber();
		auto timestamp = this->probationPacket->GetTimestamp();

		++seq;
		timestamp += 20u;

		this->probationPacket->SetSequenceNumber(seq);
		this->probationPacket->SetTimestamp(timestamp);

		// Set padding.
		this->probationPacket->SetPaddingLength(len - ProbationPacketFixedHeaderSize);

		MS_DUMP("TODO: (REMOVE) probation packet len should be %zu bytes", len);
		this->probationPacket->Dump();

		return this->probationPacket;
	}
} // namespace RTC
