#define MS_CLASS "RTC::SCTP::AddOutgoingStreamsRequestParameter"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/packet/parameters/AddOutgoingStreamsRequestParameter.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"

namespace RTC
{
	namespace SCTP
	{
		/* Class methods. */

		AddOutgoingStreamsRequestParameter* AddOutgoingStreamsRequestParameter::Parse(
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

			if (parameterType != Parameter::ParameterType::ADD_OUTGOING_STREAMS_REQUEST)
			{
				MS_WARN_DEV("invalid Parameter type");

				return nullptr;
			}

			return AddOutgoingStreamsRequestParameter::ParseStrict(
			  buffer, bufferLength, parameterLength, padding);
		}

		AddOutgoingStreamsRequestParameter* AddOutgoingStreamsRequestParameter::Factory(
		  uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			if (bufferLength < AddOutgoingStreamsRequestParameter::AddOutgoingStreamsRequestParameterHeaderLength)
			{
				MS_THROW_TYPE_ERROR("buffer too small");
			}

			auto* parameter = new AddOutgoingStreamsRequestParameter(buffer, bufferLength);

			parameter->InitializeHeader(
			  Parameter::ParameterType::ADD_OUTGOING_STREAMS_REQUEST,
			  AddOutgoingStreamsRequestParameter::AddOutgoingStreamsRequestParameterHeaderLength);

			// Initialize header extra fields to zero.
			parameter->SetReconfigurationRequestSequenceNumber(0);
			parameter->SetNumberOfNewStreams(0);
			parameter->SetReserved();

			// No need to invoke SetLength() since parent constructor invoked it.

			return parameter;
		}

		AddOutgoingStreamsRequestParameter* AddOutgoingStreamsRequestParameter::ParseStrict(
		  const uint8_t* buffer, size_t bufferLength, uint16_t parameterLength, uint8_t padding)
		{
			MS_TRACE();

			if (parameterLength != AddOutgoingStreamsRequestParameter::AddOutgoingStreamsRequestParameterHeaderLength)
			{
				MS_WARN_TAG(
				  sctp,
				  "AddOutgoingStreamsRequestParameter Length field must be %zu",
				  AddOutgoingStreamsRequestParameter::AddOutgoingStreamsRequestParameterHeaderLength);

				return nullptr;
			}

			auto* parameter =
			  new AddOutgoingStreamsRequestParameter(const_cast<uint8_t*>(buffer), bufferLength);

			// Mark the Parameter as frozen since we are parsing.
			parameter->Freeze();

			return parameter;
		}

		/* Instance methods. */

		AddOutgoingStreamsRequestParameter::AddOutgoingStreamsRequestParameter(
		  uint8_t* buffer, size_t bufferLength)
		  : Parameter(buffer, bufferLength)
		{
			MS_TRACE();

			SetLength(AddOutgoingStreamsRequestParameter::AddOutgoingStreamsRequestParameterHeaderLength);
		}

		AddOutgoingStreamsRequestParameter::~AddOutgoingStreamsRequestParameter()
		{
			MS_TRACE();
		}

		void AddOutgoingStreamsRequestParameter::Dump(int indentation) const
		{
			MS_TRACE();

			MS_DUMP_CLEAN(indentation, "<SCTP::AddOutgoingStreamsRequestParameter>");
			DumpCommon(indentation);
			MS_DUMP_CLEAN(
			  indentation,
			  "  re-configuration request sequence number: %" PRIu32,
			  GetReconfigurationRequestSequenceNumber());
			MS_DUMP_CLEAN(indentation, "  number of new streams: %" PRIu16, GetNumberOfNewStreams());
			MS_DUMP_CLEAN(indentation, "</SCTP::AddOutgoingStreamsRequestParameter>");
		}

		AddOutgoingStreamsRequestParameter* AddOutgoingStreamsRequestParameter::Clone(
		  uint8_t* buffer, size_t bufferLength) const
		{
			MS_TRACE();

			auto* clonedParameter = new AddOutgoingStreamsRequestParameter(buffer, bufferLength);

			CloneInto(clonedParameter);

			return clonedParameter;
		}

		void AddOutgoingStreamsRequestParameter::SetReconfigurationRequestSequenceNumber(uint32_t value)
		{
			MS_TRACE();

			AssertNotFrozen();

			Utils::Byte::Set4Bytes(const_cast<uint8_t*>(GetBuffer()), 4, value);
		}

		void AddOutgoingStreamsRequestParameter::SetNumberOfNewStreams(uint16_t value)
		{
			MS_TRACE();

			AssertNotFrozen();

			Utils::Byte::Set2Bytes(const_cast<uint8_t*>(GetBuffer()), 8, value);
		}

		AddOutgoingStreamsRequestParameter* AddOutgoingStreamsRequestParameter::SoftClone(
		  const uint8_t* buffer) const
		{
			MS_TRACE();

			auto* softClonedParameter =
			  new AddOutgoingStreamsRequestParameter(const_cast<uint8_t*>(buffer), GetLength());

			SoftCloneInto(softClonedParameter);

			return softClonedParameter;
		}

		void AddOutgoingStreamsRequestParameter::SetReserved()
		{
			MS_TRACE();

			Utils::Byte::Set2Bytes(const_cast<uint8_t*>(GetBuffer()), 10, 0);
		}
	} // namespace SCTP
} // namespace RTC
