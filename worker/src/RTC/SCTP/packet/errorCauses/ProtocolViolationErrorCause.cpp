#define MS_CLASS "RTC::SCTP::ProtocolViolationErrorCause"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/packet/errorCauses/ProtocolViolationErrorCause.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"

namespace RTC
{
	namespace SCTP
	{
		/* Class methods. */

		ProtocolViolationErrorCause* ProtocolViolationErrorCause::Parse(
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

			if (causeCode != ErrorCause::ErrorCauseCode::PROTOCOL_VIOLATION)
			{
				MS_WARN_DEV("invalid Error Cause code");

				return nullptr;
			}

			return ProtocolViolationErrorCause::ParseStrict(buffer, bufferLength, causeLength, padding);
		}

		ProtocolViolationErrorCause* ProtocolViolationErrorCause::Factory(uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			if (bufferLength < ErrorCause::ErrorCauseHeaderLength)
			{
				MS_THROW_TYPE_ERROR("buffer too small");
			}

			auto* errorCause = new ProtocolViolationErrorCause(buffer, bufferLength);

			errorCause->InitializeHeader(
			  ErrorCause::ErrorCauseCode::PROTOCOL_VIOLATION, ErrorCause::ErrorCauseHeaderLength);

			// No need to invoke SetLength() since parent constructor invoked it.

			return errorCause;
		}

		ProtocolViolationErrorCause* ProtocolViolationErrorCause::ParseStrict(
		  const uint8_t* buffer, size_t bufferLength, uint16_t causeLength, uint8_t padding)
		{
			MS_TRACE();

			auto* errorCause = new ProtocolViolationErrorCause(const_cast<uint8_t*>(buffer), bufferLength);

			// Must always invoke SetLength() after constructing a Serializable with
			// not fixed length.
			errorCause->SetLength(causeLength + padding);

			// Mark the Error Cause as frozen since we are parsing.
			errorCause->Freeze();

			return errorCause;
		}

		/* Instance methods. */

		ProtocolViolationErrorCause::ProtocolViolationErrorCause(uint8_t* buffer, size_t bufferLength)
		  : ErrorCause(buffer, bufferLength)
		{
			MS_TRACE();

			SetLength(ErrorCause::ErrorCauseHeaderLength);
		}

		ProtocolViolationErrorCause::~ProtocolViolationErrorCause()
		{
			MS_TRACE();
		}

		void ProtocolViolationErrorCause::Dump(int indentation) const
		{
			MS_TRACE();

			MS_DUMP_CLEAN(indentation, "<SCTP::ProtocolViolationErrorCause>");
			DumpCommon(indentation);
			MS_DUMP_CLEAN(
			  indentation,
			  "  additional information length: %" PRIu16 " (has additional information: %s)",
			  GetAdditionalInformationLength(),
			  HasAdditionalInformation() ? "yes" : "no");
			MS_DUMP_CLEAN(indentation, "</SCTP::ProtocolViolationErrorCause>");
		}

		ProtocolViolationErrorCause* ProtocolViolationErrorCause::Clone(
		  uint8_t* buffer, size_t bufferLength) const
		{
			MS_TRACE();

			auto* clonedErrorCause = new ProtocolViolationErrorCause(buffer, bufferLength);

			CloneInto(clonedErrorCause);

			return clonedErrorCause;
		}

		void ProtocolViolationErrorCause::SetAdditionalInformation(const uint8_t* info, uint16_t infoLength)
		{
			MS_TRACE();

			AssertNotFrozen();

			SetVariableLengthValue(info, infoLength);
		}

		ProtocolViolationErrorCause* ProtocolViolationErrorCause::SoftClone(const uint8_t* buffer) const
		{
			MS_TRACE();

			auto* softClonedErrorCause =
			  new ProtocolViolationErrorCause(const_cast<uint8_t*>(buffer), GetLength());

			SoftCloneInto(softClonedErrorCause);

			return softClonedErrorCause;
		}
	} // namespace SCTP
} // namespace RTC
