#define MS_CLASS "RTC::SCTP::TransmissionControlBlock"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/association/TransmissionControlBlock.hpp"
#include "Logger.hpp"

namespace RTC
{
	namespace SCTP
	{
		/* Instance methods. */

		TransmissionControlBlock::TransmissionControlBlock(
		  uint32_t myVerificationTag,
		  uint32_t peerVerificationTag,
		  uint32_t myInitialTsn,
		  uint32_t peerInitialTsn,
		  uint32_t myAdvertisedReceiverWindowCredit,
		  uint64_t tieTag,
		  const NegotiatedCapabilities& negotiatedCapabilities)
		  : myVerificationTag(myVerificationTag), peerVerificationTag(peerVerificationTag),
		    myInitialTsn(myInitialTsn), peerInitialTsn(peerInitialTsn),
		    myAdvertisedReceiverWindowCredit(myAdvertisedReceiverWindowCredit), tieTag(tieTag),
		    negotiatedCapabilities(negotiatedCapabilities)
		{
			MS_TRACE();
		}

		TransmissionControlBlock::~TransmissionControlBlock()
		{
			MS_TRACE();
		}

		void TransmissionControlBlock::Dump(int indentation) const
		{
			MS_TRACE();

			MS_DUMP_CLEAN(indentation, "<SCTP::TransmissionControlBlock>");
			MS_DUMP_CLEAN(indentation, "  my verification tag: %" PRIu32, this->myVerificationTag);
			MS_DUMP_CLEAN(indentation, "  peer verification tag: %" PRIu32, this->peerVerificationTag);
			MS_DUMP_CLEAN(indentation, "  my initial tsn: %" PRIu32, this->myInitialTsn);
			MS_DUMP_CLEAN(indentation, "  peer initial tsn: %" PRIu32, this->peerInitialTsn);
			MS_DUMP_CLEAN(
			  indentation,
			  "  my advertised receiver window credit: %" PRIu32,
			  this->myAdvertisedReceiverWindowCredit);
			MS_DUMP_CLEAN(indentation, "  tie-tag: %" PRIu64, this->tieTag);
			this->negotiatedCapabilities.Dump(indentation + 1);
			MS_DUMP_CLEAN(indentation, "</SCTP::TransmissionControlBlock>");
		}
	} // namespace SCTP
} // namespace RTC
