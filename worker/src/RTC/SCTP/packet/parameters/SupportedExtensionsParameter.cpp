#define MS_CLASS "RTC::SCTP::SupportedExtensionsParameter"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/packet/parameters/SupportedExtensionsParameter.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"

namespace RTC
{
	namespace SCTP
	{
		/* Class methods. */

		SupportedExtensionsParameter* SupportedExtensionsParameter::Parse(
		  const uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			Parameter::ParameterType parameterType;
			uint16_t parameterLength;
			uint8_t padding;

			if (!Parameter::IsParameter(buffer, bufferLength, parameterType, parameterLength, padding))
			{
				return nullptr;
			}

			if (parameterType != Parameter::ParameterType::SUPPORTED_EXTENSIONS)
			{
				MS_WARN_DEV("invalid Parameter type");

				return nullptr;
			}

			return SupportedExtensionsParameter::ParseStrict(buffer, bufferLength, parameterLength, padding);
		}

		SupportedExtensionsParameter* SupportedExtensionsParameter::Factory(
		  uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			if (bufferLength < Parameter::ParameterHeaderLength)
			{
				MS_THROW_TYPE_ERROR("buffer too small");
			}

			auto* parameter = new SupportedExtensionsParameter(buffer, bufferLength);

			parameter->InitializeHeader(
			  Parameter::ParameterType::SUPPORTED_EXTENSIONS, Parameter::ParameterHeaderLength);

			// No need to invoke SetLength() since parent constructor invoked it.

			return parameter;
		}

		SupportedExtensionsParameter* SupportedExtensionsParameter::ParseStrict(
		  const uint8_t* buffer, size_t bufferLength, uint16_t parameterLength, uint8_t padding)
		{
			MS_TRACE();

			auto* parameter = new SupportedExtensionsParameter(const_cast<uint8_t*>(buffer), bufferLength);

			// Must always invoke SetLength() after constructing a Serializable with
			// not fixed length.
			parameter->SetLength(parameterLength + padding);

			// Mark the Parameter as frozen since we are parsing.
			parameter->Freeze();

			return parameter;
		}

		/* Instance methods. */

		SupportedExtensionsParameter::SupportedExtensionsParameter(uint8_t* buffer, size_t bufferLength)
		  : Parameter(buffer, bufferLength)
		{
			MS_TRACE();

			SetLength(Parameter::ParameterHeaderLength);
		}

		SupportedExtensionsParameter::~SupportedExtensionsParameter()
		{
			MS_TRACE();
		}

		void SupportedExtensionsParameter::Dump(int indentation) const
		{
			MS_TRACE();

			MS_DUMP_CLEAN(indentation, "<SCTP::SupportedExtensionsParameter>");
			DumpCommon(indentation);
			MS_DUMP_CLEAN(indentation, "  number of chunk types: %" PRIu16, GetNumberOfChunkTypes());
			for (uint32_t idx{ 0 }; idx < GetNumberOfChunkTypes(); ++idx)
			{
				MS_DUMP_CLEAN(
				  indentation,
				  "  - idx: %" PRIu16 ", chunk type: %" PRIu8 " (%s)",
				  idx,
				  static_cast<uint8_t>(GetChunkTypeAt(idx)),
				  Chunk::ChunkType2String(GetChunkTypeAt(idx)).c_str());
			}
			MS_DUMP_CLEAN(indentation, "</SCTP::SupportedExtensionsParameter>");
		}

		SupportedExtensionsParameter* SupportedExtensionsParameter::Clone(
		  uint8_t* buffer, size_t bufferLength) const
		{
			MS_TRACE();

			auto* clonedParameter = new SupportedExtensionsParameter(buffer, bufferLength);

			CloneInto(clonedParameter);

			return clonedParameter;
		}

		void SupportedExtensionsParameter::AddChunkType(Chunk::ChunkType chunkType)
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

		SupportedExtensionsParameter* SupportedExtensionsParameter::SoftClone(const uint8_t* buffer) const
		{
			MS_TRACE();

			auto* softClonedParameter =
			  new SupportedExtensionsParameter(const_cast<uint8_t*>(buffer), GetLength());

			SoftCloneInto(softClonedParameter);

			return softClonedParameter;
		}
	} // namespace SCTP
} // namespace RTC
