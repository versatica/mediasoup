#define MS_CLASS "RTC::SCTP::HeartbeatInfoChunkParameter"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/chunkParameters/HeartbeatInfoChunkParameter.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"

namespace RTC
{
	namespace SCTP
	{
		/* Class methods. */

		HeartbeatInfoChunkParameter* HeartbeatInfoChunkParameter::Parse(
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

			if (parameterType != ChunkParameter::ChunkParameterType::HEARTBEAT_INFO)
			{
				MS_WARN_DEV("invalid Chunk Parameter type");

				return nullptr;
			}

			auto* parameter = new HeartbeatInfoChunkParameter(buffer, bufferLength);

			// Must always invoke SetLength() after constructing a Serializable with
			// not fixed length.
			parameter->SetLength(parameterLength + padding);

			// Mark the Parameter as frozen since we are parsing.
			parameter->Freeze();

			return parameter;
		}

		HeartbeatInfoChunkParameter* HeartbeatInfoChunkParameter::Factory(uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			if (bufferLength < ChunkParameter::ChunkParameterHeaderLength)
			{
				MS_THROW_TYPE_ERROR("too small buffer");
			}

			auto* parameter = new HeartbeatInfoChunkParameter(buffer, bufferLength);

			parameter->InitializeHeader(
			  ChunkParameter::ChunkParameterType::HEARTBEAT_INFO,
			  ChunkParameter::ChunkParameterHeaderLength);

			// No need to invoke SetLength() since parent constructor invoked it.

			return parameter;
		}

		/* Instance methods. */

		HeartbeatInfoChunkParameter::HeartbeatInfoChunkParameter(const uint8_t* buffer, size_t bufferLength)
		  : ChunkParameter(buffer, bufferLength)
		{
			MS_TRACE();

			SetLength(ChunkParameter::ChunkParameterHeaderLength);
		}

		HeartbeatInfoChunkParameter::~HeartbeatInfoChunkParameter()
		{
			MS_TRACE();
		}

		void HeartbeatInfoChunkParameter::Dump(int indentation) const
		{
			MS_TRACE();

			MS_DUMP_CLEAN(indentation, "<SCTP::HeartbeatInfoChunkParameter>");
			DumpCommon(indentation);
			MS_DUMP_CLEAN(indentation, "  info length: %" PRIu16, GetValueLength());
			MS_DUMP_CLEAN(
			  indentation,
			  "  info length: %" PRIu16 " (has info: %s)",
			  GetInfoLength(),
			  HasInfo() ? "yes" : "no");
			MS_DUMP_CLEAN(indentation, "</SCTP::HeartbeatInfoChunkParameter>");
		}

		HeartbeatInfoChunkParameter* HeartbeatInfoChunkParameter::Clone(
		  uint8_t* buffer, size_t bufferLength) const
		{
			MS_TRACE();

			auto* clonedParameter = new HeartbeatInfoChunkParameter(buffer, bufferLength);

			CloneInto(clonedParameter);

			return clonedParameter;
		}

		void HeartbeatInfoChunkParameter::SetInfo(const uint8_t* info, uint16_t infoLength)
		{
			MS_TRACE();

			AssertNotFrozen();

			SetValue(info, infoLength);
		}

		HeartbeatInfoChunkParameter* HeartbeatInfoChunkParameter::SoftClone(const uint8_t* buffer) const
		{
			MS_TRACE();

			auto* softClonedParameter = new HeartbeatInfoChunkParameter(buffer, GetLength());

			SoftCloneInto(softClonedParameter);

			return softClonedParameter;
		}
	} // namespace SCTP
} // namespace RTC
