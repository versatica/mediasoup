#define MS_CLASS "RTC::RtpProbationGenerator"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/RtpProbationGenerator.hpp"
#include "DepLibUV.hpp"
#include "Logger.hpp"
#include "Utils.hpp"
#include "RTC/RtpDictionaries.hpp"
#include <cstring> // std::memcpy()
#include <vector>

namespace RTC
{
	/* Static. */

	// clang-format off
	// Probation RTP header.
	static uint8_t ProbationPacketHeader[] =
	{
		0b10010000, 0b01111111, 0, 0, // PayloadType: 127, Sequence Number: 0
		0, 0, 0, 0,                   // Timestamp: 0
		0, 0, 0, 0,                   // SSRC: 0
		0xBE, 0xDE, 0, 2,             // Header Extension (One-Byte Extensions)
		0, 0, 0, 0,                   // Space for abs-send-time extension.
		0, 0, 0, 0                    // Space for transport-wide-cc-01 extension.
	};
	// clang-format on

	static constexpr size_t MaxProbationPacketSize{ 1400u };

	/* Instance methods. */

	RtpProbationGenerator::RtpProbationGenerator()
	{
		MS_TRACE();

		// Allocate the probation RTP packet buffer.
		this->probationPacketBuffer = new uint8_t[MaxProbationPacketSize];

		// Copy the generic probation RTP packet header into the buffer.
		std::memcpy(this->probationPacketBuffer, ProbationPacketHeader, sizeof(ProbationPacketHeader));

		// Create the probation RTP packet.
		this->probationPacket =
		  RTC::RtpPacket::Parse(this->probationPacketBuffer, MaxProbationPacketSize);

		// Set fixed SSRC.
		this->probationPacket->SetSsrc(RTC::RtpProbationSsrc);

		// Set random initial RTP seq number and timestamp.
		this->probationPacket->SetSequenceNumber(
		  static_cast<uint16_t>(Utils::Crypto::GetRandomUInt(0, 65535)));
		this->probationPacket->SetTimestamp(Utils::Crypto::GetRandomUInt(0, 4294967295));

		// Add BWE related RTP header extensions.
		static uint8_t buffer[4096];

		std::vector<RTC::RtpPacket::GenericExtension> extensions;
		uint8_t extenLen;
		uint8_t* bufferPtr{ buffer };

		// Add http://www.webrtc.org/experiments/rtp-hdrext/abs-send-time.
		// NOTE: Just the corresponding id and space for its value.
		{
			extenLen = 3u;

			extensions.emplace_back(
			  static_cast<uint8_t>(RTC::RtpHeaderExtensionUri::Type::ABS_SEND_TIME), extenLen, bufferPtr);

			bufferPtr += extenLen;
		}

		// Add http://www.ietf.org/id/draft-holmer-rmcat-transport-wide-cc-extensions-01.
		// NOTE: Just the corresponding id and space for its value.
		{
			extenLen = 2u;

			extensions.emplace_back(
			  static_cast<uint8_t>(RTC::RtpHeaderExtensionUri::Type::TRANSPORT_WIDE_CC_01),
			  extenLen,
			  bufferPtr);

			// Not needed since this is the latest added extension.
			// bufferPtr += extenLen;
		}

		// Set the extensions into the packet using One-Byte format.
		this->probationPacket->SetExtensions(1, extensions);

		// Set our abs-send-time extension id.
		this->probationPacket->SetAbsSendTimeExtensionId(
		  static_cast<uint8_t>(RTC::RtpHeaderExtensionUri::Type::ABS_SEND_TIME));

		// Set our transport-wide-cc-01 extension id.
		this->probationPacket->SetTransportWideCc01ExtensionId(
		  static_cast<uint8_t>(RTC::RtpHeaderExtensionUri::Type::TRANSPORT_WIDE_CC_01));
	}

	RtpProbationGenerator::~RtpProbationGenerator()
	{
		MS_TRACE();

		// Delete the probation packet buffer.
		delete[] this->probationPacketBuffer;

		// Delete the probation RTP packet.
		delete this->probationPacket;
	}

	RTC::RtpPacket* RtpProbationGenerator::GetNextPacket(size_t size)
	{
		MS_TRACE();

		// Make the packet length fit into our available limits.
		if (size > MaxProbationPacketSize)
			size = MaxProbationPacketSize;
		else if (size < sizeof(ProbationPacketHeader))
			size = sizeof(ProbationPacketHeader);

		// Just send up to StepNumPackets per step.
		// Increase RTP seq number and timestamp.
		auto seq       = this->probationPacket->GetSequenceNumber();
		auto timestamp = this->probationPacket->GetTimestamp();

		++seq;
		timestamp += 20u;

		this->probationPacket->SetSequenceNumber(seq);
		this->probationPacket->SetTimestamp(timestamp);

		// Set probation packet payload size.
		this->probationPacket->SetPayloadLength(size - sizeof(ProbationPacketHeader));

		return this->probationPacket;
	}
} // namespace RTC
