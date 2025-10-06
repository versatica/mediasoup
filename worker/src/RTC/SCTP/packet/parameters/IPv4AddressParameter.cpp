#define MS_CLASS "RTC::SCTP::IPv4AddressParameter"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/packet/parameters/IPv4AddressParameter.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include <uv.h>
#include <cstring> // std::memset(), std::memmove()

namespace RTC
{
	namespace SCTP
	{
		/* Class methods. */

		IPv4AddressParameter* IPv4AddressParameter::Parse(const uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			Parameter::ParameterType parameterType;
			uint16_t parameterLength;
			uint8_t padding;

			if (!Parameter::IsParameter(buffer, bufferLength, parameterType, parameterLength, padding))
			{
				return nullptr;
			}

			if (parameterType != Parameter::ParameterType::IPV4_ADDRESS)
			{
				MS_WARN_DEV("invalid Parameter type");

				return nullptr;
			}

			return IPv4AddressParameter::ParseStrict(buffer, bufferLength, parameterLength, padding);
		}

		IPv4AddressParameter* IPv4AddressParameter::Factory(uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			if (bufferLength < IPv4AddressParameter::IPv4AddressParameterHeaderLength)
			{
				MS_THROW_TYPE_ERROR("buffer too small");
			}

			auto* parameter = new IPv4AddressParameter(buffer, bufferLength);

			parameter->InitializeHeader(
			  Parameter::ParameterType::IPV4_ADDRESS,
			  IPv4AddressParameter::IPv4AddressParameterHeaderLength);

			// Must also initialize the IPv4 field to zero.
			parameter->ResetIPv4Address();

			// No need to invoke SetLength() since parent constructor invoked it.

			return parameter;
		}

		IPv4AddressParameter* IPv4AddressParameter::ParseStrict(
		  const uint8_t* buffer, size_t bufferLength, uint16_t parameterLength, uint8_t padding)
		{
			MS_TRACE();

			if (parameterLength != IPv4AddressParameter::IPv4AddressParameterHeaderLength)
			{
				MS_WARN_TAG(
				  sctp,
				  "IPv4AddressParameter Length field must be %zu",
				  IPv4AddressParameter::IPv4AddressParameterHeaderLength);

				return nullptr;
			}

			auto* parameter = new IPv4AddressParameter(const_cast<uint8_t*>(buffer), bufferLength);

			// Mark the Parameter as frozen since we are parsing.
			parameter->Freeze();

			return parameter;
		}

		/* Instance methods. */

		IPv4AddressParameter::IPv4AddressParameter(uint8_t* buffer, size_t bufferLength)
		  : Parameter(buffer, bufferLength)
		{
			MS_TRACE();

			SetLength(IPv4AddressParameter::IPv4AddressParameterHeaderLength);
		}

		IPv4AddressParameter::~IPv4AddressParameter()
		{
			MS_TRACE();
		}

		void IPv4AddressParameter::Dump(int indentation) const
		{
			MS_TRACE();

			char ipStr[INET_ADDRSTRLEN] = { 0 };

			uv_inet_ntop(AF_INET, GetIPv4Address(), ipStr, sizeof(ipStr));

			MS_DUMP_CLEAN(indentation, "<SCTP::IPv4AddressParameter>");
			DumpCommon(indentation);
			MS_DUMP_CLEAN(indentation, "  ipv4 address: %s", ipStr);
			MS_DUMP_CLEAN(indentation, "</SCTP::IPv4AddressParameter>");
		}

		IPv4AddressParameter* IPv4AddressParameter::Clone(uint8_t* buffer, size_t bufferLength) const
		{
			MS_TRACE();

			auto* clonedParameter = new IPv4AddressParameter(buffer, bufferLength);

			CloneInto(clonedParameter);

			return clonedParameter;
		}

		void IPv4AddressParameter::SetIPv4Address(const uint8_t* ip)
		{
			MS_TRACE();

			AssertNotFrozen();

			std::memmove(const_cast<uint8_t*>(GetBuffer()) + 4, ip, 4);
		}

		IPv4AddressParameter* IPv4AddressParameter::SoftClone(const uint8_t* buffer) const
		{
			MS_TRACE();

			auto* softClonedParameter = new IPv4AddressParameter(const_cast<uint8_t*>(buffer), GetLength());

			SoftCloneInto(softClonedParameter);

			return softClonedParameter;
		}

		void IPv4AddressParameter::ResetIPv4Address()
		{
			MS_TRACE();

			AssertNotFrozen();

			std::memset(const_cast<uint8_t*>(GetBuffer()) + 4, 0x00, 4);
		}
	} // namespace SCTP
} // namespace RTC
