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

			auto* parameter = new UnknownChunkParameter(buffer, bufferLength);

			// Must always invoke SetLength() after constructing a Serializable with
			// not fixed length.
			parameter->SetLength(parameterLength + padding);

			// Mark the Parameter as frozen since we are parsing.
			parameter->Freeze();

			return parameter;
		}

		/* Instance methods. */

		UnknownChunkParameter::UnknownChunkParameter(const uint8_t* buffer, size_t bufferLength)
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

			auto* clonedItem = new UnknownChunkParameter(buffer, bufferLength);

			CloneInto(clonedItem);

			return clonedItem;
		}
	} // namespace SCTP
} // namespace RTC
