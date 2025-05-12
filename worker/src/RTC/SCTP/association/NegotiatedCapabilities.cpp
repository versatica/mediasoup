#define MS_CLASS "RTC::SCTP::NegotiatedCapabilities"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/association/NegotiatedCapabilities.hpp"
#include "Logger.hpp"

namespace RTC
{
	namespace SCTP
	{
		/* Instance methods. */

		void NegotiatedCapabilities::Dump(int indentation) const
		{
			MS_TRACE();

			MS_DUMP_CLEAN(indentation, "<SCTP::NegotiatedCapabilities>");
			MS_DUMP_CLEAN(indentation, "  max outbound streams: %" PRIu16, this->maxOutboundStreams);
			MS_DUMP_CLEAN(indentation, "  max inbound streams: %" PRIu16, this->maxInboundStreams);
			MS_DUMP_CLEAN(indentation, "  partial reliability: %s", this->partialReliability ? "yes" : "no");
			MS_DUMP_CLEAN(
			  indentation, "  message interleaving: %s", this->messageInterleaving ? "yes" : "no");
			MS_DUMP_CLEAN(indentation, "  re-config: %s", this->reconfig ? "yes" : "no");
			MS_DUMP_CLEAN(indentation, "  zero checksum: %s", this->zeroChecksum ? "yes" : "no");
			MS_DUMP_CLEAN(indentation, "</SCTP::NegotiatedCapabilities>");
		}
	} // namespace SCTP
} // namespace RTC
