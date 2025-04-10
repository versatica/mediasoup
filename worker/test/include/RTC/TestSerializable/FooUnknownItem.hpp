#ifndef MS_RTC_SERIALIZABLE_FOO_UNKNOWN_ITEM_HPP
#define MS_RTC_SERIALIZABLE_FOO_UNKNOWN_ITEM_HPP

#include "common.hpp"
#include "RTC/TestSerializable/FooItem.hpp"

namespace RTC
{
	/**
	 * FooUnknownItem.
	 *
	 *  0                   1                   2                   3
	 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
	 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	 * |   Id  | Flags | Value Length  |            Value              |
	 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	 * \                                                               \
	 * /                             Value                             /
	 * \                                                               \
	 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	 *
	 * - Id (4 bits): Unsigned integer.
	 * - Flags (4 bits).
	 * - Value Length (8 bits): Length of the Value field.
	 * - Value (variable length).
	 *
	 * Length of a FooUnknownItem must be 4 bytes.
	 */

	// Forward declaration.
	class FooPacket;

	class FooUnknownItem : public FooItem
	{
		friend class FooPacket;

	public:
		/**
		 * Parse a FooUnknownItem.
		 *
		 * @remarks
		 * - `bufferLength` may exceed the exact length of the item.
		 */
		static FooUnknownItem* Parse(const uint8_t* buffer, size_t bufferLength);

	private:
		/**
		 * Private constructor used by Parse() static method and by Clone() method
		 * in FooPacket (friend class).
		 */
		FooUnknownItem(const uint8_t* buffer, size_t bufferLength);

	public:
		virtual ~FooUnknownItem() override;

		virtual void Dump() const override final;

		virtual FooUnknownItem* Clone(uint8_t* buffer, size_t bufferLength) const override final;

		const uint8_t* GetValue() const;
	};
} // namespace RTC

#endif
