#define MS_CLASS "RTC::SCTP::SocketMetrics"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/SocketMetrics.hpp"
#include "Logger.hpp"

namespace RTC
{
	namespace SCTP
	{
		/* Instance methods. */

		void SocketMetrics::Dump(int indentation) const
		{
			MS_TRACE();

			auto peerImplementationStringView = Types::SctpImplementationToString(this->peerImplementation);

			MS_DUMP_CLEAN(indentation, "<SCTP::SocketMetrics>");
			MS_DUMP_CLEAN(indentation, "  tx packets count: %" PRIu64, this->txPacketsCount);
			MS_DUMP_CLEAN(indentation, "  tx messages count: %" PRIu64, this->txMessagesCount);
			MS_DUMP_CLEAN(indentation, "  rtx packets count: %" PRIu64, this->rtxPacketsCount);
			// TODO: More.
			MS_DUMP_CLEAN(indentation, "  rx packets count: %" PRIu64, this->rxPacketsCount);
			MS_DUMP_CLEAN(indentation, "  rx messages count: %" PRIu64, this->rxMessagesCount);
			// TODO: More.
			MS_DUMP_CLEAN(
			  indentation,
			  "  peer implementation: %.*s",
			  static_cast<int>(peerImplementationStringView.size()),
			  peerImplementationStringView.data());
			// TODO: More.
			MS_DUMP_CLEAN(
			  indentation, "  message interleaving: %s", this->messageInterleaving ? "yes" : "no");
			MS_DUMP_CLEAN(indentation, "  zero checksum: %s", this->zeroChecksum ? "yes" : "no");
			MS_DUMP_CLEAN(indentation, "</SCTP::SocketMetrics>");
		}
	} // namespace SCTP
} // namespace RTC
