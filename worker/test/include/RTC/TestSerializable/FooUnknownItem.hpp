#ifndef MS_RTC_SERIALIZABLE_FOO_UNKNOWN_ITEM_HPP
#define MS_RTC_SERIALIZABLE_FOO_UNKNOWN_ITEM_HPP

#include "common.hpp"
#include "RTC/TestSerializable/FooItem.hpp"

using namespace RTC;

/**
 * FooUnknownItem.
 *
 *  0                   1                   2                   3
 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |   Id  | Flags | Value Length  |            Text               |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                              ...                              |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 * - Id (4 bits): Unsigned integer. Anyone but well defined ones.
 * - Flags (4 bits).
 * - Value Length (8 bits): Length of the Value field.
 * - Text (variable length bits): Whatever.
 *
 * Length of a FooUnknownItem is therefore variable.
 */

class FooUnknownItem : public FooItem
{
public:
	/**
	 * Parse a FooUnknownItem.
	 *
	 * @remarks
	 * - `bufferLength` may exceed the exact length of the item.
	 */
	static std::unique_ptr<FooUnknownItem> Parse(const uint8_t* buffer, size_t bufferLength);

private:
	/**
	 * Private constructor used by Parse() static method.
	 */
	FooUnknownItem(const uint8_t* buffer, size_t bufferLength);

public:
	virtual ~FooUnknownItem() override;

	virtual void Dump() const override final;

	virtual const uint8_t* GetValue() const final;
};

#endif
