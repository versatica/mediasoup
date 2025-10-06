#include "common.hpp"
#include "RTC/SCTP/association/StateCookie.hpp"
#include "RTC/SCTP/common.hpp" // in worker/test/include/
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memset()

using namespace RTC::SCTP;

SCENARIO("SCTP State Cookie", "[sctp][statecookie]")
{
	resetBuffers();

	SECTION("StateCookie::Parse() succeeds")
	{
		// clang-format off
		uint8_t buffer[] =
		{
			// Magic Value 1: 0xF109ABE4
			0xF1, 0x09, 0xAB, 0xE4,
			// My Verification Tag: 11223344
			0x00, 0xAB, 0x41, 0x30,
			// Peer Verification Tag: 55667788
			0x03, 0x51, 0x6C, 0x4C,
			// My Initial TSN: 12345678
			0x00, 0xBC, 0x61, 0x4E,
			// Peer Initial TSN: 87654321
			0x05, 0x39, 0x7F, 0xB1,
			// My Advertised Receiver Window Credit (a_rwnd): 66666666
			0x03, 0xF9, 0x40, 0xAA,
			// Tie-Tag: 0xABCDEF0011223344
			0xAB, 0xCD, 0xEF, 0x00,
			0x11, 0x22, 0x33, 0x44,
			// Negotiated Capabilities
			// - partialReliability: 1
			// - messageInterleaving: 0
			// - reconfig: 1
			// - zeroChecksum: 1
			// Magic Value 2: 0xAD81
			0x00, 0b00001101, 0xAD, 0x81,
			// Max Outbound Streams: 15000, Max Inbound Streams: 2500
			0x3A, 0x98, 0x09, 0xC4
		};
		// clang-format on

		auto* stateCookie = StateCookie::Parse(buffer, sizeof(buffer));

		REQUIRE(stateCookie);
		REQUIRE(stateCookie->GetBuffer() == buffer);
		REQUIRE(stateCookie->GetLength() == StateCookie::StateCookieLength);
		REQUIRE(stateCookie->GetBufferLength() == StateCookie::StateCookieLength);
		REQUIRE(stateCookie->IsFrozen() == true);
		REQUIRE(stateCookie->GetMyVerificationTag() == 11223344);
		REQUIRE(stateCookie->GetPeerVerificationTag() == 55667788);
		REQUIRE(stateCookie->GetMyInitialTsn() == 12345678);
		REQUIRE(stateCookie->GetPeerInitialTsn() == 87654321);
		REQUIRE(stateCookie->GetMyAdvertisedReceiverWindowCredit() == 66666666);
		REQUIRE(stateCookie->GetTieTag() == 0xABCDEF0011223344);

		auto negotiatedCapabilities = stateCookie->GetNegotiatedCapabilities();

		REQUIRE(negotiatedCapabilities.maxOutboundStreams == 15000);
		REQUIRE(negotiatedCapabilities.maxInboundStreams == 2500);
		REQUIRE(negotiatedCapabilities.partialReliability == true);
		REQUIRE(negotiatedCapabilities.messageInterleaving == false);
		REQUIRE(negotiatedCapabilities.reconfig == true);
		REQUIRE(negotiatedCapabilities.zeroChecksum == true);

		/* Serialize it. */

		stateCookie->Serialize(SerializeBuffer, sizeof(SerializeBuffer));

		std::memset(buffer, 0x00, sizeof(buffer));

		REQUIRE(stateCookie);
		REQUIRE(stateCookie->GetBuffer() == SerializeBuffer);
		REQUIRE(stateCookie->GetLength() == StateCookie::StateCookieLength);
		REQUIRE(stateCookie->GetBufferLength() == sizeof(SerializeBuffer));
		REQUIRE(stateCookie->IsFrozen() == false);
		REQUIRE(stateCookie->GetMyVerificationTag() == 11223344);
		REQUIRE(stateCookie->GetPeerVerificationTag() == 55667788);
		REQUIRE(stateCookie->GetMyInitialTsn() == 12345678);
		REQUIRE(stateCookie->GetPeerInitialTsn() == 87654321);
		REQUIRE(stateCookie->GetMyAdvertisedReceiverWindowCredit() == 66666666);
		REQUIRE(stateCookie->GetTieTag() == 0xABCDEF0011223344);

		negotiatedCapabilities = stateCookie->GetNegotiatedCapabilities();

		REQUIRE(negotiatedCapabilities.maxOutboundStreams == 15000);
		REQUIRE(negotiatedCapabilities.maxInboundStreams == 2500);
		REQUIRE(negotiatedCapabilities.partialReliability == true);
		REQUIRE(negotiatedCapabilities.messageInterleaving == false);
		REQUIRE(negotiatedCapabilities.reconfig == true);
		REQUIRE(negotiatedCapabilities.zeroChecksum == true);

		/* Clone it. */

		auto* clonedStateCookie = stateCookie->Clone(CloneBuffer, sizeof(CloneBuffer));

		std::memset(SerializeBuffer, 0x00, sizeof(SerializeBuffer));

		delete stateCookie;

		REQUIRE(clonedStateCookie);
		REQUIRE(clonedStateCookie->GetBuffer() == CloneBuffer);
		REQUIRE(clonedStateCookie->GetLength() == StateCookie::StateCookieLength);
		REQUIRE(clonedStateCookie->GetBufferLength() == sizeof(CloneBuffer));
		REQUIRE(clonedStateCookie->IsFrozen() == false);
		REQUIRE(clonedStateCookie->GetMyVerificationTag() == 11223344);
		REQUIRE(clonedStateCookie->GetPeerVerificationTag() == 55667788);
		REQUIRE(clonedStateCookie->GetMyInitialTsn() == 12345678);
		REQUIRE(clonedStateCookie->GetPeerInitialTsn() == 87654321);
		REQUIRE(clonedStateCookie->GetMyAdvertisedReceiverWindowCredit() == 66666666);
		REQUIRE(clonedStateCookie->GetTieTag() == 0xABCDEF0011223344);

		negotiatedCapabilities = clonedStateCookie->GetNegotiatedCapabilities();

		REQUIRE(negotiatedCapabilities.maxOutboundStreams == 15000);
		REQUIRE(negotiatedCapabilities.maxInboundStreams == 2500);
		REQUIRE(negotiatedCapabilities.partialReliability == true);
		REQUIRE(negotiatedCapabilities.messageInterleaving == false);
		REQUIRE(negotiatedCapabilities.reconfig == true);
		REQUIRE(negotiatedCapabilities.zeroChecksum == true);

		delete clonedStateCookie;
	}

	SECTION("StateCookie::Parse() fails")
	{
		// Wrong Magic Value 1.
		// clang-format off
		uint8_t buffer1[] =
		{
			// Magic Value 1: 0xF109ABE5 (instead of 0xF109ABE4)
			0xF1, 0x09, 0xAB, 0xE5,
			// My Verification Tag: 11223344
			0x00, 0xAB, 0x41, 0x30,
			// Peer Verification Tag: 55667788
			0x03, 0x51, 0x6C, 0x4C,
			// My Initial TSN: 12345678
			0x00, 0xBC, 0x61, 0x4E,
			// Peer Initial TSN: 87654321
			0x05, 0x39, 0x7F, 0xB1,
			// My Advertised Receiver Window Credit (a_rwnd): 66666666
			0x03, 0xF9, 0x40, 0xAA,
			// Tie-Tag: 0xABCDEF0011223344
			0xAB, 0xCD, 0xEF, 0x00,
			0x11, 0x22, 0x33, 0x44,
			// Negotiated Capabilities
			// - partialReliability: 1
			// - messageInterleaving: 0
			// - reconfig: 1
			// - zeroChecksum: 1
			// Magic Value 2: 0xAD81
			0x00, 0b00001101, 0xAD, 0x81,
			// Max Outbound Streams: 15000, Max Inbound Streams: 2500
			0x3A, 0x98, 0x09, 0xC4
		};
		// clang-format on

		REQUIRE(!StateCookie::Parse(buffer1, sizeof(buffer1)));

		// Wrong Magic Value 2.
		// clang-format off
		uint8_t buffer2[] =
		{
			// Magic Value 1: 0xF109ABE4
			0xF1, 0x09, 0xAB, 0xE4,
			// My Verification Tag: 11223344
			0x00, 0xAB, 0x41, 0x30,
			// Peer Verification Tag: 55667788
			0x03, 0x51, 0x6C, 0x4C,
			// My Initial TSN: 12345678
			0x00, 0xBC, 0x61, 0x4E,
			// Peer Initial TSN: 87654321
			0x05, 0x39, 0x7F, 0xB1,
			// My Advertised Receiver Window Credit (a_rwnd): 66666666
			0x03, 0xF9, 0x40, 0xAA,
			// Tie-Tag: 0xABCDEF0011223344
			0xAB, 0xCD, 0xEF, 0x00,
			0x11, 0x22, 0x33, 0x44,
			// Negotiated Capabilities
			// - partialReliability: 1
			// - messageInterleaving: 0
			// - reconfig: 1
			// - zeroChecksum: 1
			// Magic Value 2: 0xAD82 (instead of 0xAD81)
			0x00, 0b00001101, 0xAD, 0x82,
			// Max Outbound Streams: 15000, Max Inbound Streams: 2500
			0x3A, 0x98, 0x09, 0xC4
		};
		// clang-format on

		REQUIRE(!StateCookie::Parse(buffer2, sizeof(buffer2)));

		// Buffer too big.
		// clang-format off
		uint8_t buffer3[] =
		{
			// Magic Value 1: 0xF109ABE4
			0xF1, 0x09, 0xAB, 0xE4,
			// My Verification Tag: 11223344
			0x00, 0xAB, 0x41, 0x30,
			// Peer Verification Tag: 55667788
			0x03, 0x51, 0x6C, 0x4C,
			// My Initial TSN: 12345678
			0x00, 0xBC, 0x61, 0x4E,
			// Peer Initial TSN: 87654321
			0x05, 0x39, 0x7F, 0xB1,
			// My Advertised Receiver Window Credit (a_rwnd): 66666666
			0x03, 0xF9, 0x40, 0xAA,
			// Tie-Tag: 0xABCDEF0011223344
			0xAB, 0xCD, 0xEF, 0x00,
			0x11, 0x22, 0x33, 0x44,
			// Negotiated Capabilities
			// - partialReliability: 1
			// - messageInterleaving: 0
			// - reconfig: 1
			// - zeroChecksum: 1
			// Magic Value 2: 0xAD81
			0x00, 0b00001101, 0xAD, 0x81,
			// Max Outbound Streams: 15000, Max Inbound Streams: 2500
			0x3A, 0x98, 0x09, 0xC4,
			// Extra bytes that shouldn't be here.
			0x11, 0x22, 0x33, 0x44
		};
		// clang-format on

		REQUIRE(!StateCookie::Parse(buffer3, sizeof(buffer3)));
	}

	SECTION("StateCookie::Factory() succeeds")
	{
		NegotiatedCapabilities negotiatedCapabilities = { .maxOutboundStreams  = 62000,
			                                                .maxInboundStreams   = 55555,
			                                                .partialReliability  = true,
			                                                .messageInterleaving = true,
			                                                .reconfig            = true,
			                                                .zeroChecksum        = false };

		auto* stateCookie = StateCookie::Factory(
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ sizeof(FactoryBuffer),
		  /*myVerificationTag*/ 6660666,
		  /*peerVerificationTag*/ 9990999,
		  /*myInitialTsn*/ 1110111,
		  /*peerInitialTsn*/ 2220222,
		  /*myAdvertisedReceiverWindowCredit*/ 999909999,
		  /*tieTag*/ 1111222233334444,
		  negotiatedCapabilities);

		// Change values of the original NegotiatedCapabilities to assert that it
		// doesn't affect the internals of StateCookie.
		negotiatedCapabilities.partialReliability = false;
		negotiatedCapabilities.maxOutboundStreams = 1024;

		REQUIRE(stateCookie);
		REQUIRE(stateCookie->GetBuffer() == FactoryBuffer);
		REQUIRE(stateCookie->GetLength() == StateCookie::StateCookieLength);
		REQUIRE(stateCookie->GetBufferLength() == StateCookie::StateCookieLength);
		REQUIRE(stateCookie->IsFrozen() == false);
		REQUIRE(stateCookie->GetMyVerificationTag() == 6660666);
		REQUIRE(stateCookie->GetPeerVerificationTag() == 9990999);
		REQUIRE(stateCookie->GetMyInitialTsn() == 1110111);
		REQUIRE(stateCookie->GetPeerInitialTsn() == 2220222);
		REQUIRE(stateCookie->GetMyAdvertisedReceiverWindowCredit() == 999909999);
		REQUIRE(stateCookie->GetTieTag() == 1111222233334444);

		const auto retrievedNegotiatedCapabilities = stateCookie->GetNegotiatedCapabilities();

		REQUIRE(retrievedNegotiatedCapabilities.maxOutboundStreams == 62000);
		REQUIRE(retrievedNegotiatedCapabilities.maxInboundStreams == 55555);
		REQUIRE(retrievedNegotiatedCapabilities.partialReliability == true);
		REQUIRE(retrievedNegotiatedCapabilities.messageInterleaving == true);
		REQUIRE(retrievedNegotiatedCapabilities.reconfig == true);
		REQUIRE(retrievedNegotiatedCapabilities.zeroChecksum == false);

		/* Parse itself and compare. */

		auto* parsedStateCookie = StateCookie::Parse(stateCookie->GetBuffer(), stateCookie->GetLength());

		delete stateCookie;

		REQUIRE(parsedStateCookie);
		REQUIRE(parsedStateCookie->GetBuffer() == FactoryBuffer);
		REQUIRE(parsedStateCookie->GetLength() == StateCookie::StateCookieLength);
		REQUIRE(parsedStateCookie->GetBufferLength() == StateCookie::StateCookieLength);
		REQUIRE(parsedStateCookie->IsFrozen() == true);
		REQUIRE(parsedStateCookie->GetMyVerificationTag() == 6660666);
		REQUIRE(parsedStateCookie->GetPeerVerificationTag() == 9990999);
		REQUIRE(parsedStateCookie->GetMyInitialTsn() == 1110111);
		REQUIRE(parsedStateCookie->GetPeerInitialTsn() == 2220222);
		REQUIRE(parsedStateCookie->GetMyAdvertisedReceiverWindowCredit() == 999909999);
		REQUIRE(parsedStateCookie->GetTieTag() == 1111222233334444);

		const auto retrievedParsedNegotiatedCapabilities = parsedStateCookie->GetNegotiatedCapabilities();

		REQUIRE(retrievedParsedNegotiatedCapabilities.maxOutboundStreams == 62000);
		REQUIRE(retrievedParsedNegotiatedCapabilities.maxInboundStreams == 55555);
		REQUIRE(retrievedParsedNegotiatedCapabilities.partialReliability == true);
		REQUIRE(retrievedParsedNegotiatedCapabilities.messageInterleaving == true);
		REQUIRE(retrievedParsedNegotiatedCapabilities.reconfig == true);
		REQUIRE(retrievedParsedNegotiatedCapabilities.zeroChecksum == false);

		delete parsedStateCookie;
	}
}
