#define MS_CLASS "RTC::SCTP::AddIncomingStreamsRequestParameter"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/packet/parameters/AddIncomingStreamsRequestParameter.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"

namespace RTC
{
	namespace SCTP
	{
		/* Class methods. */

		AddIncomingStreamsRequestParameter* AddIncomingStreamsRequestParameter::Parse(
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

			if (parameterType != Parameter::ParameterType::ADD_INCOMING_STREAMS_REQUEST)
			{
				MS_WARN_DEV("invalid Parameter type");

				return nullptr;
			}

			return AddIncomingStreamsRequestParameter::ParseStrict(
			  buffer, bufferLength, parameterLength, padding);
		}

		AddIncomingStreamsRequestParameter* AddIncomingStreamsRequestParameter::Factory(
		  uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			if (bufferLength < AddIncomingStreamsRequestParameter::AddIncomingStreamsRequestParameterHeaderLength)
			{
				MS_THROW_TYPE_ERROR("buffer too small");
			}

			auto* parameter = new AddIncomingStreamsRequestParameter(buffer, bufferLength);

			parameter->InitializeHeader(
			  Parameter::ParameterType::ADD_INCOMING_STREAMS_REQUEST,
			  AddIncomingStreamsRequestParameter::AddIncomingStreamsRequestParameterHeaderLength);

			// Initialize header extra fields to zero.
			parameter->SetReconfigurationRequestSequenceNumber(0);
			parameter->SetNumberOfNewStreams(0);
			parameter->SetReserved();

			// No need to invoke SetLength() since parent constructor invoked it.

			return parameter;
		}

		AddIncomingStreamsRequestParameter* AddIncomingStreamsRequestParameter::ParseStrict(
		  const uint8_t* buffer, size_t bufferLength, uint16_t parameterLength, uint8_t padding)
		{
			MS_TRACE();

			if (parameterLength != AddIncomingStreamsRequestParameter::AddIncomingStreamsRequestParameterHeaderLength)
			{
				MS_WARN_TAG(
				  sctp,
				  "AddIncomingStreamsRequestParameter Length field must be %zu",
				  AddIncomingStreamsRequestParameter::AddIncomingStreamsRequestParameterHeaderLength);

				return nullptr;
			}

			auto* parameter =
			  new AddIncomingStreamsRequestParameter(const_cast<uint8_t*>(buffer), bufferLength);

			// Mark the Parameter as frozen since we are parsing.
			parameter->Freeze();

			return parameter;
		}

		/* Instance methods. */

		AddIncomingStreamsRequestParameter::AddIncomingStreamsRequestParameter(
		  uint8_t* buffer, size_t bufferLength)
		  : Parameter(buffer, bufferLength)
		{
			MS_TRACE();

			SetLength(AddIncomingStreamsRequestParameter::AddIncomingStreamsRequestParameterHeaderLength);
		}

		AddIncomingStreamsRequestParameter::~AddIncomingStreamsRequestParameter()
		{
			MS_TRACE();
		}

		void AddIncomingStreamsRequestParameter::Dump(int indentation) const
		{
			MS_TRACE();

			MS_DUMP_CLEAN(indentation, "<SCTP::AddIncomingStreamsRequestParameter>");
			DumpCommon(indentation);
			MS_DUMP_CLEAN(
			  indentation,
			  "  re-configuration request sequence number: %" PRIu32,
			  GetReconfigurationRequestSequenceNumber());
			MS_DUMP_CLEAN(indentation, "  number of new streams: %" PRIu16, GetNumberOfNewStreams());
			MS_DUMP_CLEAN(indentation, "</SCTP::AddIncomingStreamsRequestParameter>");
		}

		AddIncomingStreamsRequestParameter* AddIncomingStreamsRequestParameter::Clone(
		  uint8_t* buffer, size_t bufferLength) const
		{
			MS_TRACE();

			auto* clonedParameter = new AddIncomingStreamsRequestParameter(buffer, bufferLength);

			CloneInto(clonedParameter);

			return clonedParameter;
		}

		void AddIncomingStreamsRequestParameter::SetReconfigurationRequestSequenceNumber(uint32_t value)
		{
			MS_TRACE();

			AssertNotFrozen();

			Utils::Byte::Set4Bytes(const_cast<uint8_t*>(GetBuffer()), 4, value);
		}

		void AddIncomingStreamsRequestParameter::SetNumberOfNewStreams(uint16_t value)
		{
			MS_TRACE();

			AssertNotFrozen();

			Utils::Byte::Set2Bytes(const_cast<uint8_t*>(GetBuffer()), 8, value);
		}

		AddIncomingStreamsRequestParameter* AddIncomingStreamsRequestParameter::SoftClone(
		  const uint8_t* buffer) const
		{
			MS_TRACE();

			auto* softClonedParameter =
			  new AddIncomingStreamsRequestParameter(const_cast<uint8_t*>(buffer), GetLength());

			SoftCloneInto(softClonedParameter);

			return softClonedParameter;
		}

		void AddIncomingStreamsRequestParameter::SetReserved()
		{
			MS_TRACE();

			Utils::Byte::Set2Bytes(const_cast<uint8_t*>(GetBuffer()), 10, 0);
		}
	} // namespace SCTP
} // namespace RTC
