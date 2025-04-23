#define MS_CLASS "RTC::SCTP::CookiePreservativeChunkParameter"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/chunkParameters/CookiePreservativeChunkParameter.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"

namespace RTC
{
	namespace SCTP
	{
		/* Class methods. */

		CookiePreservativeChunkParameter* CookiePreservativeChunkParameter::Parse(
		  const uint8_t* buffer, size_t bufferLength)
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

			if (parameterType != ChunkParameter::ChunkParameterType::COOKIE_PRESERVATIVE)
			{
				MS_WARN_DEV("invalid Chunk Parameter type");

				return nullptr;
			}

			return CookiePreservativeChunkParameter::ParseStrict(
			  buffer, bufferLength, parameterLength, padding);
		}

		CookiePreservativeChunkParameter* CookiePreservativeChunkParameter::Factory(
		  uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			if (bufferLength < CookiePreservativeChunkParameter::CookiePreservativeChunkParameterLength)
			{
				MS_THROW_TYPE_ERROR("buffer too small");
			}

			auto* parameter = new CookiePreservativeChunkParameter(buffer, bufferLength);

			parameter->InitializeHeader(
			  ChunkParameter::ChunkParameterType::COOKIE_PRESERVATIVE,
			  CookiePreservativeChunkParameter::CookiePreservativeChunkParameterLength);

			// Must also initialize the value.
			parameter->SetLifeSpanIncrement(0);

			// No need to invoke SetLength() since parent constructor invoked it.

			return parameter;
		}

		CookiePreservativeChunkParameter* CookiePreservativeChunkParameter::ParseStrict(
		  const uint8_t* buffer, size_t bufferLength, uint16_t parameterLength, uint8_t padding)
		{
			MS_TRACE();

			if (parameterLength != CookiePreservativeChunkParameter::CookiePreservativeChunkParameterLength)
			{
				MS_WARN_TAG(
				  sctp,
				  "CookiePreservativeChunkParameter Length field must be %zu",
				  CookiePreservativeChunkParameter::CookiePreservativeChunkParameterLength);

				return nullptr;
			}

			auto* parameter =
			  new CookiePreservativeChunkParameter(const_cast<uint8_t*>(buffer), bufferLength);

			// Mark the Parameter as frozen since we are parsing.
			parameter->Freeze();

			return parameter;
		}

		/* Instance methods. */

		CookiePreservativeChunkParameter::CookiePreservativeChunkParameter(uint8_t* buffer, size_t bufferLength)
		  : ChunkParameter(buffer, bufferLength)
		{
			MS_TRACE();

			SetLength(CookiePreservativeChunkParameter::CookiePreservativeChunkParameterLength);
		}

		CookiePreservativeChunkParameter::~CookiePreservativeChunkParameter()
		{
			MS_TRACE();
		}

		void CookiePreservativeChunkParameter::Dump(int indentation) const
		{
			MS_TRACE();

			MS_DUMP_CLEAN(indentation, "<SCTP::CookiePreservativeChunkParameter>");
			DumpCommon(indentation);
			MS_DUMP_CLEAN(
			  indentation, "  suggested cookie life-span increment: %" PRIu32, GetLifeSpanIncrement());
			MS_DUMP_CLEAN(indentation, "</SCTP::CookiePreservativeChunkParameter>");
		}

		CookiePreservativeChunkParameter* CookiePreservativeChunkParameter::Clone(
		  uint8_t* buffer, size_t bufferLength) const
		{
			MS_TRACE();

			auto* clonedParameter = new CookiePreservativeChunkParameter(buffer, bufferLength);

			CloneInto(clonedParameter);

			return clonedParameter;
		}

		void CookiePreservativeChunkParameter::SetLifeSpanIncrement(const uint32_t increment)
		{
			MS_TRACE();

			AssertNotFrozen();

			Utils::Byte::Set4Bytes(const_cast<uint8_t*>(GetBuffer()), 4, increment);
		}

		CookiePreservativeChunkParameter* CookiePreservativeChunkParameter::SoftClone(
		  const uint8_t* buffer) const
		{
			MS_TRACE();

			auto* softClonedParameter =
			  new CookiePreservativeChunkParameter(const_cast<uint8_t*>(buffer), GetLength());

			SoftCloneInto(softClonedParameter);

			return softClonedParameter;
		}
	} // namespace SCTP
} // namespace RTC
