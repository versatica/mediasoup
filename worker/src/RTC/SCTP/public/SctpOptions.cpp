#define MS_CLASS "RTC::SCTP::SctpOptions"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/public/SctpOptions.hpp"
#include "Logger.hpp"
#include "RTC/SCTP/packet/parameters/ZeroChecksumAcceptableParameter.hpp"
#include <string>

namespace RTC
{
	namespace SCTP
	{
		/* Instance methods. */

		void SctpOptions::Dump(int indentation) const
		{
			MS_TRACE();

			MS_DUMP_CLEAN(indentation, "<SCTP::SctpOptions>");
			MS_DUMP_CLEAN(indentation, "  source port: %" PRIu16, this->sourcePort);
			MS_DUMP_CLEAN(indentation, "  destination port: %" PRIu16, this->destinationPort);
			MS_DUMP_CLEAN(
			  indentation, "  announced max outbound streams: %" PRIu16, this->announcedMaxOutboundStreams);
			MS_DUMP_CLEAN(
			  indentation, "  announced max inbound streams: %" PRIu16, this->announcedMaxInboundStreams);
			MS_DUMP_CLEAN(indentation, "  mtu: %zu", this->mtu);
			MS_DUMP_CLEAN(indentation, "  max send message size: %zu", this->maxSendMessageSize);
			MS_DUMP_CLEAN(indentation, "  max send buffer size: %zu", this->maxSendBufferSize);
			MS_DUMP_CLEAN(indentation, "  per stream send queue limit: %zu", this->perStreamSendQueueLimit);
			MS_DUMP_CLEAN(indentation, "  max receive message size: %zu", this->maxReceiveMessageSize);
			MS_DUMP_CLEAN(indentation, "  max receiver window size: %zu", this->maxReceiverWindowBufferSize);
			MS_DUMP_CLEAN(
			  indentation,
			  "  total buffered amount low threshold: %zu",
			  this->totalBufferedAmountLowThreshold);
			MS_DUMP_CLEAN(indentation, "  default stream priority: %" PRIu16, this->defaultStreamPriority);
			MS_DUMP_CLEAN(indentation, "  max rtt (ms): %" PRIu64, this->maxRttMs);
			MS_DUMP_CLEAN(indentation, "  initial rto (ms): %" PRIu64, this->initialRtoMs);
			MS_DUMP_CLEAN(indentation, "  min rto (ms): %" PRIu64, this->minRtoMs);
			MS_DUMP_CLEAN(indentation, "  max rto (ms): %" PRIu64, this->maxRtoMs);
			MS_DUMP_CLEAN(indentation, "  t1-init timeout (ms): %" PRIu64, this->t1InitTimeoutMs);
			MS_DUMP_CLEAN(indentation, "  t1-cookie timeout (ms): %" PRIu64, this->t1CookieTimeoutMs);
			MS_DUMP_CLEAN(indentation, "  t2-shutdown timeout (ms): %" PRIu64, this->t2ShutdownTimeoutMs);
			MS_DUMP_CLEAN(
			  indentation,
			  "  timer max backoff timeout (ms): %s",
			  this->timerMaxBackoffTimeoutMs
			    ? std::to_string(this->timerMaxBackoffTimeoutMs.value()).c_str()
			    : "Infinite");
			MS_DUMP_CLEAN(indentation, "  heartbeat interval (ms): %" PRIu64, this->heartbeatIntervalMs);
			MS_DUMP_CLEAN(
			  indentation, "  delayed ack max timeout (ms): %" PRIu64, this->delayedAckMaxTimeoutMs);
			MS_DUMP_CLEAN(indentation, "  min rtt variance (ms): %" PRIu64, this->minRttVarianceMs);
			MS_DUMP_CLEAN(indentation, "  initial cwnd mtus: %zu", this->initialCwndMtus);
			MS_DUMP_CLEAN(indentation, "  min cwnd mtus: %zu", this->minCwndMtus);
			MS_DUMP_CLEAN(
			  indentation, "  avoid fragmentation cwnd mtus: %zu", this->avoidFragmentationCwndMtus);
			MS_DUMP_CLEAN(
			  indentation, "  immediate sack under cwnd mtus: %zu", this->immediateSackUnderCwndMtus);
			MS_DUMP_CLEAN(indentation, "  max burst: %zu", this->maxBurst);
			MS_DUMP_CLEAN(
			  indentation,
			  "  max retransmissions: %s",
			  this->maxRetransmissions ? std::to_string(this->maxRetransmissions.value()).c_str()
			                           : "Infinite");
			MS_DUMP_CLEAN(
			  indentation,
			  "  max init retransmissions: %s",
			  this->maxInitRetransmissions ? std::to_string(this->maxInitRetransmissions.value()).c_str()
			                               : "Infinite");
			MS_DUMP_CLEAN(
			  indentation, "  enable partial reliability: %s", this->enablePartialReliability ? "yes" : "no");
			MS_DUMP_CLEAN(
			  indentation,
			  "  enable message interleaving: %s",
			  this->enableMessageInterleaving ? "yes" : "no");
			MS_DUMP_CLEAN(
			  indentation,
			  "  heartbeat interval include rtt: %s",
			  this->heartbeatIntervalIncludeRtt ? "yes" : "no");
			MS_DUMP_CLEAN(
			  indentation,
			  "  zero checksum alternate error detection method: %s",
			  ZeroChecksumAcceptableParameter::AlternateErrorDetectionMethodToString(
			    this->zeroChecksumAlternateErrorDetectionMethod)
			    .c_str());
			MS_DUMP_CLEAN(indentation, "</SCTP::SctpOptions>");
		}
	} // namespace SCTP
} // namespace RTC
