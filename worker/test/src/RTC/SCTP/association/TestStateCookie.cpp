#include "common.hpp"
#include "RTC/SCTP/association/Capabilities.hpp"
#include "RTC/SCTP/association/StateCookie.hpp"
#include "RTC/SCTP/packet/parameters/ZeroChecksumAcceptableParameter.hpp"
#include "RTC/SCTP/public/SctpTypes.hpp"
#include "test/include/RTC/SCTP/sctpCommon.hpp" // in worker/test/include/
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memset()

SCENARIO("SCTP State Cookie", "[sctp][statecookie]")
{
	sctpCommon::ResetBuffers();

	SECTION("StateCookie::Parse() succeeds")
	{
		// clang-format off
		alignas(4) uint8_t buffer[] =
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
			// Remote Capabilities
			// - partialReliability: 1
			// - messageInterleaving: 0
			// - re-config: 1
			// Magic 2: 0xAD81
			0x00, 0b00000101, 0xAD, 0x81,
			// Zero Checksum Alternate Error Detection Method: SCTP_OVER_DTLS (1)
			0x00, 0x00, 0x00, 0x01,
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

		auto remoteCapabilities = stateCookie->GetRemoteCapabilities();

		REQUIRE(remoteCapabilities.maxOutboundStreams == 15000);
		REQUIRE(remoteCapabilities.maxInboundStreams == 2500);
		REQUIRE(remoteCapabilities.partialReliability == true);
		REQUIRE(remoteCapabilities.messageInterleaving == false);
		REQUIRE(remoteCapabilities.reConfig == true);
		REQUIRE(
		  remoteCapabilities.zeroChecksumAlternateErrorDetectionMethod ==
		  RTC::SCTP::ZeroChecksumAcceptableParameter::AlternateErrorDetectionMethod::SCTP_OVER_DTLS);

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

		remoteCapabilities = stateCookie->GetRemoteCapabilities();

		REQUIRE(remoteCapabilities.maxOutboundStreams == 15000);
		REQUIRE(remoteCapabilities.maxInboundStreams == 2500);
		REQUIRE(remoteCapabilities.partialReliability == true);
		REQUIRE(remoteCapabilities.messageInterleaving == false);
		REQUIRE(remoteCapabilities.reConfig == true);
		REQUIRE(
		  remoteCapabilities.zeroChecksumAlternateErrorDetectionMethod ==
		  RTC::SCTP::ZeroChecksumAcceptableParameter::AlternateErrorDetectionMethod::SCTP_OVER_DTLS);

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

		remoteCapabilities = clonedStateCookie->GetRemoteCapabilities();

		REQUIRE(remoteCapabilities.maxOutboundStreams == 15000);
		REQUIRE(remoteCapabilities.maxInboundStreams == 2500);
		REQUIRE(remoteCapabilities.partialReliability == true);
		REQUIRE(remoteCapabilities.messageInterleaving == false);
		REQUIRE(remoteCapabilities.reConfig == true);
		REQUIRE(
		  remoteCapabilities.zeroChecksumAlternateErrorDetectionMethod ==
		  RTC::SCTP::ZeroChecksumAcceptableParameter::AlternateErrorDetectionMethod::SCTP_OVER_DTLS);

		delete clonedStateCookie;
	}

	SECTION("StateCookie::Parse() fails")
	{
		// Wrong Magic 1.
		// clang-format off
		alignas(4) uint8_t buffer1[] =
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
			// Remote Capabilities
			// - partialReliability: 1
			// - messageInterleaving: 0
			// - re-config: 1
			// Magic 2: 0xAD81
			0x00, 0b00000101, 0xAD, 0x81,
			// Zero Checksum Alternate Error Detection Method: SCTP_OVER_DTLS (1)
			0x00, 0x00, 0x00, 0x01,
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
		alignas(4) uint8_t buffer2[] =
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
			// Remote Capabilities
			// - partialReliability: 1
			// - messageInterleaving: 0
			// - re-config: 1
			// Magic 2: 0xAD82 (instead of 0xAD81)
			0x00, 0b00000101, 0xAD, 0x82,
			// Zero Checksum Alternate Error Detection Method: SCTP_OVER_DTLS (1)
			0x00, 0x00, 0x00, 0x01,
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
		alignas(4) uint8_t buffer3[] =
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
			// Remote Capabilities
			// - partialReliability: 1
			// - messageInterleaving: 0
			// - re-config: 1
			// Magic 2: 0xAD81
			0x00, 0b00000101, 0xAD, 0x81,
			// Zero Checksum Alternate Error Detection Method: SCTP_OVER_DTLS (1)
			0x00, 0x00, 0x00, 0x01,
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

	SECTION("StateCookie::Parse() fails on a zero verification tag")
	{
		// Zero Local Verification Tag.
		// clang-format off
		alignas(4) uint8_t buffer1[] =
		{
			// Magic 1: 0x6D73776F726B6572
			0x6D, 0x73, 0x77, 0x6F,
			0x72, 0x6B, 0x65, 0x72,
			// Local Verification Tag: 0 (invalid)
			0x00, 0x00, 0x00, 0x00,
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
			// Remote Capabilities
			// Magic 2: 0xAD81
			0x00, 0b00000101, 0xAD, 0x81,
			// Zero Checksum Alternate Error Detection Method: SCTP_OVER_DTLS (1)
			0x00, 0x00, 0x00, 0x01,
			// Max Outbound Streams: 15000, Max Inbound Streams: 2500
			0x3A, 0x98, 0x09, 0xC4
		};
		// clang-format on

		// It is still recognized as a mediasoup State Cookie (magic values and
		// length are fine) but Parse() must reject it.
		REQUIRE(RTC::SCTP::StateCookie::IsMediasoupStateCookie(buffer1, sizeof(buffer1)) == true);
		REQUIRE(!RTC::SCTP::StateCookie::Parse(buffer1, sizeof(buffer1)));

		// Zero Remote Verification Tag.
		// clang-format off
		alignas(4) uint8_t buffer2[] =
		{
			// Magic 1: 0x6D73776F726B6572
			0x6D, 0x73, 0x77, 0x6F,
			0x72, 0x6B, 0x65, 0x72,
			// Local Verification Tag: 11223344
			0x00, 0xAB, 0x41, 0x30,
			// Remote Verification Tag: 0 (invalid)
			0x00, 0x00, 0x00, 0x00,
			// Local Initial TSN: 12345678
			0x00, 0xBC, 0x61, 0x4E,
			// Remote Initial TSN: 87654321
			0x05, 0x39, 0x7F, 0xB1,
			// Remote Advertised Receiver Window Credit (a_rwnd): 66666666
			0x03, 0xF9, 0x40, 0xAA,
			// Tie-Tag: 0xABCDEF0011223344
			0xAB, 0xCD, 0xEF, 0x00,
			0x11, 0x22, 0x33, 0x44,
			// Remote Capabilities
			// Magic 2: 0xAD81
			0x00, 0b00000101, 0xAD, 0x81,
			// Zero Checksum Alternate Error Detection Method: SCTP_OVER_DTLS (1)
			0x00, 0x00, 0x00, 0x01,
			// Max Outbound Streams: 15000, Max Inbound Streams: 2500
			0x3A, 0x98, 0x09, 0xC4
		};
		// clang-format on

		REQUIRE(RTC::SCTP::StateCookie::IsMediasoupStateCookie(buffer2, sizeof(buffer2)) == true);
		REQUIRE(!RTC::SCTP::StateCookie::Parse(buffer2, sizeof(buffer2)));
	}

	SECTION("StateCookie::Factory() succeeds")
	{
		RTC::SCTP::Capabilities remoteCapabilities = {
			.maxOutboundStreams  = 62000,
			.maxInboundStreams   = 55555,
			.partialReliability  = true,
			.messageInterleaving = true,
			.reConfig            = true,
			.zeroChecksumAlternateErrorDetectionMethod =
			  RTC::SCTP::ZeroChecksumAcceptableParameter::AlternateErrorDetectionMethod::NONE
		};

		auto* stateCookie = RTC::SCTP::StateCookie::Factory(
		  /*buffer*/ sctpCommon::FactoryBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::FactoryBuffer),
		  /*localVerificationTag*/ 6660666,
		  /*remoteVerificationTag*/ 9990999,
		  /*localInitialTsn*/ 1110111,
		  /*remoteInitialTsn*/ 2220222,
		  /*remoteAdvertisedReceiverWindowCredit*/ 999909999,
		  /*tieTag*/ 1111222233334444,
		  remoteCapabilities);

		// Change values of the original Capabilities to assert that it doesn't
		// affect the internals of StateCookie.
		remoteCapabilities.partialReliability = false;
		remoteCapabilities.maxOutboundStreams = 1024;

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

		const auto retrievedRemoteCapabilities = stateCookie->GetRemoteCapabilities();

		REQUIRE(retrievedRemoteCapabilities.maxOutboundStreams == 62000);
		REQUIRE(retrievedRemoteCapabilities.maxInboundStreams == 55555);
		REQUIRE(retrievedRemoteCapabilities.partialReliability == true);
		REQUIRE(retrievedRemoteCapabilities.messageInterleaving == true);
		REQUIRE(retrievedRemoteCapabilities.reConfig == true);
		REQUIRE(
		  retrievedRemoteCapabilities.zeroChecksumAlternateErrorDetectionMethod ==
		  RTC::SCTP::ZeroChecksumAcceptableParameter::AlternateErrorDetectionMethod::NONE);

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

		const auto retrievedParsedRemoteCapabilities = parsedStateCookie->GetRemoteCapabilities();

		REQUIRE(retrievedParsedRemoteCapabilities.maxOutboundStreams == 62000);
		REQUIRE(retrievedParsedRemoteCapabilities.maxInboundStreams == 55555);
		REQUIRE(retrievedParsedRemoteCapabilities.partialReliability == true);
		REQUIRE(retrievedParsedRemoteCapabilities.messageInterleaving == true);
		REQUIRE(retrievedParsedRemoteCapabilities.reConfig == true);
		REQUIRE(
		  retrievedParsedRemoteCapabilities.zeroChecksumAlternateErrorDetectionMethod ==
		  RTC::SCTP::ZeroChecksumAcceptableParameter::AlternateErrorDetectionMethod::NONE);

		delete parsedStateCookie;
	}

	SECTION("StateCookie::Write() succeeds")
	{
		RTC::SCTP::Capabilities remoteCapabilities = {
			.maxOutboundStreams  = 62000,
			.maxInboundStreams   = 55555,
			.partialReliability  = true,
			.messageInterleaving = true,
			.reConfig            = true,
			.zeroChecksumAlternateErrorDetectionMethod =
			  RTC::SCTP::ZeroChecksumAcceptableParameter::AlternateErrorDetectionMethod::NONE
		};

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
		  remoteCapabilities);

		// Change values of the original Capabilities to assert that it doesn't
		// affect the internals of StateCookie.
		remoteCapabilities.partialReliability = false;
		remoteCapabilities.maxOutboundStreams = 1024;

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

		const auto retrievedRemoteCapabilities = stateCookie->GetRemoteCapabilities();

		REQUIRE(retrievedRemoteCapabilities.maxOutboundStreams == 62000);
		REQUIRE(retrievedRemoteCapabilities.maxInboundStreams == 55555);
		REQUIRE(retrievedRemoteCapabilities.partialReliability == true);
		REQUIRE(retrievedRemoteCapabilities.messageInterleaving == true);
		REQUIRE(retrievedRemoteCapabilities.reConfig == true);
		REQUIRE(
		  retrievedRemoteCapabilities.zeroChecksumAlternateErrorDetectionMethod ==
		  RTC::SCTP::ZeroChecksumAcceptableParameter::AlternateErrorDetectionMethod::NONE);

		delete stateCookie;
	}

	SECTION("StateCookie::DetermineSctpImplementation() succeeds")
	{
		// usrsctp generated State Cookie.
		// clang-format off
		alignas(4) uint8_t buffer1[] =
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
		alignas(4) uint8_t buffer2[] =
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
		alignas(4) uint8_t buffer3[] =
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
		alignas(4) uint8_t buffer4[] =
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

	SECTION("authenticated StateCookie::Write() and StateCookie::VerifyMac() succeed")
	{
		const RTC::SCTP::Capabilities remoteCapabilities = {
			.maxOutboundStreams  = 62000,
			.maxInboundStreams   = 55555,
			.partialReliability  = true,
			.messageInterleaving = true,
			.reConfig            = true,
			.zeroChecksumAlternateErrorDetectionMethod =
			  RTC::SCTP::ZeroChecksumAcceptableParameter::AlternateErrorDetectionMethod::NONE
		};

		const uint8_t macKey[]             = { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
			                                     0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF };
		const uint64_t creationTimestampMs = 1234567890;

		auto* buffer = sctpCommon::FactoryBuffer;

		RTC::SCTP::StateCookie::Write(
		  /*buffer*/ buffer,
		  /*bufferLength*/ RTC::SCTP::StateCookie::AuthenticatedStateCookieLength,
		  /*localVerificationTag*/ 6660666,
		  /*remoteVerificationTag*/ 9990999,
		  /*localInitialTsn*/ 1110111,
		  /*remoteInitialTsn*/ 2220222,
		  /*remoteAdvertisedReceiverWindowCredit*/ 999909999,
		  /*tieTag*/ 1111222233334444,
		  remoteCapabilities,
		  /*creationTimestampMs*/ creationTimestampMs,
		  /*macKey*/ macKey,
		  /*macKeyLength*/ sizeof(macKey));

		/* It must be recognized as a (longer) mediasoup State Cookie. */

		REQUIRE(
		  RTC::SCTP::StateCookie::IsMediasoupStateCookie(
		    buffer, RTC::SCTP::StateCookie::AuthenticatedStateCookieLength) == true);

		/* Parse it. */

		auto* stateCookie =
		  RTC::SCTP::StateCookie::Parse(buffer, RTC::SCTP::StateCookie::AuthenticatedStateCookieLength);

		REQUIRE(stateCookie);
		REQUIRE(stateCookie->GetLength() == RTC::SCTP::StateCookie::AuthenticatedStateCookieLength);
		REQUIRE(stateCookie->IsAuthenticated() == true);
		REQUIRE(stateCookie->GetCreationTimestampMs() == creationTimestampMs);
		REQUIRE(stateCookie->GetLocalVerificationTag() == 6660666);
		REQUIRE(stateCookie->GetRemoteVerificationTag() == 9990999);

		/* The MAC must verify with the right key. */

		REQUIRE(
		  RTC::SCTP::StateCookie::VerifyMac(
		    buffer, RTC::SCTP::StateCookie::AuthenticatedStateCookieLength, macKey, sizeof(macKey)) ==
		  true);

		/* The MAC must NOT verify with a wrong key. */

		const uint8_t wrongMacKey[] = { 0xFF, 0xEE, 0xDD, 0xCC, 0xBB, 0xAA, 0x99, 0x88,
			                              0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11, 0x00 };

		REQUIRE(
		  RTC::SCTP::StateCookie::VerifyMac(
		    buffer,
		    RTC::SCTP::StateCookie::AuthenticatedStateCookieLength,
		    wrongMacKey,
		    sizeof(wrongMacKey)) == false);

		/* Tampering with any byte must invalidate the MAC. */

		buffer[8] ^= 0x01;

		REQUIRE(
		  RTC::SCTP::StateCookie::VerifyMac(
		    buffer, RTC::SCTP::StateCookie::AuthenticatedStateCookieLength, macKey, sizeof(macKey)) ==
		  false);

		buffer[8] ^= 0x01;

		/* It verifies again once restored. */

		REQUIRE(
		  RTC::SCTP::StateCookie::VerifyMac(
		    buffer, RTC::SCTP::StateCookie::AuthenticatedStateCookieLength, macKey, sizeof(macKey)) ==
		  true);

		delete stateCookie;
	}

	SECTION("StateCookie::VerifyMac() fails on a non-authenticated (plain) cookie")
	{
		const RTC::SCTP::Capabilities remoteCapabilities = {
			.maxOutboundStreams  = 62000,
			.maxInboundStreams   = 55555,
			.partialReliability  = true,
			.messageInterleaving = true,
			.reConfig            = true,
			.zeroChecksumAlternateErrorDetectionMethod =
			  RTC::SCTP::ZeroChecksumAcceptableParameter::AlternateErrorDetectionMethod::NONE
		};

		const uint8_t macKey[] = { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77 };

		auto* buffer = sctpCommon::FactoryBuffer;

		// Write a plain (48 bytes) cookie.
		RTC::SCTP::StateCookie::Write(
		  /*buffer*/ buffer,
		  /*bufferLength*/ RTC::SCTP::StateCookie::StateCookieLength,
		  /*localVerificationTag*/ 6660666,
		  /*remoteVerificationTag*/ 9990999,
		  /*localInitialTsn*/ 1110111,
		  /*remoteInitialTsn*/ 2220222,
		  /*remoteAdvertisedReceiverWindowCredit*/ 999909999,
		  /*tieTag*/ 1111222233334444,
		  remoteCapabilities);

		// A plain cookie has no MAC, so `VerifyMac()` must fail regardless of the
		// key.
		REQUIRE(
		  RTC::SCTP::StateCookie::VerifyMac(
		    buffer, RTC::SCTP::StateCookie::StateCookieLength, macKey, sizeof(macKey)) == false);
	}
}
