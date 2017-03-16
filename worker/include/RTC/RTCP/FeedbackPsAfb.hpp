#ifndef MS_RTC_RTCP_FEEDBACK_PS_AFB_HPP
#define MS_RTC_RTCP_FEEDBACK_PS_AFB_HPP

#include "common.hpp"
#include "RTC/RTCP/Feedback.hpp"

namespace RTC { namespace RTCP
{
	class FeedbackPsAfbPacket
		: public FeedbackPsPacket
	{
	public:
		static FeedbackPsAfbPacket* Parse(const uint8_t* data, size_t len);

	public:
		// Parsed Report. Points to an external data.
		explicit FeedbackPsAfbPacket(CommonHeader* commonHeader);
		FeedbackPsAfbPacket(uint32_t sender_ssrc, uint32_t media_ssrc);
		virtual ~FeedbackPsAfbPacket() {};

		uint8_t* GetData() const;

	/* Pure virtual methods inherited from Packet. */
	public:
		virtual void Dump() const override;
		virtual size_t Serialize(uint8_t* buffer) override;
		virtual size_t GetSize() const override;

	private:
		uint8_t* data = nullptr;
		size_t size = 0;
	};

	/* Inline instance methods. */

	inline
	FeedbackPsAfbPacket::FeedbackPsAfbPacket(CommonHeader* commonHeader):
		FeedbackPsPacket(commonHeader)
	{
		this->size = ((ntohs(commonHeader->length) + 1) * 4) - (sizeof(CommonHeader) + sizeof(FeedbackPacket::Header));
		this->data = (uint8_t*)commonHeader + sizeof(CommonHeader) + sizeof(FeedbackPacket::Header);
	}

	inline
	FeedbackPsAfbPacket::FeedbackPsAfbPacket(uint32_t sender_ssrc, uint32_t media_ssrc):
		FeedbackPsPacket(FeedbackPs::AFB, sender_ssrc, media_ssrc)
	{}

	inline
	uint8_t* FeedbackPsAfbPacket::GetData() const
	{
		return this->data;
	}

	inline
	size_t FeedbackPsAfbPacket::GetSize() const
	{
		return FeedbackPsPacket::GetSize() + this->size;
	}
}}

#endif
