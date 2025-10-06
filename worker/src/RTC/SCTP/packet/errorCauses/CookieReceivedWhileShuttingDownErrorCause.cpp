#define MS_CLASS "RTC::SCTP::CookieReceivedWhileShuttingDownErrorCause"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/packet/errorCauses/CookieReceivedWhileShuttingDownErrorCause.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"

namespace RTC
{
	namespace SCTP
	{
		/* Class methods. */

		CookieReceivedWhileShuttingDownErrorCause* CookieReceivedWhileShuttingDownErrorCause::Parse(
		  const uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			ErrorCause::ErrorCauseCode causeCode;
			uint16_t causeLength;
			uint8_t padding;

			if (!ErrorCause::IsErrorCause(buffer, bufferLength, causeCode, causeLength, padding))
			{
				return nullptr;
			}

			if (causeCode != ErrorCause::ErrorCauseCode::COOKIE_RECEIVED_WHILE_SHUTTING_DOWN)
			{
				MS_WARN_DEV("invalid Error Cause code");

				return nullptr;
			}

			return CookieReceivedWhileShuttingDownErrorCause::ParseStrict(
			  buffer, bufferLength, causeLength, padding);
		}

		CookieReceivedWhileShuttingDownErrorCause* CookieReceivedWhileShuttingDownErrorCause::Factory(
		  uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			if (bufferLength < ErrorCause::ErrorCauseHeaderLength)
			{
				MS_THROW_TYPE_ERROR("buffer too small");
			}

			auto* errorCause = new CookieReceivedWhileShuttingDownErrorCause(buffer, bufferLength);

			errorCause->InitializeHeader(
			  ErrorCause::ErrorCauseCode::COOKIE_RECEIVED_WHILE_SHUTTING_DOWN,
			  ErrorCause::ErrorCauseHeaderLength);

			// No need to invoke SetLength() since parent constructor invoked it.

			return errorCause;
		}

		CookieReceivedWhileShuttingDownErrorCause* CookieReceivedWhileShuttingDownErrorCause::ParseStrict(
		  const uint8_t* buffer, size_t bufferLength, uint16_t causeLength, uint8_t padding)
		{
			MS_TRACE();

			if (causeLength != ErrorCause::ErrorCauseHeaderLength)
			{
				MS_WARN_TAG(
				  sctp,
				  "CookieReceivedWhileShuttingDownErrorCause Length field must be %zu",
				  ErrorCause::ErrorCauseHeaderLength);

				return nullptr;
			}

			auto* errorCause =
			  new CookieReceivedWhileShuttingDownErrorCause(const_cast<uint8_t*>(buffer), bufferLength);

			// Mark the Error Cause as frozen since we are parsing.
			errorCause->Freeze();

			return errorCause;
		}

		/* Instance methods. */

		CookieReceivedWhileShuttingDownErrorCause::CookieReceivedWhileShuttingDownErrorCause(
		  uint8_t* buffer, size_t bufferLength)
		  : ErrorCause(buffer, bufferLength)
		{
			MS_TRACE();

			SetLength(ErrorCause::ErrorCauseHeaderLength);
		}

		CookieReceivedWhileShuttingDownErrorCause::~CookieReceivedWhileShuttingDownErrorCause()
		{
			MS_TRACE();
		}

		void CookieReceivedWhileShuttingDownErrorCause::Dump(int indentation) const
		{
			MS_TRACE();

			MS_DUMP_CLEAN(indentation, "<SCTP::CookieReceivedWhileShuttingDownErrorCause>");
			DumpCommon(indentation);
			MS_DUMP_CLEAN(indentation, "</SCTP::CookieReceivedWhileShuttingDownErrorCause>");
		}

		CookieReceivedWhileShuttingDownErrorCause* CookieReceivedWhileShuttingDownErrorCause::Clone(
		  uint8_t* buffer, size_t bufferLength) const
		{
			MS_TRACE();

			auto* clonedErrorCause = new CookieReceivedWhileShuttingDownErrorCause(buffer, bufferLength);

			CloneInto(clonedErrorCause);

			return clonedErrorCause;
		}

		CookieReceivedWhileShuttingDownErrorCause* CookieReceivedWhileShuttingDownErrorCause::SoftClone(
		  const uint8_t* buffer) const
		{
			MS_TRACE();

			auto* softClonedErrorCause =
			  new CookieReceivedWhileShuttingDownErrorCause(const_cast<uint8_t*>(buffer), GetLength());

			SoftCloneInto(softClonedErrorCause);

			return softClonedErrorCause;
		}
	} // namespace SCTP
} // namespace RTC
