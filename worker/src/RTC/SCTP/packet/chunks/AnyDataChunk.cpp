#define MS_CLASS "RTC::SCTP::AnyDataChunk"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/packet/chunks/AnyDataChunk.hpp"
#include "Logger.hpp"
#include "RTC/SCTP/packet/chunks/DataChunk.hpp"
#include "RTC/SCTP/packet/chunks/IDataChunk.hpp"

namespace RTC
{
	namespace SCTP
	{
		/* Class methods. */

		size_t AnyDataChunk::ComputeLengthFor(bool useMessageInterleaving, size_t userDataPayloadLength)
		{
			MS_TRACE();

			if (useMessageInterleaving)
			{
				return IDataChunk::ComputeLengthFor(userDataPayloadLength);
			}
			else
			{
				return DataChunk::ComputeLengthFor(userDataPayloadLength);
			}
		}
	} // namespace SCTP
} // namespace RTC
