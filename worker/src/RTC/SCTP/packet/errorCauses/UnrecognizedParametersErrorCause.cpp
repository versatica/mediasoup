#define MS_CLASS "RTC::SCTP::UnrecognizedParametersErrorCause"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/packet/errorCauses/UnrecognizedParametersErrorCause.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"

namespace RTC
{
	namespace SCTP
	{
		/* Class methods. */

		UnrecognizedParametersErrorCause* UnrecognizedParametersErrorCause::Parse(
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

			if (causeCode != ErrorCause::ErrorCauseCode::UNRECOGNIZED_PARAMETERS)
			{
				MS_WARN_DEV("invalid Error Cause code");

				return nullptr;
			}

			return UnrecognizedParametersErrorCause::ParseStrict(buffer, bufferLength, causeLength, padding);
		}

		UnrecognizedParametersErrorCause* UnrecognizedParametersErrorCause::Factory(
		  uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			if (bufferLength < ErrorCause::ErrorCauseHeaderLength)
			{
				MS_THROW_TYPE_ERROR("buffer too small");
			}

			auto* errorCause = new UnrecognizedParametersErrorCause(buffer, bufferLength);

			errorCause->InitializeHeader(
			  ErrorCause::ErrorCauseCode::UNRECOGNIZED_PARAMETERS, ErrorCause::ErrorCauseHeaderLength);

			// No need to invoke SetLength() since parent constructor invoked it.

			return errorCause;
		}

		UnrecognizedParametersErrorCause* UnrecognizedParametersErrorCause::ParseStrict(
		  const uint8_t* buffer, size_t bufferLength, uint16_t causeLength, uint8_t padding)
		{
			MS_TRACE();

			auto* errorCause =
			  new UnrecognizedParametersErrorCause(const_cast<uint8_t*>(buffer), bufferLength);

			// Must always invoke SetLength() after constructing a Serializable with
			// not fixed length.
			errorCause->SetLength(causeLength + padding);

			// Mark the Error Cause as frozen since we are parsing.
			errorCause->Freeze();

			return errorCause;
		}

		/* Instance methods. */

		UnrecognizedParametersErrorCause::UnrecognizedParametersErrorCause(uint8_t* buffer, size_t bufferLength)
		  : ErrorCause(buffer, bufferLength)
		{
			MS_TRACE();

			SetLength(ErrorCause::ErrorCauseHeaderLength);
		}

		UnrecognizedParametersErrorCause::~UnrecognizedParametersErrorCause()
		{
			MS_TRACE();
		}

		void UnrecognizedParametersErrorCause::Dump(int indentation) const
		{
			MS_TRACE();

			MS_DUMP_CLEAN(indentation, "<SCTP::UnrecognizedParametersErrorCause>");
			DumpCommon(indentation);
			MS_DUMP_CLEAN(
			  indentation,
			  "  unrecognized parameters length: %" PRIu16 " (has unrecognized parameters: %s)",
			  GetUnrecognizedParametersLength(),
			  HasUnrecognizedParameters() ? "yes" : "no");
			MS_DUMP_CLEAN(indentation, "</SCTP::UnrecognizedParametersErrorCause>");
		}

		UnrecognizedParametersErrorCause* UnrecognizedParametersErrorCause::Clone(
		  uint8_t* buffer, size_t bufferLength) const
		{
			MS_TRACE();

			auto* clonedErrorCause = new UnrecognizedParametersErrorCause(buffer, bufferLength);

			CloneInto(clonedErrorCause);

			return clonedErrorCause;
		}

		void UnrecognizedParametersErrorCause::SetUnrecognizedParameters(
		  const uint8_t* parameters, uint16_t parametersLength)
		{
			MS_TRACE();

			AssertNotFrozen();

			SetVariableLengthValue(parameters, parametersLength);
		}

		UnrecognizedParametersErrorCause* UnrecognizedParametersErrorCause::SoftClone(
		  const uint8_t* buffer) const
		{
			MS_TRACE();

			auto* softClonedErrorCause =
			  new UnrecognizedParametersErrorCause(const_cast<uint8_t*>(buffer), GetLength());

			SoftCloneInto(softClonedErrorCause);

			return softClonedErrorCause;
		}
	} // namespace SCTP
} // namespace RTC
