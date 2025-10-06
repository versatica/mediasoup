#define MS_CLASS "RTC::SCTP::UserInitiatedAbortErrorCause"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/packet/errorCauses/UserInitiatedAbortErrorCause.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"

namespace RTC
{
	namespace SCTP
	{
		/* Class methods. */

		UserInitiatedAbortErrorCause* UserInitiatedAbortErrorCause::Parse(
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

			if (causeCode != ErrorCause::ErrorCauseCode::USER_INITIATED_ABORT)
			{
				MS_WARN_DEV("invalid Error Cause code");

				return nullptr;
			}

			return UserInitiatedAbortErrorCause::ParseStrict(buffer, bufferLength, causeLength, padding);
		}

		UserInitiatedAbortErrorCause* UserInitiatedAbortErrorCause::Factory(
		  uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			if (bufferLength < ErrorCause::ErrorCauseHeaderLength)
			{
				MS_THROW_TYPE_ERROR("buffer too small");
			}

			auto* errorCause = new UserInitiatedAbortErrorCause(buffer, bufferLength);

			errorCause->InitializeHeader(
			  ErrorCause::ErrorCauseCode::USER_INITIATED_ABORT, ErrorCause::ErrorCauseHeaderLength);

			// No need to invoke SetLength() since parent constructor invoked it.

			return errorCause;
		}

		UserInitiatedAbortErrorCause* UserInitiatedAbortErrorCause::ParseStrict(
		  const uint8_t* buffer, size_t bufferLength, uint16_t causeLength, uint8_t padding)
		{
			MS_TRACE();

			auto* errorCause = new UserInitiatedAbortErrorCause(const_cast<uint8_t*>(buffer), bufferLength);

			// Must always invoke SetLength() after constructing a Serializable with
			// not fixed length.
			errorCause->SetLength(causeLength + padding);

			// Mark the Error Cause as frozen since we are parsing.
			errorCause->Freeze();

			return errorCause;
		}

		/* Instance methods. */

		UserInitiatedAbortErrorCause::UserInitiatedAbortErrorCause(uint8_t* buffer, size_t bufferLength)
		  : ErrorCause(buffer, bufferLength)
		{
			MS_TRACE();

			SetLength(ErrorCause::ErrorCauseHeaderLength);
		}

		UserInitiatedAbortErrorCause::~UserInitiatedAbortErrorCause()
		{
			MS_TRACE();
		}

		void UserInitiatedAbortErrorCause::Dump(int indentation) const
		{
			MS_TRACE();

			MS_DUMP_CLEAN(indentation, "<SCTP::UserInitiatedAbortErrorCause>");
			DumpCommon(indentation);
			MS_DUMP_CLEAN(
			  indentation,
			  "  upper layer abort reason length: %" PRIu16 " (has upper layer abort reason: %s)",
			  GetUpperLayerAbortReasonLength(),
			  HasUpperLayerAbortReason() ? "yes" : "no");
			MS_DUMP_CLEAN(indentation, "</SCTP::UserInitiatedAbortErrorCause>");
		}

		UserInitiatedAbortErrorCause* UserInitiatedAbortErrorCause::Clone(
		  uint8_t* buffer, size_t bufferLength) const
		{
			MS_TRACE();

			auto* clonedErrorCause = new UserInitiatedAbortErrorCause(buffer, bufferLength);

			CloneInto(clonedErrorCause);

			return clonedErrorCause;
		}

		void UserInitiatedAbortErrorCause::SetUpperLayerAbortReason(
		  const uint8_t* reason, uint16_t reasonLength)
		{
			MS_TRACE();

			AssertNotFrozen();

			SetVariableLengthValue(reason, reasonLength);
		}

		UserInitiatedAbortErrorCause* UserInitiatedAbortErrorCause::SoftClone(const uint8_t* buffer) const
		{
			MS_TRACE();

			auto* softClonedErrorCause =
			  new UserInitiatedAbortErrorCause(const_cast<uint8_t*>(buffer), GetLength());

			SoftCloneInto(softClonedErrorCause);

			return softClonedErrorCause;
		}
	} // namespace SCTP
} // namespace RTC
