#define MS_CLASS "RTC::SCTP::StateCookieParameter"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/packet/parameters/StateCookieParameter.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"

namespace RTC
{
	namespace SCTP
	{
		/* Class methods. */

		StateCookieParameter* StateCookieParameter::Parse(const uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			Parameter::ParameterType parameterType;
			uint16_t parameterLength;
			uint8_t padding;

			if (!Parameter::IsParameter(buffer, bufferLength, parameterType, parameterLength, padding))
			{
				return nullptr;
			}

			if (parameterType != Parameter::ParameterType::STATE_COOKIE)
			{
				MS_WARN_DEV("invalid Parameter type");

				return nullptr;
			}

			return StateCookieParameter::ParseStrict(buffer, bufferLength, parameterLength, padding);
		}

		StateCookieParameter* StateCookieParameter::Factory(uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			if (bufferLength < Parameter::ParameterHeaderLength)
			{
				MS_THROW_TYPE_ERROR("buffer too small");
			}

			auto* parameter = new StateCookieParameter(buffer, bufferLength);

			parameter->InitializeHeader(
			  Parameter::ParameterType::STATE_COOKIE, Parameter::ParameterHeaderLength);

			// No need to invoke SetLength() since parent constructor invoked it.

			return parameter;
		}

		StateCookieParameter* StateCookieParameter::ParseStrict(
		  const uint8_t* buffer, size_t bufferLength, uint16_t parameterLength, uint8_t padding)
		{
			MS_TRACE();

			auto* parameter = new StateCookieParameter(const_cast<uint8_t*>(buffer), bufferLength);

			// Must always invoke SetLength() after constructing a Serializable with
			// not fixed length.
			parameter->SetLength(parameterLength + padding);

			// Mark the Parameter as frozen since we are parsing.
			parameter->Freeze();

			return parameter;
		}

		/* Instance methods. */

		StateCookieParameter::StateCookieParameter(uint8_t* buffer, size_t bufferLength)
		  : Parameter(buffer, bufferLength)
		{
			MS_TRACE();

			SetLength(Parameter::ParameterHeaderLength);
		}

		StateCookieParameter::~StateCookieParameter()
		{
			MS_TRACE();
		}

		void StateCookieParameter::Dump(int indentation) const
		{
			MS_TRACE();

			MS_DUMP_CLEAN(indentation, "<SCTP::StateCookieParameter>");
			DumpCommon(indentation);
			MS_DUMP_CLEAN(
			  indentation,
			  "  cookie length: %" PRIu16 " (has cookie: %s)",
			  GetCookieLength(),
			  HasCookie() ? "yes" : "no");
			MS_DUMP_CLEAN(indentation, "</SCTP::StateCookieParameter>");
		}

		StateCookieParameter* StateCookieParameter::Clone(uint8_t* buffer, size_t bufferLength) const
		{
			MS_TRACE();

			auto* clonedParameter = new StateCookieParameter(buffer, bufferLength);

			CloneInto(clonedParameter);

			return clonedParameter;
		}

		void StateCookieParameter::SetCookie(const uint8_t* cookie, uint16_t cookieLength)
		{
			MS_TRACE();

			AssertNotFrozen();

			SetVariableLengthValue(cookie, cookieLength);
		}

		StateCookieParameter* StateCookieParameter::SoftClone(const uint8_t* buffer) const
		{
			MS_TRACE();

			auto* softClonedParameter = new StateCookieParameter(const_cast<uint8_t*>(buffer), GetLength());

			SoftCloneInto(softClonedParameter);

			return softClonedParameter;
		}
	} // namespace SCTP
} // namespace RTC
