#define MS_CLASS "RTC::SCTP::AssociationMetrics"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/public/AssociationMetrics.hpp"
#include "Logger.hpp"

namespace RTC
{
	namespace SCTP
	{
		/* Instance methods. */

		void AssociationMetrics::Dump(int indentation) const
		{
			MS_TRACE();

			auto peerImplementationStringView = Types::SctpImplementationToString(this->peerImplementation);

			MS_DUMP_CLEAN(indentation, "<SCTP::AssociationMetrics>");
			MS_DUMP_CLEAN(indentation, "  tx packets count: %" PRIu64, this->txPacketsCount);
			MS_DUMP_CLEAN(indentation, "  tx messages count: %" PRIu64, this->txMessagesCount);
			MS_DUMP_CLEAN(indentation, "  rx packets count: %" PRIu64, this->rxPacketsCount);
			MS_DUMP_CLEAN(indentation, "  rx messages count: %" PRIu64, this->rxMessagesCount);
			MS_DUMP_CLEAN(indentation, "  rtx packets count: %" PRIu64, this->rtxPacketsCount);
			MS_DUMP_CLEAN(indentation, "  rtx bytes count: %" PRIu64, this->rtxBytesCount);
			MS_DUMP_CLEAN(indentation, "  current congestion window (bytes): %zu", this->cwndBytes);
			MS_DUMP_CLEAN(indentation, "  smoothed round trip time (ms): %" PRIu64, this->srttMs);
			MS_DUMP_CLEAN(indentation, "  unacked data count: %zu", this->unackDataCount);
			MS_DUMP_CLEAN(
			  indentation, "  peer's last announced receiver window size: %" PRIu32, this->peerRwndBytes);
			MS_DUMP_CLEAN(
			  indentation,
			  "  peer implementation: %.*s",
			  static_cast<int>(peerImplementationStringView.size()),
			  peerImplementationStringView.data());
			MS_DUMP_CLEAN(
			  indentation, "  uses partial reliability: %s", this->usesPartialReliability ? "yes" : "no");
			MS_DUMP_CLEAN(
			  indentation, "  uses message interleaving: %s", this->usesMessageInterleaving ? "yes" : "no");
			MS_DUMP_CLEAN(indentation, "  uses re-config: %s", this->usesReConfig ? "yes" : "no");
			MS_DUMP_CLEAN(indentation, "  uses zero checksum: %s", this->usesZeroChecksum ? "yes" : "no");
			MS_DUMP_CLEAN(indentation, "</SCTP::AssociationMetrics>");
		}
	} // namespace SCTP
} // namespace RTC
