#define MS_CLASS "RTC::SCTP::UnknownChunkParameter"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/chunkParameters/UnknownChunkParameter.hpp"
#include "Logger.hpp"

namespace RTC
{
	namespace SCTP
	{
		/* Class methods. */

		UnknownChunkParameter* UnknownChunkParameter::Parse(const uint8_t* buffer, size_t bufferLength)
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

			return UnknownChunkParameter::ParseStrict(buffer, bufferLength, parameterLength, padding);
		}

		UnknownChunkParameter* UnknownChunkParameter::ParseStrict(
		  const uint8_t* buffer, size_t bufferLength, uint16_t parameterLength, uint8_t padding)
		{
			MS_TRACE();

			auto* parameter = new UnknownChunkParameter(const_cast<uint8_t*>(buffer), bufferLength);

			// Must always invoke SetLength() after constructing a Serializable with
			// not fixed length.
			parameter->SetLength(parameterLength + padding);

			// Mark the Parameter as frozen since we are parsing.
			parameter->Freeze();

			return parameter;
		}

		/* Instance methods. */

		UnknownChunkParameter::UnknownChunkParameter(uint8_t* buffer, size_t bufferLength)
		  : ChunkParameter(buffer, bufferLength)
		{
			MS_TRACE();

			SetLength(ChunkParameter::ChunkParameterHeaderLength);
		}

		UnknownChunkParameter::~UnknownChunkParameter()
		{
			MS_TRACE();
		}

		void UnknownChunkParameter::Dump(int indentation) const
		{
			MS_TRACE();

			MS_DUMP_CLEAN(indentation, "<SCTP::UnknownChunkParameter>");
			DumpCommon(indentation);
			MS_DUMP_CLEAN(indentation, "</SCTP::UnknownChunkParameter>");
		}

		UnknownChunkParameter* UnknownChunkParameter::Clone(uint8_t* buffer, size_t bufferLength) const
		{
			MS_TRACE();

			auto* clonedParameter = new UnknownChunkParameter(buffer, bufferLength);

			CloneInto(clonedParameter);

			return clonedParameter;
		}

		UnknownChunkParameter* UnknownChunkParameter::SoftClone(const uint8_t* buffer) const
		{
			MS_TRACE();

			auto* softClonedParameter =
			  new UnknownChunkParameter(const_cast<uint8_t*>(buffer), GetLength());

			SoftCloneInto(softClonedParameter);

			return softClonedParameter;
		}
	} // namespace SCTP
} // namespace RTC
