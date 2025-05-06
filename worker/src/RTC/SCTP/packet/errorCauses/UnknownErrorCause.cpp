#define MS_CLASS "RTC::SCTP::UnknownErrorCause"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/packet/errorCauses/UnknownErrorCause.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"

namespace RTC
{
	namespace SCTP
	{
		/* Class methods. */

		UnknownErrorCause* UnknownErrorCause::Parse(const uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			ErrorCause::ErrorCauseCode causeCode;
			uint16_t causeLength;
			uint8_t padding;

			if (!ErrorCause::IsErrorCause(buffer, bufferLength, causeCode, causeLength, padding))
			{
				return nullptr;
			}

			return UnknownErrorCause::ParseStrict(buffer, bufferLength, causeLength, padding);
		}

		UnknownErrorCause* UnknownErrorCause::ParseStrict(
		  const uint8_t* buffer, size_t bufferLength, uint16_t causeLength, uint8_t padding)
		{
			MS_TRACE();

			auto* errorCause = new UnknownErrorCause(const_cast<uint8_t*>(buffer), bufferLength);

			// Must always invoke SetLength() after constructing a Serializable with
			// not fixed length.
			errorCause->SetLength(causeLength + padding);

			// Mark the Error Cause as frozen since we are parsing.
			errorCause->Freeze();

			return errorCause;
		}

		/* Instance methods. */

		UnknownErrorCause::UnknownErrorCause(uint8_t* buffer, size_t bufferLength)
		  : ErrorCause(buffer, bufferLength)
		{
			MS_TRACE();

			SetLength(ErrorCause::ErrorCauseHeaderLength);
		}

		UnknownErrorCause::~UnknownErrorCause()
		{
			MS_TRACE();
		}

		void UnknownErrorCause::Dump(int indentation) const
		{
			MS_TRACE();

			MS_DUMP_CLEAN(indentation, "<SCTP::UnknownErrorCause>");
			DumpCommon(indentation);
			MS_DUMP_CLEAN(indentation, "</SCTP::UnknownErrorCause>");
		}

		UnknownErrorCause* UnknownErrorCause::Clone(uint8_t* buffer, size_t bufferLength) const
		{
			MS_TRACE();

			auto* clonedErrorCause = new UnknownErrorCause(buffer, bufferLength);

			CloneInto(clonedErrorCause);

			return clonedErrorCause;
		}

		UnknownErrorCause* UnknownErrorCause::SoftClone(const uint8_t* buffer) const
		{
			MS_TRACE();

			auto* softClonedErrorCause = new UnknownErrorCause(const_cast<uint8_t*>(buffer), GetLength());

			SoftCloneInto(softClonedErrorCause);

			return softClonedErrorCause;
		}
	} // namespace SCTP
} // namespace RTC
