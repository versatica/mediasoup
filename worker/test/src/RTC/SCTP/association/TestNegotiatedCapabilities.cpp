#include "common.hpp"
#include "RTC/SCTP/association/NegotiatedCapabilities.hpp"
#include "RTC/SCTP/association/SocketOptions.hpp"
#include "RTC/SCTP/common.hpp" // in worker/test/include/
#include "RTC/SCTP/packet/Chunk.hpp"
#include "RTC/SCTP/packet/chunks/InitChunk.hpp"
#include "RTC/SCTP/packet/parameters/ForwardTsnSupportedParameter.hpp"
#include "RTC/SCTP/packet/parameters/SupportedExtensionsParameter.hpp"
#include "RTC/SCTP/packet/parameters/ZeroChecksumAcceptableParameter.hpp"
#include <catch2/catch_test_macros.hpp>

using namespace RTC::SCTP;

SCENARIO("SCTP Negotiated Capabilities", "[sctp]")
{
	resetBuffers();

	SECTION("NegotiatedCapabilities::Factory() succeeds (1)")
	{
		SocketOptions socketOptions{};

		socketOptions.maxOutboundStreams  = 8192;
		socketOptions.maxInboundStreams   = 2048;
		socketOptions.partialReliability  = true;
		socketOptions.messageInterleaving = true;
		socketOptions.zeroChecksumAlternateErrorDetectionMethod =
		  ZeroChecksumAcceptableParameter::AlternateErrorDetectionMethod::SCTP_OVER_DTLS;

		auto* remoteChunk = InitChunk::Factory(FactoryBuffer, sizeof(FactoryBuffer));

		remoteChunk->SetNumberOfOutboundStreams(4096);
		remoteChunk->SetNumberOfInboundStreams(1024);

		auto* remoteSupportedExtensionsParameter =
		  remoteChunk->BuildParameterInPlace<SupportedExtensionsParameter>();

		remoteSupportedExtensionsParameter->AddChunkType(Chunk::ChunkType::FORWARD_TSN);
		remoteSupportedExtensionsParameter->AddChunkType(Chunk::ChunkType::RE_CONFIG);
		remoteSupportedExtensionsParameter->AddChunkType(Chunk::ChunkType::I_DATA);
		remoteSupportedExtensionsParameter->AddChunkType(Chunk::ChunkType::I_FORWARD_TSN);
		remoteSupportedExtensionsParameter->Consolidate();

		auto* remoteZeroChecksumAcceptableParameter =
		  remoteChunk->BuildParameterInPlace<ZeroChecksumAcceptableParameter>();

		remoteZeroChecksumAcceptableParameter->SetAlternateErrorDetectionMethod(
		  ZeroChecksumAcceptableParameter::AlternateErrorDetectionMethod::SCTP_OVER_DTLS);
		remoteZeroChecksumAcceptableParameter->Consolidate();

		auto negotiatedCapabilities = NegotiatedCapabilities::Factory(socketOptions, remoteChunk);

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
		SocketOptions socketOptions{};

		socketOptions.maxOutboundStreams  = 1000;
		socketOptions.maxInboundStreams   = 2000;
		socketOptions.partialReliability  = true;
		socketOptions.messageInterleaving = true;
		socketOptions.zeroChecksumAlternateErrorDetectionMethod =
		  ZeroChecksumAcceptableParameter::AlternateErrorDetectionMethod::SCTP_OVER_DTLS;

		auto* remoteChunk = InitChunk::Factory(FactoryBuffer, sizeof(FactoryBuffer));

		remoteChunk->SetNumberOfOutboundStreams(4000);
		remoteChunk->SetNumberOfInboundStreams(3000);

		auto* remoteSupportedExtensionsParameter =
		  remoteChunk->BuildParameterInPlace<SupportedExtensionsParameter>();

		// NOTE: Missing FORWARD_TSN, but peer announced support for it via
		// Forward-TSN-Supported Parameter negotiation).
		// NOTE: Missing RE_CONFIG (needed for Partial Reliability Extension
		// negotiation).
		// NOTE: Missing I_FORWARD_TSN (needed for Message Interleaving negotiation).
		remoteSupportedExtensionsParameter->AddChunkType(Chunk::ChunkType::I_DATA);
		remoteSupportedExtensionsParameter->Consolidate();

		auto* remoteForwardTsnSupportedParameter =
		  remoteChunk->BuildParameterInPlace<ForwardTsnSupportedParameter>();

		remoteForwardTsnSupportedParameter->Consolidate();

		auto* remoteZeroChecksumAcceptableParameter =
		  remoteChunk->BuildParameterInPlace<ZeroChecksumAcceptableParameter>();

		remoteZeroChecksumAcceptableParameter->SetAlternateErrorDetectionMethod(
		  static_cast<ZeroChecksumAcceptableParameter::AlternateErrorDetectionMethod>(666));
		remoteZeroChecksumAcceptableParameter->Consolidate();

		auto negotiatedCapabilities = NegotiatedCapabilities::Factory(socketOptions, remoteChunk);

		delete remoteChunk;

		REQUIRE(negotiatedCapabilities.maxOutboundStreams == 1000);
		REQUIRE(negotiatedCapabilities.maxInboundStreams == 2000);
		REQUIRE(negotiatedCapabilities.partialReliability == true);
		REQUIRE(negotiatedCapabilities.messageInterleaving == false);
		REQUIRE(negotiatedCapabilities.reconfig == false);
		REQUIRE(negotiatedCapabilities.zeroChecksum == false);
	}
}
