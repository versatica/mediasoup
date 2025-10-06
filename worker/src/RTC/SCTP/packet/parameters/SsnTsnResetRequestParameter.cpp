#define MS_CLASS "RTC::SCTP::SsnTsnResetRequestParameter"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/packet/parameters/SsnTsnResetRequestParameter.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"

namespace RTC
{
	namespace SCTP
	{
		/* Class methods. */

		SsnTsnResetRequestParameter* SsnTsnResetRequestParameter::Parse(
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

			if (parameterType != Parameter::ParameterType::SSN_TSN_RESET_REQUEST)
			{
				MS_WARN_DEV("invalid Parameter type");

				return nullptr;
			}

			return SsnTsnResetRequestParameter::ParseStrict(buffer, bufferLength, parameterLength, padding);
		}

		SsnTsnResetRequestParameter* SsnTsnResetRequestParameter::Factory(uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			if (bufferLength < SsnTsnResetRequestParameter::SsnTsnResetRequestParameterHeaderLength)
			{
				MS_THROW_TYPE_ERROR("buffer too small");
			}

			auto* parameter = new SsnTsnResetRequestParameter(buffer, bufferLength);

			parameter->InitializeHeader(
			  Parameter::ParameterType::SSN_TSN_RESET_REQUEST,
			  SsnTsnResetRequestParameter::SsnTsnResetRequestParameterHeaderLength);

			// Initialize header extra fields to zero.
			parameter->SetReconfigurationRequestSequenceNumber(0);

			// No need to invoke SetLength() since parent constructor invoked it.

			return parameter;
		}

		SsnTsnResetRequestParameter* SsnTsnResetRequestParameter::ParseStrict(
		  const uint8_t* buffer, size_t bufferLength, uint16_t parameterLength, uint8_t padding)
		{
			MS_TRACE();

			if (parameterLength != SsnTsnResetRequestParameter::SsnTsnResetRequestParameterHeaderLength)
			{
				MS_WARN_TAG(
				  sctp,
				  "SsnTsnResetRequestParameter Length field must be %zu",
				  SsnTsnResetRequestParameter::SsnTsnResetRequestParameterHeaderLength);

				return nullptr;
			}

			auto* parameter = new SsnTsnResetRequestParameter(const_cast<uint8_t*>(buffer), bufferLength);

			// Mark the Parameter as frozen since we are parsing.
			parameter->Freeze();

			return parameter;
		}

		/* Instance methods. */

		SsnTsnResetRequestParameter::SsnTsnResetRequestParameter(uint8_t* buffer, size_t bufferLength)
		  : Parameter(buffer, bufferLength)
		{
			MS_TRACE();

			SetLength(SsnTsnResetRequestParameter::SsnTsnResetRequestParameterHeaderLength);
		}

		SsnTsnResetRequestParameter::~SsnTsnResetRequestParameter()
		{
			MS_TRACE();
		}

		void SsnTsnResetRequestParameter::Dump(int indentation) const
		{
			MS_TRACE();

			MS_DUMP_CLEAN(indentation, "<SCTP::SsnTsnResetRequestParameter>");
			DumpCommon(indentation);
			MS_DUMP_CLEAN(
			  indentation,
			  "  re-configuration request sequence number: %" PRIu32,
			  GetReconfigurationRequestSequenceNumber());
			MS_DUMP_CLEAN(indentation, "</SCTP::SsnTsnResetRequestParameter>");
		}

		SsnTsnResetRequestParameter* SsnTsnResetRequestParameter::Clone(
		  uint8_t* buffer, size_t bufferLength) const
		{
			MS_TRACE();

			auto* clonedParameter = new SsnTsnResetRequestParameter(buffer, bufferLength);

			CloneInto(clonedParameter);

			return clonedParameter;
		}

		void SsnTsnResetRequestParameter::SetReconfigurationRequestSequenceNumber(uint32_t value)
		{
			MS_TRACE();

			AssertNotFrozen();

			Utils::Byte::Set4Bytes(const_cast<uint8_t*>(GetBuffer()), 4, value);
		}

		SsnTsnResetRequestParameter* SsnTsnResetRequestParameter::SoftClone(const uint8_t* buffer) const
		{
			MS_TRACE();

			auto* softClonedParameter =
			  new SsnTsnResetRequestParameter(const_cast<uint8_t*>(buffer), GetLength());

			SoftCloneInto(softClonedParameter);

			return softClonedParameter;
		}
	} // namespace SCTP
} // namespace RTC
