#define MS_CLASS "RTC::SCTP::IPv6AddressParameter"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/packet/parameters/IPv6AddressParameter.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include <uv.h>
#include <cstring> // std::memset(), std::memmove()

namespace RTC
{
	namespace SCTP
	{
		/* Class methods. */

		IPv6AddressParameter* IPv6AddressParameter::Parse(const uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			Parameter::ParameterType parameterType;
			uint16_t parameterLength;
			uint8_t padding;

			if (!Parameter::IsParameter(buffer, bufferLength, parameterType, parameterLength, padding))
			{
				return nullptr;
			}

			if (parameterType != Parameter::ParameterType::IPV6_ADDRESS)
			{
				MS_WARN_DEV("invalid Parameter type");

				return nullptr;
			}

			return IPv6AddressParameter::ParseStrict(buffer, bufferLength, parameterLength, padding);
		}

		IPv6AddressParameter* IPv6AddressParameter::Factory(uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			if (bufferLength < IPv6AddressParameter::IPv6AddressParameterHeaderLength)
			{
				MS_THROW_TYPE_ERROR("buffer too small");
			}

			auto* parameter = new IPv6AddressParameter(buffer, bufferLength);

			parameter->InitializeHeader(
			  Parameter::ParameterType::IPV6_ADDRESS,
			  IPv6AddressParameter::IPv6AddressParameterHeaderLength);

			// Must also initialize the IPv6 field to zero.
			parameter->ResetIPv6Address();

			// No need to invoke SetLength() since parent constructor invoked it.

			return parameter;
		}

		IPv6AddressParameter* IPv6AddressParameter::ParseStrict(
		  const uint8_t* buffer, size_t bufferLength, uint16_t parameterLength, uint8_t padding)
		{
			MS_TRACE();

			if (parameterLength != IPv6AddressParameter::IPv6AddressParameterHeaderLength)
			{
				MS_WARN_TAG(
				  sctp,
				  "IPv6AddressParameter Length field must be %zu",
				  IPv6AddressParameter::IPv6AddressParameterHeaderLength);

				return nullptr;
			}

			auto* parameter = new IPv6AddressParameter(const_cast<uint8_t*>(buffer), bufferLength);

			// Mark the Parameter as frozen since we are parsing.
			parameter->Freeze();

			return parameter;
		}

		/* Instance methods. */

		IPv6AddressParameter::IPv6AddressParameter(uint8_t* buffer, size_t bufferLength)
		  : Parameter(buffer, bufferLength)
		{
			MS_TRACE();

			SetLength(IPv6AddressParameter::IPv6AddressParameterHeaderLength);
		}

		IPv6AddressParameter::~IPv6AddressParameter()
		{
			MS_TRACE();
		}

		void IPv6AddressParameter::Dump(int indentation) const
		{
			MS_TRACE();

			char ipStr[INET6_ADDRSTRLEN] = { 0 };

			uv_inet_ntop(AF_INET6, GetIPv6Address(), ipStr, sizeof(ipStr));

			MS_DUMP_CLEAN(indentation, "<SCTP::IPv6AddressParameter>");
			DumpCommon(indentation);
			MS_DUMP_CLEAN(indentation, "  ipv6 address: %s", ipStr);
			MS_DUMP_CLEAN(indentation, "</SCTP::IPv6AddressParameter>");
		}

		IPv6AddressParameter* IPv6AddressParameter::Clone(uint8_t* buffer, size_t bufferLength) const
		{
			MS_TRACE();

			auto* clonedParameter = new IPv6AddressParameter(buffer, bufferLength);

			CloneInto(clonedParameter);

			return clonedParameter;
		}

		void IPv6AddressParameter::SetIPv6Address(const uint8_t* ip)
		{
			MS_TRACE();

			AssertNotFrozen();

			std::memmove(const_cast<uint8_t*>(GetBuffer()) + 4, ip, 16);
		}

		IPv6AddressParameter* IPv6AddressParameter::SoftClone(const uint8_t* buffer) const
		{
			MS_TRACE();

			auto* softClonedParameter = new IPv6AddressParameter(const_cast<uint8_t*>(buffer), GetLength());

			SoftCloneInto(softClonedParameter);

			return softClonedParameter;
		}

		void IPv6AddressParameter::ResetIPv6Address()
		{
			MS_TRACE();

			AssertNotFrozen();

			std::memset(const_cast<uint8_t*>(GetBuffer()) + 4, 0x00, 16);
		}
	} // namespace SCTP
} // namespace RTC
