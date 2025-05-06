#define MS_CLASS "RTC::SCTP::CookiePreservativeParameter"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/packet/parameters/CookiePreservativeParameter.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"

namespace RTC
{
	namespace SCTP
	{
		/* Class methods. */

		CookiePreservativeParameter* CookiePreservativeParameter::Parse(
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

			if (parameterType != Parameter::ParameterType::COOKIE_PRESERVATIVE)
			{
				MS_WARN_DEV("invalid Parameter type");

				return nullptr;
			}

			return CookiePreservativeParameter::ParseStrict(buffer, bufferLength, parameterLength, padding);
		}

		CookiePreservativeParameter* CookiePreservativeParameter::Factory(uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			if (bufferLength < CookiePreservativeParameter::CookiePreservativeParameterHeaderLength)
			{
				MS_THROW_TYPE_ERROR("buffer too small");
			}

			auto* parameter = new CookiePreservativeParameter(buffer, bufferLength);

			parameter->InitializeHeader(
			  Parameter::ParameterType::COOKIE_PRESERVATIVE,
			  CookiePreservativeParameter::CookiePreservativeParameterHeaderLength);

			// Must also initialize the value.
			parameter->SetLifeSpanIncrement(0);

			// No need to invoke SetLength() since parent constructor invoked it.

			return parameter;
		}

		CookiePreservativeParameter* CookiePreservativeParameter::ParseStrict(
		  const uint8_t* buffer, size_t bufferLength, uint16_t parameterLength, uint8_t padding)
		{
			MS_TRACE();

			if (parameterLength != CookiePreservativeParameter::CookiePreservativeParameterHeaderLength)
			{
				MS_WARN_TAG(
				  sctp,
				  "CookiePreservativeParameter Length field must be %zu",
				  CookiePreservativeParameter::CookiePreservativeParameterHeaderLength);

				return nullptr;
			}

			auto* parameter = new CookiePreservativeParameter(const_cast<uint8_t*>(buffer), bufferLength);

			// Mark the Parameter as frozen since we are parsing.
			parameter->Freeze();

			return parameter;
		}

		/* Instance methods. */

		CookiePreservativeParameter::CookiePreservativeParameter(uint8_t* buffer, size_t bufferLength)
		  : Parameter(buffer, bufferLength)
		{
			MS_TRACE();

			SetLength(CookiePreservativeParameter::CookiePreservativeParameterHeaderLength);
		}

		CookiePreservativeParameter::~CookiePreservativeParameter()
		{
			MS_TRACE();
		}

		void CookiePreservativeParameter::Dump(int indentation) const
		{
			MS_TRACE();

			MS_DUMP_CLEAN(indentation, "<SCTP::CookiePreservativeParameter>");
			DumpCommon(indentation);
			MS_DUMP_CLEAN(
			  indentation, "  suggested cookie life-span increment: %" PRIu32, GetLifeSpanIncrement());
			MS_DUMP_CLEAN(indentation, "</SCTP::CookiePreservativeParameter>");
		}

		CookiePreservativeParameter* CookiePreservativeParameter::Clone(
		  uint8_t* buffer, size_t bufferLength) const
		{
			MS_TRACE();

			auto* clonedParameter = new CookiePreservativeParameter(buffer, bufferLength);

			CloneInto(clonedParameter);

			return clonedParameter;
		}

		void CookiePreservativeParameter::SetLifeSpanIncrement(uint32_t increment)
		{
			MS_TRACE();

			AssertNotFrozen();

			Utils::Byte::Set4Bytes(const_cast<uint8_t*>(GetBuffer()), 4, increment);
		}

		CookiePreservativeParameter* CookiePreservativeParameter::SoftClone(const uint8_t* buffer) const
		{
			MS_TRACE();

			auto* softClonedParameter =
			  new CookiePreservativeParameter(const_cast<uint8_t*>(buffer), GetLength());

			SoftCloneInto(softClonedParameter);

			return softClonedParameter;
		}
	} // namespace SCTP
} // namespace RTC
