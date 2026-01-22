#include "common.hpp"
#include "RTC/ICE/StunPacket.hpp"
#include "RTC/ICE/iceCommon.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memset()

using namespace RTC::ICE;

SCENARIO("ICE StunPacket", "[serializable][ice][stunpacket]")
{
	ResetBuffers();

	SECTION("StunPacket::Parse() succeeds")
	{
		// clang-format off
		uint8_t buffer[] =
		{
			0x00, 0x01, 0x00, 0x6C,
			0x21, 0x12, 0xA4, 0x42,
			0x34, 0x77, 0x77, 0x32,
			0x77, 0x74, 0x6F, 0x58,
			0x2F, 0x41, 0x57, 0x4E,
			0x00, 0x06, 0x00, 0x25,
			0x7A, 0x67, 0x70, 0x6D,
			0x6C, 0x6C, 0x74, 0x77,
			0x79, 0x6C, 0x67, 0x61,
			0x76, 0x68, 0x76, 0x39,
			0x38, 0x64, 0x32, 0x37,
			0x75, 0x71, 0x6D, 0x70,
			0x62, 0x6D, 0x78, 0x66,
			0x63, 0x75, 0x6B, 0x36,
			0x3A, 0x46, 0x35, 0x42,
			0x5A, 0x00, 0x00, 0x00,
			0xC0, 0x57, 0x00, 0x04,
			0x00, 0x01, 0x00, 0x0A,
			0x80, 0x2A, 0x00, 0x08,
			0xBC, 0x8C, 0xB0, 0x45,
			0x39, 0xA9, 0xB8, 0x4F,
			0x00, 0x25, 0x00, 0x00,
			0x00, 0x24, 0x00, 0x04,
			0x6E, 0x7F, 0x1E, 0xFF,
			0x00, 0x08, 0x00, 0x14,
			0xE6, 0xFE, 0x32, 0x6F,
			0xB7, 0x4E, 0xC7, 0xC6,
			0x00, 0x5F, 0x18, 0x25,
			0x49, 0xE9, 0x9E, 0xF7,
			0x3D, 0x4F, 0x89, 0x83,
			0x80, 0x28, 0x00, 0x04,
			0x93, 0x39, 0xAF, 0x64
		};
		// clang-format on

		std::unique_ptr<StunPacket> packet{ StunPacket::Parse(buffer, sizeof(buffer)) };

		CHECK_ICE_PACKET(/*packet*/ packet.get(),
		                 /*buffert*/ buffer,
		                 /*bufferLength*/ sizeof(buffer),
		                 /*length*/ sizeof(buffer),
		                 /*klass*/ StunPacket::Class::REQUEST,
		                 /*method*/ StunPacket::Method::BINDING,
		                 /*hasUsername*/ true,
		                 /*username*/ "zgpmlltwylgavhv98d27uqmpbmxfcuk6:F5BZ",
		                 /*hasPriority*/ true,
		                 /*priority*/ 1853824767,
		                 /*hasIceControlling*/ true,
		                 /*iceControlling*/ 13586427987236599887u,
		                 /*hasIceControlled*/ false,
		                 /*iceControlledg*/ 0,
		                 /*hasUseCandidate*/ true,
		                 /*hasNomination*/ false,
		                 /*nomination*/ 0,
		                 /*hasSoftware*/ false,
		                 /*software*/ "",
		                 /*hasErrorCode*/ false,
		                 /*errorCode*/ 0,
		                 /*hasMessageIntegrity*/ true,
		                 /*hasFingerprint*/ true);

		/* Serialize it. */

		packet->Serialize(SerializeBuffer, sizeof(SerializeBuffer));

		std::memset(buffer, 0x00, sizeof(buffer));

		CHECK_ICE_PACKET(/*packet*/ packet.get(),
		                 /*buffer*/ SerializeBuffer,
		                 /*bufferLength*/ sizeof(SerializeBuffer),
		                 /*length*/ sizeof(buffer),
		                 /*klass*/ StunPacket::Class::REQUEST,
		                 /*method*/ StunPacket::Method::BINDING,
		                 /*hasUsername*/ true,
		                 /*username*/ "zgpmlltwylgavhv98d27uqmpbmxfcuk6:F5BZ",
		                 /*hasPriority*/ true,
		                 /*priority*/ 1853824767,
		                 /*hasIceControlling*/ true,
		                 /*iceControlling*/ 13586427987236599887u,
		                 /*hasIceControlled*/ false,
		                 /*iceControlledg*/ 0,
		                 /*hasUseCandidate*/ true,
		                 /*hasNomination*/ false,
		                 /*nomination*/ 0,
		                 /*hasSoftware*/ false,
		                 /*software*/ "",
		                 /*hasErrorCode*/ false,
		                 /*errorCode*/ 0,
		                 /*hasMessageIntegrity*/ true,
		                 /*hasFingerprint*/ true);

		/* Clone it. */

		packet.reset(packet->Clone(CloneBuffer, sizeof(CloneBuffer)));

		std::memset(SerializeBuffer, 0x00, sizeof(SerializeBuffer));

		CHECK_ICE_PACKET(/*packet*/ packet.get(),
		                 /*buffer*/ CloneBuffer,
		                 /*bufferLength*/ sizeof(CloneBuffer),
		                 /*length*/ sizeof(buffer),
		                 /*klass*/ StunPacket::Class::REQUEST,
		                 /*method*/ StunPacket::Method::BINDING,
		                 /*hasUsername*/ true,
		                 /*username*/ "zgpmlltwylgavhv98d27uqmpbmxfcuk6:F5BZ",
		                 /*hasPriority*/ true,
		                 /*priority*/ 1853824767,
		                 /*hasIceControlling*/ true,
		                 /*iceControlling*/ 13586427987236599887u,
		                 /*hasIceControlled*/ false,
		                 /*iceControlledg*/ 0,
		                 /*hasUseCandidate*/ true,
		                 /*hasNomination*/ false,
		                 /*nomination*/ 0,
		                 /*hasSoftware*/ false,
		                 /*software*/ "",
		                 /*hasErrorCode*/ false,
		                 /*errorCode*/ 0,
		                 /*hasMessageIntegrity*/ true,
		                 /*hasFingerprint*/ true);
	}
}
