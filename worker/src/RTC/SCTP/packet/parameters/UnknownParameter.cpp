#define MS_CLASS "RTC::SCTP::UnknownParameter"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/packet/parameters/UnknownParameter.hpp"
#include "Logger.hpp"

namespace RTC
{
	namespace SCTP
	{
		/* Class methods. */

		UnknownParameter* UnknownParameter::Parse(const uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			Parameter::ParameterType parameterType;
			uint16_t parameterLength;
			uint8_t padding;

			if (!Parameter::IsParameter(buffer, bufferLength, parameterType, parameterLength, padding))
			{
				return nullptr;
			}

			return UnknownParameter::ParseStrict(buffer, bufferLength, parameterLength, padding);
		}

		UnknownParameter* UnknownParameter::ParseStrict(
		  const uint8_t* buffer, size_t bufferLength, uint16_t parameterLength, uint8_t padding)
		{
			MS_TRACE();

			auto* parameter = new UnknownParameter(const_cast<uint8_t*>(buffer), bufferLength);

			// Must always invoke SetLength() after constructing a Serializable with
			// not fixed length.
			parameter->SetLength(parameterLength + padding);

			// Mark the Parameter as frozen since we are parsing.
			parameter->Freeze();

			return parameter;
		}

		/* Instance methods. */

		UnknownParameter::UnknownParameter(uint8_t* buffer, size_t bufferLength)
		  : Parameter(buffer, bufferLength)
		{
			MS_TRACE();

			SetLength(Parameter::ParameterHeaderLength);
		}

		UnknownParameter::~UnknownParameter()
		{
			MS_TRACE();
		}

		void UnknownParameter::Dump(int indentation) const
		{
			MS_TRACE();

			MS_DUMP_CLEAN(indentation, "<SCTP::UnknownParameter>");
			DumpCommon(indentation);
			MS_DUMP_CLEAN(indentation, "</SCTP::UnknownParameter>");
		}

		UnknownParameter* UnknownParameter::Clone(uint8_t* buffer, size_t bufferLength) const
		{
			MS_TRACE();

			auto* clonedParameter = new UnknownParameter(buffer, bufferLength);

			CloneInto(clonedParameter);

			return clonedParameter;
		}

		UnknownParameter* UnknownParameter::SoftClone(const uint8_t* buffer) const
		{
			MS_TRACE();

			auto* softClonedParameter = new UnknownParameter(const_cast<uint8_t*>(buffer), GetLength());

			SoftCloneInto(softClonedParameter);

			return softClonedParameter;
		}
	} // namespace SCTP
} // namespace RTC
