#define MS_CLASS "RTC::SCTP::StaleCookieErrorCause"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/packet/errorCauses/StaleCookieErrorCause.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"

namespace RTC
{
	namespace SCTP
	{
		/* Class methods. */

		StaleCookieErrorCause* StaleCookieErrorCause::Parse(const uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			ErrorCause::ErrorCauseCode causeCode;
			uint16_t causeLength;
			uint8_t padding;

			if (!ErrorCause::IsErrorCause(buffer, bufferLength, causeCode, causeLength, padding))
			{
				return nullptr;
			}

			if (causeCode != ErrorCause::ErrorCauseCode::STALE_COOKIE)
			{
				MS_WARN_DEV("invalid Error Cause code");

				return nullptr;
			}

			return StaleCookieErrorCause::ParseStrict(buffer, bufferLength, causeLength, padding);
		}

		StaleCookieErrorCause* StaleCookieErrorCause::Factory(uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			if (bufferLength < StaleCookieErrorCause::StaleCookieErrorCauseHeaderLength)
			{
				MS_THROW_TYPE_ERROR("buffer too small");
			}

			auto* errorCause = new StaleCookieErrorCause(buffer, bufferLength);

			errorCause->InitializeHeader(
			  ErrorCause::ErrorCauseCode::STALE_COOKIE,
			  StaleCookieErrorCause::StaleCookieErrorCauseHeaderLength);

			// Initialize value.
			errorCause->SetMeasureOfStaleness(0);

			// No need to invoke SetLength() since parent constructor invoked it.

			return errorCause;
		}

		StaleCookieErrorCause* StaleCookieErrorCause::ParseStrict(
		  const uint8_t* buffer, size_t bufferLength, uint16_t causeLength, uint8_t padding)
		{
			MS_TRACE();

			if (causeLength != StaleCookieErrorCause::StaleCookieErrorCauseHeaderLength)
			{
				MS_WARN_TAG(
				  sctp,
				  "StaleCookieErrorCause Length field must be %zu",
				  StaleCookieErrorCause::StaleCookieErrorCauseHeaderLength);

				return nullptr;
			}

			auto* errorCause = new StaleCookieErrorCause(const_cast<uint8_t*>(buffer), bufferLength);

			// Mark the Error Cause as frozen since we are parsing.
			errorCause->Freeze();

			return errorCause;
		}

		/* Instance methods. */

		StaleCookieErrorCause::StaleCookieErrorCause(uint8_t* buffer, size_t bufferLength)
		  : ErrorCause(buffer, bufferLength)
		{
			MS_TRACE();

			SetLength(StaleCookieErrorCause::StaleCookieErrorCauseHeaderLength);
		}

		StaleCookieErrorCause::~StaleCookieErrorCause()
		{
			MS_TRACE();
		}

		void StaleCookieErrorCause::Dump(int indentation) const
		{
			MS_TRACE();

			MS_DUMP_CLEAN(indentation, "<SCTP::StaleCookieErrorCause>");
			DumpCommon(indentation);
			MS_DUMP_CLEAN(indentation, "  measure of staleness: %" PRIu32, GetMeasureOfStaleness());
			MS_DUMP_CLEAN(indentation, "</SCTP::StaleCookieErrorCause>");
		}

		StaleCookieErrorCause* StaleCookieErrorCause::Clone(uint8_t* buffer, size_t bufferLength) const
		{
			MS_TRACE();

			auto* clonedErrorCause = new StaleCookieErrorCause(buffer, bufferLength);

			CloneInto(clonedErrorCause);

			return clonedErrorCause;
		}

		void StaleCookieErrorCause::SetMeasureOfStaleness(uint32_t value)
		{
			MS_TRACE();

			AssertNotFrozen();

			Utils::Byte::Set4Bytes(const_cast<uint8_t*>(GetBuffer()), 4, value);
		}

		StaleCookieErrorCause* StaleCookieErrorCause::SoftClone(const uint8_t* buffer) const
		{
			MS_TRACE();

			auto* softClonedErrorCause =
			  new StaleCookieErrorCause(const_cast<uint8_t*>(buffer), GetLength());

			SoftCloneInto(softClonedErrorCause);

			return softClonedErrorCause;
		}
	} // namespace SCTP
} // namespace RTC
