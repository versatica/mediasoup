#define MS_CLASS "RTC::SCTP::IncomingSsnResetRequestChunkParameter"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/chunkParameters/IncomingSsnResetRequestChunkParameter.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"

namespace RTC
{
	namespace SCTP
	{
		/* Class methods. */

		IncomingSsnResetRequestChunkParameter* IncomingSsnResetRequestChunkParameter::Parse(
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

			if (parameterType != ChunkParameter::ChunkParameterType::INCOMING_SSN_RESET_REQUEST)
			{
				MS_WARN_DEV("invalid Chunk Parameter type");

				return nullptr;
			}

			return IncomingSsnResetRequestChunkParameter::ParseStrict(
			  buffer, bufferLength, parameterLength, padding);
		}

		IncomingSsnResetRequestChunkParameter* IncomingSsnResetRequestChunkParameter::Factory(
		  uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			if (bufferLength < IncomingSsnResetRequestChunkParameter::IncomingSsnResetRequestChunkParameterHeaderLength)
			{
				MS_THROW_TYPE_ERROR("buffer too small");
			}

			auto* parameter = new IncomingSsnResetRequestChunkParameter(buffer, bufferLength);

			parameter->InitializeHeader(
			  ChunkParameter::ChunkParameterType::INCOMING_SSN_RESET_REQUEST,
			  IncomingSsnResetRequestChunkParameter::IncomingSsnResetRequestChunkParameterHeaderLength);

			// Must also initialize extra fields in the header.
			parameter->SetReconfigurationRequestSequenceNumber(0);

			// No need to invoke SetLength() since parent constructor invoked it.

			return parameter;
		}

		IncomingSsnResetRequestChunkParameter* IncomingSsnResetRequestChunkParameter::ParseStrict(
		  const uint8_t* buffer, size_t bufferLength, uint16_t parameterLength, uint8_t padding)
		{
			MS_TRACE();

			auto* parameter =
			  new IncomingSsnResetRequestChunkParameter(const_cast<uint8_t*>(buffer), bufferLength);

			// Must always invoke SetLength() after constructing a Serializable with
			// not fixed length.
			parameter->SetLength(parameterLength + padding);

			// Mark the Parameter as frozen since we are parsing.
			parameter->Freeze();

			return parameter;
		}

		/* Instance methods. */

		IncomingSsnResetRequestChunkParameter::IncomingSsnResetRequestChunkParameter(
		  uint8_t* buffer, size_t bufferLength)
		  : ChunkParameter(buffer, bufferLength)
		{
			MS_TRACE();

			SetLength(
			  IncomingSsnResetRequestChunkParameter::IncomingSsnResetRequestChunkParameterHeaderLength);
		}

		IncomingSsnResetRequestChunkParameter::~IncomingSsnResetRequestChunkParameter()
		{
			MS_TRACE();
		}

		void IncomingSsnResetRequestChunkParameter::Dump(int indentation) const
		{
			MS_TRACE();

			MS_DUMP_CLEAN(indentation, "<SCTP::IncomingSsnResetRequestChunkParameter>");
			DumpCommon(indentation);
			MS_DUMP_CLEAN(
			  indentation,
			  "  re-configuration request sequence number: %" PRIu32,
			  GetReconfigurationRequestSequenceNumber());
			MS_DUMP_CLEAN(indentation, "  number of streams: %" PRIu16, GetNumberOfStreams());
			for (uint32_t idx{ 0 }; idx < GetNumberOfStreams(); ++idx)
			{
				MS_DUMP_CLEAN(indentation, "  - idx: %" PRIu16 ", stream: %" PRIu16, idx, GetStreamAt(idx));
			}
			MS_DUMP_CLEAN(indentation, "</SCTP::IncomingSsnResetRequestChunkParameter>");
		}

		IncomingSsnResetRequestChunkParameter* IncomingSsnResetRequestChunkParameter::Clone(
		  uint8_t* buffer, size_t bufferLength) const
		{
			MS_TRACE();

			auto* clonedParameter = new IncomingSsnResetRequestChunkParameter(buffer, bufferLength);

			CloneInto(clonedParameter);

			return clonedParameter;
		}

		void IncomingSsnResetRequestChunkParameter::SetReconfigurationRequestSequenceNumber(uint32_t value)
		{
			MS_TRACE();

			AssertNotFrozen();

			Utils::Byte::Set4Bytes(const_cast<uint8_t*>(GetBuffer()), 4, value);
		}

		void IncomingSsnResetRequestChunkParameter::AddStream(uint16_t stream)
		{
			MS_TRACE();

			AssertNotFrozen();

			auto previousVariableLengthValueLength = GetVariableLengthValueLength();

			// NOTE: This may throw.
			SetVariableLengthValueLength(previousVariableLengthValueLength + 2);

			// Add the new stream.
			Utils::Byte::Set2Bytes(
			  GetVariableLengthValuePointer(), previousVariableLengthValueLength, stream);
		}

		IncomingSsnResetRequestChunkParameter* IncomingSsnResetRequestChunkParameter::SoftClone(
		  const uint8_t* buffer) const
		{
			MS_TRACE();

			auto* softClonedParameter =
			  new IncomingSsnResetRequestChunkParameter(const_cast<uint8_t*>(buffer), GetLength());

			SoftCloneInto(softClonedParameter);

			return softClonedParameter;
		}
	} // namespace SCTP
} // namespace RTC
