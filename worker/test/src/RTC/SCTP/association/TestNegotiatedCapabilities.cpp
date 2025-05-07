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
		socketOptions.zeroCheksumAlternateErrorDetectionMethod =
		  ZeroChecksumAcceptableParameter::AlternateErrorDetectionMethod::SCTP_OVER_DTLS;

		uint16_t peerNumberOfOutboundStreams = 4096;
		uint16_t peerNumberOfInboundStreams  = 1024;

		auto* peerChunk = InitChunk::Factory(FactoryBuffer, sizeof(FactoryBuffer));

		auto* peerSupportedExtensionsParameter =
		  peerChunk->BuildParameterInPlace<SupportedExtensionsParameter>();

		peerSupportedExtensionsParameter->AddChunkType(Chunk::ChunkType::FORWARD_TSN);
		peerSupportedExtensionsParameter->AddChunkType(Chunk::ChunkType::RE_CONFIG);
		peerSupportedExtensionsParameter->AddChunkType(Chunk::ChunkType::I_DATA);
		peerSupportedExtensionsParameter->AddChunkType(Chunk::ChunkType::I_FORWARD_TSN);
		peerSupportedExtensionsParameter->Consolidate();

		auto* peerZeroChecksumAcceptableParameter =
		  peerChunk->BuildParameterInPlace<ZeroChecksumAcceptableParameter>();

		peerZeroChecksumAcceptableParameter->SetAlternateErrorDetectionMethod(
		  ZeroChecksumAcceptableParameter::AlternateErrorDetectionMethod::SCTP_OVER_DTLS);
		peerZeroChecksumAcceptableParameter->Consolidate();

		auto negotiatedCapabilities = NegotiatedCapabilities::Factory(
		  socketOptions, peerNumberOfOutboundStreams, peerNumberOfInboundStreams, peerChunk);

		delete peerChunk;

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
		socketOptions.zeroCheksumAlternateErrorDetectionMethod =
		  ZeroChecksumAcceptableParameter::AlternateErrorDetectionMethod::SCTP_OVER_DTLS;

		uint16_t peerNumberOfOutboundStreams = 4000;
		uint16_t peerNumberOfInboundStreams  = 3000;

		auto* peerChunk = InitChunk::Factory(FactoryBuffer, sizeof(FactoryBuffer));

		auto* peerSupportedExtensionsParameter =
		  peerChunk->BuildParameterInPlace<SupportedExtensionsParameter>();

		// NOTE: Missing FORWARD_TSN, but peer announced support for it via
		// Forward-TSN-Supported Parameter.
		// negotiation).
		// NOTE: Missing RE_CONFIG (needed for Partial Reliability Extension
		// negotiation).
		// NOTE: Missing I_FORWARD_TSN (needed for Message Interleaving negotiation).
		peerSupportedExtensionsParameter->AddChunkType(Chunk::ChunkType::I_DATA);
		peerSupportedExtensionsParameter->Consolidate();

		auto* peerForwardTsnSupportedParameter =
		  peerChunk->BuildParameterInPlace<ForwardTsnSupportedParameter>();

		peerForwardTsnSupportedParameter->Consolidate();

		auto* peerZeroChecksumAcceptableParameter =
		  peerChunk->BuildParameterInPlace<ZeroChecksumAcceptableParameter>();

		peerZeroChecksumAcceptableParameter->SetAlternateErrorDetectionMethod(
		  static_cast<ZeroChecksumAcceptableParameter::AlternateErrorDetectionMethod>(666));
		peerZeroChecksumAcceptableParameter->Consolidate();

		auto negotiatedCapabilities = NegotiatedCapabilities::Factory(
		  socketOptions, peerNumberOfOutboundStreams, peerNumberOfInboundStreams, peerChunk);

		delete peerChunk;

		REQUIRE(negotiatedCapabilities.maxOutboundStreams == 1000);
		REQUIRE(negotiatedCapabilities.maxInboundStreams == 2000);
		REQUIRE(negotiatedCapabilities.partialReliability == true);
		REQUIRE(negotiatedCapabilities.messageInterleaving == false);
		REQUIRE(negotiatedCapabilities.reconfig == false);
		REQUIRE(negotiatedCapabilities.zeroChecksum == false);
	}
}
