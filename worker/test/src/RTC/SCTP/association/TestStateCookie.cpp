#include "common.hpp"
#include "RTC/SCTP/association/NegotiatedCapabilities.hpp"
#include "RTC/SCTP/association/StateCookie.hpp"
#include "RTC/SCTP/public/SctpTypes.hpp"
#include "RTC/SCTP/sctpCommon.hpp" // in worker/test/include/
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memset()

SCENARIO("SCTP State Cookie", "[sctp][statecookie]")
{
	sctpCommon::ResetBuffers();

	SECTION("StateCookie::Parse() succeeds")
	{
		// clang-format off
		uint8_t buffer[] =
		{
			// Magic 1: 0x6D73776F726B6572
			0x6D, 0x73, 0x77, 0x6F,
			0x72, 0x6B, 0x65, 0x72,
			// Local Verification Tag: 11223344
			0x00, 0xAB, 0x41, 0x30,
			// Remote Verification Tag: 55667788
			0x03, 0x51, 0x6C, 0x4C,
			// Local Initial TSN: 12345678
			0x00, 0xBC, 0x61, 0x4E,
			// Remote Initial TSN: 87654321
			0x05, 0x39, 0x7F, 0xB1,
			// Remote Advertised Receiver Window Credit (a_rwnd): 66666666
			0x03, 0xF9, 0x40, 0xAA,
			// Tie-Tag: 0xABCDEF0011223344
			0xAB, 0xCD, 0xEF, 0x00,
			0x11, 0x22, 0x33, 0x44,
			// Negotiated Capabilities
			// - partialReliability: 1
			// - messageInterleaving: 0
			// - re-config: 1
			// - zeroChecksum: 1
			// Magic 2: 0xAD81
			0x00, 0b00001101, 0xAD, 0x81,
			// Max Outbound Streams: 15000, Max Inbound Streams: 2500
			0x3A, 0x98, 0x09, 0xC4
		};
		// clang-format on

		REQUIRE(RTC::SCTP::StateCookie::IsMediasoupStateCookie(buffer, sizeof(buffer)) == true);
		REQUIRE(
		  RTC::SCTP::StateCookie::DetermineSctpImplementation(buffer, sizeof(buffer)) ==
		  RTC::SCTP::Types::SctpImplementation::MEDIASOUP);

		auto* stateCookie = RTC::SCTP::StateCookie::Parse(buffer, sizeof(buffer));

		REQUIRE(stateCookie);
		REQUIRE(stateCookie->GetBuffer() == buffer);
		REQUIRE(stateCookie->GetLength() == RTC::SCTP::StateCookie::StateCookieLength);
		REQUIRE(stateCookie->GetBufferLength() == RTC::SCTP::StateCookie::StateCookieLength);
		REQUIRE(stateCookie->GetLocalVerificationTag() == 11223344);
		REQUIRE(stateCookie->GetRemoteVerificationTag() == 55667788);
		REQUIRE(stateCookie->GetLocalInitialTsn() == 12345678);
		REQUIRE(stateCookie->GetRemoteInitialTsn() == 87654321);
		REQUIRE(stateCookie->GetRemoteAdvertisedReceiverWindowCredit() == 66666666);
		REQUIRE(stateCookie->GetTieTag() == 0xABCDEF0011223344);
		REQUIRE(
		  RTC::SCTP::StateCookie::IsMediasoupStateCookie(
		    stateCookie->GetBuffer(), stateCookie->GetLength()) == true);
		REQUIRE(
		  RTC::SCTP::StateCookie::DetermineSctpImplementation(
		    stateCookie->GetBuffer(), stateCookie->GetLength()) ==
		  RTC::SCTP::Types::SctpImplementation::MEDIASOUP);

		auto negotiatedCapabilities = stateCookie->GetNegotiatedCapabilities();

		REQUIRE(negotiatedCapabilities.maxOutboundStreams == 15000);
		REQUIRE(negotiatedCapabilities.maxInboundStreams == 2500);
		REQUIRE(negotiatedCapabilities.partialReliability == true);
		REQUIRE(negotiatedCapabilities.messageInterleaving == false);
		REQUIRE(negotiatedCapabilities.reConfig == true);
		REQUIRE(negotiatedCapabilities.zeroChecksum == true);

		/* Serialize it. */

		stateCookie->Serialize(sctpCommon::SerializeBuffer, sizeof(sctpCommon::SerializeBuffer));

		std::memset(buffer, 0x00, sizeof(buffer));

		REQUIRE(stateCookie);
		REQUIRE(stateCookie->GetBuffer() == sctpCommon::SerializeBuffer);
		REQUIRE(stateCookie->GetLength() == RTC::SCTP::StateCookie::StateCookieLength);
		REQUIRE(stateCookie->GetBufferLength() == sizeof(sctpCommon::SerializeBuffer));
		REQUIRE(stateCookie->GetLocalVerificationTag() == 11223344);
		REQUIRE(stateCookie->GetRemoteVerificationTag() == 55667788);
		REQUIRE(stateCookie->GetLocalInitialTsn() == 12345678);
		REQUIRE(stateCookie->GetRemoteInitialTsn() == 87654321);
		REQUIRE(stateCookie->GetRemoteAdvertisedReceiverWindowCredit() == 66666666);
		REQUIRE(stateCookie->GetTieTag() == 0xABCDEF0011223344);
		REQUIRE(
		  RTC::SCTP::StateCookie::IsMediasoupStateCookie(
		    stateCookie->GetBuffer(), stateCookie->GetLength()) == true);
		REQUIRE(
		  RTC::SCTP::StateCookie::DetermineSctpImplementation(
		    stateCookie->GetBuffer(), stateCookie->GetLength()) ==
		  RTC::SCTP::Types::SctpImplementation::MEDIASOUP);

		negotiatedCapabilities = stateCookie->GetNegotiatedCapabilities();

		REQUIRE(negotiatedCapabilities.maxOutboundStreams == 15000);
		REQUIRE(negotiatedCapabilities.maxInboundStreams == 2500);
		REQUIRE(negotiatedCapabilities.partialReliability == true);
		REQUIRE(negotiatedCapabilities.messageInterleaving == false);
		REQUIRE(negotiatedCapabilities.reConfig == true);
		REQUIRE(negotiatedCapabilities.zeroChecksum == true);

		/* Clone it. */

		auto* clonedStateCookie =
		  stateCookie->Clone(sctpCommon::CloneBuffer, sizeof(sctpCommon::CloneBuffer));

		std::memset(sctpCommon::SerializeBuffer, 0x00, sizeof(sctpCommon::SerializeBuffer));

		delete stateCookie;

		REQUIRE(clonedStateCookie);
		REQUIRE(clonedStateCookie->GetBuffer() == sctpCommon::CloneBuffer);
		REQUIRE(clonedStateCookie->GetLength() == RTC::SCTP::StateCookie::StateCookieLength);
		REQUIRE(clonedStateCookie->GetBufferLength() == sizeof(sctpCommon::CloneBuffer));
		REQUIRE(clonedStateCookie->GetLocalVerificationTag() == 11223344);
		REQUIRE(clonedStateCookie->GetRemoteVerificationTag() == 55667788);
		REQUIRE(clonedStateCookie->GetLocalInitialTsn() == 12345678);
		REQUIRE(clonedStateCookie->GetRemoteInitialTsn() == 87654321);
		REQUIRE(clonedStateCookie->GetRemoteAdvertisedReceiverWindowCredit() == 66666666);
		REQUIRE(clonedStateCookie->GetTieTag() == 0xABCDEF0011223344);
		REQUIRE(
		  RTC::SCTP::StateCookie::IsMediasoupStateCookie(
		    clonedStateCookie->GetBuffer(), clonedStateCookie->GetLength()) == true);
		REQUIRE(
		  RTC::SCTP::StateCookie::DetermineSctpImplementation(
		    clonedStateCookie->GetBuffer(), clonedStateCookie->GetLength()) ==
		  RTC::SCTP::Types::SctpImplementation::MEDIASOUP);

		negotiatedCapabilities = clonedStateCookie->GetNegotiatedCapabilities();

		REQUIRE(negotiatedCapabilities.maxOutboundStreams == 15000);
		REQUIRE(negotiatedCapabilities.maxInboundStreams == 2500);
		REQUIRE(negotiatedCapabilities.partialReliability == true);
		REQUIRE(negotiatedCapabilities.messageInterleaving == false);
		REQUIRE(negotiatedCapabilities.reConfig == true);
		REQUIRE(negotiatedCapabilities.zeroChecksum == true);

		delete clonedStateCookie;
	}

	SECTION("StateCookie::Parse() fails")
	{
		// Wrong Magic 1.
		// clang-format off
		uint8_t buffer1[] =
		{
			// Magic 1: 0x6D73776F726B6573 (wrong)
			0x6D, 0x73, 0x77, 0x6F,
			0x72, 0x6B, 0x65, 0x73,
			// Local Verification Tag: 11223344
			0x00, 0xAB, 0x41, 0x30,
			// Remote Verification Tag: 55667788
			0x03, 0x51, 0x6C, 0x4C,
			// Local Initial TSN: 12345678
			0x00, 0xBC, 0x61, 0x4E,
			// Remote Initial TSN: 87654321
			0x05, 0x39, 0x7F, 0xB1,
			// Remote Advertised Receiver Window Credit (a_rwnd): 66666666
			0x03, 0xF9, 0x40, 0xAA,
			// Tie-Tag: 0xABCDEF0011223344
			0xAB, 0xCD, 0xEF, 0x00,
			0x11, 0x22, 0x33, 0x44,
			// Negotiated Capabilities
			// - partialReliability: 1
			// - messageInterleaving: 0
			// - re-config: 1
			// - zeroChecksum: 1
			// Magic 2: 0xAD81
			0x00, 0b00001101, 0xAD, 0x81,
			// Max Outbound Streams: 15000, Max Inbound Streams: 2500
			0x3A, 0x98, 0x09, 0xC4
		};
		// clang-format on

		REQUIRE(RTC::SCTP::StateCookie::IsMediasoupStateCookie(buffer1, sizeof(buffer1)) == false);
		REQUIRE(
		  RTC::SCTP::StateCookie::DetermineSctpImplementation(buffer1, sizeof(buffer1)) ==
		  RTC::SCTP::Types::SctpImplementation::UNKNOWN);
		REQUIRE(!RTC::SCTP::StateCookie::Parse(buffer1, sizeof(buffer1)));

		// Wrong Magic 2.
		// clang-format off
		uint8_t buffer2[] =
		{
			// Magic 1: 0x6D73776F726B6572
			0x6D, 0x73, 0x77, 0x6F,
			0x72, 0x6B, 0x65, 0x72,
			// Local Verification Tag: 11223344
			0x00, 0xAB, 0x41, 0x30,
			// Remote Verification Tag: 55667788
			0x03, 0x51, 0x6C, 0x4C,
			// Local Initial TSN: 12345678
			0x00, 0xBC, 0x61, 0x4E,
			// Remote Initial TSN: 87654321
			0x05, 0x39, 0x7F, 0xB1,
			// Remote Advertised Receiver Window Credit (a_rwnd): 66666666
			0x03, 0xF9, 0x40, 0xAA,
			// Tie-Tag: 0xABCDEF0011223344
			0xAB, 0xCD, 0xEF, 0x00,
			0x11, 0x22, 0x33, 0x44,
			// Negotiated Capabilities
			// - partialReliability: 1
			// - messageInterleaving: 0
			// - re-config: 1
			// - zeroChecksum: 1
			// Magic 2: 0xAD82 (instead of 0xAD81)
			0x00, 0b00001101, 0xAD, 0x82,
			// Max Outbound Streams: 15000, Max Inbound Streams: 2500
			0x3A, 0x98, 0x09, 0xC4
		};
		// clang-format on

		REQUIRE(RTC::SCTP::StateCookie::IsMediasoupStateCookie(buffer2, sizeof(buffer2)) == false);
		REQUIRE(
		  RTC::SCTP::StateCookie::DetermineSctpImplementation(buffer2, sizeof(buffer2)) ==
		  RTC::SCTP::Types::SctpImplementation::MEDIASOUP);
		REQUIRE(!RTC::SCTP::StateCookie::Parse(buffer2, sizeof(buffer2)));

		// Buffer too big.
		// clang-format off
		uint8_t buffer3[] =
		{
			// Magic 1: 0x6D73776F726B6572
			0x6D, 0x73, 0x77, 0x6F,
			0x72, 0x6B, 0x65, 0x72,
			// Local Verification Tag: 11223344
			0x00, 0xAB, 0x41, 0x30,
			// Remote Verification Tag: 55667788
			0x03, 0x51, 0x6C, 0x4C,
			// Local Initial TSN: 12345678
			0x00, 0xBC, 0x61, 0x4E,
			// Remote Initial TSN: 87654321
			0x05, 0x39, 0x7F, 0xB1,
			// Remote Advertised Receiver Window Credit (a_rwnd): 66666666
			0x03, 0xF9, 0x40, 0xAA,
			// Tie-Tag: 0xABCDEF0011223344
			0xAB, 0xCD, 0xEF, 0x00,
			0x11, 0x22, 0x33, 0x44,
			// Negotiated Capabilities
			// - partialReliability: 1
			// - messageInterleaving: 0
			// - re-config: 1
			// - zeroChecksum: 1
			// Magic 2: 0xAD81
			0x00, 0b00001101, 0xAD, 0x81,
			// Max Outbound Streams: 15000, Max Inbound Streams: 2500
			0x3A, 0x98, 0x09, 0xC4,
			// Extra bytes that shouldn't be here.
			0x11, 0x22, 0x33, 0x44
		};
		// clang-format on

		REQUIRE(RTC::SCTP::StateCookie::IsMediasoupStateCookie(buffer3, sizeof(buffer3)) == false);
		REQUIRE(
		  RTC::SCTP::StateCookie::DetermineSctpImplementation(buffer3, sizeof(buffer3)) ==
		  RTC::SCTP::Types::SctpImplementation::MEDIASOUP);
		REQUIRE(!RTC::SCTP::StateCookie::Parse(buffer3, sizeof(buffer3)));
	}

	SECTION("StateCookie::Factory() succeeds")
	{
		RTC::SCTP::NegotiatedCapabilities negotiatedCapabilities = { .maxOutboundStreams  = 62000,
			                                                           .maxInboundStreams   = 55555,
			                                                           .partialReliability  = true,
			                                                           .messageInterleaving = true,
			                                                           .reConfig            = true,
			                                                           .zeroChecksum        = false };

		auto* stateCookie = RTC::SCTP::StateCookie::Factory(
		  /*buffer*/ sctpCommon::FactoryBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::FactoryBuffer),
		  /*localVerificationTag*/ 6660666,
		  /*remoteVerificationTag*/ 9990999,
		  /*localInitialTsn*/ 1110111,
		  /*remoteInitialTsn*/ 2220222,
		  /*remoteAdvertisedReceiverWindowCredit*/ 999909999,
		  /*tieTag*/ 1111222233334444,
		  negotiatedCapabilities);

		// Change values of the original NegotiatedCapabilities to assert that it
		// doesn't affect the internals of StateCookie.
		negotiatedCapabilities.partialReliability = false;
		negotiatedCapabilities.maxOutboundStreams = 1024;

		REQUIRE(stateCookie);
		REQUIRE(stateCookie->GetBuffer() == sctpCommon::FactoryBuffer);
		REQUIRE(stateCookie->GetLength() == RTC::SCTP::StateCookie::StateCookieLength);
		REQUIRE(stateCookie->GetBufferLength() == RTC::SCTP::StateCookie::StateCookieLength);
		REQUIRE(stateCookie->GetLocalVerificationTag() == 6660666);
		REQUIRE(stateCookie->GetRemoteVerificationTag() == 9990999);
		REQUIRE(stateCookie->GetLocalInitialTsn() == 1110111);
		REQUIRE(stateCookie->GetRemoteInitialTsn() == 2220222);
		REQUIRE(stateCookie->GetRemoteAdvertisedReceiverWindowCredit() == 999909999);
		REQUIRE(stateCookie->GetTieTag() == 1111222233334444);
		REQUIRE(
		  RTC::SCTP::StateCookie::IsMediasoupStateCookie(
		    stateCookie->GetBuffer(), stateCookie->GetLength()) == true);
		REQUIRE(
		  RTC::SCTP::StateCookie::DetermineSctpImplementation(
		    stateCookie->GetBuffer(), stateCookie->GetLength()) ==
		  RTC::SCTP::Types::SctpImplementation::MEDIASOUP);

		const auto retrievedNegotiatedCapabilities = stateCookie->GetNegotiatedCapabilities();

		REQUIRE(retrievedNegotiatedCapabilities.maxOutboundStreams == 62000);
		REQUIRE(retrievedNegotiatedCapabilities.maxInboundStreams == 55555);
		REQUIRE(retrievedNegotiatedCapabilities.partialReliability == true);
		REQUIRE(retrievedNegotiatedCapabilities.messageInterleaving == true);
		REQUIRE(retrievedNegotiatedCapabilities.reConfig == true);
		REQUIRE(retrievedNegotiatedCapabilities.zeroChecksum == false);

		/* Parse itself and compare. */

		auto* parsedStateCookie =
		  RTC::SCTP::StateCookie::Parse(stateCookie->GetBuffer(), stateCookie->GetLength());

		delete stateCookie;

		REQUIRE(parsedStateCookie);
		REQUIRE(parsedStateCookie->GetBuffer() == sctpCommon::FactoryBuffer);
		REQUIRE(parsedStateCookie->GetLength() == RTC::SCTP::StateCookie::StateCookieLength);
		REQUIRE(parsedStateCookie->GetBufferLength() == RTC::SCTP::StateCookie::StateCookieLength);
		REQUIRE(parsedStateCookie->GetLocalVerificationTag() == 6660666);
		REQUIRE(parsedStateCookie->GetRemoteVerificationTag() == 9990999);
		REQUIRE(parsedStateCookie->GetLocalInitialTsn() == 1110111);
		REQUIRE(parsedStateCookie->GetRemoteInitialTsn() == 2220222);
		REQUIRE(parsedStateCookie->GetRemoteAdvertisedReceiverWindowCredit() == 999909999);
		REQUIRE(parsedStateCookie->GetTieTag() == 1111222233334444);
		REQUIRE(
		  RTC::SCTP::StateCookie::IsMediasoupStateCookie(
		    parsedStateCookie->GetBuffer(), parsedStateCookie->GetLength()) == true);
		REQUIRE(
		  RTC::SCTP::StateCookie::DetermineSctpImplementation(
		    parsedStateCookie->GetBuffer(), parsedStateCookie->GetLength()) ==
		  RTC::SCTP::Types::SctpImplementation::MEDIASOUP);

		const auto retrievedParsedNegotiatedCapabilities = parsedStateCookie->GetNegotiatedCapabilities();

		REQUIRE(retrievedParsedNegotiatedCapabilities.maxOutboundStreams == 62000);
		REQUIRE(retrievedParsedNegotiatedCapabilities.maxInboundStreams == 55555);
		REQUIRE(retrievedParsedNegotiatedCapabilities.partialReliability == true);
		REQUIRE(retrievedParsedNegotiatedCapabilities.messageInterleaving == true);
		REQUIRE(retrievedParsedNegotiatedCapabilities.reConfig == true);
		REQUIRE(retrievedParsedNegotiatedCapabilities.zeroChecksum == false);

		delete parsedStateCookie;
	}

	SECTION("StateCookie::Write() succeeds")
	{
		RTC::SCTP::NegotiatedCapabilities negotiatedCapabilities = { .maxOutboundStreams  = 62000,
			                                                           .maxInboundStreams   = 55555,
			                                                           .partialReliability  = true,
			                                                           .messageInterleaving = true,
			                                                           .reConfig            = true,
			                                                           .zeroChecksum        = false };

		auto* buffer = sctpCommon::FactoryBuffer;

		RTC::SCTP::StateCookie::Write(
		  /*buffer*/ buffer,
		  /*bufferLength*/ RTC::SCTP::StateCookie::StateCookieLength,
		  /*localVerificationTag*/ 6660666,
		  /*remoteVerificationTag*/ 9990999,
		  /*localInitialTsn*/ 1110111,
		  /*remoteInitialTsn*/ 2220222,
		  /*remoteAdvertisedReceiverWindowCredit*/ 999909999,
		  /*tieTag*/ 1111222233334444,
		  negotiatedCapabilities);

		// Change values of the original NegotiatedCapabilities to assert that it
		// doesn't affect the internals of StateCookie.
		negotiatedCapabilities.partialReliability = false;
		negotiatedCapabilities.maxOutboundStreams = 1024;

		/* Parse the buffer. */

		auto* stateCookie =
		  RTC::SCTP::StateCookie::Parse(buffer, RTC::SCTP::StateCookie::StateCookieLength);

		REQUIRE(stateCookie);
		REQUIRE(stateCookie->GetBuffer() == buffer);
		REQUIRE(stateCookie->GetLength() == RTC::SCTP::StateCookie::StateCookieLength);
		REQUIRE(stateCookie->GetBufferLength() == RTC::SCTP::StateCookie::StateCookieLength);
		REQUIRE(stateCookie->GetLocalVerificationTag() == 6660666);
		REQUIRE(stateCookie->GetRemoteVerificationTag() == 9990999);
		REQUIRE(stateCookie->GetLocalInitialTsn() == 1110111);
		REQUIRE(stateCookie->GetRemoteInitialTsn() == 2220222);
		REQUIRE(stateCookie->GetRemoteAdvertisedReceiverWindowCredit() == 999909999);
		REQUIRE(stateCookie->GetTieTag() == 1111222233334444);
		REQUIRE(
		  RTC::SCTP::StateCookie::IsMediasoupStateCookie(
		    stateCookie->GetBuffer(), stateCookie->GetLength()) == true);
		REQUIRE(
		  RTC::SCTP::StateCookie::DetermineSctpImplementation(
		    stateCookie->GetBuffer(), stateCookie->GetLength()) ==
		  RTC::SCTP::Types::SctpImplementation::MEDIASOUP);

		const auto retrievedNegotiatedCapabilities = stateCookie->GetNegotiatedCapabilities();

		REQUIRE(retrievedNegotiatedCapabilities.maxOutboundStreams == 62000);
		REQUIRE(retrievedNegotiatedCapabilities.maxInboundStreams == 55555);
		REQUIRE(retrievedNegotiatedCapabilities.partialReliability == true);
		REQUIRE(retrievedNegotiatedCapabilities.messageInterleaving == true);
		REQUIRE(retrievedNegotiatedCapabilities.reConfig == true);
		REQUIRE(retrievedNegotiatedCapabilities.zeroChecksum == false);

		delete stateCookie;
	}

	SECTION("StateCookie::DetermineSctpImplementation() succeeds")
	{
		// usrsctp generated State Cookie.
		// clang-format off
		uint8_t buffer1[] =
		{
			// Magic 1: 0x4B414D452D425344
			0x4B, 0x41, 0x4D, 0x45,
			0x2D, 0x42, 0x53, 0x44,
			0x11, 0x22, 0x33, 0x44,
			0x6D, 0x73, 0x77, 0x6F,
			0x72, 0x6B, 0x65, 0x72,
			0x11, 0x22, 0x33, 0x44,
			0x00, 0xAB, 0x41, 0x30,
			0x03, 0x51, 0x6C, 0x4C,
			0x00, 0xBC, 0x61, 0x4E,
			0x11, 0x22, 0x33, 0x44,
			0x05, 0x39, 0x7F, 0xB1,
			0x03, 0xF9, 0x40, 0xAA,
			0xAB, 0xCD, 0xEF, 0x00,
			0x11, 0x22, 0x33, 0x44,
			// etc
		};
		// clang-format on

		REQUIRE(RTC::SCTP::StateCookie::IsMediasoupStateCookie(buffer1, sizeof(buffer1)) == false);
		REQUIRE(
		  RTC::SCTP::StateCookie::DetermineSctpImplementation(buffer1, sizeof(buffer1)) ==
		  RTC::SCTP::Types::SctpImplementation::USRSCTP);

		// dcSCTP generated State Cookie.
		// clang-format off
		uint8_t buffer2[] =
		{
			// Magic 1: 0x6463534354503030
			0x64, 0x63, 0x53, 0x43,
			0x54, 0x50, 0x30, 0x30,
			0x5D, 0x0E, 0x21, 0xE4,
			0x0F, 0xA8, 0x44, 0x3F,
			0x11, 0x80, 0x89, 0x5D,
			0x2F, 0x4E, 0x17, 0x1F,
			0x00, 0x02, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00,
			0x01, 0x00, 0x01, 0x00,
		};
		// clang-format on

		REQUIRE(RTC::SCTP::StateCookie::IsMediasoupStateCookie(buffer2, sizeof(buffer2)) == false);
		REQUIRE(
		  RTC::SCTP::StateCookie::DetermineSctpImplementation(buffer2, sizeof(buffer2)) ==
		  RTC::SCTP::Types::SctpImplementation::DCSCTP);

		// State Cookie generated by unknown implementation.
		// clang-format off
		uint8_t buffer3[] =
		{
			// Magic 1: 0x1122334455667788
			0x11, 0x22, 0x33, 0x44,
			0x55, 0x66, 0x77, 0x88,
			0x11, 0x80, 0x89, 0x5D,
			0x2F, 0x4E, 0x17, 0x1F,
			0x00, 0x02, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00,
			0x01, 0x00, 0x01, 0x00,
		};
		// clang-format on

		REQUIRE(RTC::SCTP::StateCookie::IsMediasoupStateCookie(buffer3, sizeof(buffer3)) == false);
		REQUIRE(
		  RTC::SCTP::StateCookie::DetermineSctpImplementation(buffer3, sizeof(buffer3)) ==
		  RTC::SCTP::Types::SctpImplementation::UNKNOWN);

		// Too short State Cookie so we don't know.
		// clang-format off
		uint8_t buffer4[] =
		{
			// Magic 1: 0xAABBCCDD
			0xAA, 0xBB, 0xCC, 0xDD,
		};
		// clang-format on

		REQUIRE(RTC::SCTP::StateCookie::IsMediasoupStateCookie(buffer4, sizeof(buffer4)) == false);
		REQUIRE(
		  RTC::SCTP::StateCookie::DetermineSctpImplementation(buffer4, sizeof(buffer4)) ==
		  RTC::SCTP::Types::SctpImplementation::UNKNOWN);
	}
}
