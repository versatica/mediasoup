#ifndef MS_RTC_RTCP_FEEDBACK_ITEM_H
#define MS_RTC_RTCP_FEEDBACK_ITEM_H

#include "common.h"

namespace RTC { namespace RTCP
{
	class FeedbackItem
	{
	public:
		bool IsCorrect();

	protected:
		virtual ~FeedbackItem();

	public:
		virtual void Dump() = 0;
		virtual void Serialize();
		virtual size_t Serialize(uint8_t* buffer) = 0;
		virtual size_t GetSize() = 0;

	protected:
		uint8_t* raw = nullptr;
		bool isCorrect = true;
	};

	/* Inline instance methods */

	inline
	FeedbackItem::~FeedbackItem()
	{
		if (this->raw)
			delete this->raw;
	}

	inline
	void FeedbackItem::Serialize()
	{
		if (this->raw)
			delete this->raw;

		this->raw = new uint8_t[this->GetSize()];
		this->Serialize(this->raw);
	}

	inline
	bool FeedbackItem::IsCorrect()
	{
		return this->isCorrect;
	}
}}

#endif
