#ifndef MS_RTC_SERIALIZABLE_FOO_DATA_ITEM_HPP
#define MS_RTC_SERIALIZABLE_FOO_DATA_ITEM_HPP

#include "common.hpp"
#include "Utils.hpp"
#include "RTC/Serializable.hpp"
#include "RTC/TestSerializable/FooItem.hpp"

using namespace RTC;

/**
 * FooDataItem.
 *
 *  0                   1                   2                   3
 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |   Id  | Flags | Value Length  |            Number             |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 * - Id (4 bits): Unsigned integer.
 * - Flags (4 bits).
 * - Value Length (8 bits): Length of the Value field. Must be 2.
 * - Number (16 bits): Unsigned integer.
 *
 * Length of a FooDataItem must be 4 bytes.
 */

class FooDataItem : public FooItem
{
public:
	/**
	 * FooDataItem has fixed length.
	 */
	static const size_t Length{ 4u };
	static const size_t NumberLength{ 2u };

public:
	/**
	 * Parse a FooDataItem.
	 *
	 * @remarks
	 * - `bufferLength` may exceed the exact length of the item.
	 */
	static std::unique_ptr<FooDataItem> Parse(const uint8_t* buffer, size_t bufferLength);

	static std::unique_ptr<FooDataItem> Factory(
	  const uint8_t* buffer, size_t bufferLength, uint8_t flags, uint16_t number);

private:
	/**
	 * Constructor is private because we only want to create FooDataItem instances
	 * via Parse() and Factory().
	 */
	FooDataItem(const uint8_t* buffer, size_t bufferLength, bool initializeHeader);

public:
	virtual ~FooDataItem() override;

	virtual void Dump() const override final;

	virtual uint16_t GetNumber() const final
	{
		return Utils::Byte::Get2Bytes(GetValuePointer(), 0);
	}

	virtual void SetNumber(uint16_t number) final
	{
		// Convert number to network byte order.
		number = uint16_t{ htons(number) };

		SetValue(reinterpret_cast<const uint8_t*>(std::addressof(number)), NumberLength);
	}
};

#endif
