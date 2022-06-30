#ifndef MS_RTC_RTCP_FEEDBACK_PS_AFB_HPP
#define MS_RTC_RTCP_FEEDBACK_PS_AFB_HPP

#include "common.hpp"
#include "RTC/RTCP/Feedback.hpp"

namespace RTC
{
	namespace RTCP
	{
		class FeedbackPsAfbPacket : public FeedbackPsPacket
		{
		public:
			enum class Application : uint8_t
			{
				UNKNOWN = 0,
				REMB    = 1
			};

		public:
			static FeedbackPsAfbPacket* Parse(const uint8_t* data, size_t len);

		public:
			// Parsed Report. Points to an external data.
			explicit FeedbackPsAfbPacket(
			  CommonHeader* commonHeader, Application application = Application::UNKNOWN)
			  : FeedbackPsPacket(commonHeader)
			{
				this->size = ((static_cast<size_t>(ntohs(commonHeader->length)) + 1) * 4) -
				             (Packet::CommonHeaderSize + FeedbackPacket::HeaderSize);

				this->data = (uint8_t*)commonHeader + Packet::CommonHeaderSize + FeedbackPacket::HeaderSize;

				this->application = application;
			}
			FeedbackPsAfbPacket(
			  uint32_t senderSsrc, uint32_t mediaSsrc, Application application = Application::UNKNOWN)
			  : FeedbackPsPacket(FeedbackPs::MessageType::AFB, senderSsrc, mediaSsrc)
			{
				this->application = application;
			}
			~FeedbackPsAfbPacket() override = default;

			Application GetApplication() const
			{
				return this->application;
			}

			/* Pure virtual methods inherited from Packet. */
		public:
			void Dump() const override;
			size_t Serialize(uint8_t* buffer) override;
			size_t GetSize() const override
			{
				return FeedbackPsPacket::GetSize() + this->size;
			}

		private:
			Application application{ Application::UNKNOWN };
			uint8_t* data{ nullptr };
			size_t size{ 0 };
		};
	} // namespace RTCP
} // namespace RTC

#endif
