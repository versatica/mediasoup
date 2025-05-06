#define MS_CLASS "RTC::SCTP::InvalidMandatoryParameterErrorCause"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/packet/errorCauses/InvalidMandatoryParameterErrorCause.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"

namespace RTC
{
	namespace SCTP
	{
		/* Class methods. */

		InvalidMandatoryParameterErrorCause* InvalidMandatoryParameterErrorCause::Parse(
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

			if (causeCode != ErrorCause::ErrorCauseCode::INVALID_MANDATORY_PARAMETER)
			{
				MS_WARN_DEV("invalid Error Cause code");

				return nullptr;
			}

			return InvalidMandatoryParameterErrorCause::ParseStrict(
			  buffer, bufferLength, causeLength, padding);
		}

		InvalidMandatoryParameterErrorCause* InvalidMandatoryParameterErrorCause::Factory(
		  uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			if (bufferLength < ErrorCause::ErrorCauseHeaderLength)
			{
				MS_THROW_TYPE_ERROR("buffer too small");
			}

			auto* errorCause = new InvalidMandatoryParameterErrorCause(buffer, bufferLength);

			errorCause->InitializeHeader(
			  ErrorCause::ErrorCauseCode::INVALID_MANDATORY_PARAMETER, ErrorCause::ErrorCauseHeaderLength);

			// No need to invoke SetLength() since parent constructor invoked it.

			return errorCause;
		}

		InvalidMandatoryParameterErrorCause* InvalidMandatoryParameterErrorCause::ParseStrict(
		  const uint8_t* buffer, size_t bufferLength, uint16_t causeLength, uint8_t padding)
		{
			MS_TRACE();

			if (causeLength != ErrorCause::ErrorCauseHeaderLength)
			{
				MS_WARN_TAG(
				  sctp,
				  "InvalidMandatoryParameterErrorCause Length field must be %zu",
				  ErrorCause::ErrorCauseHeaderLength);

				return nullptr;
			}

			auto* errorCause =
			  new InvalidMandatoryParameterErrorCause(const_cast<uint8_t*>(buffer), bufferLength);

			// Mark the Error Cause as frozen since we are parsing.
			errorCause->Freeze();

			return errorCause;
		}

		/* Instance methods. */

		InvalidMandatoryParameterErrorCause::InvalidMandatoryParameterErrorCause(
		  uint8_t* buffer, size_t bufferLength)
		  : ErrorCause(buffer, bufferLength)
		{
			MS_TRACE();

			SetLength(ErrorCause::ErrorCauseHeaderLength);
		}

		InvalidMandatoryParameterErrorCause::~InvalidMandatoryParameterErrorCause()
		{
			MS_TRACE();
		}

		void InvalidMandatoryParameterErrorCause::Dump(int indentation) const
		{
			MS_TRACE();

			MS_DUMP_CLEAN(indentation, "<SCTP::InvalidMandatoryParameterErrorCause>");
			DumpCommon(indentation);
			MS_DUMP_CLEAN(indentation, "</SCTP::InvalidMandatoryParameterErrorCause>");
		}

		InvalidMandatoryParameterErrorCause* InvalidMandatoryParameterErrorCause::Clone(
		  uint8_t* buffer, size_t bufferLength) const
		{
			MS_TRACE();

			auto* clonedErrorCause = new InvalidMandatoryParameterErrorCause(buffer, bufferLength);

			CloneInto(clonedErrorCause);

			return clonedErrorCause;
		}

		InvalidMandatoryParameterErrorCause* InvalidMandatoryParameterErrorCause::SoftClone(
		  const uint8_t* buffer) const
		{
			MS_TRACE();

			auto* softClonedErrorCause =
			  new InvalidMandatoryParameterErrorCause(const_cast<uint8_t*>(buffer), GetLength());

			SoftCloneInto(softClonedErrorCause);

			return softClonedErrorCause;
		}
	} // namespace SCTP
} // namespace RTC
