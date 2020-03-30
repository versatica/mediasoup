#ifndef MS_RTC_RTCP_FEEDBACK_PS_PLI_HPP
#define MS_RTC_RTCP_FEEDBACK_PS_PLI_HPP

#include "common.hpp"
#include "RTC/RTCP/Feedback.hpp"

namespace RTC
{
	namespace RTCP
	{
		class FeedbackPsPliPacket : public FeedbackPsPacket
		{
		public:
			static FeedbackPsPliPacket* Parse(const uint8_t* data, size_t len);

		public:
			// Parsed Report. Points to an external data.
			explicit FeedbackPsPliPacket(CommonHeader* commonHeader) : FeedbackPsPacket(commonHeader)
			{
			}
			FeedbackPsPliPacket(uint32_t senderSsrc, uint32_t mediaSsrc)
			  : FeedbackPsPacket(FeedbackPs::MessageType::PLI, senderSsrc, mediaSsrc)
			{
			}
			~FeedbackPsPliPacket() override = default;

		public:
			void Dump() const override;
		};
	} // namespace RTCP
} // namespace RTC

#endif
