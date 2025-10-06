#define MS_CLASS "RTC::SCTP::UnresolvableAddressErrorCause"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/packet/errorCauses/UnresolvableAddressErrorCause.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"

namespace RTC
{
	namespace SCTP
	{
		/* Class methods. */

		UnresolvableAddressErrorCause* UnresolvableAddressErrorCause::Parse(
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

			if (causeCode != ErrorCause::ErrorCauseCode::UNRESOLVABLE_ADDRESS)
			{
				MS_WARN_DEV("invalid Error Cause code");

				return nullptr;
			}

			return UnresolvableAddressErrorCause::ParseStrict(buffer, bufferLength, causeLength, padding);
		}

		UnresolvableAddressErrorCause* UnresolvableAddressErrorCause::Factory(
		  uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			if (bufferLength < ErrorCause::ErrorCauseHeaderLength)
			{
				MS_THROW_TYPE_ERROR("buffer too small");
			}

			auto* errorCause = new UnresolvableAddressErrorCause(buffer, bufferLength);

			errorCause->InitializeHeader(
			  ErrorCause::ErrorCauseCode::UNRESOLVABLE_ADDRESS, ErrorCause::ErrorCauseHeaderLength);

			// No need to invoke SetLength() since parent constructor invoked it.

			return errorCause;
		}

		UnresolvableAddressErrorCause* UnresolvableAddressErrorCause::ParseStrict(
		  const uint8_t* buffer, size_t bufferLength, uint16_t causeLength, uint8_t padding)
		{
			MS_TRACE();

			auto* errorCause =
			  new UnresolvableAddressErrorCause(const_cast<uint8_t*>(buffer), bufferLength);

			// Must always invoke SetLength() after constructing a Serializable with
			// not fixed length.
			errorCause->SetLength(causeLength + padding);

			// Mark the Error Cause as frozen since we are parsing.
			errorCause->Freeze();

			return errorCause;
		}

		/* Instance methods. */

		UnresolvableAddressErrorCause::UnresolvableAddressErrorCause(uint8_t* buffer, size_t bufferLength)
		  : ErrorCause(buffer, bufferLength)
		{
			MS_TRACE();

			SetLength(ErrorCause::ErrorCauseHeaderLength);
		}

		UnresolvableAddressErrorCause::~UnresolvableAddressErrorCause()
		{
			MS_TRACE();
		}

		void UnresolvableAddressErrorCause::Dump(int indentation) const
		{
			MS_TRACE();

			MS_DUMP_CLEAN(indentation, "<SCTP::UnresolvableAddressErrorCause>");
			DumpCommon(indentation);
			MS_DUMP_CLEAN(
			  indentation,
			  "  unresolvable address length: %" PRIu16 " (has unresolvable address: %s)",
			  GetUnresolvableAddressLength(),
			  HasUnresolvableAddress() ? "yes" : "no");
			MS_DUMP_CLEAN(indentation, "</SCTP::UnresolvableAddressErrorCause>");
		}

		UnresolvableAddressErrorCause* UnresolvableAddressErrorCause::Clone(
		  uint8_t* buffer, size_t bufferLength) const
		{
			MS_TRACE();

			auto* clonedErrorCause = new UnresolvableAddressErrorCause(buffer, bufferLength);

			CloneInto(clonedErrorCause);

			return clonedErrorCause;
		}

		void UnresolvableAddressErrorCause::SetUnresolvableAddress(
		  const uint8_t* address, uint16_t addressLength)
		{
			MS_TRACE();

			AssertNotFrozen();

			SetVariableLengthValue(address, addressLength);
		}

		UnresolvableAddressErrorCause* UnresolvableAddressErrorCause::SoftClone(const uint8_t* buffer) const
		{
			MS_TRACE();

			auto* softClonedErrorCause =
			  new UnresolvableAddressErrorCause(const_cast<uint8_t*>(buffer), GetLength());

			SoftCloneInto(softClonedErrorCause);

			return softClonedErrorCause;
		}
	} // namespace SCTP
} // namespace RTC
