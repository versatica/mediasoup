#define MS_CLASS "RTC::RTCP::FeedbackPsPli"
// #define MS_LOG_DEV

#include "RTC/RTCP/FeedbackPsPli.hpp"
#include "Logger.hpp"

namespace RTC
{
	namespace RTCP
	{
		/* Class methods. */

		FeedbackPsPliPacket* FeedbackPsPliPacket::Parse(const uint8_t* data, size_t len)
		{
			MS_TRACE();

			if (sizeof(CommonHeader) + sizeof(FeedbackPacket::Header) > len)
			{
				MS_WARN_TAG(rtcp, "not enough space for Feedback packet, discarded");

				return nullptr;
			}

			auto* commonHeader = const_cast<CommonHeader*>(reinterpret_cast<const CommonHeader*>(data));

			std::unique_ptr<FeedbackPsPliPacket> packet(new FeedbackPsPliPacket(commonHeader));

			return packet.release();
		}

		void FeedbackPsPliPacket::Dump() const
		{
			MS_TRACE();

			MS_DEBUG_DEV("<FeedbackPsPliPacket>");
			FeedbackPsPacket::Dump();
			MS_DEBUG_DEV("</FeedbackPsPliPacket>");
		}
	} // namespace RTCP
} // namespace RTC
