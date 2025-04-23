#define MS_CLASS "RTC::SCTP::ForwardTsnSupportedChunkParameter"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/chunkParameters/ForwardTsnSupportedChunkParameter.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"

namespace RTC
{
	namespace SCTP
	{
		/* Class methods. */

		ForwardTsnSupportedChunkParameter* ForwardTsnSupportedChunkParameter::Parse(
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

			if (parameterType != ChunkParameter::ChunkParameterType::FORWARD_TSN_SUPPORTED)
			{
				MS_WARN_DEV("invalid Chunk Parameter type");

				return nullptr;
			}

			return ForwardTsnSupportedChunkParameter::ParseStrict(
			  buffer, bufferLength, parameterLength, padding);
		}

		ForwardTsnSupportedChunkParameter* ForwardTsnSupportedChunkParameter::Factory(
		  uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			if (bufferLength < ChunkParameter::ChunkParameterHeaderLength)
			{
				MS_THROW_TYPE_ERROR("buffer too small");
			}

			auto* parameter = new ForwardTsnSupportedChunkParameter(buffer, bufferLength);

			parameter->InitializeHeader(
			  ChunkParameter::ChunkParameterType::FORWARD_TSN_SUPPORTED,
			  ChunkParameter::ChunkParameterHeaderLength);

			// No need to invoke SetLength() since parent constructor invoked it.

			return parameter;
		}

		ForwardTsnSupportedChunkParameter* ForwardTsnSupportedChunkParameter::ParseStrict(
		  const uint8_t* buffer, size_t bufferLength, uint16_t parameterLength, uint8_t padding)
		{
			MS_TRACE();

			if (parameterLength != ChunkParameter::ChunkParameterHeaderLength)
			{
				MS_WARN_TAG(
				  sctp,
				  "ForwardTsnSupportedChunkParameter Length field must be %zu",
				  ChunkParameter::ChunkParameterHeaderLength);

				return nullptr;
			}

			auto* parameter =
			  new ForwardTsnSupportedChunkParameter(const_cast<uint8_t*>(buffer), bufferLength);

			// Mark the Parameter as frozen since we are parsing.
			parameter->Freeze();

			return parameter;
		}

		/* Instance methods. */

		ForwardTsnSupportedChunkParameter::ForwardTsnSupportedChunkParameter(
		  uint8_t* buffer, size_t bufferLength)
		  : ChunkParameter(buffer, bufferLength)
		{
			MS_TRACE();

			SetLength(ChunkParameter::ChunkParameterHeaderLength);
		}

		ForwardTsnSupportedChunkParameter::~ForwardTsnSupportedChunkParameter()
		{
			MS_TRACE();
		}

		void ForwardTsnSupportedChunkParameter::Dump(int indentation) const
		{
			MS_TRACE();

			MS_DUMP_CLEAN(indentation, "<SCTP::ForwardTsnSupportedChunkParameter>");
			DumpCommon(indentation);
			MS_DUMP_CLEAN(indentation, "</SCTP::ForwardTsnSupportedChunkParameter>");
		}

		ForwardTsnSupportedChunkParameter* ForwardTsnSupportedChunkParameter::Clone(
		  uint8_t* buffer, size_t bufferLength) const
		{
			MS_TRACE();

			auto* clonedParameter = new ForwardTsnSupportedChunkParameter(buffer, bufferLength);

			CloneInto(clonedParameter);

			return clonedParameter;
		}

		ForwardTsnSupportedChunkParameter* ForwardTsnSupportedChunkParameter::SoftClone(
		  const uint8_t* buffer) const
		{
			MS_TRACE();

			auto* softClonedParameter =
			  new ForwardTsnSupportedChunkParameter(const_cast<uint8_t*>(buffer), GetLength());

			SoftCloneInto(softClonedParameter);

			return softClonedParameter;
		}
	} // namespace SCTP
} // namespace RTC
