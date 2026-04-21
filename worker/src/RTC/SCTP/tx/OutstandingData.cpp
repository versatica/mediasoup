#define MS_CLASS "RTC::SCTP::OutstandingData"
// TODO: SCTP: COMMENT
#define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/tx/OutstandingData.hpp"
#include "Logger.hpp"

namespace RTC
{
	namespace SCTP
	{
		/* Instance methods. */

		OutstandingData::OutstandingData(
		  size_t dataChunkHeaderLength,
		  UnwrappedTsn lastCumulativeTsnAck,
		  std::function<bool(uint16_t /*streamId*/, uint32_t /*outgoingMessageId*/)> discardFromSendQueue)
		  : dataChunkHeaderLength(dataChunkHeaderLength),
		    lastCumulativeTsnAck(lastCumulativeTsnAck),
		    discardFromSendQueue(std::move(discardFromSendQueue))
		{
			MS_TRACE()
		}
	} // namespace SCTP
} // namespace RTC
