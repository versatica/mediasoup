#define MS_CLASS "RTC::SCTP::RestartOfAnAssociationWithNewAddressesErrorCause"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/packet/errorCauses/RestartOfAnAssociationWithNewAddressesErrorCause.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"

namespace RTC
{
	namespace SCTP
	{
		/* Class methods. */

		RestartOfAnAssociationWithNewAddressesErrorCause* RestartOfAnAssociationWithNewAddressesErrorCause::Parse(
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

			if (causeCode != ErrorCause::ErrorCauseCode::RESTART_OF_AN_ASSOCIATION_WITH_NEW_ADDRESSES)
			{
				MS_WARN_DEV("invalid Error Cause code");

				return nullptr;
			}

			return RestartOfAnAssociationWithNewAddressesErrorCause::ParseStrict(
			  buffer, bufferLength, causeLength, padding);
		}

		RestartOfAnAssociationWithNewAddressesErrorCause* RestartOfAnAssociationWithNewAddressesErrorCause::Factory(
		  uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			if (bufferLength < ErrorCause::ErrorCauseHeaderLength)
			{
				MS_THROW_TYPE_ERROR("buffer too small");
			}

			auto* errorCause = new RestartOfAnAssociationWithNewAddressesErrorCause(buffer, bufferLength);

			errorCause->InitializeHeader(
			  ErrorCause::ErrorCauseCode::RESTART_OF_AN_ASSOCIATION_WITH_NEW_ADDRESSES,
			  ErrorCause::ErrorCauseHeaderLength);

			// No need to invoke SetLength() since parent constructor invoked it.

			return errorCause;
		}

		RestartOfAnAssociationWithNewAddressesErrorCause* RestartOfAnAssociationWithNewAddressesErrorCause::ParseStrict(
		  const uint8_t* buffer, size_t bufferLength, uint16_t causeLength, uint8_t padding)
		{
			MS_TRACE();

			auto* errorCause = new RestartOfAnAssociationWithNewAddressesErrorCause(
			  const_cast<uint8_t*>(buffer), bufferLength);

			// Must always invoke SetLength() after constructing a Serializable with
			// not fixed length.
			errorCause->SetLength(causeLength + padding);

			// Mark the Error Cause as frozen since we are parsing.
			errorCause->Freeze();

			return errorCause;
		}

		/* Instance methods. */

		RestartOfAnAssociationWithNewAddressesErrorCause::RestartOfAnAssociationWithNewAddressesErrorCause(
		  uint8_t* buffer, size_t bufferLength)
		  : ErrorCause(buffer, bufferLength)
		{
			MS_TRACE();

			SetLength(ErrorCause::ErrorCauseHeaderLength);
		}

		RestartOfAnAssociationWithNewAddressesErrorCause::~RestartOfAnAssociationWithNewAddressesErrorCause()
		{
			MS_TRACE();
		}

		void RestartOfAnAssociationWithNewAddressesErrorCause::Dump(int indentation) const
		{
			MS_TRACE();

			MS_DUMP_CLEAN(indentation, "<SCTP::RestartOfAnAssociationWithNewAddressesErrorCause>");
			DumpCommon(indentation);
			MS_DUMP_CLEAN(
			  indentation,
			  "  new address tlvs length: %" PRIu16 " (has new address tlvs: %s)",
			  GetNewAddressTlvsLength(),
			  HasNewAddressTlvs() ? "yes" : "no");
			MS_DUMP_CLEAN(indentation, "</SCTP::RestartOfAnAssociationWithNewAddressesErrorCause>");
		}

		RestartOfAnAssociationWithNewAddressesErrorCause* RestartOfAnAssociationWithNewAddressesErrorCause::Clone(
		  uint8_t* buffer, size_t bufferLength) const
		{
			MS_TRACE();

			auto* clonedErrorCause =
			  new RestartOfAnAssociationWithNewAddressesErrorCause(buffer, bufferLength);

			CloneInto(clonedErrorCause);

			return clonedErrorCause;
		}

		void RestartOfAnAssociationWithNewAddressesErrorCause::SetNewAddressTlvs(
		  const uint8_t* tlvs, uint16_t tlvsLength)
		{
			MS_TRACE();

			AssertNotFrozen();

			SetVariableLengthValue(tlvs, tlvsLength);
		}

		RestartOfAnAssociationWithNewAddressesErrorCause* RestartOfAnAssociationWithNewAddressesErrorCause::SoftClone(
		  const uint8_t* buffer) const
		{
			MS_TRACE();

			auto* softClonedErrorCause = new RestartOfAnAssociationWithNewAddressesErrorCause(
			  const_cast<uint8_t*>(buffer), GetLength());

			SoftCloneInto(softClonedErrorCause);

			return softClonedErrorCause;
		}
	} // namespace SCTP
} // namespace RTC
