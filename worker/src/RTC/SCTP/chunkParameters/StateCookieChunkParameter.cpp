#define MS_CLASS "RTC::SCTP::StateCookieChunkParameter"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/chunkParameters/StateCookieChunkParameter.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"

namespace RTC
{
	namespace SCTP
	{
		/* Class methods. */

		StateCookieChunkParameter* StateCookieChunkParameter::Parse(const uint8_t* buffer, size_t bufferLength)
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

			if (parameterType != ChunkParameter::ChunkParameterType::STATE_COOKIE)
			{
				MS_WARN_DEV("invalid Chunk Parameter type");

				return nullptr;
			}

			return StateCookieChunkParameter::ParseStrict(buffer, bufferLength, parameterLength, padding);
		}

		StateCookieChunkParameter* StateCookieChunkParameter::Factory(uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			if (bufferLength < ChunkParameter::ChunkParameterHeaderLength)
			{
				MS_THROW_TYPE_ERROR("buffer too small");
			}

			auto* parameter = new StateCookieChunkParameter(buffer, bufferLength);

			parameter->InitializeHeader(
			  ChunkParameter::ChunkParameterType::STATE_COOKIE, ChunkParameter::ChunkParameterHeaderLength);

			// No need to invoke SetLength() since parent constructor invoked it.

			return parameter;
		}

		StateCookieChunkParameter* StateCookieChunkParameter::ParseStrict(
		  const uint8_t* buffer, size_t bufferLength, uint16_t parameterLength, uint8_t padding)
		{
			MS_TRACE();

			auto* parameter = new StateCookieChunkParameter(const_cast<uint8_t*>(buffer), bufferLength);

			// Must always invoke SetLength() after constructing a Serializable with
			// not fixed length.
			parameter->SetLength(parameterLength + padding);

			// Mark the Parameter as frozen since we are parsing.
			parameter->Freeze();

			return parameter;
		}

		/* Instance methods. */

		StateCookieChunkParameter::StateCookieChunkParameter(uint8_t* buffer, size_t bufferLength)
		  : ChunkParameter(buffer, bufferLength)
		{
			MS_TRACE();

			SetLength(ChunkParameter::ChunkParameterHeaderLength);
		}

		StateCookieChunkParameter::~StateCookieChunkParameter()
		{
			MS_TRACE();
		}

		void StateCookieChunkParameter::Dump(int indentation) const
		{
			MS_TRACE();

			MS_DUMP_CLEAN(indentation, "<SCTP::StateCookieChunkParameter>");
			DumpCommon(indentation);
			MS_DUMP_CLEAN(
			  indentation,
			  "  cookie length: %" PRIu16 " (has cookie: %s)",
			  GetCookieLength(),
			  HasCookie() ? "yes" : "no");
			MS_DUMP_CLEAN(indentation, "</SCTP::StateCookieChunkParameter>");
		}

		StateCookieChunkParameter* StateCookieChunkParameter::Clone(uint8_t* buffer, size_t bufferLength) const
		{
			MS_TRACE();

			auto* clonedParameter = new StateCookieChunkParameter(buffer, bufferLength);

			CloneInto(clonedParameter);

			return clonedParameter;
		}

		void StateCookieChunkParameter::SetCookie(const uint8_t* cookie, uint16_t cookieLength)
		{
			MS_TRACE();

			AssertNotFrozen();

			SetVariableLengthValue(cookie, cookieLength);
		}

		StateCookieChunkParameter* StateCookieChunkParameter::SoftClone(const uint8_t* buffer) const
		{
			MS_TRACE();

			auto* softClonedParameter =
			  new StateCookieChunkParameter(const_cast<uint8_t*>(buffer), GetLength());

			SoftCloneInto(softClonedParameter);

			return softClonedParameter;
		}
	} // namespace SCTP
} // namespace RTC
