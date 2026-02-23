#include "common.hpp"
#include "RTC/SCTP/NegotiatedCapabilities.hpp"
#include "RTC/SCTP/SocketOptions.hpp"
#include "RTC/SCTP/packet/Chunk.hpp"
#include "RTC/SCTP/packet/chunks/InitChunk.hpp"
#include "RTC/SCTP/packet/parameters/ForwardTsnSupportedParameter.hpp"
#include "RTC/SCTP/packet/parameters/SupportedExtensionsParameter.hpp"
#include "RTC/SCTP/packet/parameters/ZeroChecksumAcceptableParameter.hpp"
#include "RTC/SCTP/sctpCommon.hpp"
#include <catch2/catch_test_macros.hpp>

SCENARIO("SCTP Negotiated Capabilities", "[sctp][negotiatedcapabilities]")
{
	sctpCommon::ResetBuffers();

	SECTION("NegotiatedCapabilities::Factory() succeeds (1)")
	{
		RTC::SCTP::SocketOptions socketOptions{};

		socketOptions.maxOutboundStreams        = 8192;
		socketOptions.maxInboundStreams         = 2048;
		socketOptions.enablePartialReliability  = true;
		socketOptions.enableMessageInterleaving = true;
		socketOptions.zeroChecksumAlternateErrorDetectionMethod =
		  RTC::SCTP::ZeroChecksumAcceptableParameter::AlternateErrorDetectionMethod::SCTP_OVER_DTLS;

		auto* remoteChunk =
		  RTC::SCTP::InitChunk::Factory(sctpCommon::FactoryBuffer, sizeof(sctpCommon::FactoryBuffer));

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

		auto negotiatedCapabilities =
		  RTC::SCTP::NegotiatedCapabilities::Factory(socketOptions, remoteChunk);

		delete remoteChunk;

		REQUIRE(negotiatedCapabilities.maxOutboundStreams == 1024);
		REQUIRE(negotiatedCapabilities.maxInboundStreams == 2048);
		REQUIRE(negotiatedCapabilities.partialReliability == true);
		REQUIRE(negotiatedCapabilities.messageInterleaving == true);
		REQUIRE(negotiatedCapabilities.reconfig == true);
		REQUIRE(negotiatedCapabilities.zeroChecksum == true);
	}

	SECTION("NegotiatedCapabilities::Factory() succeeds (2)")
	{
		RTC::SCTP::SocketOptions socketOptions{};

		socketOptions.maxOutboundStreams        = 1000;
		socketOptions.maxInboundStreams         = 2000;
		socketOptions.enablePartialReliability  = true;
		socketOptions.enableMessageInterleaving = true;
		socketOptions.zeroChecksumAlternateErrorDetectionMethod =
		  RTC::SCTP::ZeroChecksumAcceptableParameter::AlternateErrorDetectionMethod::SCTP_OVER_DTLS;

		auto* remoteChunk =
		  RTC::SCTP::InitChunk::Factory(sctpCommon::FactoryBuffer, sizeof(sctpCommon::FactoryBuffer));

		remoteChunk->SetNumberOfOutboundStreams(4000);
		remoteChunk->SetNumberOfInboundStreams(3000);

		auto* remoteSupportedExtensionsParameter =
		  remoteChunk->BuildParameterInPlace<RTC::SCTP::SupportedExtensionsParameter>();

		// NOTE: Missing FORWARD_TSN, but peer announced support for it via
		// Forward-TSN-Supported Parameter negotiation).
		// NOTE: Missing RE_CONFIG (needed for Partial Reliability Extension
		// negotiation).
		// NOTE: Missing I_FORWARD_TSN (needed for Message Interleaving negotiation).
		remoteSupportedExtensionsParameter->AddChunkType(RTC::SCTP::Chunk::ChunkType::I_DATA);
		remoteSupportedExtensionsParameter->Consolidate();

		auto* remoteForwardTsnSupportedParameter =
		  remoteChunk->BuildParameterInPlace<RTC::SCTP::ForwardTsnSupportedParameter>();

		remoteForwardTsnSupportedParameter->Consolidate();

		auto* remoteZeroChecksumAcceptableParameter =
		  remoteChunk->BuildParameterInPlace<RTC::SCTP::ZeroChecksumAcceptableParameter>();

		remoteZeroChecksumAcceptableParameter->SetAlternateErrorDetectionMethod(
		  static_cast<RTC::SCTP::ZeroChecksumAcceptableParameter::AlternateErrorDetectionMethod>(666));
		remoteZeroChecksumAcceptableParameter->Consolidate();

		auto negotiatedCapabilities =
		  RTC::SCTP::NegotiatedCapabilities::Factory(socketOptions, remoteChunk);

		delete remoteChunk;

		REQUIRE(negotiatedCapabilities.maxOutboundStreams == 1000);
		REQUIRE(negotiatedCapabilities.maxInboundStreams == 2000);
		REQUIRE(negotiatedCapabilities.partialReliability == true);
		REQUIRE(negotiatedCapabilities.messageInterleaving == false);
		REQUIRE(negotiatedCapabilities.reconfig == false);
		REQUIRE(negotiatedCapabilities.zeroChecksum == false);
	}
}
