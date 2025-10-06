#define MS_CLASS "RTC::SCTP::UnrecognizedChunkTypeErrorCause"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/packet/errorCauses/UnrecognizedChunkTypeErrorCause.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"

namespace RTC
{
	namespace SCTP
	{
		/* Class methods. */

		UnrecognizedChunkTypeErrorCause* UnrecognizedChunkTypeErrorCause::Parse(
		  const uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			ErrorCause::ErrorCauseCode causeCode;
			uint16_t causeLength;
			uint8_t padding;

			if (!ErrorCause::IsErrorCause(buffer, bufferLength, causeCode, causeLength, padding))
			{
				return nullptr;
			}

			if (causeCode != ErrorCause::ErrorCauseCode::UNRECOGNIZED_CHUNK_TYPE)
			{
				MS_WARN_DEV("invalid Error Cause code");

				return nullptr;
			}

			return UnrecognizedChunkTypeErrorCause::ParseStrict(buffer, bufferLength, causeLength, padding);
		}

		UnrecognizedChunkTypeErrorCause* UnrecognizedChunkTypeErrorCause::Factory(
		  uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			if (bufferLength < ErrorCause::ErrorCauseHeaderLength)
			{
				MS_THROW_TYPE_ERROR("buffer too small");
			}

			auto* errorCause = new UnrecognizedChunkTypeErrorCause(buffer, bufferLength);

			errorCause->InitializeHeader(
			  ErrorCause::ErrorCauseCode::UNRECOGNIZED_CHUNK_TYPE, ErrorCause::ErrorCauseHeaderLength);

			// No need to invoke SetLength() since parent constructor invoked it.

			return errorCause;
		}

		UnrecognizedChunkTypeErrorCause* UnrecognizedChunkTypeErrorCause::ParseStrict(
		  const uint8_t* buffer, size_t bufferLength, uint16_t causeLength, uint8_t padding)
		{
			MS_TRACE();

			auto* errorCause =
			  new UnrecognizedChunkTypeErrorCause(const_cast<uint8_t*>(buffer), bufferLength);

			// Must always invoke SetLength() after constructing a Serializable with
			// not fixed length.
			errorCause->SetLength(causeLength + padding);

			// Mark the Error Cause as frozen since we are parsing.
			errorCause->Freeze();

			return errorCause;
		}

		/* Instance methods. */

		UnrecognizedChunkTypeErrorCause::UnrecognizedChunkTypeErrorCause(uint8_t* buffer, size_t bufferLength)
		  : ErrorCause(buffer, bufferLength)
		{
			MS_TRACE();

			SetLength(ErrorCause::ErrorCauseHeaderLength);
		}

		UnrecognizedChunkTypeErrorCause::~UnrecognizedChunkTypeErrorCause()
		{
			MS_TRACE();
		}

		void UnrecognizedChunkTypeErrorCause::Dump(int indentation) const
		{
			MS_TRACE();

			MS_DUMP_CLEAN(indentation, "<SCTP::UnrecognizedChunkTypeErrorCause>");
			DumpCommon(indentation);
			MS_DUMP_CLEAN(
			  indentation,
			  "  unrecognized chunk length: %" PRIu16 " (has unrecognized chunk: %s)",
			  GetUnrecognizedChunkLength(),
			  HasUnrecognizedChunk() ? "yes" : "no");
			MS_DUMP_CLEAN(indentation, "</SCTP::UnrecognizedChunkTypeErrorCause>");
		}

		UnrecognizedChunkTypeErrorCause* UnrecognizedChunkTypeErrorCause::Clone(
		  uint8_t* buffer, size_t bufferLength) const
		{
			MS_TRACE();

			auto* clonedErrorCause = new UnrecognizedChunkTypeErrorCause(buffer, bufferLength);

			CloneInto(clonedErrorCause);

			return clonedErrorCause;
		}

		void UnrecognizedChunkTypeErrorCause::SetUnrecognizedChunk(const uint8_t* chunk, uint16_t chunkLength)
		{
			MS_TRACE();

			AssertNotFrozen();

			SetVariableLengthValue(chunk, chunkLength);
		}

		UnrecognizedChunkTypeErrorCause* UnrecognizedChunkTypeErrorCause::SoftClone(const uint8_t* buffer) const
		{
			MS_TRACE();

			auto* softClonedErrorCause =
			  new UnrecognizedChunkTypeErrorCause(const_cast<uint8_t*>(buffer), GetLength());

			SoftCloneInto(softClonedErrorCause);

			return softClonedErrorCause;
		}
	} // namespace SCTP
} // namespace RTC
