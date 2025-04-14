#define MS_CLASS "RTC::SCTP::IPv6AddressChunkParameter"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/chunkParameters/IPv6AddressChunkParameter.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include <uv.h>
#include <cstring> // std::memset(), std::memmove()

namespace RTC
{
	namespace SCTP
	{
		/* Class methods. */

		IPv6AddressChunkParameter* IPv6AddressChunkParameter::Parse(const uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			ChunkParameter::ChunkParameterType parameterType;
			uint16_t parameterLength;
			uint8_t padding;

			if (!ChunkParameter::IsChunkParameter(
			      buffer, bufferLength, parameterType, parameterLength, padding))
			{
				return nullptr;
			}

			if (parameterType != ChunkParameter::ChunkParameterType::IPV6_ADDRESS)
			{
				MS_WARN_DEV("invalid Chunk Parameter type");

				return nullptr;
			}

			if (parameterLength != IPv6AddressChunkParameter::IPv6AddressChunkParameterLength)
			{
				MS_WARN_TAG(
				  sctp,
				  "IPv6AddressChunkParameter Length field must be %zu",
				  IPv6AddressChunkParameter::IPv6AddressChunkParameterLength);

				return nullptr;
			}

			auto* parameter = new IPv6AddressChunkParameter(buffer, bufferLength);

			// Mark the Parameter as frozen since we are parsing.
			parameter->Freeze();

			return parameter;
		}

		IPv6AddressChunkParameter* IPv6AddressChunkParameter::Factory(uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			if (bufferLength < IPv6AddressChunkParameter::IPv6AddressChunkParameterLength)
			{
				MS_THROW_TYPE_ERROR("too small buffer");
			}

			auto* parameter = new IPv6AddressChunkParameter(buffer, bufferLength);

			parameter->InitializeHeader(
			  ChunkParameter::ChunkParameterType::IPV6_ADDRESS,
			  IPv6AddressChunkParameter::IPv6AddressChunkParameterLength);

			// Must also initialize the IPv6 field to zero.
			std::memset(
			  parameter->GetValuePointer(),
			  0x00,
			  IPv6AddressChunkParameter::IPv6AddressChunkParameterLength -
			    ChunkParameter::ChunkParameterHeaderLength);

			// No need to invoke SetLength() since parent constructor invoked it.

			return parameter;
		}

		/* Instance methods. */

		IPv6AddressChunkParameter::IPv6AddressChunkParameter(const uint8_t* buffer, size_t bufferLength)
		  : ChunkParameter(buffer, bufferLength)
		{
			MS_TRACE();

			SetLength(IPv6AddressChunkParameter::IPv6AddressChunkParameterLength);
		}

		IPv6AddressChunkParameter::~IPv6AddressChunkParameter()
		{
			MS_TRACE();
		}

		void IPv6AddressChunkParameter::Dump(int indentation) const
		{
			MS_TRACE();

			char ipStr[INET6_ADDRSTRLEN] = { 0 };

			uv_inet_ntop(AF_INET6, GetIPv6Address(), ipStr, sizeof(ipStr));

			MS_DUMP_CLEAN(indentation, "<SCTP::IPv6AddressChunkParameter>");
			DumpCommon(indentation);
			MS_DUMP_CLEAN(indentation, "  ipv6 address: %s", ipStr);
			MS_DUMP_CLEAN(indentation, "</SCTP::IPv6AddressChunkParameter>");
		}

		IPv6AddressChunkParameter* IPv6AddressChunkParameter::Clone(uint8_t* buffer, size_t bufferLength) const
		{
			MS_TRACE();

			auto* clonedItem = new IPv6AddressChunkParameter(buffer, bufferLength);

			CloneInto(clonedItem);

			return clonedItem;
		}

		void IPv6AddressChunkParameter::SetIPv6Address(const uint8_t* ip)
		{
			MS_TRACE();

			AssertNotFrozen();

			std::memmove(
			  GetValuePointer(),
			  ip,
			  IPv6AddressChunkParameter::IPv6AddressChunkParameterLength -
			    ChunkParameter::ChunkParameterHeaderLength);
		}
	} // namespace SCTP
} // namespace RTC
