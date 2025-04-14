#define MS_CLASS "RTC::SCTP::HeartbeatInfoChunkParameter"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/chunkParameters/HeartbeatInfoChunkParameter.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "Utils.hpp"
#include <cstring> // std::memmove()

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

			parameter->InitializeHeader(ChunkParameter::ChunkParameterType::HEARTBEAT_INFO);

			// No need to invoke SetLength() since parent constructor invoked it.

			return parameter;
		}

		/* Instance methods. */

		HeartbeatInfoChunkParameter::HeartbeatInfoChunkParameter(const uint8_t* buffer, size_t bufferLength)
		  : ChunkParameter(buffer, bufferLength)
		{
			MS_TRACE();

			// No need to invoke SetLength() since parent constructor invoked it.
		}

		HeartbeatInfoChunkParameter::~HeartbeatInfoChunkParameter()
		{
			MS_TRACE();
		}

		void HeartbeatInfoChunkParameter::Dump() const
		{
			MS_TRACE();

			MS_DUMP("<HeartbeatInfoChunkParameter>");
			MS_DUMP("  length: %zu (buffer length: %zu)", GetLength(), GetBufferLength());
			MS_DUMP(
			  "  type: %" PRIu16 " (%s) (unknown: %s)",
			  static_cast<uint16_t>(GetType()),
			  ChunkParameter::ChunkParameterType2String(GetType()).c_str(),
			  HasUnknownType() ? "yes" : "no");
			MS_DUMP(
			  "  length field: %" PRIu16 " (has value: %s, value length: %" PRIu16 ")",
			  GetLengthField(),
			  HasValue() ? "yes" : "no",
			  GetValueLength());
			MS_DUMP("  info length: %" PRIu16, GetValueLength());
			MS_DUMP("</HeartbeatInfoChunkParameter>");
		}

		HeartbeatInfoChunkParameter* HeartbeatInfoChunkParameter::Clone(
		  uint8_t* buffer, size_t bufferLength) const
		{
			MS_TRACE();

			auto* clonedItem = new HeartbeatInfoChunkParameter(buffer, bufferLength);

			CloneInto(clonedItem);

			return clonedItem;
		}

		void HeartbeatInfoChunkParameter::SetInfo(const uint8_t* info, uint16_t infoLength)
		{
			MS_TRACE();

			AssertNotFrozen();

			auto previousLength      = GetLength();
			auto previousLengthField = GetLengthField();
			auto previousInfoLength  = GetValueLength();
			auto newNotPaddedLength  = previousLength - previousInfoLength + infoLength;
			auto newPaddedLength     = Utils::Byte::PadTo4Bytes(newNotPaddedLength);

			try
			{
				// Let's call SetLength() on parent with the new computed Parameter length.
				// NOTE: If there is no space in the buffer for it, it will throw.
				// NOTE: Parameters must be padded to 4 bytes.
				SetLength(newPaddedLength);

				// Update the Parameter Length field.
				// NOTE: This will throw if computed value is too big.
				SetLengthField(newNotPaddedLength);
			}
			catch (const MediaSoupError& error)
			{
				// Rollback.
				SetLength(previousLength);
				SetLengthField(previousLengthField);

				throw;
			}

			// Copy the given info into the buffer.
			std::memmove(GetValuePointer(), info, infoLength);

			// Fill padding bytes with zero.
			FillPadding(newPaddedLength - newNotPaddedLength);
		}
	} // namespace SCTP
} // namespace RTC
