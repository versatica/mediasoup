#define MS_CLASS "RTC::SCTP::IPv4AddressChunkParameter"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/chunkParameters/IPv4AddressChunkParameter.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include <uv.h>
#include <cstring> // std::memset(), std::memmove()

namespace RTC
{
	namespace SCTP
	{
		/* Class methods. */

		IPv4AddressChunkParameter* IPv4AddressChunkParameter::Parse(const uint8_t* buffer, size_t bufferLength)
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

			if (parameterType != ChunkParameter::ChunkParameterType::IPV4_ADDRESS)
			{
				MS_WARN_DEV("invalid Chunk Parameter type");

				return nullptr;
			}

			return IPv4AddressChunkParameter::ParseStrict(buffer, bufferLength, parameterLength, padding);
		}

		IPv4AddressChunkParameter* IPv4AddressChunkParameter::Factory(uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			if (bufferLength < IPv4AddressChunkParameter::IPv4AddressChunkParameterHeaderLength)
			{
				MS_THROW_TYPE_ERROR("buffer too small");
			}

			auto* parameter = new IPv4AddressChunkParameter(buffer, bufferLength);

			parameter->InitializeHeader(
			  ChunkParameter::ChunkParameterType::IPV4_ADDRESS,
			  IPv4AddressChunkParameter::IPv4AddressChunkParameterHeaderLength);

			// Must also initialize the IPv4 field to zero.
			parameter->ResetIPv4Address();

			// No need to invoke SetLength() since parent constructor invoked it.

			return parameter;
		}

		IPv4AddressChunkParameter* IPv4AddressChunkParameter::ParseStrict(
		  const uint8_t* buffer, size_t bufferLength, uint16_t parameterLength, uint8_t padding)
		{
			MS_TRACE();

			if (parameterLength != IPv4AddressChunkParameter::IPv4AddressChunkParameterHeaderLength)
			{
				MS_WARN_TAG(
				  sctp,
				  "IPv4AddressChunkParameter Length field must be %zu",
				  IPv4AddressChunkParameter::IPv4AddressChunkParameterHeaderLength);

				return nullptr;
			}

			auto* parameter = new IPv4AddressChunkParameter(const_cast<uint8_t*>(buffer), bufferLength);

			// Mark the Parameter as frozen since we are parsing.
			parameter->Freeze();

			return parameter;
		}

		/* Instance methods. */

		IPv4AddressChunkParameter::IPv4AddressChunkParameter(uint8_t* buffer, size_t bufferLength)
		  : ChunkParameter(buffer, bufferLength)
		{
			MS_TRACE();

			SetLength(IPv4AddressChunkParameter::IPv4AddressChunkParameterHeaderLength);
		}

		IPv4AddressChunkParameter::~IPv4AddressChunkParameter()
		{
			MS_TRACE();
		}

		void IPv4AddressChunkParameter::Dump(int indentation) const
		{
			MS_TRACE();

			char ipStr[INET_ADDRSTRLEN] = { 0 };

			uv_inet_ntop(AF_INET, GetIPv4Address(), ipStr, sizeof(ipStr));

			MS_DUMP_CLEAN(indentation, "<SCTP::IPv4AddressChunkParameter>");
			DumpCommon(indentation);
			MS_DUMP_CLEAN(indentation, "  ipv4 address: %s", ipStr);
			MS_DUMP_CLEAN(indentation, "</SCTP::IPv4AddressChunkParameter>");
		}

		IPv4AddressChunkParameter* IPv4AddressChunkParameter::Clone(uint8_t* buffer, size_t bufferLength) const
		{
			MS_TRACE();

			auto* clonedParameter = new IPv4AddressChunkParameter(buffer, bufferLength);

			CloneInto(clonedParameter);

			return clonedParameter;
		}

		void IPv4AddressChunkParameter::SetIPv4Address(const uint8_t* ip)
		{
			MS_TRACE();

			AssertNotFrozen();

			std::memmove(const_cast<uint8_t*>(GetBuffer()) + 4, ip, 4);
		}

		IPv4AddressChunkParameter* IPv4AddressChunkParameter::SoftClone(const uint8_t* buffer) const
		{
			MS_TRACE();

			auto* softClonedParameter =
			  new IPv4AddressChunkParameter(const_cast<uint8_t*>(buffer), GetLength());

			SoftCloneInto(softClonedParameter);

			return softClonedParameter;
		}

		void IPv4AddressChunkParameter::ResetIPv4Address()
		{
			MS_TRACE();

			AssertNotFrozen();

			std::memset(const_cast<uint8_t*>(GetBuffer()) + 4, 0x00, 4);
		}
	} // namespace SCTP
} // namespace RTC
