#define MS_CLASS "RTC::SCTP::NegotiatedCapabilities"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/association/NegotiatedCapabilities.hpp"
#include "Logger.hpp"
#include "RTC/SCTP/packet/parameters/ZeroChecksumAcceptableParameter.hpp"

namespace RTC
{
	namespace SCTP
	{
		/* Class methods. */

		NegotiatedCapabilities NegotiatedCapabilities::Factory(
		  const SctpOptions& sctpOptions, const Capabilities& remoteCapabilities)
		{
			MS_TRACE();

			NegotiatedCapabilities negotiatedCapabilities{};

			negotiatedCapabilities.maxOutboundStreams =
			  std::min(sctpOptions.announcedMaxOutboundStreams, remoteCapabilities.maxInboundStreams);

			negotiatedCapabilities.maxInboundStreams =
			  std::min(sctpOptions.announcedMaxInboundStreams, remoteCapabilities.maxOutboundStreams);

			// Partial Reliability Extension is negotiated if we desire it and the
			// remote announces support for it.
			negotiatedCapabilities.partialReliability =
			  sctpOptions.enablePartialReliability && remoteCapabilities.partialReliability;

			// Message Interleaving is negotiated if we desire it and the remote
			// announces support for it.
			negotiatedCapabilities.messageInterleaving =
			  sctpOptions.enableMessageInterleaving && remoteCapabilities.messageInterleaving;

			// Stream Re-Configuration is negotiated if the remote announces support
			// for it.
			negotiatedCapabilities.reConfig = remoteCapabilities.reConfig;

			// Alternate Error Detection Method for Zero Checksum is negotiated if we
			// desire it and the remote announces the same non-none alternate error
			// detection method.
			negotiatedCapabilities.zeroChecksum =
			  sctpOptions.zeroChecksumAlternateErrorDetectionMethod !=
			    ZeroChecksumAcceptableParameter::AlternateErrorDetectionMethod::NONE &&
			  remoteCapabilities.zeroChecksumAlternateErrorDetectionMethod ==
			    sctpOptions.zeroChecksumAlternateErrorDetectionMethod;

			return negotiatedCapabilities;
		}

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
			MS_DUMP_CLEAN(indentation, "  re-config: %s", this->reConfig ? "yes" : "no");
			MS_DUMP_CLEAN(indentation, "  zero checksum: %s", this->zeroChecksum ? "yes" : "no");
			MS_DUMP_CLEAN(indentation, "</SCTP::NegotiatedCapabilities>");
		}
	} // namespace SCTP
} // namespace RTC
