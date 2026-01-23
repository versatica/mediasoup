#include "common.hpp"
#include "RTC/ICE/StunPacket.hpp"
#include "RTC/ICE/iceCommon.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memset()

using namespace RTC::ICE;

SCENARIO("ICE StunPacket", "[serializable][ice][stunpacket]")
{
	ResetBuffers();

	SECTION("StunPacket::Parse() a STUN request with message integrity and fingerprint succeeds")
	{
		// Binding Request
		// - buffer length: 128 bytes
		// - transaction id: 0x3477773277746F582F41574E
		// - username: "zgpmlltwylgavhv98d27uqmpbmxfcuk6:F5BZ"
		// - priority: 1853824767
		// - ice controlling: 13586427987236599887
		// - use candidate: yes
		// - message integrity: e6fe326fb74ec7c6005f182549e99ef73d4f8983
		// - fingerprint: yes
		//
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

		std::unique_ptr<StunPacket> request{ StunPacket::Parse(buffer, sizeof(buffer)) };

		CHECK_STUN_PACKET(/*packet*/ request.get(),
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
		                  /*iceControlled*/ 0,
		                  /*hasUseCandidate*/ true,
		                  /*hasNomination*/ false,
		                  /*nomination*/ 0,
		                  /*hasSoftware*/ false,
		                  /*software*/ "",
		                  /*hasErrorCode*/ false,
		                  /*errorCode*/ 0,
		                  /*errorReason*/ "",
		                  /*hasMessageIntegrity*/ true,
		                  /*hasFingerprint*/ true);

		/* Serialize it. */

		request->Serialize(SerializeBuffer, sizeof(SerializeBuffer));

		std::memset(buffer, 0x00, sizeof(buffer));

		CHECK_STUN_PACKET(/*packet*/ request.get(),
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
		                  /*iceControlled*/ 0,
		                  /*hasUseCandidate*/ true,
		                  /*hasNomination*/ false,
		                  /*nomination*/ 0,
		                  /*hasSoftware*/ false,
		                  /*software*/ "",
		                  /*hasErrorCode*/ false,
		                  /*errorCode*/ 0,
		                  /*errorReason*/ "",
		                  /*hasMessageIntegrity*/ true,
		                  /*hasFingerprint*/ true);

		/* Clone it. */

		request.reset(request->Clone(CloneBuffer, sizeof(CloneBuffer)));

		std::memset(SerializeBuffer, 0x00, sizeof(SerializeBuffer));

		CHECK_STUN_PACKET(/*packet*/ request.get(),
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
		                  /*iceControlled*/ 0,
		                  /*hasUseCandidate*/ true,
		                  /*hasNomination*/ false,
		                  /*nomination*/ 0,
		                  /*hasSoftware*/ false,
		                  /*software*/ "",
		                  /*hasErrorCode*/ false,
		                  /*errorCode*/ 0,
		                  /*errorReason*/ "",
		                  /*hasMessageIntegrity*/ true,
		                  /*hasFingerprint*/ true);
	}

	SECTION(
	  "StunPacket::Parse() a STUN success response without message integrity or fingerprint succeeds")
	{
		// Binding Success Response
		// - buffer length: 44 bytes
		// - transaction id: 0x0102030405060708090A0B0C
		// - xor-mapped-address: ip 2001:db8:85a3:0:0:8a2e:370:7334, port 1234
		//
		// clang-format off
		uint8_t buffer[] =
		{
			0x01, 0x01, 0x00, 0x18,
			0x21, 0x12, 0xA4, 0x42,
			0x01, 0x02, 0x03, 0x04,
			0x05, 0x06, 0x07, 0x08,
			0x09, 0x0A, 0x0B, 0x0C,
			0x00, 0x20, 0x00, 0x14,
			0x00, 0x02, 0x25, 0xC0,
			0x01, 0x13, 0xA9, 0xFA,
			0x84, 0xA1, 0x03, 0x04,
			0x05, 0x06, 0x8D, 0x26,
			0x0A, 0x7A, 0x78, 0x38
		};
		// clang-format on

		std::unique_ptr<StunPacket> successResponse{ StunPacket::Parse(buffer, sizeof(buffer)) };

		CHECK_STUN_PACKET(/*packet*/ successResponse.get(),
		                  /*buffert*/ buffer,
		                  /*bufferLength*/ sizeof(buffer),
		                  /*length*/ sizeof(buffer),
		                  /*klass*/ StunPacket::Class::SUCCESS_RESPONSE,
		                  /*method*/ StunPacket::Method::BINDING,
		                  /*hasUsername*/ false,
		                  /*username*/ "",
		                  /*hasPriority*/ false,
		                  /*priority*/ 0,
		                  /*hasIceControlling*/ false,
		                  /*iceControlling*/ 0,
		                  /*hasIceControlled*/ false,
		                  /*iceControlled*/ 0,
		                  /*hasUseCandidate*/ false,
		                  /*hasNomination*/ false,
		                  /*nomination*/ 0,
		                  /*hasSoftware*/ false,
		                  /*software*/ "",
		                  /*hasErrorCode*/ false,
		                  /*errorCode*/ 0,
		                  /*errorReason*/ "",
		                  /*hasMessageIntegrity*/ false,
		                  /*hasFingerprint*/ false);

		/* Serialize it. */

		successResponse->Serialize(SerializeBuffer, sizeof(SerializeBuffer));

		std::memset(buffer, 0x00, sizeof(buffer));

		CHECK_STUN_PACKET(/*packet*/ successResponse.get(),
		                  /*buffer*/ SerializeBuffer,
		                  /*bufferLength*/ sizeof(SerializeBuffer),
		                  /*length*/ sizeof(buffer),
		                  /*klass*/ StunPacket::Class::SUCCESS_RESPONSE,
		                  /*method*/ StunPacket::Method::BINDING,
		                  /*hasUsername*/ false,
		                  /*username*/ "",
		                  /*hasPriority*/ false,
		                  /*priority*/ 0,
		                  /*hasIceControlling*/ false,
		                  /*iceControlling*/ 0,
		                  /*hasIceControlled*/ false,
		                  /*iceControlled*/ 0,
		                  /*hasUseCandidate*/ false,
		                  /*hasNomination*/ false,
		                  /*nomination*/ 0,
		                  /*hasSoftware*/ false,
		                  /*software*/ "",
		                  /*hasErrorCode*/ false,
		                  /*errorCode*/ 0,
		                  /*errorReason*/ "",
		                  /*hasMessageIntegrity*/ false,
		                  /*hasFingerprint*/ false);

		/* Clone it. */

		successResponse.reset(successResponse->Clone(CloneBuffer, sizeof(CloneBuffer)));

		std::memset(SerializeBuffer, 0x00, sizeof(SerializeBuffer));

		CHECK_STUN_PACKET(/*packet*/ successResponse.get(),
		                  /*buffer*/ CloneBuffer,
		                  /*bufferLength*/ sizeof(CloneBuffer),
		                  /*length*/ sizeof(buffer),
		                  /*klass*/ StunPacket::Class::SUCCESS_RESPONSE,
		                  /*method*/ StunPacket::Method::BINDING,
		                  /*hasUsername*/ false,
		                  /*username*/ "",
		                  /*hasPriority*/ false,
		                  /*priority*/ 0,
		                  /*hasIceControlling*/ false,
		                  /*iceControlling*/ 0,
		                  /*hasIceControlled*/ false,
		                  /*iceControlled*/ 0,
		                  /*hasUseCandidate*/ false,
		                  /*hasNomination*/ false,
		                  /*nomination*/ 0,
		                  /*hasSoftware*/ false,
		                  /*software*/ "",
		                  /*hasErrorCode*/ false,
		                  /*errorCode*/ 0,
		                  /*errorReason*/ "",
		                  /*hasMessageIntegrity*/ false,
		                  /*hasFingerprint*/ false);
	}

	SECTION("StunPacket::Parse() a STUN error response without message integrity or fingerprint succeeds")
	{
		// Binding Error Response
		// - buffer length: 108 bytes
		// - transaction id: 0x0102030405060708090A0B0C
		// - username: "œæ€å∫∂"
		// - ice controlled: 12345678
		// - software: "mediasoup test"
		// - error code: 456
		// - error reason phrase: "Something failed Ω∑© :)"
		//
		// clang-format off
		uint8_t buffer[] =
		{
			0x01, 0x11, 0x00, 0x58,
	  	0x21, 0x12, 0xA4, 0x42,
	  	0x01, 0x02, 0x03, 0x04,
	  	0x05, 0x06, 0x07, 0x08,
	  	0x09, 0x0A, 0x0B, 0x0C,
	  	0x00, 0x06, 0x00, 0x0F,
	  	0xC5, 0x93, 0xC3, 0xA6,
	  	0xE2, 0x82, 0xAC, 0xC3,
	  	0xA5, 0xE2, 0x88, 0xAB,
	  	0xE2, 0x88, 0x82, 0x00,
	  	0x80, 0x29, 0x00, 0x08,
	  	0x00, 0x00, 0x00, 0x00,
	  	0x00, 0xBC, 0x61, 0x4E,
	  	0x80, 0x22, 0x00, 0x0E,
	  	0x6D, 0x65, 0x64, 0x69,
	  	0x61, 0x73, 0x6F, 0x75,
	  	0x70, 0x20, 0x74, 0x65,
	  	0x73, 0x74, 0x00, 0x00,
	  	0x00, 0x09, 0x00, 0x1F,
	  	0x00, 0x00, 0x04, 0x38,
	  	0x53, 0x6F, 0x6D, 0x65,
	  	0x74, 0x68, 0x69, 0x6E,
	  	0x67, 0x20, 0x66, 0x61,
	  	0x69, 0x6C, 0x65, 0x64,
	  	0x20, 0xCE, 0xA9, 0xE2,
	  	0x88, 0x91, 0xC2, 0xA9,
	  	0x20, 0x3A, 0x29, 0x00
		};
		// clang-format on

		std::unique_ptr<StunPacket> errorResponse{ StunPacket::Parse(buffer, sizeof(buffer)) };

		CHECK_STUN_PACKET(/*packet*/ errorResponse.get(),
		                  /*buffert*/ buffer,
		                  /*bufferLength*/ sizeof(buffer),
		                  /*length*/ sizeof(buffer),
		                  /*klass*/ StunPacket::Class::ERROR_RESPONSE,
		                  /*method*/ StunPacket::Method::BINDING,
		                  /*hasUsername*/ true,
		                  /*username*/ "œæ€å∫∂",
		                  /*hasPriority*/ false,
		                  /*priority*/ 0,
		                  /*hasIceControlling*/ false,
		                  /*iceControlling*/ 0,
		                  /*hasIceControlled*/ true,
		                  /*iceControlled*/ 12345678,
		                  /*hasUseCandidate*/ false,
		                  /*hasNomination*/ false,
		                  /*nomination*/ 0,
		                  /*hasSoftware*/ true,
		                  /*software*/ "mediasoup test",
		                  /*hasErrorCode*/ true,
		                  /*errorCode*/ 456,
		                  /*errorReason*/ "Something failed Ω∑© :)",
		                  /*hasMessageIntegrity*/ false,
		                  /*hasFingerprint*/ false);

		/* Serialize it. */

		errorResponse->Serialize(SerializeBuffer, sizeof(SerializeBuffer));

		std::memset(buffer, 0x00, sizeof(buffer));

		CHECK_STUN_PACKET(/*packet*/ errorResponse.get(),
		                  /*buffer*/ SerializeBuffer,
		                  /*bufferLength*/ sizeof(SerializeBuffer),
		                  /*length*/ sizeof(buffer),
		                  /*klass*/ StunPacket::Class::ERROR_RESPONSE,
		                  /*method*/ StunPacket::Method::BINDING,
		                  /*hasUsername*/ true,
		                  /*username*/ "œæ€å∫∂",
		                  /*hasPriority*/ false,
		                  /*priority*/ 0,
		                  /*hasIceControlling*/ false,
		                  /*iceControlling*/ 0,
		                  /*hasIceControlled*/ true,
		                  /*iceControlled*/ 12345678,
		                  /*hasUseCandidate*/ false,
		                  /*hasNomination*/ false,
		                  /*nomination*/ 0,
		                  /*hasSoftware*/ true,
		                  /*software*/ "mediasoup test",
		                  /*hasErrorCode*/ true,
		                  /*errorCode*/ 456,
		                  /*errorReason*/ "Something failed Ω∑© :)",
		                  /*hasMessageIntegrity*/ false,
		                  /*hasFingerprint*/ false);

		/* Clone it. */

		errorResponse.reset(errorResponse->Clone(CloneBuffer, sizeof(CloneBuffer)));

		std::memset(SerializeBuffer, 0x00, sizeof(SerializeBuffer));

		CHECK_STUN_PACKET(/*packet*/ errorResponse.get(),
		                  /*buffer*/ CloneBuffer,
		                  /*bufferLength*/ sizeof(CloneBuffer),
		                  /*length*/ sizeof(buffer),
		                  /*klass*/ StunPacket::Class::ERROR_RESPONSE,
		                  /*method*/ StunPacket::Method::BINDING,
		                  /*hasUsername*/ true,
		                  /*username*/ "œæ€å∫∂",
		                  /*hasPriority*/ false,
		                  /*priority*/ 0,
		                  /*hasIceControlling*/ false,
		                  /*iceControlling*/ 0,
		                  /*hasIceControlled*/ true,
		                  /*iceControlled*/ 12345678,
		                  /*hasUseCandidate*/ false,
		                  /*hasNomination*/ false,
		                  /*nomination*/ 0,
		                  /*hasSoftware*/ true,
		                  /*software*/ "mediasoup test",
		                  /*hasErrorCode*/ true,
		                  /*errorCode*/ 456,
		                  /*errorReason*/ "Something failed Ω∑© :)",
		                  /*hasMessageIntegrity*/ false,
		                  /*hasFingerprint*/ false);
	}

	SECTION("StunPacket::Factory() succeeds")
	{
		std::unique_ptr<StunPacket> request{ StunPacket::Factory(
			FactoryBuffer, sizeof(FactoryBuffer), StunPacket::Class::REQUEST, StunPacket::Method::BINDING) };

		// TODO
		request->Dump();

		CHECK_STUN_PACKET(/*packet*/ request.get(),
		                  /*buffer*/ FactoryBuffer,
		                  /*bufferLength*/ sizeof(FactoryBuffer),
		                  /*length*/ StunPacket::FixedHeaderLength,
		                  /*klass*/ StunPacket::Class::REQUEST,
		                  /*method*/ StunPacket::Method::BINDING,
		                  /*hasUsername*/ false,
		                  /*username*/ "",
		                  /*hasPriority*/ false,
		                  /*priority*/ 0,
		                  /*hasIceControlling*/ false,
		                  /*iceControlling*/ 0,
		                  /*hasIceControlled*/ false,
		                  /*iceControlled*/ 0,
		                  /*hasUseCandidate*/ false,
		                  /*hasNomination*/ false,
		                  /*nomination*/ 0,
		                  /*hasSoftware*/ false,
		                  /*software*/ "",
		                  /*hasErrorCode*/ false,
		                  /*errorCode*/ 0,
		                  /*errorReason*/ "",
		                  /*hasMessageIntegrity*/ false,
		                  /*hasFingerprint*/ false);
	}
}
