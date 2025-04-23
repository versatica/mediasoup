#define MS_CLASS "RTC::SCTP::SupportedExtensionsChunkParameter"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/chunkParameters/SupportedExtensionsChunkParameter.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"

namespace RTC
{
	namespace SCTP
	{
		/* Class methods. */

		SupportedExtensionsChunkParameter* SupportedExtensionsChunkParameter::Parse(
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

			if (parameterType != ChunkParameter::ChunkParameterType::SUPPORTED_EXTENSIONS)
			{
				MS_WARN_DEV("invalid Chunk Parameter type");

				return nullptr;
			}

			return SupportedExtensionsChunkParameter::ParseStrict(
			  buffer, bufferLength, parameterLength, padding);
		}

		SupportedExtensionsChunkParameter* SupportedExtensionsChunkParameter::Factory(
		  uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			if (bufferLength < ChunkParameter::ChunkParameterHeaderLength)
			{
				MS_THROW_TYPE_ERROR("buffer too small");
			}

			auto* parameter = new SupportedExtensionsChunkParameter(buffer, bufferLength);

			parameter->InitializeHeader(
			  ChunkParameter::ChunkParameterType::SUPPORTED_EXTENSIONS,
			  ChunkParameter::ChunkParameterHeaderLength);

			// No need to invoke SetLength() since parent constructor invoked it.

			return parameter;
		}

		SupportedExtensionsChunkParameter* SupportedExtensionsChunkParameter::ParseStrict(
		  const uint8_t* buffer, size_t bufferLength, uint16_t parameterLength, uint8_t padding)
		{
			MS_TRACE();

			auto* parameter =
			  new SupportedExtensionsChunkParameter(const_cast<uint8_t*>(buffer), bufferLength);

			// Must always invoke SetLength() after constructing a Serializable with
			// not fixed length.
			parameter->SetLength(parameterLength + padding);

			// Mark the Parameter as frozen since we are parsing.
			parameter->Freeze();

			return parameter;
		}

		/* Instance methods. */

		SupportedExtensionsChunkParameter::SupportedExtensionsChunkParameter(
		  uint8_t* buffer, size_t bufferLength)
		  : ChunkParameter(buffer, bufferLength)
		{
			MS_TRACE();

			SetLength(ChunkParameter::ChunkParameterHeaderLength);
		}

		SupportedExtensionsChunkParameter::~SupportedExtensionsChunkParameter()
		{
			MS_TRACE();
		}

		void SupportedExtensionsChunkParameter::Dump(int indentation) const
		{
			MS_TRACE();

			MS_DUMP_CLEAN(indentation, "<SCTP::SupportedExtensionsChunkParameter>");
			DumpCommon(indentation);
			MS_DUMP_CLEAN(indentation, "  number of chunk types: %" PRIu16, GetNumberOfChunkTypes());
			for (uint32_t idx{ 0 }; idx < GetNumberOfChunkTypes(); ++idx)
			{
				MS_DUMP_CLEAN(
				  indentation,
				  "  - idx: %" PRIu16 ", chunk type: %" PRIu8,
				  idx,
				  static_cast<uint8_t>(GetChunkTypeAt(idx)));
			}
			MS_DUMP_CLEAN(indentation, "</SCTP::SupportedExtensionsChunkParameter>");
		}

		SupportedExtensionsChunkParameter* SupportedExtensionsChunkParameter::Clone(
		  uint8_t* buffer, size_t bufferLength) const
		{
			MS_TRACE();

			auto* clonedParameter = new SupportedExtensionsChunkParameter(buffer, bufferLength);

			CloneInto(clonedParameter);

			return clonedParameter;
		}

		void SupportedExtensionsChunkParameter::AddChunkType(Chunk::ChunkType chunkType)
		{
			MS_TRACE();

			AssertNotFrozen();

			// We must save previous count since SetVariableLengthValueLength() will
			// make GetNumberOfChunkTypes() return a different value.
			auto previousNumberOfChunkTypes = GetNumberOfChunkTypes();

			// NOTE: This may throw.
			SetVariableLengthValueLength(GetVariableLengthValueLength() + 1);

			// Add the new missing mandatory parameter type.
			Utils::Byte::Set1Byte(
			  GetVariableLengthValuePointer(), previousNumberOfChunkTypes, static_cast<uint8_t>(chunkType));
		}

		SupportedExtensionsChunkParameter* SupportedExtensionsChunkParameter::SoftClone(
		  const uint8_t* buffer) const
		{
			MS_TRACE();

			auto* softClonedParameter =
			  new SupportedExtensionsChunkParameter(const_cast<uint8_t*>(buffer), GetLength());

			SoftCloneInto(softClonedParameter);

			return softClonedParameter;
		}
	} // namespace SCTP
} // namespace RTC
