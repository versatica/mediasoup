#ifndef MS_RTC_SERIALIZABLE_FOO_TEXT_ITEM_HPP
#define MS_RTC_SERIALIZABLE_FOO_TEXT_ITEM_HPP

#include "common.hpp"
#include "RTC/TestSerializable/FooItem.hpp"
#include <string>
#include <string_view>

namespace RTC
{
	/**
	 * FooTextItem.
	 *
	 *  0                   1                   2                   3
	 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
	 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	 * |   Id  | Flags | Value Length  |            Text               |
	 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	 * |                              ...                              |
	 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	 *
	 * - Id (4 bits): Unsigned integer. Must be 2 (TEXT).
	 * - Flags (4 bits).
	 * - Value Length (8 bits): Length of the Value field.
	 * - Text (variable length bits): String not ended in \0.
	 *
	 * Length of a FooTextItem is therefore variable.
	 */

	class FooTextItem : public FooItem
	{
	public:
		/**
		 * Parse a FooTextItem.
		 *
		 * @remarks
		 * - `bufferLength` may exceed the exact length of the item.
		 */
		static FooTextItem* Parse(const uint8_t* buffer, size_t bufferLength);

		/**
		 * Create a FooTextItem.
		 *
		 * @remarks
		 * - `bufferLength` could be greater than the item real length.
		 */
		static FooTextItem* Factory(
		  uint8_t* buffer, size_t bufferLength, uint8_t flags, const std::string& text);

	private:
		/**
		 * Private constructor used by Parse() and Factory() static methods.
		 */
		FooTextItem(const uint8_t* buffer, size_t bufferLength);

	public:
		virtual ~FooTextItem() override;

		virtual void Dump() const override final;

		virtual FooTextItem* Clone(uint8_t* buffer, size_t bufferLength) const override final;

		virtual const std::string_view GetText() const final;

		virtual void SetText(const std::string& text) final;
	};
} // namespace RTC

#endif
