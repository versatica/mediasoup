#define MS_CLASS "RTC::SCTP::IncomingSsnResetRequestParameter"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/packet/parameters/IncomingSsnResetRequestParameter.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"

namespace RTC
{
	namespace SCTP
	{
		/* Class methods. */

		IncomingSsnResetRequestParameter* IncomingSsnResetRequestParameter::Parse(
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

			if (parameterType != Parameter::ParameterType::INCOMING_SSN_RESET_REQUEST)
			{
				MS_WARN_DEV("invalid Parameter type");

				return nullptr;
			}

			return IncomingSsnResetRequestParameter::ParseStrict(
			  buffer, bufferLength, parameterLength, padding);
		}

		IncomingSsnResetRequestParameter* IncomingSsnResetRequestParameter::Factory(
		  uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			if (bufferLength < IncomingSsnResetRequestParameter::IncomingSsnResetRequestParameterHeaderLength)
			{
				MS_THROW_TYPE_ERROR("buffer too small");
			}

			auto* parameter = new IncomingSsnResetRequestParameter(buffer, bufferLength);

			parameter->InitializeHeader(
			  Parameter::ParameterType::INCOMING_SSN_RESET_REQUEST,
			  IncomingSsnResetRequestParameter::IncomingSsnResetRequestParameterHeaderLength);

			// Must also initialize extra fields in the header.
			parameter->SetReconfigurationRequestSequenceNumber(0);

			// No need to invoke SetLength() since parent constructor invoked it.

			return parameter;
		}

		IncomingSsnResetRequestParameter* IncomingSsnResetRequestParameter::ParseStrict(
		  const uint8_t* buffer, size_t bufferLength, uint16_t parameterLength, uint8_t padding)
		{
			MS_TRACE();

			if (parameterLength < IncomingSsnResetRequestParameter::IncomingSsnResetRequestParameterHeaderLength)
			{
				MS_WARN_TAG(
				  sctp,
				  "IncomingSsnResetRequestParameter Length field must be equal or greater than %zu",
				  IncomingSsnResetRequestParameter::IncomingSsnResetRequestParameterHeaderLength);

				return nullptr;
			}

			auto* parameter =
			  new IncomingSsnResetRequestParameter(const_cast<uint8_t*>(buffer), bufferLength);

			// Must always invoke SetLength() after constructing a Serializable with
			// not fixed length.
			parameter->SetLength(parameterLength + padding);

			// Mark the Parameter as frozen since we are parsing.
			parameter->Freeze();

			return parameter;
		}

		/* Instance methods. */

		IncomingSsnResetRequestParameter::IncomingSsnResetRequestParameter(uint8_t* buffer, size_t bufferLength)
		  : Parameter(buffer, bufferLength)
		{
			MS_TRACE();

			SetLength(IncomingSsnResetRequestParameter::IncomingSsnResetRequestParameterHeaderLength);
		}

		IncomingSsnResetRequestParameter::~IncomingSsnResetRequestParameter()
		{
			MS_TRACE();
		}

		void IncomingSsnResetRequestParameter::Dump(int indentation) const
		{
			MS_TRACE();

			MS_DUMP_CLEAN(indentation, "<SCTP::IncomingSsnResetRequestParameter>");
			DumpCommon(indentation);
			MS_DUMP_CLEAN(
			  indentation,
			  "  re-configuration request sequence number: %" PRIu32,
			  GetReconfigurationRequestSequenceNumber());
			MS_DUMP_CLEAN(indentation, "  number of streams: %" PRIu16, GetNumberOfStreams());
			for (uint32_t idx{ 0 }; idx < GetNumberOfStreams(); ++idx)
			{
				MS_DUMP_CLEAN(indentation, "  - idx: %" PRIu16 ", stream: %" PRIu16, idx, GetStreamAt(idx));
			}
			MS_DUMP_CLEAN(indentation, "</SCTP::IncomingSsnResetRequestParameter>");
		}

		IncomingSsnResetRequestParameter* IncomingSsnResetRequestParameter::Clone(
		  uint8_t* buffer, size_t bufferLength) const
		{
			MS_TRACE();

			auto* clonedParameter = new IncomingSsnResetRequestParameter(buffer, bufferLength);

			CloneInto(clonedParameter);

			return clonedParameter;
		}

		void IncomingSsnResetRequestParameter::SetReconfigurationRequestSequenceNumber(uint32_t value)
		{
			MS_TRACE();

			AssertNotFrozen();

			Utils::Byte::Set4Bytes(const_cast<uint8_t*>(GetBuffer()), 4, value);
		}

		void IncomingSsnResetRequestParameter::AddStream(uint16_t stream)
		{
			MS_TRACE();

			AssertNotFrozen();

			auto previousVariableLengthValueLength = GetVariableLengthValueLength();

			// NOTE: This may throw.
			SetVariableLengthValueLength(previousVariableLengthValueLength + 2);

			// Add the new stream.
			Utils::Byte::Set2Bytes(
			  GetVariableLengthValuePointer(), previousVariableLengthValueLength, stream);
		}

		IncomingSsnResetRequestParameter* IncomingSsnResetRequestParameter::SoftClone(
		  const uint8_t* buffer) const
		{
			MS_TRACE();

			auto* softClonedParameter =
			  new IncomingSsnResetRequestParameter(const_cast<uint8_t*>(buffer), GetLength());

			SoftCloneInto(softClonedParameter);

			return softClonedParameter;
		}
	} // namespace SCTP
} // namespace RTC
