#define MS_CLASS "RTC::SCTP::NoUserDataErrorCause"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/packet/errorCauses/NoUserDataErrorCause.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"

namespace RTC
{
	namespace SCTP
	{
		/* Class methods. */

		NoUserDataErrorCause* NoUserDataErrorCause::Parse(const uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			ErrorCause::ErrorCauseCode causeCode;
			uint16_t causeLength;
			uint8_t padding;

			if (!ErrorCause::IsErrorCause(buffer, bufferLength, causeCode, causeLength, padding))
			{
				return nullptr;
			}

			if (causeCode != ErrorCause::ErrorCauseCode::NO_USER_DATA)
			{
				MS_WARN_DEV("invalid Error Cause code");

				return nullptr;
			}

			return NoUserDataErrorCause::ParseStrict(buffer, bufferLength, causeLength, padding);
		}

		NoUserDataErrorCause* NoUserDataErrorCause::Factory(uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			if (bufferLength < NoUserDataErrorCause::NoUserDataErrorCauseHeaderLength)
			{
				MS_THROW_TYPE_ERROR("buffer too small");
			}

			auto* errorCause = new NoUserDataErrorCause(buffer, bufferLength);

			errorCause->InitializeHeader(
			  ErrorCause::ErrorCauseCode::NO_USER_DATA,
			  NoUserDataErrorCause::NoUserDataErrorCauseHeaderLength);

			// Initialize value.
			errorCause->SetTsn(0);

			// No need to invoke SetLength() since parent constructor invoked it.

			return errorCause;
		}

		NoUserDataErrorCause* NoUserDataErrorCause::ParseStrict(
		  const uint8_t* buffer, size_t bufferLength, uint16_t causeLength, uint8_t padding)
		{
			MS_TRACE();

			if (causeLength != NoUserDataErrorCause::NoUserDataErrorCauseHeaderLength)
			{
				MS_WARN_TAG(
				  sctp,
				  "NoUserDataErrorCause Length field must be %zu",
				  NoUserDataErrorCause::NoUserDataErrorCauseHeaderLength);

				return nullptr;
			}

			auto* errorCause = new NoUserDataErrorCause(const_cast<uint8_t*>(buffer), bufferLength);

			// Mark the Error Cause as frozen since we are parsing.
			errorCause->Freeze();

			return errorCause;
		}

		/* Instance methods. */

		NoUserDataErrorCause::NoUserDataErrorCause(uint8_t* buffer, size_t bufferLength)
		  : ErrorCause(buffer, bufferLength)
		{
			MS_TRACE();

			SetLength(NoUserDataErrorCause::NoUserDataErrorCauseHeaderLength);
		}

		NoUserDataErrorCause::~NoUserDataErrorCause()
		{
			MS_TRACE();
		}

		void NoUserDataErrorCause::Dump(int indentation) const
		{
			MS_TRACE();

			MS_DUMP_CLEAN(indentation, "<SCTP::NoUserDataErrorCause>");
			DumpCommon(indentation);
			MS_DUMP_CLEAN(indentation, "  tsn: %" PRIu32, GetTsn());
			MS_DUMP_CLEAN(indentation, "</SCTP::NoUserDataErrorCause>");
		}

		NoUserDataErrorCause* NoUserDataErrorCause::Clone(uint8_t* buffer, size_t bufferLength) const
		{
			MS_TRACE();

			auto* clonedErrorCause = new NoUserDataErrorCause(buffer, bufferLength);

			CloneInto(clonedErrorCause);

			return clonedErrorCause;
		}

		void NoUserDataErrorCause::SetTsn(uint32_t value)
		{
			MS_TRACE();

			AssertNotFrozen();

			Utils::Byte::Set4Bytes(const_cast<uint8_t*>(GetBuffer()), 4, value);
		}

		NoUserDataErrorCause* NoUserDataErrorCause::SoftClone(const uint8_t* buffer) const
		{
			MS_TRACE();

			auto* softClonedErrorCause =
			  new NoUserDataErrorCause(const_cast<uint8_t*>(buffer), GetLength());

			SoftCloneInto(softClonedErrorCause);

			return softClonedErrorCause;
		}
	} // namespace SCTP
} // namespace RTC
