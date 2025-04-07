#ifndef MS_RTC_SERIALIZABLE_FOO_NUMERIC_ITEM_HPP
#define MS_RTC_SERIALIZABLE_FOO_NUMERIC_ITEM_HPP

#include "common.hpp"
#include "RTC/TestSerializable/FooItem.hpp"

using namespace RTC;

/**
 * FooNumericItem.
 *
 *  0                   1                   2                   3
 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |   Id  | Flags | Value Length  |            Number             |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 * - Id (4 bits): Unsigned integer. Must be 1 (NUMERIC).
 * - Flags (4 bits).
 * - Value Length (8 bits): Length of the Value field. Must be 2.
 * - Number (16 bits): Unsigned integer.
 *
 * Length of a FooNumericItem must be 4 bytes.
 */

class FooNumericItem : public FooItem
{
public:
	/**
	 * FooNumericItem has fixed length.
	 */
	static const size_t NumberLength{ 2u };

public:
	/**
	 * Parse a FooNumericItem.
	 *
	 * @remarks
	 * - `bufferLength` may exceed the exact length of the item.
	 */
	static FooNumericItem* Parse(const uint8_t* buffer, size_t bufferLength);

	/**
	 * Create a FooNumericItem.
	 *
	 * @remarks
	 * - `bufferLength` could be greater than the item real length.
	 */
	static FooNumericItem* Factory(uint8_t* buffer, size_t bufferLength, uint8_t flags, uint16_t number);

private:
	/**
	 * Private constructor used by Parse() and Factory() static methods.
	 */
	FooNumericItem(const uint8_t* buffer, size_t bufferLength);

public:
	virtual ~FooNumericItem() override final;

	virtual void Dump() const override final;

	virtual FooNumericItem* Clone(uint8_t* buffer, size_t bufferLength) const override final;

	virtual uint16_t GetNumber() const final;

	virtual void SetNumber(uint16_t number) final;
};

#endif
