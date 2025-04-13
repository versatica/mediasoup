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

			// No need to invoke SetLength() since parent constructor invoked it.
		}

		UnknownChunkParameter::~UnknownChunkParameter()
		{
			MS_TRACE();
		}

		void UnknownChunkParameter::Dump() const
		{
			MS_TRACE();

			MS_DUMP("<UnknownChunkParameter>");
			MS_DUMP("  length: %zu (buffer length: %zu)", GetLength(), GetBufferLength());
			MS_DUMP(
			  "  type: %" PRIu16 " (%s) (unknown:%s)",
			  static_cast<uint16_t>(GetType()),
			  ChunkParameter::ChunkParameterType2String(GetType()).c_str(),
			  HasUnknownType() ? "yes" : "no");
			MS_DUMP(
			  "  length field: %" PRIu16 " (value length: %" PRIu16 ")", GetLengthField(), GetValueLength());
			MS_DUMP("</UnknownChunkParameter>");
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
