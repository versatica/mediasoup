#define MS_CLASS "RTC::SCTP::UserData"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/packet/UserData.hpp"
#include "Logger.hpp"

namespace RTC
{
	namespace SCTP
	{
		UserData::UserData(
		  uint16_t streamId,
		  uint16_t ssn,
		  uint32_t mid,
		  uint32_t fsn,
		  uint32_t ppid,
		  std::vector<uint8_t> payload,
		  bool isBeginning,
		  bool isEnd,
		  bool isUnordered)
		  : streamId(streamId),
		    ssn(ssn),
		    mid(mid),
		    fsn(fsn),
		    ppid(ppid),
		    payload(std::move(payload)),
		    isBeginning(isBeginning),
		    isEnd(isEnd),
		    isUnordered(isUnordered)
		{
			MS_TRACE();
		}

		UserData::~UserData()
		{
			MS_TRACE();
		}

		void UserData::Dump(int indentation) const
		{
			MS_TRACE();

			MS_DUMP_CLEAN(indentation, "<SCTP::UserData>");
			MS_DUMP_CLEAN(indentation, "  stream id: %" PRIu16, GetStreamId());
			MS_DUMP_CLEAN(indentation, "  ssn: %" PRIu16, GetStreamSequenceNumber());
			MS_DUMP_CLEAN(indentation, "  mid: %" PRIu32, GetMessageId());
			MS_DUMP_CLEAN(indentation, "  fsn: %" PRIu32, GetFragmentSequenceNumber());
			MS_DUMP_CLEAN(indentation, "  ppid: %" PRIu32, GetPayloadProtocolId());
			MS_DUMP_CLEAN(indentation, "  payload length: %zu", GetPayloadLength());
			MS_DUMP_CLEAN(indentation, "  is beginning: %s", IsBeginning() ? "yes" : "no");
			MS_DUMP_CLEAN(indentation, "  is end: %s", IsEnd() ? "yes" : "no");
			MS_DUMP_CLEAN(indentation, "  is unordered: %s", IsUnordered() ? "yes" : "no");
			MS_DUMP_CLEAN(indentation, "</SCTP::UserData>");
		}
	} // namespace SCTP
} // namespace RTC
