#include "common.hpp"
#include "MediaSoupErrors.hpp"
#include "RTC/SCTP/packet/chunks/AbortAssociationChunk.hpp"
#include "RTC/SCTP/packet/chunks/InitChunk.hpp"
#include "RTC/SCTP/packet/errorCauses/OutOfResourceErrorCause.hpp"
#include "RTC/SCTP/packet/errorCauses/ProtocolViolationErrorCause.hpp"
#include "RTC/SCTP/packet/parameters/ForwardTsnSupportedParameter.hpp"
#include "RTC/SCTP/packet/parameters/ZeroChecksumAcceptableParameter.hpp"
#include "RTC/SCTP/sctpCommon.hpp"
#include <catch2/catch_test_macros.hpp>

SCENARIO("SCTP Chunk", "[serializable][sctp][chunk]")
{
	sctpCommon::ResetBuffers();

	SECTION("alignof() SCTP structs")
	{
		REQUIRE(alignof(RTC::SCTP::Chunk::ChunkHeader) == 2);
		REQUIRE(alignof(RTC::SCTP::Chunk::ChunkFlags) == 1);
	}

	SECTION("BuildParameterInPlace() and AddParameter() throw if the Chunk needs consolidation")
	{
		std::unique_ptr<RTC::SCTP::InitChunk> chunk{ RTC::SCTP::InitChunk::Factory(
			sctpCommon::FactoryBuffer, 1000) };

		REQUIRE(chunk->NeedsConsolidation() == false);

		const auto* parameter1 = chunk->BuildParameterInPlace<RTC::SCTP::ForwardTsnSupportedParameter>();

		REQUIRE(chunk->NeedsConsolidation() == true);

		// We didn't call parameter1->Consolidate() yet so this must throw.
		REQUIRE_THROWS_AS(
		  chunk->BuildParameterInPlace<RTC::SCTP::ZeroChecksumAcceptableParameter>(), MediaSoupError);

		const auto* parameter2 = RTC::SCTP::ZeroChecksumAcceptableParameter::Factory(
		  sctpCommon::FactoryBuffer + 1000, sizeof(sctpCommon::FactoryBuffer));

		// We didn't call parameter1->Consolidate() yet so this must throw.
		REQUIRE_THROWS_AS(chunk->AddParameter(parameter2), MediaSoupError);

		delete parameter2;

		parameter1->Consolidate();

		REQUIRE(chunk->NeedsConsolidation() == false);

		// This shouldn't throw now.
		const auto* parameter3 =
		  chunk->BuildParameterInPlace<RTC::SCTP::ZeroChecksumAcceptableParameter>();

		REQUIRE(chunk->NeedsConsolidation() == true);

		parameter3->Consolidate();

		REQUIRE(chunk->NeedsConsolidation() == false);

		const auto* parameter4 = RTC::SCTP::ZeroChecksumAcceptableParameter::Factory(
		  sctpCommon::FactoryBuffer + 1000, sizeof(sctpCommon::FactoryBuffer));

		// This shouldn't throw now.
		chunk->AddParameter(parameter4);

		REQUIRE(chunk->NeedsConsolidation() == false);

		delete parameter4;
	}

	SECTION("BuildErrorCauseInPlace() and AddErrorCause() throw if the Chunk needs consolidation")
	{
		std::unique_ptr<RTC::SCTP::AbortAssociationChunk> chunk{
			RTC::SCTP::AbortAssociationChunk::Factory(sctpCommon::FactoryBuffer, 1000)
		};

		REQUIRE(chunk->NeedsConsolidation() == false);

		const auto* errorCause1 = chunk->BuildErrorCauseInPlace<RTC::SCTP::OutOfResourceErrorCause>();

		REQUIRE(chunk->NeedsConsolidation() == true);

		// We didn't call errorCause1->Consolidate() yet so this must throw.
		REQUIRE_THROWS_AS(
		  chunk->BuildErrorCauseInPlace<RTC::SCTP::ProtocolViolationErrorCause>(), MediaSoupError);

		const auto* errorCause2 = RTC::SCTP::ProtocolViolationErrorCause::Factory(
		  sctpCommon::FactoryBuffer + 1000, sizeof(sctpCommon::FactoryBuffer));

		// We didn't call errorCause1->Consolidate() yet so this must throw.
		REQUIRE_THROWS_AS(chunk->AddErrorCause(errorCause2), MediaSoupError);

		delete errorCause2;

		errorCause1->Consolidate();

		REQUIRE(chunk->NeedsConsolidation() == false);

		// This shouldn't throw now.
		const auto* errorCause3 = chunk->BuildErrorCauseInPlace<RTC::SCTP::ProtocolViolationErrorCause>();

		REQUIRE(chunk->NeedsConsolidation() == true);

		errorCause3->Consolidate();

		REQUIRE(chunk->NeedsConsolidation() == false);

		const auto* errorCause4 = RTC::SCTP::ProtocolViolationErrorCause::Factory(
		  sctpCommon::FactoryBuffer + 1000, sizeof(sctpCommon::FactoryBuffer));

		// This shouldn't throw now.
		chunk->AddErrorCause(errorCause4);

		REQUIRE(chunk->NeedsConsolidation() == false);

		delete errorCause4;
	}
}
