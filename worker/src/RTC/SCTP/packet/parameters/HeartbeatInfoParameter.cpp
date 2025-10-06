#define MS_CLASS "RTC::SCTP::HeartbeatInfoParameter"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/packet/parameters/HeartbeatInfoParameter.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"

namespace RTC
{
	namespace SCTP
	{
		/* Class methods. */

		HeartbeatInfoParameter* HeartbeatInfoParameter::Parse(const uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			Parameter::ParameterType parameterType;
			uint16_t parameterLength;
			uint8_t padding;

			if (!Parameter::IsParameter(buffer, bufferLength, parameterType, parameterLength, padding))
			{
				return nullptr;
			}

			if (parameterType != Parameter::ParameterType::HEARTBEAT_INFO)
			{
				MS_WARN_DEV("invalid Parameter type");

				return nullptr;
			}

			return HeartbeatInfoParameter::ParseStrict(buffer, bufferLength, parameterLength, padding);
		}

		HeartbeatInfoParameter* HeartbeatInfoParameter::Factory(uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			if (bufferLength < Parameter::ParameterHeaderLength)
			{
				MS_THROW_TYPE_ERROR("buffer too small");
			}

			auto* parameter = new HeartbeatInfoParameter(buffer, bufferLength);

			parameter->InitializeHeader(
			  Parameter::ParameterType::HEARTBEAT_INFO, Parameter::ParameterHeaderLength);

			// No need to invoke SetLength() since parent constructor invoked it.

			return parameter;
		}

		HeartbeatInfoParameter* HeartbeatInfoParameter::ParseStrict(
		  const uint8_t* buffer, size_t bufferLength, uint16_t parameterLength, uint8_t padding)
		{
			MS_TRACE();

			auto* parameter = new HeartbeatInfoParameter(const_cast<uint8_t*>(buffer), bufferLength);

			// Must always invoke SetLength() after constructing a Serializable with
			// not fixed length.
			parameter->SetLength(parameterLength + padding);

			// Mark the Parameter as frozen since we are parsing.
			parameter->Freeze();

			return parameter;
		}

		/* Instance methods. */

		HeartbeatInfoParameter::HeartbeatInfoParameter(uint8_t* buffer, size_t bufferLength)
		  : Parameter(buffer, bufferLength)
		{
			MS_TRACE();

			SetLength(Parameter::ParameterHeaderLength);
		}

		HeartbeatInfoParameter::~HeartbeatInfoParameter()
		{
			MS_TRACE();
		}

		void HeartbeatInfoParameter::Dump(int indentation) const
		{
			MS_TRACE();

			MS_DUMP_CLEAN(indentation, "<SCTP::HeartbeatInfoParameter>");
			DumpCommon(indentation);
			MS_DUMP_CLEAN(
			  indentation,
			  "  info length: %" PRIu16 " (has info: %s)",
			  GetInfoLength(),
			  HasInfo() ? "yes" : "no");
			MS_DUMP_CLEAN(indentation, "</SCTP::HeartbeatInfoParameter>");
		}

		HeartbeatInfoParameter* HeartbeatInfoParameter::Clone(uint8_t* buffer, size_t bufferLength) const
		{
			MS_TRACE();

			auto* clonedParameter = new HeartbeatInfoParameter(buffer, bufferLength);

			CloneInto(clonedParameter);

			return clonedParameter;
		}

		void HeartbeatInfoParameter::SetInfo(const uint8_t* info, uint16_t infoLength)
		{
			MS_TRACE();

			AssertNotFrozen();

			SetVariableLengthValue(info, infoLength);
		}

		HeartbeatInfoParameter* HeartbeatInfoParameter::SoftClone(const uint8_t* buffer) const
		{
			MS_TRACE();

			auto* softClonedParameter =
			  new HeartbeatInfoParameter(const_cast<uint8_t*>(buffer), GetLength());

			SoftCloneInto(softClonedParameter);

			return softClonedParameter;
		}
	} // namespace SCTP
} // namespace RTC
