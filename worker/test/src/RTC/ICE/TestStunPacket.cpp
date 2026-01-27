#include "common.hpp"
#include "Utils.hpp"
#include "testHelpers.hpp"
#include "RTC/ICE/StunPacket.hpp"
#include "RTC/ICE/iceCommon.hpp"
#include <uv.h>
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memset()
#include <string>

using namespace RTC::ICE;

SCENARIO("ICE StunPacket", "[serializable][ice][stunpacket]")
{
	ResetBuffers();

	SECTION("StunPacket::Parse() a STUN request with message integrity and fingerprint succeeds")
	{
		// Binding Request
		// - buffer length: 128 bytes
		// - transaction id: 0x4A31775941764E5470644B33
		// - username: "78tal5pc6dkyv1rpg56vuay5je13cewm:s3Jg"
		// - priority: 1853693695
		// - ice controlling: 15897499370457501716
		// - use candidate: yes
		// - message integrity: f39a23b3a6054e75b39df2177100182da76834f8
		// - fingerprint: 1782005644
		//
		// clang-format off
		uint8_t buffer[] =
		{
			0x00, 0x01, 0x00, 0x6C,
			0x21, 0x12, 0xA4, 0x42,
			0x4A, 0x31, 0x77, 0x59,
			0x41, 0x76, 0x4E, 0x54,
			0x70, 0x64, 0x4B, 0x33,
			0x00, 0x06, 0x00, 0x25,
			0x37, 0x38, 0x74, 0x61,
			0x6C, 0x35, 0x70, 0x63,
			0x36, 0x64, 0x6B, 0x79,
			0x76, 0x31, 0x72, 0x70,
			0x67, 0x35, 0x36, 0x76,
			0x75, 0x61, 0x79, 0x35,
			0x6A, 0x65, 0x31, 0x33,
			0x63, 0x65, 0x77, 0x6D,
			0x3A, 0x73, 0x33, 0x4A,
			0x67, 0x00, 0x00, 0x00,
			0xC0, 0x57, 0x00, 0x04,
			0x00, 0x03, 0x00, 0x0A,
			0x80, 0x2A, 0x00, 0x08,
			0xDC, 0x9F, 0x43, 0x72,
			0xE9, 0x1D, 0x90, 0x14,
			0x00, 0x25, 0x00, 0x00,
			0x00, 0x24, 0x00, 0x04,
			0x6E, 0x7D, 0x1E, 0xFF,
			0x00, 0x08, 0x00, 0x14,
			0xF3, 0x9A, 0x23, 0xB3,
			0xA6, 0x05, 0x4E, 0x75,
			0xB3, 0x9D, 0xF2, 0x17,
			0x71, 0x00, 0x18, 0x2D,
			0xA7, 0x68, 0x34, 0xF8,
			0x80, 0x28, 0x00, 0x04,
			0x6A, 0x37, 0x3F, 0x8C
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
		                  /*username*/ "78tal5pc6dkyv1rpg56vuay5je13cewm:s3Jg",
		                  /*hasPriority*/ true,
		                  /*priority*/ 1853693695,
		                  /*hasIceControlling*/ true,
		                  /*iceControlling*/ 15897499370457501716u,
		                  /*hasIceControlled*/ false,
		                  /*iceControlled*/ 0,
		                  /*hasUseCandidate*/ true,
		                  /*hasNomination*/ false,
		                  /*nomination*/ 0,
		                  /*hasSoftware*/ false,
		                  /*software*/ "",
		                  /*hasXorMappedAddress*/ false,
		                  /*hasErrorCode*/ false,
		                  /*errorCode*/ 0,
		                  /*errorReasonPhrase*/ "",
		                  /*hasMessageIntegrity*/ true,
		                  /*hasFingerprint*/ true);

		const std::string usernameFragment1 = "78tal5pc6dkyv1rpg56vuay5je13cewm";
		const std::string password          = "1ezk7fni4jeo5bt7ibcdk4wjl8712suw";

		REQUIRE(
		  request->CheckAuthentication(usernameFragment1, password) ==
		  StunPacket::AuthenticationResult::OK);

		// Trying to modify a STUN Packet once protected must throw.
		REQUIRE_THROWS_AS(request->Protect("qweqwe"), MediaSoupError);
		REQUIRE_THROWS_AS(request->Protect(), MediaSoupError);

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
		                  /*username*/ "78tal5pc6dkyv1rpg56vuay5je13cewm:s3Jg",
		                  /*hasPriority*/ true,
		                  /*priority*/ 1853693695,
		                  /*hasIceControlling*/ true,
		                  /*iceControlling*/ 15897499370457501716u,
		                  /*hasIceControlled*/ false,
		                  /*iceControlled*/ 0,
		                  /*hasUseCandidate*/ true,
		                  /*hasNomination*/ false,
		                  /*nomination*/ 0,
		                  /*hasSoftware*/ false,
		                  /*software*/ "",
		                  /*hasXorMappedAddress*/ false,
		                  /*hasErrorCode*/ false,
		                  /*errorCode*/ 0,
		                  /*errorReasonPhrase*/ "",
		                  /*hasMessageIntegrity*/ true,
		                  /*hasFingerprint*/ true);

		REQUIRE(
		  request->CheckAuthentication(usernameFragment1, password) ==
		  StunPacket::AuthenticationResult::OK);

		// Trying to modify a STUN Packet once protected must throw.
		REQUIRE_THROWS_AS(request->Protect("qweqwe"), MediaSoupError);
		REQUIRE_THROWS_AS(request->Protect(), MediaSoupError);

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
		                  /*username*/ "78tal5pc6dkyv1rpg56vuay5je13cewm:s3Jg",
		                  /*hasPriority*/ true,
		                  /*priority*/ 1853693695,
		                  /*hasIceControlling*/ true,
		                  /*iceControlling*/ 15897499370457501716u,
		                  /*hasIceControlled*/ false,
		                  /*iceControlled*/ 0,
		                  /*hasUseCandidate*/ true,
		                  /*hasNomination*/ false,
		                  /*nomination*/ 0,
		                  /*hasSoftware*/ false,
		                  /*software*/ "",
		                  /*hasXorMappedAddress*/ false,
		                  /*hasErrorCode*/ false,
		                  /*errorCode*/ 0,
		                  /*errorReasonPhrase*/ "",
		                  /*hasMessageIntegrity*/ true,
		                  /*hasFingerprint*/ true);

		REQUIRE(
		  request->CheckAuthentication(usernameFragment1, password) ==
		  StunPacket::AuthenticationResult::OK);

		// Trying to modify a STUN Packet once protected must throw.
		REQUIRE_THROWS_AS(request->Protect("qweqwe"), MediaSoupError);
		REQUIRE_THROWS_AS(request->Protect(), MediaSoupError);
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
		                  /*hasXorMappedAddress*/ true,
		                  /*hasErrorCode*/ false,
		                  /*errorCode*/ 0,
		                  /*errorReasonPhrase*/ "",
		                  /*hasMessageIntegrity*/ false,
		                  /*hasFingerprint*/ false);

		struct sockaddr_storage obtainedXorMappedAddressStorage{};

		REQUIRE(successResponse->GetXorMappedAddress(std::addressof(obtainedXorMappedAddressStorage)));

		int family;
		uint16_t port;
		std::string ip;

		Utils::IP::GetAddressInfo(
		  reinterpret_cast<sockaddr*>(std::addressof(obtainedXorMappedAddressStorage)), family, ip, port);

		REQUIRE(family == AF_INET6);
		std::string expectedIp = "2001:db8:85a3:0:0:8a2e:370:7334";
		REQUIRE(ip == Utils::IP::NormalizeIp(expectedIp));
		REQUIRE(port == 1234);

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
		                  /*hasXorMappedAddress*/ true,
		                  /*hasErrorCode*/ false,
		                  /*errorCode*/ 0,
		                  /*errorReasonPhrase*/ "",
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
		                  /*hasXorMappedAddress*/ true,
		                  /*hasErrorCode*/ false,
		                  /*errorCode*/ 0,
		                  /*errorReasonPhrase*/ "",
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
		                  /*hasXorMappedAddress*/ false,
		                  /*hasErrorCode*/ true,
		                  /*errorCode*/ 456,
		                  /*errorReasonPhrase*/ "Something failed Ω∑© :)",
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
		                  /*hasXorMappedAddress*/ false,
		                  /*hasErrorCode*/ true,
		                  /*errorCode*/ 456,
		                  /*errorReasonPhrase*/ "Something failed Ω∑© :)",
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
		                  /*hasXorMappedAddress*/ false,
		                  /*hasErrorCode*/ true,
		                  /*errorCode*/ 456,
		                  /*errorReasonPhrase*/ "Something failed Ω∑© :)",
		                  /*hasMessageIntegrity*/ false,
		                  /*hasFingerprint*/ false);
	}

	SECTION("StunPacket::Factory() creating a request succeeds")
	{
		// clang-format off
		uint8_t transactionId[StunPacket::TransactionIdLength] =
		{
			0x11, 0x22, 0x33, 0x44, 0x55, 0x66,
			0x77, 0x88, 0x99, 0xAA, 0xBB, 0xCC
		};
		// clang-format on

		std::unique_ptr<StunPacket> request{ StunPacket::Factory(
			FactoryBuffer,
			sizeof(FactoryBuffer),
			StunPacket::Class::REQUEST,
			StunPacket::Method::BINDING,
			transactionId) };

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
		                  /*hasXorMappedAddress*/ false,
		                  /*hasErrorCode*/ false,
		                  /*errorCode*/ 0,
		                  /*errorReasonPhrase*/ "",
		                  /*hasMessageIntegrity*/ false,
		                  /*hasFingerprint*/ false);

		// Byte length: 27 (1 byte of padding needed).
		std::string username          = "œæ€å∫∂:¢∞¬÷12";
		std::string usernameFragment1 = "œæ€å∫∂";
		// Byte length: 4.
		uint32_t priority = 999888777u;
		// Byte length: 8.
		uint64_t iceControlling = 15697499370457501716u;
		// Byte length of USE_CANDIDATE: 0.
		// // Byte length: 4.
		uint32_t nomination = 12345678u;
		// Byte length: 18 (2 byte of padding needed).
		std::string software = "mediasoup x.y.z :)";
		// Byte length: 4 + 23 (1 byte of padding needed).
		uint16_t errorCode            = 666;
		std::string errorReasonPhrase = "UPPS UNKNOWN ERROR 😊";

		// Total length of the Attributes.
		size_t attributesLen =
		  (4 + 27 + 1) + (4 + 4) + (4 + 8) + (4) + (4 + 4) + (4 + 18 + 2) + (4 + 4 + 23 + 1);

		request->AddUsername(username);
		request->AddPriority(priority);
		request->AddIceControlling(iceControlling);
		request->AddUseCandidate();
		request->AddNomination(nomination);
		request->AddSoftware(software);
		request->AddErrorCode(errorCode, errorReasonPhrase);

		// It should fail if we try to add a duplicated Attribute.
		REQUIRE_THROWS_AS(request->AddUsername(username), MediaSoupError);
		REQUIRE_THROWS_AS(request->AddPriority(priority), MediaSoupError);
		REQUIRE_THROWS_AS(request->AddIceControlling(iceControlling), MediaSoupError);
		REQUIRE_THROWS_AS(request->AddUseCandidate(), MediaSoupError);
		REQUIRE_THROWS_AS(request->AddNomination(nomination), MediaSoupError);
		REQUIRE_THROWS_AS(request->AddSoftware(software), MediaSoupError);
		REQUIRE_THROWS_AS(request->AddErrorCode(errorCode, errorReasonPhrase), MediaSoupError);

		CHECK_STUN_PACKET(/*packet*/ request.get(),
		                  /*buffer*/ FactoryBuffer,
		                  /*bufferLength*/ sizeof(FactoryBuffer),
		                  /*length*/ StunPacket::FixedHeaderLength + attributesLen,
		                  /*klass*/ StunPacket::Class::REQUEST,
		                  /*method*/ StunPacket::Method::BINDING,
		                  /*hasUsername*/ true,
		                  /*username*/ username,
		                  /*hasPriority*/ true,
		                  /*priority*/ priority,
		                  /*hasIceControlling*/ true,
		                  /*iceControlling*/ iceControlling,
		                  /*hasIceControlled*/ false,
		                  /*iceControlled*/ 0,
		                  /*hasUseCandidate*/ true,
		                  /*hasNomination*/ true,
		                  /*nomination*/ nomination,
		                  /*hasSoftware*/ true,
		                  /*software*/ software,
		                  /*hasXorMappedAddress*/ false,
		                  /*hasErrorCode*/ true,
		                  /*errorCode*/ errorCode,
		                  /*errorReasonPhrase*/ errorReasonPhrase,
		                  /*hasMessageIntegrity*/ false,
		                  /*hasFingerprint*/ false);

		REQUIRE(
		  helpers::AreBuffersEqual(
		    request->GetTransactionId(),
		    StunPacket::TransactionIdLength,
		    transactionId,
		    StunPacket::TransactionIdLength));

		/* Serialize it. */

		request->Serialize(SerializeBuffer, sizeof(SerializeBuffer));

		CHECK_STUN_PACKET(/*packet*/ request.get(),
		                  /*buffer*/ SerializeBuffer,
		                  /*bufferLength*/ sizeof(SerializeBuffer),
		                  /*length*/ StunPacket::FixedHeaderLength + attributesLen,
		                  /*klass*/ StunPacket::Class::REQUEST,
		                  /*method*/ StunPacket::Method::BINDING,
		                  /*hasUsername*/ true,
		                  /*username*/ username,
		                  /*hasPriority*/ true,
		                  /*priority*/ priority,
		                  /*hasIceControlling*/ true,
		                  /*iceControlling*/ iceControlling,
		                  /*hasIceControlled*/ false,
		                  /*iceControlled*/ 0,
		                  /*hasUseCandidate*/ true,
		                  /*hasNomination*/ true,
		                  /*nomination*/ nomination,
		                  /*hasSoftware*/ true,
		                  /*software*/ software,
		                  /*hasXorMappedAddress*/ false,
		                  /*hasErrorCode*/ true,
		                  /*errorCode*/ errorCode,
		                  /*errorReasonPhrase*/ errorReasonPhrase,
		                  /*hasMessageIntegrity*/ false,
		                  /*hasFingerprint*/ false);

		REQUIRE(
		  helpers::AreBuffersEqual(
		    request->GetTransactionId(),
		    StunPacket::TransactionIdLength,
		    transactionId,
		    StunPacket::TransactionIdLength));

		/* Clone it. */

		request.reset(request->Clone(CloneBuffer, sizeof(CloneBuffer)));

		std::memset(SerializeBuffer, 0x00, sizeof(SerializeBuffer));

		CHECK_STUN_PACKET(/*packet*/ request.get(),
		                  /*buffer*/ CloneBuffer,
		                  /*bufferLength*/ sizeof(CloneBuffer),
		                  /*length*/ StunPacket::FixedHeaderLength + attributesLen,
		                  /*klass*/ StunPacket::Class::REQUEST,
		                  /*method*/ StunPacket::Method::BINDING,
		                  /*hasUsername*/ true,
		                  /*username*/ username,
		                  /*hasPriority*/ true,
		                  /*priority*/ priority,
		                  /*hasIceControlling*/ true,
		                  /*iceControlling*/ iceControlling,
		                  /*hasIceControlled*/ false,
		                  /*iceControlled*/ 0,
		                  /*hasUseCandidate*/ true,
		                  /*hasNomination*/ true,
		                  /*nomination*/ nomination,
		                  /*hasSoftware*/ true,
		                  /*software*/ software,
		                  /*hasXorMappedAddress*/ false,
		                  /*hasErrorCode*/ true,
		                  /*errorCode*/ errorCode,
		                  /*errorReasonPhrase*/ errorReasonPhrase,
		                  /*hasMessageIntegrity*/ false,
		                  /*hasFingerprint*/ false);

		REQUIRE(
		  helpers::AreBuffersEqual(
		    request->GetTransactionId(),
		    StunPacket::TransactionIdLength,
		    transactionId,
		    StunPacket::TransactionIdLength));

		/* Protect the STUN Packet. */

		std::string password = "asjhdkjhkasd";

		request->Protect(password);

		CHECK_STUN_PACKET(/*packet*/ request.get(),
		                  /*buffer*/ CloneBuffer,
		                  /*bufferLength*/ sizeof(CloneBuffer),
		                  /*length*/ StunPacket::FixedHeaderLength + attributesLen + 4 +
		                    StunPacket::MessageIntegrityAttributeLength + 4 + 4,
		                  /*klass*/ StunPacket::Class::REQUEST,
		                  /*method*/ StunPacket::Method::BINDING,
		                  /*hasUsername*/ true,
		                  /*username*/ username,
		                  /*hasPriority*/ true,
		                  /*priority*/ priority,
		                  /*hasIceControlling*/ true,
		                  /*iceControlling*/ iceControlling,
		                  /*hasIceControlled*/ false,
		                  /*iceControlled*/ 0,
		                  /*hasUseCandidate*/ true,
		                  /*hasNomination*/ true,
		                  /*nomination*/ nomination,
		                  /*hasSoftware*/ true,
		                  /*software*/ software,
		                  /*hasXorMappedAddress*/ false,
		                  /*hasErrorCode*/ true,
		                  /*errorCode*/ errorCode,
		                  /*errorReasonPhrase*/ errorReasonPhrase,
		                  /*hasMessageIntegrity*/ true,
		                  /*hasFingerprint*/ true);

		REQUIRE(
		  request->CheckAuthentication(usernameFragment1, password) ==
		  StunPacket::AuthenticationResult::OK);

		// Trying to modify a STUN Packet once protected must throw.
		REQUIRE_THROWS_AS(request->Protect("qweqwe"), MediaSoupError);
		REQUIRE_THROWS_AS(request->Protect(), MediaSoupError);
	}

	SECTION("StunPacket::Factory() creating a success response succeeds")
	{
		std::unique_ptr<StunPacket> successResponse{ StunPacket::Factory(
			FactoryBuffer,
			sizeof(FactoryBuffer),
			StunPacket::Class::SUCCESS_RESPONSE,
			StunPacket::Method::BINDING) };

		CHECK_STUN_PACKET(/*packet*/ successResponse.get(),
		                  /*buffer*/ FactoryBuffer,
		                  /*bufferLength*/ sizeof(FactoryBuffer),
		                  /*length*/ StunPacket::FixedHeaderLength,
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
		                  /*hasXorMappedAddress*/ false,
		                  /*hasErrorCode*/ false,
		                  /*errorCode*/ 0,
		                  /*errorReasonPhrase*/ "",
		                  /*hasMessageIntegrity*/ false,
		                  /*hasFingerprint*/ false);

		struct sockaddr_storage xorMappedAddressStorage{};

		// Byte length: 8.
		auto* xorMappedAddressIn =
		  reinterpret_cast<struct sockaddr_in*>(std::addressof(xorMappedAddressStorage));
		auto* xorMappedAddress =
		  reinterpret_cast<struct sockaddr*>(std::addressof(xorMappedAddressStorage));

		uv_ip4_addr("22.33.0.125", 5678, xorMappedAddressIn);

		// Total length of the Attributes.
		size_t attributesLen = (4 + 8);

		successResponse->AddXorMappedAddress(xorMappedAddress);

		CHECK_STUN_PACKET(/*packet*/ successResponse.get(),
		                  /*buffer*/ FactoryBuffer,
		                  /*bufferLength*/ sizeof(FactoryBuffer),
		                  /*length*/ StunPacket::FixedHeaderLength + attributesLen,
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
		                  /*hasXorMappedAddress*/ true,
		                  /*hasErrorCode*/ false,
		                  /*errorCode*/ 0,
		                  /*errorReasonPhrase*/ "",
		                  /*hasMessageIntegrity*/ false,
		                  /*hasFingerprint*/ false);

		struct sockaddr_storage obtainedXorMappedAddressStorage{};

		REQUIRE(successResponse->GetXorMappedAddress(std::addressof(obtainedXorMappedAddressStorage)));

		int family;
		uint16_t port;
		std::string ip;

		Utils::IP::GetAddressInfo(
		  reinterpret_cast<sockaddr*>(std::addressof(obtainedXorMappedAddressStorage)), family, ip, port);

		REQUIRE(family == AF_INET);
		std::string expectedIp = "22.33.0.125";
		REQUIRE(ip == Utils::IP::NormalizeIp(expectedIp));
		REQUIRE(port == 5678);

		std::memset(FactoryBuffer, 0x00, sizeof(FactoryBuffer));

		/* Create a new fresh success response. */

		successResponse.reset(
		  StunPacket::Factory(
		    FactoryBuffer,
		    sizeof(FactoryBuffer),
		    StunPacket::Class::SUCCESS_RESPONSE,
		    StunPacket::Method::BINDING));

		// Byte length: 20.
		auto* xorMappedAddressIn6 =
		  reinterpret_cast<struct sockaddr_in6*>(std::addressof(xorMappedAddressStorage));
		xorMappedAddress = reinterpret_cast<struct sockaddr*>(std::addressof(xorMappedAddressStorage));

		uv_ip6_addr("2001:db8::1234", 20002, xorMappedAddressIn6);

		// Total length of the Attributes.
		attributesLen = (4 + 20);

		successResponse->AddXorMappedAddress(xorMappedAddress);

		CHECK_STUN_PACKET(/*packet*/ successResponse.get(),
		                  /*buffer*/ FactoryBuffer,
		                  /*bufferLength*/ sizeof(FactoryBuffer),
		                  /*length*/ StunPacket::FixedHeaderLength + attributesLen,
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
		                  /*hasXorMappedAddress*/ true,
		                  /*hasErrorCode*/ false,
		                  /*errorCode*/ 0,
		                  /*errorReasonPhrase*/ "",
		                  /*hasMessageIntegrity*/ false,
		                  /*hasFingerprint*/ false);

		REQUIRE(successResponse->GetXorMappedAddress(std::addressof(obtainedXorMappedAddressStorage)));

		Utils::IP::GetAddressInfo(
		  reinterpret_cast<sockaddr*>(std::addressof(obtainedXorMappedAddressStorage)), family, ip, port);

		REQUIRE(family == AF_INET6);
		expectedIp = "2001:db8::1234";
		REQUIRE(ip == Utils::IP::NormalizeIp(expectedIp));
		REQUIRE(port == 20002);

		/* Protect the STUN Packet. */

		std::string password = "asjhdkjhkasd";

		successResponse->Protect(password);

		CHECK_STUN_PACKET(/*packet*/ successResponse.get(),
		                  /*buffer*/ FactoryBuffer,
		                  /*bufferLength*/ sizeof(FactoryBuffer),
		                  /*length*/ StunPacket::FixedHeaderLength + attributesLen + 4 +
		                    StunPacket::MessageIntegrityAttributeLength + 4 + 4,
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
		                  /*hasXorMappedAddress*/ true,
		                  /*hasErrorCode*/ false,
		                  /*errorCode*/ 0,
		                  /*errorReasonPhrase*/ "",
		                  /*hasMessageIntegrity*/ true,
		                  /*hasFingerprint*/ true);

		REQUIRE(successResponse->CheckAuthentication(password) == StunPacket::AuthenticationResult::OK);

		// Trying to modify a STUN Packet once protected must throw.
		REQUIRE_THROWS_AS(successResponse->Protect("qweqwe"), MediaSoupError);
		REQUIRE_THROWS_AS(successResponse->Protect(), MediaSoupError);
	}

	SECTION("StunPacket::Factory() creating an error response succeeds")
	{
		std::unique_ptr<StunPacket> errorResponse{ StunPacket::Factory(
			FactoryBuffer,
			sizeof(FactoryBuffer),
			StunPacket::Class::ERROR_RESPONSE,
			StunPacket::Method::BINDING) };

		CHECK_STUN_PACKET(/*packet*/ errorResponse.get(),
		                  /*buffer*/ FactoryBuffer,
		                  /*bufferLength*/ sizeof(FactoryBuffer),
		                  /*length*/ StunPacket::FixedHeaderLength,
		                  /*klass*/ StunPacket::Class::ERROR_RESPONSE,
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
		                  /*hasXorMappedAddress*/ false,
		                  /*hasErrorCode*/ false,
		                  /*errorCode*/ 0,
		                  /*errorReasonPhrase*/ "",
		                  /*hasMessageIntegrity*/ false,
		                  /*hasFingerprint*/ false);

		// Byte length: 4 + 23 (1 byte of padding needed).
		uint16_t errorCode            = 666;
		std::string errorReasonPhrase = "UPPS UNKNOWN ERROR 😊";

		// Total length of the Attributes.
		size_t attributesLen = (4 + 4 + 23 + 1);

		errorResponse->AddErrorCode(errorCode, errorReasonPhrase);

		CHECK_STUN_PACKET(/*packet*/ errorResponse.get(),
		                  /*buffer*/ FactoryBuffer,
		                  /*bufferLength*/ sizeof(FactoryBuffer),
		                  /*length*/ StunPacket::FixedHeaderLength + attributesLen,
		                  /*klass*/ StunPacket::Class::ERROR_RESPONSE,
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
		                  /*hasXorMappedAddress*/ false,
		                  /*hasErrorCode*/ true,
		                  /*errorCode*/ errorCode,
		                  /*errorReasonPhrase*/ errorReasonPhrase,
		                  /*hasMessageIntegrity*/ false,
		                  /*hasFingerprint*/ false);

		/* Protect the STUN Packet. */

		std::string password = "23786asdas123";

		errorResponse->Protect(password);

		CHECK_STUN_PACKET(/*packet*/ errorResponse.get(),
		                  /*buffer*/ FactoryBuffer,
		                  /*bufferLength*/ sizeof(FactoryBuffer),
		                  /*length*/ StunPacket::FixedHeaderLength + attributesLen + 4 +
		                    StunPacket::MessageIntegrityAttributeLength + 4 + 4,
		                  /*klass*/ StunPacket::Class::ERROR_RESPONSE,
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
		                  /*hasXorMappedAddress*/ false,
		                  /*hasErrorCode*/ true,
		                  /*errorCode*/ errorCode,
		                  /*errorReasonPhrase*/ errorReasonPhrase,
		                  /*hasMessageIntegrity*/ true,
		                  /*hasFingerprint*/ true);

		REQUIRE(errorResponse->CheckAuthentication(password) == StunPacket::AuthenticationResult::OK);

		// Trying to modify a STUN Packet once protected must throw.
		REQUIRE_THROWS_AS(errorResponse->Protect("qweqwe"), MediaSoupError);
		REQUIRE_THROWS_AS(errorResponse->Protect(), MediaSoupError);

		/* Create a new fresh error response. */

		errorResponse.reset(
		  StunPacket::Factory(
		    FactoryBuffer,
		    sizeof(FactoryBuffer),
		    StunPacket::Class::ERROR_RESPONSE,
		    StunPacket::Method::BINDING));

		// Byte length: 4 + 11 (1 byte of padding needed).
		errorCode         = 400;
		errorReasonPhrase = "Bad Request";

		// Total length of the Attributes.
		attributesLen = (4 + 4 + 11 + 1);

		errorResponse->AddErrorCode(errorCode, errorReasonPhrase);

		CHECK_STUN_PACKET(/*packet*/ errorResponse.get(),
		                  /*buffer*/ FactoryBuffer,
		                  /*bufferLength*/ sizeof(FactoryBuffer),
		                  /*length*/ StunPacket::FixedHeaderLength + attributesLen,
		                  /*klass*/ StunPacket::Class::ERROR_RESPONSE,
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
		                  /*hasXorMappedAddress*/ false,
		                  /*hasErrorCode*/ true,
		                  /*errorCode*/ errorCode,
		                  /*errorReasonPhrase*/ errorReasonPhrase,
		                  /*hasMessageIntegrity*/ false,
		                  /*hasFingerprint*/ false);

		/* Protect the STUN Packet (without password). */

		// Protect() without password only adds FINGERPRINT Attribute.
		errorResponse->Protect();

		CHECK_STUN_PACKET(/*packet*/ errorResponse.get(),
		                  /*buffer*/ FactoryBuffer,
		                  /*bufferLength*/ sizeof(FactoryBuffer),
		                  /*length*/ StunPacket::FixedHeaderLength + attributesLen + 4 + 4,
		                  /*klass*/ StunPacket::Class::ERROR_RESPONSE,
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
		                  /*hasXorMappedAddress*/ false,
		                  /*hasErrorCode*/ true,
		                  /*errorCode*/ errorCode,
		                  /*errorReasonPhrase*/ errorReasonPhrase,
		                  /*hasMessageIntegrity*/ false,
		                  /*hasFingerprint*/ true);

		// Cannot check authentication in a STUN Packet without MESSAGE-INTEGRITY.
		REQUIRE(
		  errorResponse->CheckAuthentication(password) == StunPacket::AuthenticationResult::BAD_MESSAGE);

		// Trying to modify a STUN Packet once protected must throw.
		REQUIRE_THROWS_AS(errorResponse->Protect("qweqwe"), MediaSoupError);
		REQUIRE_THROWS_AS(errorResponse->Protect(), MediaSoupError);
	}

	SECTION("StunPacket::CreateSuccessResponse() succeeds")
	{
		std::unique_ptr<StunPacket> request{ StunPacket::Factory(
			FactoryBuffer, sizeof(FactoryBuffer), StunPacket::Class::REQUEST, StunPacket::Method::BINDING) };

		std::unique_ptr<StunPacket> successResponse{ request->CreateSuccessResponse(
			ResponseFactoryBuffer, sizeof(ResponseFactoryBuffer)) };

		CHECK_STUN_PACKET(/*packet*/ successResponse.get(),
		                  /*buffer*/ ResponseFactoryBuffer,
		                  /*bufferLength*/ sizeof(ResponseFactoryBuffer),
		                  /*length*/ StunPacket::FixedHeaderLength,
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
		                  /*hasXorMappedAddress*/ false,
		                  /*hasErrorCode*/ false,
		                  /*errorCode*/ 0,
		                  /*errorReasonPhrase*/ "",
		                  /*hasMessageIntegrity*/ false,
		                  /*hasFingerprint*/ false);

		REQUIRE(
		  helpers::AreBuffersEqual(
		    successResponse->GetTransactionId(),
		    StunPacket::TransactionIdLength,
		    request->GetTransactionId(),
		    StunPacket::TransactionIdLength) == true);

		successResponse->Protect("qwekqjhwekjahsd");

		CHECK_STUN_PACKET(/*packet*/ successResponse.get(),
		                  /*buffer*/ ResponseFactoryBuffer,
		                  /*bufferLength*/ sizeof(ResponseFactoryBuffer),
		                  /*length*/ StunPacket::FixedHeaderLength + 4 +
		                    StunPacket::MessageIntegrityAttributeLength + 4 + 4,
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
		                  /*hasXorMappedAddress*/ false,
		                  /*hasErrorCode*/ false,
		                  /*errorCode*/ 0,
		                  /*errorReasonPhrase*/ "",
		                  /*hasMessageIntegrity*/ true,
		                  /*hasFingerprint*/ true);

		REQUIRE(
		  successResponse->CheckAuthentication("qwekqjhwekjahsd") == StunPacket::AuthenticationResult::OK);

		// Trying to modify a STUN Packet once protected must throw.
		REQUIRE_THROWS_AS(successResponse->Protect("qweqwe"), MediaSoupError);
		REQUIRE_THROWS_AS(successResponse->Protect(), MediaSoupError);
	}

	SECTION("StunPacket::CreateErrorResponse() succeeds")
	{
		std::unique_ptr<StunPacket> request{ StunPacket::Factory(
			FactoryBuffer, sizeof(FactoryBuffer), StunPacket::Class::REQUEST, StunPacket::Method::BINDING) };

		std::unique_ptr<StunPacket> errorResponse{ request->CreateErrorResponse(
			ResponseFactoryBuffer, sizeof(ResponseFactoryBuffer), 666, "BAD STUFF") };

		// Total length of the Attributes (ERROR-CODE).
		const size_t attributesLen = (4 + 4 + 9 + 3);

		CHECK_STUN_PACKET(/*packet*/ errorResponse.get(),
		                  /*buffer*/ ResponseFactoryBuffer,
		                  /*bufferLength*/ sizeof(ResponseFactoryBuffer),
		                  /*length*/ StunPacket::FixedHeaderLength + attributesLen,
		                  /*klass*/ StunPacket::Class::ERROR_RESPONSE,
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
		                  /*hasXorMappedAddress*/ false,
		                  /*hasErrorCode*/ true,
		                  /*errorCode*/ 666,
		                  /*errorReasonPhrase*/ "BAD STUFF",
		                  /*hasMessageIntegrity*/ false,
		                  /*hasFingerprint*/ false);

		REQUIRE(
		  helpers::AreBuffersEqual(
		    errorResponse->GetTransactionId(),
		    StunPacket::TransactionIdLength,
		    request->GetTransactionId(),
		    StunPacket::TransactionIdLength) == true);

		errorResponse->Protect("qwekqjhwekjahsd");

		CHECK_STUN_PACKET(/*packet*/ errorResponse.get(),
		                  /*buffer*/ ResponseFactoryBuffer,
		                  /*bufferLength*/ sizeof(ResponseFactoryBuffer),
		                  /*length*/ StunPacket::FixedHeaderLength + attributesLen + 4 +
		                    StunPacket::MessageIntegrityAttributeLength + 4 + 4,
		                  /*klass*/ StunPacket::Class::ERROR_RESPONSE,
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
		                  /*hasXorMappedAddress*/ false,
		                  /*hasErrorCode*/ true,
		                  /*errorCode*/ 666,
		                  /*errorReasonPhrase*/ "BAD STUFF",
		                  /*hasMessageIntegrity*/ true,
		                  /*hasFingerprint*/ true);

		REQUIRE(
		  errorResponse->CheckAuthentication("qwekqjhwekjahsd") == StunPacket::AuthenticationResult::OK);

		// Trying to modify a STUN Packet once protected must throw.
		REQUIRE_THROWS_AS(errorResponse->Protect("qweqwe"), MediaSoupError);
		REQUIRE_THROWS_AS(errorResponse->Protect(), MediaSoupError);
	}
}
