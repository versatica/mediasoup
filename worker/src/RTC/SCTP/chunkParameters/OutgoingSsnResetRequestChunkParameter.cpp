#define MS_CLASS "RTC::SCTP::OutgoingSsnResetRequestChunkParameter"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/chunkParameters/OutgoingSsnResetRequestChunkParameter.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"

namespace RTC
{
	namespace SCTP
	{
		/* Class methods. */

		OutgoingSsnResetRequestChunkParameter* OutgoingSsnResetRequestChunkParameter::Parse(
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

			if (parameterType != ChunkParameter::ChunkParameterType::OUTGOING_SSN_RESET_REQUEST)
			{
				MS_WARN_DEV("invalid Chunk Parameter type");

				return nullptr;
			}

			return OutgoingSsnResetRequestChunkParameter::ParseStrict(
			  buffer, bufferLength, parameterLength, padding);
		}

		OutgoingSsnResetRequestChunkParameter* OutgoingSsnResetRequestChunkParameter::Factory(
		  uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			if (bufferLength < OutgoingSsnResetRequestChunkParameter::OutgoingSsnResetRequestChunkParameterHeaderLength)
			{
				MS_THROW_TYPE_ERROR("buffer too small");
			}

			auto* parameter = new OutgoingSsnResetRequestChunkParameter(buffer, bufferLength);

			parameter->InitializeHeader(
			  ChunkParameter::ChunkParameterType::OUTGOING_SSN_RESET_REQUEST,
			  OutgoingSsnResetRequestChunkParameter::OutgoingSsnResetRequestChunkParameterHeaderLength);

			// Must also initialize extra fields in the header.
			parameter->SetReconfigurationRequestSequenceNumber(0);
			parameter->SetReconfigurationResponseSequenceNumber(0);
			parameter->SetSenderLastAssignedTsn(0);

			// No need to invoke SetLength() since parent constructor invoked it.

			return parameter;
		}

		OutgoingSsnResetRequestChunkParameter* OutgoingSsnResetRequestChunkParameter::ParseStrict(
		  const uint8_t* buffer, size_t bufferLength, uint16_t parameterLength, uint8_t padding)
		{
			MS_TRACE();

			auto* parameter =
			  new OutgoingSsnResetRequestChunkParameter(const_cast<uint8_t*>(buffer), bufferLength);

			// Must always invoke SetLength() after constructing a Serializable with
			// not fixed length.
			parameter->SetLength(parameterLength + padding);

			// Mark the Parameter as frozen since we are parsing.
			parameter->Freeze();

			return parameter;
		}

		/* Instance methods. */

		OutgoingSsnResetRequestChunkParameter::OutgoingSsnResetRequestChunkParameter(
		  uint8_t* buffer, size_t bufferLength)
		  : ChunkParameter(buffer, bufferLength)
		{
			MS_TRACE();

			SetLength(
			  OutgoingSsnResetRequestChunkParameter::OutgoingSsnResetRequestChunkParameterHeaderLength);
		}

		OutgoingSsnResetRequestChunkParameter::~OutgoingSsnResetRequestChunkParameter()
		{
			MS_TRACE();
		}

		void OutgoingSsnResetRequestChunkParameter::Dump(int indentation) const
		{
			MS_TRACE();

			MS_DUMP_CLEAN(indentation, "<SCTP::OutgoingSsnResetRequestChunkParameter>");
			DumpCommon(indentation);
			MS_DUMP_CLEAN(
			  indentation,
			  "  re-configuration request sequence number: %" PRIu32,
			  GetReconfigurationRequestSequenceNumber());
			MS_DUMP_CLEAN(
			  indentation,
			  "  re-configuration response sequence number: %" PRIu32,
			  GetReconfigurationResponseSequenceNumber());
			MS_DUMP_CLEAN(indentation, "  sender last assigned tsn: %" PRIu32, GetSenderLastAssignedTsn());
			MS_DUMP_CLEAN(indentation, "  number of streams: %" PRIu16, GetNumberOfStreams());
			for (uint32_t idx{ 0 }; idx < GetNumberOfStreams(); ++idx)
			{
				MS_DUMP_CLEAN(indentation, "  - idx: %" PRIu16 ", stream: %" PRIu16, idx, GetStreamAt(idx));
			}
			MS_DUMP_CLEAN(indentation, "</SCTP::OutgoingSsnResetRequestChunkParameter>");
		}

		OutgoingSsnResetRequestChunkParameter* OutgoingSsnResetRequestChunkParameter::Clone(
		  uint8_t* buffer, size_t bufferLength) const
		{
			MS_TRACE();

			auto* clonedParameter = new OutgoingSsnResetRequestChunkParameter(buffer, bufferLength);

			CloneInto(clonedParameter);

			return clonedParameter;
		}

		void OutgoingSsnResetRequestChunkParameter::SetReconfigurationRequestSequenceNumber(uint32_t value)
		{
			MS_TRACE();

			AssertNotFrozen();

			Utils::Byte::Set4Bytes(const_cast<uint8_t*>(GetBuffer()), 4, value);
		}

		void OutgoingSsnResetRequestChunkParameter::SetReconfigurationResponseSequenceNumber(uint32_t value)
		{
			MS_TRACE();

			AssertNotFrozen();

			Utils::Byte::Set4Bytes(const_cast<uint8_t*>(GetBuffer()), 8, value);
		}

		void OutgoingSsnResetRequestChunkParameter::SetSenderLastAssignedTsn(uint32_t value)
		{
			MS_TRACE();

			AssertNotFrozen();

			Utils::Byte::Set4Bytes(const_cast<uint8_t*>(GetBuffer()), 12, value);
		}

		void OutgoingSsnResetRequestChunkParameter::AddStream(uint16_t stream)
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

		OutgoingSsnResetRequestChunkParameter* OutgoingSsnResetRequestChunkParameter::SoftClone(
		  const uint8_t* buffer) const
		{
			MS_TRACE();

			auto* softClonedParameter =
			  new OutgoingSsnResetRequestChunkParameter(const_cast<uint8_t*>(buffer), GetLength());

			SoftCloneInto(softClonedParameter);

			return softClonedParameter;
		}
	} // namespace SCTP
} // namespace RTC
