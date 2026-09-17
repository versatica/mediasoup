#define MS_CLASS "RTC::RtcLogger::SendBurst"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/RtcLogger/SendBurst.hpp"
#include "Logger.hpp"
#include <sstream>

namespace RTC
{
	namespace RtcLogger
	{
		/* Instance methods. */

		void SendBurst::Sent(uint64_t loopTimeMs, size_t length, bool isRetransmission, bool isProbation)
		{
			MS_TRACE();

			if (loopTimeMs != this->loopTimeMs)
			{
				CloseBurst();

				this->loopTimeMs = loopTimeMs;
			}

			this->packets++;
			this->bytes += length;

			if (isRetransmission)
			{
				this->retransmissions++;
			}

			if (isProbation)
			{
				this->probations++;
			}
		}

		void SendBurst::Log()
		{
			MS_TRACE();

			// The iteration this runs in is not the one any burst belongs to, so whatever
			// is in progress is already complete.
			CloseBurst();

			// Nothing has been sent since the last time, and a transport that sends
			// nothing has nothing to say.
			if (this->maxPackets == 0)
			{
				return;
			}

			std::stringstream ss;

			ss << "{";
			ss << R"("transportId": ")" << this->transportId << "\"";
			ss << ", \"maxPackets\": " << this->maxPackets;
			ss << ", \"maxBytes\": " << this->maxBytes;
			ss << ", \"bursts\": [";

			bool first{ true };

			for (const auto& kv : this->bursts)
			{
				const auto packets   = kv.first;
				const auto& counters = kv.second;

				if (!first)
				{
					ss << ", ";
				}

				first = false;

				ss << "{\"packets\": " << packets;
				ss << ", \"count\": " << counters.count;
				ss << ", \"retransmissions\": " << counters.retransmissions;
				ss << ", \"probations\": " << counters.probations;
				ss << "}";
			}

			ss << "]}";

			MS_DUMP_CLEAN(0, "%s", ss.str().c_str());

			Clear();
		}

		void SendBurst::CloseBurst()
		{
			MS_TRACE();

			if (this->packets == 0)
			{
				return;
			}

			auto& counters = this->bursts[this->packets];

			counters.count++;
			counters.retransmissions += this->retransmissions;
			counters.probations += this->probations;

			this->maxPackets = std::max(this->maxPackets, this->packets);
			this->maxBytes   = std::max(this->maxBytes, this->bytes);

			this->packets         = 0;
			this->retransmissions = 0;
			this->probations      = 0;
			this->bytes           = 0;
		}

		void SendBurst::Clear()
		{
			MS_TRACE();

			this->bursts.clear();

			this->maxPackets = 0;
			this->maxBytes   = 0;
		}
	} // namespace RtcLogger
} // namespace RTC
