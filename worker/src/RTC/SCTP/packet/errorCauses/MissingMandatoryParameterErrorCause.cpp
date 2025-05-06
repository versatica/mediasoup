#define MS_CLASS "RTC::SCTP::MissingMandatoryParameterErrorCause"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/packet/errorCauses/MissingMandatoryParameterErrorCause.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"

namespace RTC
{
	namespace SCTP
	{
		/* Class methods. */

		MissingMandatoryParameterErrorCause* MissingMandatoryParameterErrorCause::Parse(
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

			if (causeCode != ErrorCause::ErrorCauseCode::MISSING_MANDATORY_PARAMETER)
			{
				MS_WARN_DEV("invalid Error Cause code");

				return nullptr;
			}

			return MissingMandatoryParameterErrorCause::ParseStrict(
			  buffer, bufferLength, causeLength, padding);
		}

		MissingMandatoryParameterErrorCause* MissingMandatoryParameterErrorCause::Factory(
		  uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			if (bufferLength < MissingMandatoryParameterErrorCause::MissingMandatoryParameterErrorCauseHeaderLength)
			{
				MS_THROW_TYPE_ERROR("buffer too small");
			}

			auto* errorCause = new MissingMandatoryParameterErrorCause(buffer, bufferLength);

			errorCause->InitializeHeader(
			  ErrorCause::ErrorCauseCode::MISSING_MANDATORY_PARAMETER,
			  MissingMandatoryParameterErrorCause::MissingMandatoryParameterErrorCauseHeaderLength);

			// Initialize extra fields.
			errorCause->SetNumberOfMissingParameters(0);

			// No need to invoke SetLength() since parent constructor invoked it.

			return errorCause;
		}

		MissingMandatoryParameterErrorCause* MissingMandatoryParameterErrorCause::ParseStrict(
		  const uint8_t* buffer, size_t bufferLength, uint16_t causeLength, uint8_t padding)
		{
			MS_TRACE();

			if (causeLength < MissingMandatoryParameterErrorCause::MissingMandatoryParameterErrorCauseHeaderLength)
			{
				MS_WARN_TAG(
				  sctp,
				  "MissingMandatoryParameterErrorCause Length field must be equal or greater than %zu",
				  MissingMandatoryParameterErrorCause::MissingMandatoryParameterErrorCauseHeaderLength);

				return nullptr;
			}

			auto* errorCause =
			  new MissingMandatoryParameterErrorCause(const_cast<uint8_t*>(buffer), bufferLength);

			// In this Chunk we must validate that some fields have correct values.
			if (
			  (errorCause->GetNumberOfMissingParameters() * 2) !=
			  causeLength -
			    MissingMandatoryParameterErrorCause::MissingMandatoryParameterErrorCauseHeaderLength)
			{
				MS_WARN_TAG(sctp, "wrong values in Number of Missing Parameters field");

				delete errorCause;
				return nullptr;
			}

			// Must always invoke SetLength() after constructing a Serializable with
			// not fixed length.
			errorCause->SetLength(causeLength + padding);

			// Mark the Error Cause as frozen since we are parsing.
			errorCause->Freeze();

			return errorCause;
		}

		/* Instance methods. */

		MissingMandatoryParameterErrorCause::MissingMandatoryParameterErrorCause(
		  uint8_t* buffer, size_t bufferLength)
		  : ErrorCause(buffer, bufferLength)
		{
			MS_TRACE();

			SetLength(MissingMandatoryParameterErrorCause::MissingMandatoryParameterErrorCauseHeaderLength);
		}

		MissingMandatoryParameterErrorCause::~MissingMandatoryParameterErrorCause()
		{
			MS_TRACE();
		}

		void MissingMandatoryParameterErrorCause::Dump(int indentation) const
		{
			MS_TRACE();

			MS_DUMP_CLEAN(indentation, "<SCTP::MissingMandatoryParameterErrorCause>");
			DumpCommon(indentation);
			MS_DUMP_CLEAN(
			  indentation, "  number of missing parameters: %" PRIu32, GetNumberOfMissingParameters());
			for (uint32_t idx{ 0 }; idx < GetNumberOfMissingParameters(); ++idx)
			{
				MS_DUMP_CLEAN(
				  indentation,
				  "  - idx: %" PRIu32 ", parameter type: %" PRIu16,
				  idx,
				  static_cast<uint16_t>(GetMissingParameterTypeAt(idx)));
			}
			MS_DUMP_CLEAN(indentation, "</SCTP::MissingMandatoryParameterErrorCause>");
		}

		MissingMandatoryParameterErrorCause* MissingMandatoryParameterErrorCause::Clone(
		  uint8_t* buffer, size_t bufferLength) const
		{
			MS_TRACE();

			auto* clonedErrorCause = new MissingMandatoryParameterErrorCause(buffer, bufferLength);

			CloneInto(clonedErrorCause);

			return clonedErrorCause;
		}

		void MissingMandatoryParameterErrorCause::AddMissingParameterType(
		  Parameter::ParameterType parameterType)
		{
			MS_TRACE();

			AssertNotFrozen();

			// NOTE: This may throw.
			SetVariableLengthValueLength(GetVariableLengthValueLength() + 2);

			// Add the new missing mandatory parameter type.
			Utils::Byte::Set2Bytes(
			  GetVariableLengthValuePointer(),
			  GetNumberOfMissingParameters() * 2,
			  static_cast<uint16_t>(parameterType));

			// Update the counter field.
			SetNumberOfMissingParameters(GetNumberOfMissingParameters() + 1);
		}

		MissingMandatoryParameterErrorCause* MissingMandatoryParameterErrorCause::SoftClone(
		  const uint8_t* buffer) const
		{
			MS_TRACE();

			auto* softClonedErrorCause =
			  new MissingMandatoryParameterErrorCause(const_cast<uint8_t*>(buffer), GetLength());

			SoftCloneInto(softClonedErrorCause);

			return softClonedErrorCause;
		}

		void MissingMandatoryParameterErrorCause::SetNumberOfMissingParameters(uint32_t value)
		{
			MS_TRACE();

			AssertNotFrozen();

			Utils::Byte::Set4Bytes(const_cast<uint8_t*>(GetBuffer()), 4, value);
		}
	} // namespace SCTP
} // namespace RTC
