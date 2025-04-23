#define MS_CLASS "RTC::SCTP::SupportedAddressTypesChunkParameter"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/chunkParameters/SupportedAddressTypesChunkParameter.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"

namespace RTC
{
	namespace SCTP
	{
		/* Class methods. */

		SupportedAddressTypesChunkParameter* SupportedAddressTypesChunkParameter::Parse(
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

			if (parameterType != ChunkParameter::ChunkParameterType::SUPPORTED_ADDRESS_TYPES)
			{
				MS_WARN_DEV("invalid Chunk Parameter type");

				return nullptr;
			}

			return SupportedAddressTypesChunkParameter::ParseStrict(
			  buffer, bufferLength, parameterLength, padding);
		}

		SupportedAddressTypesChunkParameter* SupportedAddressTypesChunkParameter::Factory(
		  uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			if (bufferLength < ChunkParameter::ChunkParameterHeaderLength)
			{
				MS_THROW_TYPE_ERROR("buffer too small");
			}

			auto* parameter = new SupportedAddressTypesChunkParameter(buffer, bufferLength);

			parameter->InitializeHeader(
			  ChunkParameter::ChunkParameterType::SUPPORTED_ADDRESS_TYPES,
			  ChunkParameter::ChunkParameterHeaderLength);

			// No need to invoke SetLength() since parent constructor invoked it.

			return parameter;
		}

		SupportedAddressTypesChunkParameter* SupportedAddressTypesChunkParameter::ParseStrict(
		  const uint8_t* buffer, size_t bufferLength, uint16_t parameterLength, uint8_t padding)
		{
			MS_TRACE();

			auto* parameter =
			  new SupportedAddressTypesChunkParameter(const_cast<uint8_t*>(buffer), bufferLength);

			// Here we must validate that Length field is even.
			if (parameter->GetLengthField() % 2 != 0)
			{
				MS_WARN_TAG(sctp, "wrong Length value (not even)");

				delete parameter;
				return nullptr;
			}

			// Must always invoke SetLength() after constructing a Serializable with
			// not fixed length.
			parameter->SetLength(parameterLength + padding);

			// Mark the Parameter as frozen since we are parsing.
			parameter->Freeze();

			return parameter;
		}

		/* Instance methods. */

		SupportedAddressTypesChunkParameter::SupportedAddressTypesChunkParameter(
		  uint8_t* buffer, size_t bufferLength)
		  : ChunkParameter(buffer, bufferLength)
		{
			MS_TRACE();

			SetLength(ChunkParameter::ChunkParameterHeaderLength);
		}

		SupportedAddressTypesChunkParameter::~SupportedAddressTypesChunkParameter()
		{
			MS_TRACE();
		}

		void SupportedAddressTypesChunkParameter::Dump(int indentation) const
		{
			MS_TRACE();

			MS_DUMP_CLEAN(indentation, "<SCTP::SupportedAddressTypesChunkParameter>");
			DumpCommon(indentation);
			MS_DUMP_CLEAN(indentation, "  number of address types: %" PRIu16, GetNumberOfAddressTypes());
			for (uint32_t idx{ 0 }; idx < GetNumberOfAddressTypes(); ++idx)
			{
				MS_DUMP_CLEAN(
				  indentation, "  - idx: %" PRIu16 ", address type: %" PRIu16, idx, GetAddressTypeAt(idx));
			}
			MS_DUMP_CLEAN(indentation, "</SCTP::SupportedAddressTypesChunkParameter>");
		}

		SupportedAddressTypesChunkParameter* SupportedAddressTypesChunkParameter::Clone(
		  uint8_t* buffer, size_t bufferLength) const
		{
			MS_TRACE();

			auto* clonedParameter = new SupportedAddressTypesChunkParameter(buffer, bufferLength);

			CloneInto(clonedParameter);

			return clonedParameter;
		}

		void SupportedAddressTypesChunkParameter::AddAddressType(uint16_t addressType)
		{
			MS_TRACE();

			AssertNotFrozen();

			// We must save previous count since SetVariableLengthValueLength() will
			// make GetNumberOfAddressTypes() return a different value.
			auto previousNumberOfAddressTypes = GetNumberOfAddressTypes();

			// NOTE: This may throw.
			SetVariableLengthValueLength(GetVariableLengthValueLength() + 2);

			// Add the new missing mandatory parameter type.
			Utils::Byte::Set2Bytes(
			  GetVariableLengthValuePointer(), previousNumberOfAddressTypes * 2, addressType);
		}

		SupportedAddressTypesChunkParameter* SupportedAddressTypesChunkParameter::SoftClone(
		  const uint8_t* buffer) const
		{
			MS_TRACE();

			auto* softClonedParameter =
			  new SupportedAddressTypesChunkParameter(const_cast<uint8_t*>(buffer), GetLength());

			SoftCloneInto(softClonedParameter);

			return softClonedParameter;
		}
	} // namespace SCTP
} // namespace RTC
