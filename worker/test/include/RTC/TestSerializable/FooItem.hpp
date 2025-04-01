#ifndef MS_RTC_SERIALIZABLE_FOO_ITEM_HPP
#define MS_RTC_SERIALIZABLE_FOO_ITEM_HPP

#include "common.hpp"
#include "RTC/Serializable.hpp"

using namespace RTC;

/**
 * Foo Item.
 *  0                   1                   2                   3
 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |   Id  | Flags | Value Length  |            Value              |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                              ...                              |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 * - Id (4 bits): Unsigned integer.
 * - Flags (4 bits).
 * - Value Length (8 bits): Length of the Value field. It can be 0.
 *
 * Given that Value Length field is the length of the Value field, the total
 * length of a FooItem can be between 2 and 17 bytes.
 *
 * It's NOT mandatory that the FooItem total length is multiple of 4 bytes and
 * it doesn't use padding bytes.
 */

class FooItem : public Serializable
{
public:
	/**
	 * Struct of a the FooItem Header.
	 */
	struct ItemHeader
	{
#if defined(MS_LITTLE_ENDIAN)
		uint8_t flags : 4;
		uint8_t id : 4;
#elif defined(MS_BIG_ENDIAN)
		uint8_t id : 4;
		uint8_t flags : 4;
#endif
		uint8_t valueLength;
	};

public:
	static const size_t ItemHeaderLength{ 2u };

public:
	/**
	 * Parse a FooItem.
	 *
	 * @remarks
	 * - `bufferLength` may exceed the exact length of the item.
	 */
	static std::unique_ptr<FooItem> Parse(const uint8_t* buffer, size_t bufferLength);

	static std::unique_ptr<FooItem> Factory(
	  const uint8_t* buffer,
	  size_t bufferLength,
	  uint8_t id,
	  uint8_t flags,
	  const uint8_t* value,
	  uint8_t valueLength);

private:
	/**
	 * Constructor is private because we only want to create FooItem instances
	 * via Parse() and Factory().
	 */
	FooItem(const uint8_t* buffer, size_t bufferLength, bool initializeHeader);

public:
	~FooItem() override;

	void Dump() const override;

	std::unique_ptr<Serializable> Clone(const uint8_t* buffer, size_t bufferLength) const override;

	uint8_t GetId() const
	{
		return GetHeaderPointer()->id;
	}

	void SetId(uint8_t id)
	{
		GetHeaderPointer()->id = id;
	}

	uint8_t GetFlags() const
	{
		return GetHeaderPointer()->flags;
	}

	void SetFlags(uint8_t flags)
	{
		GetHeaderPointer()->flags = flags;
	}

	bool HasValue() const
	{
		return GetValueLengthField() > 0u;
		;
	}

	const uint8_t* GetValue() const
	{
		if (!HasValue())
		{
			return nullptr;
		}

		return GetValuePointer();
	}

	uint8_t GetValueLength() const
	{
		if (!HasValue())
		{
			return 0u;
		}

		return GetValueLengthField();
	}

	void SetValue(const uint8_t* value, uint8_t valueLength);

private:
	ItemHeader* GetHeaderPointer() const
	{
		return reinterpret_cast<ItemHeader*>(const_cast<uint8_t*>(GetBuffer()));
	}

	// We make this method private because it returns the value of the Value
	// Length field, which is not useful for the application.
	uint8_t GetValueLengthField() const
	{
		return GetHeaderPointer()->valueLength;
	}

	void SetValueLengthField(uint8_t valueLength)
	{
		GetHeaderPointer()->valueLength = valueLength;
	}

	const uint8_t* GetValuePointer() const
	{
		return GetBuffer() + ItemHeaderLength;
	}

	const uint8_t* GetEndPointer() const
	{
		return GetBuffer() + ItemHeaderLength + GetValueLength();
	}
};

#endif
