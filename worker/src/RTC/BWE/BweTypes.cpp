#define MS_CLASS "RTC::BWE::Types"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/BWE/BweTypes.hpp"
#include "Logger.hpp"

namespace RTC
{
	namespace BWE
	{
		namespace Types
		{
			/* Instance methods. */

			bool PacketResult::ReceiveTimeOrder::operator()(const PacketResult& lhs, const PacketResult& rhs) const
			{
				MS_TRACE();

				if (lhs.receiveTimeUs != rhs.receiveTimeUs)
				{
					return lhs.receiveTimeUs < rhs.receiveTimeUs;
				}

				if (lhs.sentPacket.sendTimeUs != rhs.sentPacket.sendTimeUs)
				{
					return lhs.sentPacket.sendTimeUs < rhs.sentPacket.sendTimeUs;
				}

				return lhs.sentPacket.sequenceNumber < rhs.sentPacket.sequenceNumber;
			}

			std::vector<PacketResult> TransportPacketsFeedback::SortedByReceiveTime() const
			{
				MS_TRACE();

				auto packetResults = ReceivedWithSendInfo();

				std::ranges::sort(packetResults, PacketResult::ReceiveTimeOrder());

				return packetResults;
			}

			std::vector<PacketResult> TransportPacketsFeedback::ReceivedWithSendInfo() const
			{
				MS_TRACE();

				std::vector<PacketResult> packetResults;

				for (const auto& packetResult : this->packetFeedbacks)
				{
					if (packetResult.IsReceived())
					{
						packetResults.push_back(packetResult);
					}
				}

				return packetResults;
			}
		} // namespace Types
	} // namespace BWE
} // namespace RTC
