#define MS_CLASS "RTC::SCTP::UnrecognizedParameterParameter"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/packet/parameters/UnrecognizedParameterParameter.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"

namespace RTC
{
	namespace SCTP
	{
		/* Class methods. */

		UnrecognizedParameterParameter* UnrecognizedParameterParameter::Parse(
		  const uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			Parameter::ParameterType parameterType;
			uint16_t parameterLength;
			uint8_t padding;

			if (!Parameter::IsParameter(buffer, bufferLength, parameterType, parameterLength, padding))
			{
				return nullptr;
			}

			if (parameterType != Parameter::ParameterType::UNRECOGNIZED_PARAMETER)
			{
				MS_WARN_DEV("invalid Parameter type");

				return nullptr;
			}

			return UnrecognizedParameterParameter::ParseStrict(
			  buffer, bufferLength, parameterLength, padding);
		}

		UnrecognizedParameterParameter* UnrecognizedParameterParameter::Factory(
		  uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			if (bufferLength < Parameter::ParameterHeaderLength)
			{
				MS_THROW_TYPE_ERROR("buffer too small");
			}

			auto* parameter = new UnrecognizedParameterParameter(buffer, bufferLength);

			parameter->InitializeHeader(
			  Parameter::ParameterType::UNRECOGNIZED_PARAMETER, Parameter::ParameterHeaderLength);

			// No need to invoke SetLength() since parent constructor invoked it.

			return parameter;
		}

		UnrecognizedParameterParameter* UnrecognizedParameterParameter::ParseStrict(
		  const uint8_t* buffer, size_t bufferLength, uint16_t parameterLength, uint8_t padding)
		{
			MS_TRACE();

			auto* parameter =
			  new UnrecognizedParameterParameter(const_cast<uint8_t*>(buffer), bufferLength);

			// Must always invoke SetLength() after constructing a Serializable with
			// not fixed length.
			parameter->SetLength(parameterLength + padding);

			// Mark the Parameter as frozen since we are parsing.
			parameter->Freeze();

			return parameter;
		}

		/* Instance methods. */

		UnrecognizedParameterParameter::UnrecognizedParameterParameter(uint8_t* buffer, size_t bufferLength)
		  : Parameter(buffer, bufferLength)
		{
			MS_TRACE();

			SetLength(Parameter::ParameterHeaderLength);
		}

		UnrecognizedParameterParameter::~UnrecognizedParameterParameter()
		{
			MS_TRACE();
		}

		void UnrecognizedParameterParameter::Dump(int indentation) const
		{
			MS_TRACE();

			MS_DUMP_CLEAN(indentation, "<SCTP::UnrecognizedParameterParameter>");
			DumpCommon(indentation);
			MS_DUMP_CLEAN(
			  indentation,
			  "  unrecognized parameter length: %" PRIu16 " (has unrecognized parameter: %s)",
			  GetUnrecognizedParameterLength(),
			  HasUnrecognizedParameter() ? "yes" : "no");
			MS_DUMP_CLEAN(indentation, "</SCTP::UnrecognizedParameterParameter>");
		}

		UnrecognizedParameterParameter* UnrecognizedParameterParameter::Clone(
		  uint8_t* buffer, size_t bufferLength) const
		{
			MS_TRACE();

			auto* clonedParameter = new UnrecognizedParameterParameter(buffer, bufferLength);

			CloneInto(clonedParameter);

			return clonedParameter;
		}

		void UnrecognizedParameterParameter::SetUnrecognizedParameter(
		  const uint8_t* parameter, uint16_t parameterLength)
		{
			MS_TRACE();

			AssertNotFrozen();

			SetVariableLengthValue(parameter, parameterLength);
		}

		UnrecognizedParameterParameter* UnrecognizedParameterParameter::SoftClone(const uint8_t* buffer) const
		{
			MS_TRACE();

			auto* softClonedParameter =
			  new UnrecognizedParameterParameter(const_cast<uint8_t*>(buffer), GetLength());

			SoftCloneInto(softClonedParameter);

			return softClonedParameter;
		}
	} // namespace SCTP
} // namespace RTC
