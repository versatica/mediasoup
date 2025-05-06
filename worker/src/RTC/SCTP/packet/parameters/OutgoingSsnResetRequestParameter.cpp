#define MS_CLASS "RTC::SCTP::OutgoingSsnResetRequestParameter"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/packet/parameters/OutgoingSsnResetRequestParameter.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"

namespace RTC
{
	namespace SCTP
	{
		/* Class methods. */

		OutgoingSsnResetRequestParameter* OutgoingSsnResetRequestParameter::Parse(
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

			if (parameterType != Parameter::ParameterType::OUTGOING_SSN_RESET_REQUEST)
			{
				MS_WARN_DEV("invalid Parameter type");

				return nullptr;
			}

			return OutgoingSsnResetRequestParameter::ParseStrict(
			  buffer, bufferLength, parameterLength, padding);
		}

		OutgoingSsnResetRequestParameter* OutgoingSsnResetRequestParameter::Factory(
		  uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			if (bufferLength < OutgoingSsnResetRequestParameter::OutgoingSsnResetRequestParameterHeaderLength)
			{
				MS_THROW_TYPE_ERROR("buffer too small");
			}

			auto* parameter = new OutgoingSsnResetRequestParameter(buffer, bufferLength);

			parameter->InitializeHeader(
			  Parameter::ParameterType::OUTGOING_SSN_RESET_REQUEST,
			  OutgoingSsnResetRequestParameter::OutgoingSsnResetRequestParameterHeaderLength);

			// Must also initialize extra fields in the header.
			parameter->SetReconfigurationRequestSequenceNumber(0);
			parameter->SetReconfigurationResponseSequenceNumber(0);
			parameter->SetSenderLastAssignedTsn(0);

			// No need to invoke SetLength() since parent constructor invoked it.

			return parameter;
		}

		OutgoingSsnResetRequestParameter* OutgoingSsnResetRequestParameter::ParseStrict(
		  const uint8_t* buffer, size_t bufferLength, uint16_t parameterLength, uint8_t padding)
		{
			MS_TRACE();

			if (parameterLength < OutgoingSsnResetRequestParameter::OutgoingSsnResetRequestParameterHeaderLength)
			{
				MS_WARN_TAG(
				  sctp,
				  "OutgoingSsnResetRequestParameter Length field must be equal or greater than %zu",
				  OutgoingSsnResetRequestParameter::OutgoingSsnResetRequestParameterHeaderLength);

				return nullptr;
			}

			auto* parameter =
			  new OutgoingSsnResetRequestParameter(const_cast<uint8_t*>(buffer), bufferLength);

			// Must always invoke SetLength() after constructing a Serializable with
			// not fixed length.
			parameter->SetLength(parameterLength + padding);

			// Mark the Parameter as frozen since we are parsing.
			parameter->Freeze();

			return parameter;
		}

		/* Instance methods. */

		OutgoingSsnResetRequestParameter::OutgoingSsnResetRequestParameter(uint8_t* buffer, size_t bufferLength)
		  : Parameter(buffer, bufferLength)
		{
			MS_TRACE();

			SetLength(OutgoingSsnResetRequestParameter::OutgoingSsnResetRequestParameterHeaderLength);
		}

		OutgoingSsnResetRequestParameter::~OutgoingSsnResetRequestParameter()
		{
			MS_TRACE();
		}

		void OutgoingSsnResetRequestParameter::Dump(int indentation) const
		{
			MS_TRACE();

			MS_DUMP_CLEAN(indentation, "<SCTP::OutgoingSsnResetRequestParameter>");
			DumpCommon(indentation);
			MS_DUMP_CLEAN(
			  indentation,
			  "  re-configuration request sequence number: %" PRIu32,
			  GetReconfigurationRequestSequenceNumber());
			MS_DUMP_CLEAN(
			  indentation,
			  "  re-configuration response sequence number: %" PRIu32,
			  GetReconfigurationResponseSequenceNumber());
			MS_DUMP_CLEAN(indentation, "  sender last assigned tsn: %" PRIu32, GetSenderLastAssignedTsn());
			MS_DUMP_CLEAN(indentation, "  number of streams: %" PRIu16, GetNumberOfStreams());
			for (uint32_t idx{ 0 }; idx < GetNumberOfStreams(); ++idx)
			{
				MS_DUMP_CLEAN(indentation, "  - idx: %" PRIu16 ", stream: %" PRIu16, idx, GetStreamAt(idx));
			}
			MS_DUMP_CLEAN(indentation, "</SCTP::OutgoingSsnResetRequestParameter>");
		}

		OutgoingSsnResetRequestParameter* OutgoingSsnResetRequestParameter::Clone(
		  uint8_t* buffer, size_t bufferLength) const
		{
			MS_TRACE();

			auto* clonedParameter = new OutgoingSsnResetRequestParameter(buffer, bufferLength);

			CloneInto(clonedParameter);

			return clonedParameter;
		}

		void OutgoingSsnResetRequestParameter::SetReconfigurationRequestSequenceNumber(uint32_t value)
		{
			MS_TRACE();

			AssertNotFrozen();

			Utils::Byte::Set4Bytes(const_cast<uint8_t*>(GetBuffer()), 4, value);
		}

		void OutgoingSsnResetRequestParameter::SetReconfigurationResponseSequenceNumber(uint32_t value)
		{
			MS_TRACE();

			AssertNotFrozen();

			Utils::Byte::Set4Bytes(const_cast<uint8_t*>(GetBuffer()), 8, value);
		}

		void OutgoingSsnResetRequestParameter::SetSenderLastAssignedTsn(uint32_t value)
		{
			MS_TRACE();

			AssertNotFrozen();

			Utils::Byte::Set4Bytes(const_cast<uint8_t*>(GetBuffer()), 12, value);
		}

		void OutgoingSsnResetRequestParameter::AddStream(uint16_t stream)
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

		OutgoingSsnResetRequestParameter* OutgoingSsnResetRequestParameter::SoftClone(
		  const uint8_t* buffer) const
		{
			MS_TRACE();

			auto* softClonedParameter =
			  new OutgoingSsnResetRequestParameter(const_cast<uint8_t*>(buffer), GetLength());

			SoftCloneInto(softClonedParameter);

			return softClonedParameter;
		}
	} // namespace SCTP
} // namespace RTC
