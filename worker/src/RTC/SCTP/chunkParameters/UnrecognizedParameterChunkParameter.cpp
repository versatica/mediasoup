#define MS_CLASS "RTC::SCTP::UnrecognizedParameterChunkParameter"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/chunkParameters/UnrecognizedParameterChunkParameter.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"

namespace RTC
{
	namespace SCTP
	{
		/* Class methods. */

		UnrecognizedParameterChunkParameter* UnrecognizedParameterChunkParameter::Parse(
		  const uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			ChunkParameter::ChunkParameterType parameterType;
			uint16_t parameterLength;
			uint8_t padding;

			if (!ChunkParameter::IsChunkParameter(
			      buffer, bufferLength, parameterType, parameterLength, padding))
			{
				return nullptr;
			}

			if (parameterType != ChunkParameter::ChunkParameterType::UNRECOGNIZED_PARAMETER)
			{
				MS_WARN_DEV("invalid Chunk Parameter type");

				return nullptr;
			}

			return UnrecognizedParameterChunkParameter::ParseStrict(
			  buffer, bufferLength, parameterLength, padding);
		}

		UnrecognizedParameterChunkParameter* UnrecognizedParameterChunkParameter::Factory(
		  uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			if (bufferLength < ChunkParameter::ChunkParameterHeaderLength)
			{
				MS_THROW_TYPE_ERROR("buffer too small");
			}

			auto* parameter = new UnrecognizedParameterChunkParameter(buffer, bufferLength);

			parameter->InitializeHeader(
			  ChunkParameter::ChunkParameterType::UNRECOGNIZED_PARAMETER,
			  ChunkParameter::ChunkParameterHeaderLength);

			// No need to invoke SetLength() since parent constructor invoked it.

			return parameter;
		}

		UnrecognizedParameterChunkParameter* UnrecognizedParameterChunkParameter::ParseStrict(
		  const uint8_t* buffer, size_t bufferLength, uint16_t parameterLength, uint8_t padding)
		{
			MS_TRACE();

			auto* parameter =
			  new UnrecognizedParameterChunkParameter(const_cast<uint8_t*>(buffer), bufferLength);

			// Must always invoke SetLength() after constructing a Serializable with
			// not fixed length.
			parameter->SetLength(parameterLength + padding);

			// Mark the Parameter as frozen since we are parsing.
			parameter->Freeze();

			return parameter;
		}

		/* Instance methods. */

		UnrecognizedParameterChunkParameter::UnrecognizedParameterChunkParameter(
		  uint8_t* buffer, size_t bufferLength)
		  : ChunkParameter(buffer, bufferLength)
		{
			MS_TRACE();

			SetLength(ChunkParameter::ChunkParameterHeaderLength);
		}

		UnrecognizedParameterChunkParameter::~UnrecognizedParameterChunkParameter()
		{
			MS_TRACE();
		}

		void UnrecognizedParameterChunkParameter::Dump(int indentation) const
		{
			MS_TRACE();

			MS_DUMP_CLEAN(indentation, "<SCTP::UnrecognizedParameterChunkParameter>");
			DumpCommon(indentation);
			MS_DUMP_CLEAN(
			  indentation,
			  "  unrecognized parameter length: %" PRIu16 " (has unrecognized parameter: %s)",
			  GetUnrecognizedParameterLength(),
			  HasUnrecognizedParameter() ? "yes" : "no");
			MS_DUMP_CLEAN(indentation, "</SCTP::UnrecognizedParameterChunkParameter>");
		}

		UnrecognizedParameterChunkParameter* UnrecognizedParameterChunkParameter::Clone(
		  uint8_t* buffer, size_t bufferLength) const
		{
			MS_TRACE();

			auto* clonedParameter = new UnrecognizedParameterChunkParameter(buffer, bufferLength);

			CloneInto(clonedParameter);

			return clonedParameter;
		}

		void UnrecognizedParameterChunkParameter::SetUnrecognizedParameter(
		  const uint8_t* parameter, uint16_t parameterLength)
		{
			MS_TRACE();

			AssertNotFrozen();

			SetVariableLengthValue(parameter, parameterLength);
		}

		UnrecognizedParameterChunkParameter* UnrecognizedParameterChunkParameter::SoftClone(
		  const uint8_t* buffer) const
		{
			MS_TRACE();

			auto* softClonedParameter =
			  new UnrecognizedParameterChunkParameter(const_cast<uint8_t*>(buffer), GetLength());

			SoftCloneInto(softClonedParameter);

			return softClonedParameter;
		}
	} // namespace SCTP
} // namespace RTC
