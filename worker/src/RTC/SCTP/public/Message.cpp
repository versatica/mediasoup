#define MS_CLASS "RTC::SCTP::Message"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/public/Message.hpp"
#include "Logger.hpp"

namespace RTC
{
	namespace SCTP
	{
		Message::Message(uint16_t streamId, uint32_t ppid, std::vector<uint8_t> payload)
		  : streamId(streamId), ppid(ppid), payload(std::move(payload))
		{
			MS_TRACE();
		}

		Message::~Message()
		{
			MS_TRACE();
		}

		void Message::Dump(int indentation) const
		{
			MS_TRACE();

			MS_DUMP_CLEAN(indentation, "<SCTP::Message>");
			MS_DUMP_CLEAN(indentation, "  stream id: %" PRIu16, GetStreamId());
			MS_DUMP_CLEAN(indentation, "  ppid: %" PRIu32, GetPayloadProtocolId());
			MS_DUMP_CLEAN(indentation, "  payload length: %zu", GetPayloadLength());
			MS_DUMP_CLEAN(indentation, "</SCTP::Message>");
		}
	} // namespace SCTP
} // namespace RTC
