#include "common.hpp"
#include "RTC/SCTP/association/Capabilities.hpp"
#include "RTC/SCTP/packet/Chunk.hpp"
#include "RTC/SCTP/packet/chunks/InitChunk.hpp"
#include "RTC/SCTP/packet/parameters/ForwardTsnSupportedParameter.hpp"
#include "RTC/SCTP/packet/parameters/SupportedExtensionsParameter.hpp"
#include "RTC/SCTP/packet/parameters/ZeroChecksumAcceptableParameter.hpp"
#include "test/include/RTC/SCTP/sctpCommon.hpp"
#include <catch2/catch_test_macros.hpp>

SCENARIO("SCTP Capabilities", "[sctp][capabilities]")
{
	sctpCommon::ResetBuffers();

	SECTION("Capabilities::Factory() succeeds (1)")
	{
		const std::unique_ptr<RTC::SCTP::InitChunk> remoteChunk{ RTC::SCTP::InitChunk::Factory(
			sctpCommon::FactoryBuffer, sizeof(sctpCommon::FactoryBuffer)) };

		remoteChunk->SetNumberOfOutboundStreams(4096);
		remoteChunk->SetNumberOfInboundStreams(1024);

		auto* remoteSupportedExtensionsParameter =
		  remoteChunk->BuildParameterInPlace<RTC::SCTP::SupportedExtensionsParameter>();

		remoteSupportedExtensionsParameter->AddChunkType(RTC::SCTP::Chunk::ChunkType::FORWARD_TSN);
		remoteSupportedExtensionsParameter->AddChunkType(RTC::SCTP::Chunk::ChunkType::RE_CONFIG);
		remoteSupportedExtensionsParameter->AddChunkType(RTC::SCTP::Chunk::ChunkType::I_DATA);
		remoteSupportedExtensionsParameter->AddChunkType(RTC::SCTP::Chunk::ChunkType::I_FORWARD_TSN);
		remoteSupportedExtensionsParameter->Consolidate();

		auto* remoteZeroChecksumAcceptableParameter =
		  remoteChunk->BuildParameterInPlace<RTC::SCTP::ZeroChecksumAcceptableParameter>();

		remoteZeroChecksumAcceptableParameter->SetAlternateErrorDetectionMethod(
		  RTC::SCTP::ZeroChecksumAcceptableParameter::AlternateErrorDetectionMethod::SCTP_OVER_DTLS);
		remoteZeroChecksumAcceptableParameter->Consolidate();

		const auto remoteCapabilities = RTC::SCTP::Capabilities::Factory(remoteChunk.get());

		// Raw remote-announced values (no local option is applied).
		REQUIRE(remoteCapabilities.maxOutboundStreams == 4096);
		REQUIRE(remoteCapabilities.maxInboundStreams == 1024);
		REQUIRE(remoteCapabilities.partialReliability == true);
		REQUIRE(remoteCapabilities.messageInterleaving == true);
		REQUIRE(remoteCapabilities.reConfig == true);
		REQUIRE(
		  remoteCapabilities.zeroChecksumAlternateErrorDetectionMethod ==
		  RTC::SCTP::ZeroChecksumAcceptableParameter::AlternateErrorDetectionMethod::SCTP_OVER_DTLS);
	}

	SECTION("Capabilities::Factory() succeeds (2)")
	{
		const std::unique_ptr<RTC::SCTP::InitChunk> remoteChunk{ RTC::SCTP::InitChunk::Factory(
			sctpCommon::FactoryBuffer, sizeof(sctpCommon::FactoryBuffer)) };

		remoteChunk->SetNumberOfOutboundStreams(4000);
		remoteChunk->SetNumberOfInboundStreams(3000);

		auto* remoteSupportedExtensionsParameter =
		  remoteChunk->BuildParameterInPlace<RTC::SCTP::SupportedExtensionsParameter>();

		// NOTE: Missing FORWARD-TSN, but remote announces support for it via
		// Forward-TSN-Supported parameter.
		// NOTE: Missing RE-CONFIG (needed for Stream Re-Configuration).
		// NOTE: Missing I-FORWARD-TSN (needed for Message Interleaving).
		remoteSupportedExtensionsParameter->AddChunkType(RTC::SCTP::Chunk::ChunkType::I_DATA);
		remoteSupportedExtensionsParameter->Consolidate();

		auto* remoteForwardTsnSupportedParameter =
		  remoteChunk->BuildParameterInPlace<RTC::SCTP::ForwardTsnSupportedParameter>();

		remoteForwardTsnSupportedParameter->Consolidate();

		auto* remoteZeroChecksumAcceptableParameter =
		  remoteChunk->BuildParameterInPlace<RTC::SCTP::ZeroChecksumAcceptableParameter>();

		remoteZeroChecksumAcceptableParameter->SetAlternateErrorDetectionMethod(
		  // NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange)
		  static_cast<RTC::SCTP::ZeroChecksumAcceptableParameter::AlternateErrorDetectionMethod>(666));
		remoteZeroChecksumAcceptableParameter->Consolidate();

		const auto remoteCapabilities = RTC::SCTP::Capabilities::Factory(remoteChunk.get());

		// Raw remote-announced values (no local option is applied).
		REQUIRE(remoteCapabilities.maxOutboundStreams == 4000);
		REQUIRE(remoteCapabilities.maxInboundStreams == 3000);
		// Announced via Forward-TSN-Supported parameter.
		REQUIRE(remoteCapabilities.partialReliability == true);
		// Not announced (missing I-FORWARD-TSN).
		REQUIRE(remoteCapabilities.messageInterleaving == false);
		// Not announced (missing RE-CONFIG).
		REQUIRE(remoteCapabilities.reConfig == false);
		// Unknown/invalid method is parsed as NONE.
		REQUIRE(
		  remoteCapabilities.zeroChecksumAlternateErrorDetectionMethod ==
		  RTC::SCTP::ZeroChecksumAcceptableParameter::AlternateErrorDetectionMethod::NONE);
	}
}
