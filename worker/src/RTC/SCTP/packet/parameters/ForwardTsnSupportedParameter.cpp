#define MS_CLASS "RTC::SCTP::ForwardTsnSupportedParameter"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/packet/parameters/ForwardTsnSupportedParameter.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"

namespace RTC
{
	namespace SCTP
	{
		/* Class methods. */

		ForwardTsnSupportedParameter* ForwardTsnSupportedParameter::Parse(
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

			if (parameterType != Parameter::ParameterType::FORWARD_TSN_SUPPORTED)
			{
				MS_WARN_DEV("invalid Parameter type");

				return nullptr;
			}

			return ForwardTsnSupportedParameter::ParseStrict(buffer, bufferLength, parameterLength, padding);
		}

		ForwardTsnSupportedParameter* ForwardTsnSupportedParameter::Factory(
		  uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			if (bufferLength < Parameter::ParameterHeaderLength)
			{
				MS_THROW_TYPE_ERROR("buffer too small");
			}

			auto* parameter = new ForwardTsnSupportedParameter(buffer, bufferLength);

			parameter->InitializeHeader(
			  Parameter::ParameterType::FORWARD_TSN_SUPPORTED, Parameter::ParameterHeaderLength);

			// No need to invoke SetLength() since parent constructor invoked it.

			return parameter;
		}

		ForwardTsnSupportedParameter* ForwardTsnSupportedParameter::ParseStrict(
		  const uint8_t* buffer, size_t bufferLength, uint16_t parameterLength, uint8_t padding)
		{
			MS_TRACE();

			if (parameterLength != Parameter::ParameterHeaderLength)
			{
				MS_WARN_TAG(
				  sctp,
				  "ForwardTsnSupportedParameter Length field must be %zu",
				  Parameter::ParameterHeaderLength);

				return nullptr;
			}

			auto* parameter = new ForwardTsnSupportedParameter(const_cast<uint8_t*>(buffer), bufferLength);

			// Mark the Parameter as frozen since we are parsing.
			parameter->Freeze();

			return parameter;
		}

		/* Instance methods. */

		ForwardTsnSupportedParameter::ForwardTsnSupportedParameter(uint8_t* buffer, size_t bufferLength)
		  : Parameter(buffer, bufferLength)
		{
			MS_TRACE();

			SetLength(Parameter::ParameterHeaderLength);
		}

		ForwardTsnSupportedParameter::~ForwardTsnSupportedParameter()
		{
			MS_TRACE();
		}

		void ForwardTsnSupportedParameter::Dump(int indentation) const
		{
			MS_TRACE();

			MS_DUMP_CLEAN(indentation, "<SCTP::ForwardTsnSupportedParameter>");
			DumpCommon(indentation);
			MS_DUMP_CLEAN(indentation, "</SCTP::ForwardTsnSupportedParameter>");
		}

		ForwardTsnSupportedParameter* ForwardTsnSupportedParameter::Clone(
		  uint8_t* buffer, size_t bufferLength) const
		{
			MS_TRACE();

			auto* clonedParameter = new ForwardTsnSupportedParameter(buffer, bufferLength);

			CloneInto(clonedParameter);

			return clonedParameter;
		}

		ForwardTsnSupportedParameter* ForwardTsnSupportedParameter::SoftClone(const uint8_t* buffer) const
		{
			MS_TRACE();

			auto* softClonedParameter =
			  new ForwardTsnSupportedParameter(const_cast<uint8_t*>(buffer), GetLength());

			SoftCloneInto(softClonedParameter);

			return softClonedParameter;
		}
	} // namespace SCTP
} // namespace RTC
