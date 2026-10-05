#define MS_CLASS "RTC::BWE::PacketLossTracker"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/BWE/PacketLossTracker.hpp"
#include "Logger.hpp"
#include "RTC/Consts.hpp"

namespace RTC
{
	namespace BWE
	{
		/* Instance methods. */

		std::optional<PacketLossTracker::Loss> PacketLossTracker::ReceiveReceiverReport(
		  RTC::RTCP::ReceiverReportPacket* packet)
		{
			MS_TRACE();

			int64_t lostPackets{ 0 };
			int64_t expectedPackets{ 0 };

			for (auto it = packet->Begin(); it != packet->End(); ++it)
			{
				auto* report        = *it;
				const uint32_t ssrc = report->GetSsrc();

				// The packets of the probation stream go out above the target on
				// purpose, so what happens to them says nothing about what the link
				// bears.
				if (ssrc == RTC::Consts::BweProbeRtpSsrc)
				{
					continue;
				}

				const uint32_t extendedHighestSequenceNumber = report->GetLastSeq();
				const int32_t cumulativeLost                 = report->GetTotalLost();

				auto [mapIt, inserted] = this->mapSsrcReportTotals.try_emplace(ssrc);
				auto& reportTotals     = mapIt->second;

				// The first report of a stream is only taken down, since there is
				// nothing to measure it against.
				if (!inserted)
				{
					// NOTE: The difference is taken as signed so that it wraps the way
					// the extended sequence number does. A report that arrives after a
					// newer one then counts as the small negative number it is, instead
					// of as the four thousand million the unsigned subtraction gives.
					expectedPackets += static_cast<int32_t>(
					  extendedHighestSequenceNumber - reportTotals.extendedHighestSequenceNumber);
					// NOTE: This one may be negative on its own, since the running total
					// a report carries goes down when duplicates arrive.
					lostPackets += cumulativeLost - reportTotals.cumulativeLost;
				}

				reportTotals.extendedHighestSequenceNumber = extendedHighestSequenceNumber;
				reportTotals.cumulativeLost                = cumulativeLost;
			}

			// Nothing moved since the previous report, or every block of this one was
			// a first sighting.
			if (expectedPackets == 0)
			{
				return std::nullopt;
			}

			// Not a single packet got through. That is not a loss ratio of one but a
			// stretch the remote endpoint could not report on, so it is left out
			// rather than fed as the worst possible measurement.
			if (expectedPackets - lostPackets < 1)
			{
				return std::nullopt;
			}

			return Loss{ .lostPackets = lostPackets, .expectedPackets = expectedPackets };
		}

		void PacketLossTracker::RemoveStream(uint32_t ssrc)
		{
			MS_TRACE();

			this->mapSsrcReportTotals.erase(ssrc);
		}
	} // namespace BWE
} // namespace RTC
