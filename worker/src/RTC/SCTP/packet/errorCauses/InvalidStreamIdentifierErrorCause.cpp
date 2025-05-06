#define MS_CLASS "RTC::SCTP::InvalidStreamIdentifierErrorCause"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/packet/errorCauses/InvalidStreamIdentifierErrorCause.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"

namespace RTC
{
	namespace SCTP
	{
		/* Class methods. */

		InvalidStreamIdentifierErrorCause* InvalidStreamIdentifierErrorCause::Parse(
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

			if (causeCode != ErrorCause::ErrorCauseCode::INVALID_STREAM_IDENTIFIER)
			{
				MS_WARN_DEV("invalid Error Cause code");

				return nullptr;
			}

			return InvalidStreamIdentifierErrorCause::ParseStrict(
			  buffer, bufferLength, causeLength, padding);
		}

		InvalidStreamIdentifierErrorCause* InvalidStreamIdentifierErrorCause::Factory(
		  uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			if (bufferLength < InvalidStreamIdentifierErrorCause::InvalidStreamIdentifierErrorCauseHeaderLength)
			{
				MS_THROW_TYPE_ERROR("buffer too small");
			}

			auto* errorCause = new InvalidStreamIdentifierErrorCause(buffer, bufferLength);

			errorCause->InitializeHeader(
			  ErrorCause::ErrorCauseCode::INVALID_STREAM_IDENTIFIER,
			  InvalidStreamIdentifierErrorCause::InvalidStreamIdentifierErrorCauseHeaderLength);

			// Initialize header extra fields.
			errorCause->SetStreamIdentifier(0);
			errorCause->SetReserved();

			// No need to invoke SetLength() since parent constructor invoked it.

			return errorCause;
		}

		InvalidStreamIdentifierErrorCause* InvalidStreamIdentifierErrorCause::ParseStrict(
		  const uint8_t* buffer, size_t bufferLength, uint16_t causeLength, uint8_t padding)
		{
			MS_TRACE();

			if (causeLength != InvalidStreamIdentifierErrorCause::InvalidStreamIdentifierErrorCauseHeaderLength)
			{
				MS_WARN_TAG(
				  sctp,
				  "InvalidStreamIdentifierErrorCause Length field must be %zu",
				  InvalidStreamIdentifierErrorCause::InvalidStreamIdentifierErrorCauseHeaderLength);

				return nullptr;
			}

			auto* errorCause =
			  new InvalidStreamIdentifierErrorCause(const_cast<uint8_t*>(buffer), bufferLength);

			// Mark the Error Cause as frozen since we are parsing.
			errorCause->Freeze();

			return errorCause;
		}

		/* Instance methods. */

		InvalidStreamIdentifierErrorCause::InvalidStreamIdentifierErrorCause(
		  uint8_t* buffer, size_t bufferLength)
		  : ErrorCause(buffer, bufferLength)
		{
			MS_TRACE();

			SetLength(InvalidStreamIdentifierErrorCause::InvalidStreamIdentifierErrorCauseHeaderLength);
		}

		InvalidStreamIdentifierErrorCause::~InvalidStreamIdentifierErrorCause()
		{
			MS_TRACE();
		}

		void InvalidStreamIdentifierErrorCause::Dump(int indentation) const
		{
			MS_TRACE();

			MS_DUMP_CLEAN(indentation, "<SCTP::InvalidStreamIdentifierErrorCause>");
			DumpCommon(indentation);
			MS_DUMP_CLEAN(indentation, "  stream identifier: %" PRIu16, GetStreamIdentifier());
			MS_DUMP_CLEAN(indentation, "</SCTP::InvalidStreamIdentifierErrorCause>");
		}

		InvalidStreamIdentifierErrorCause* InvalidStreamIdentifierErrorCause::Clone(
		  uint8_t* buffer, size_t bufferLength) const
		{
			MS_TRACE();

			auto* clonedErrorCause = new InvalidStreamIdentifierErrorCause(buffer, bufferLength);

			CloneInto(clonedErrorCause);

			return clonedErrorCause;
		}

		void InvalidStreamIdentifierErrorCause::SetStreamIdentifier(uint16_t value)
		{
			MS_TRACE();

			AssertNotFrozen();

			Utils::Byte::Set2Bytes(const_cast<uint8_t*>(GetBuffer()), 4, value);
		}

		InvalidStreamIdentifierErrorCause* InvalidStreamIdentifierErrorCause::SoftClone(
		  const uint8_t* buffer) const
		{
			MS_TRACE();

			auto* softClonedErrorCause =
			  new InvalidStreamIdentifierErrorCause(const_cast<uint8_t*>(buffer), GetLength());

			SoftCloneInto(softClonedErrorCause);

			return softClonedErrorCause;
		}

		void InvalidStreamIdentifierErrorCause::SetReserved()
		{
			MS_TRACE();

			Utils::Byte::Set2Bytes(const_cast<uint8_t*>(GetBuffer()), 6, 0);
		}
	} // namespace SCTP
} // namespace RTC
