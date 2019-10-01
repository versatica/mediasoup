#ifndef MS_RTC_RTCP_FEEDBACK_ITEM_HPP
#define MS_RTC_RTCP_FEEDBACK_ITEM_HPP

#include "common.hpp"

namespace RTC
{
	namespace RTCP
	{
		class FeedbackItem
		{
		public:
			template<typename Item>
			static Item* Parse(const uint8_t* data, size_t len);

		public:
			bool IsCorrect() const;

		protected:
			virtual ~FeedbackItem();

		public:
			virtual void Dump() const = 0;
			virtual void Serialize();
			virtual size_t Serialize(uint8_t* buffer) = 0;
			virtual size_t GetSize() const            = 0;

		protected:
			uint8_t* raw{ nullptr };
			bool isCorrect{ true };
		};

		/* Inline static methods */
		template<typename Item>
		Item* FeedbackItem::Parse(const uint8_t* data, size_t len)
		{
			// data size must be >= header.
			if (sizeof(typename Item::Header) > len)
				return nullptr;

			auto* header =
			  const_cast<typename Item::Header*>(reinterpret_cast<const typename Item::Header*>(data));

			return new Item(header);
		}

		/* Inline instance methods */

		inline FeedbackItem::~FeedbackItem()
		{
			if (this->raw)
				delete[] this->raw;
		}

		inline void FeedbackItem::Serialize()
		{
			if (this->raw)
				delete[] this->raw;

			this->raw = new uint8_t[this->GetSize()];
			this->Serialize(this->raw);
		}

		inline bool FeedbackItem::IsCorrect() const
		{
			return this->isCorrect;
		}
	} // namespace RTCP
} // namespace RTC

#endif
