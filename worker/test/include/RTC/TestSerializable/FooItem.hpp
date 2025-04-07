#ifndef MS_RTC_SERIALIZABLE_FOO_ITEM_HPP
#define MS_RTC_SERIALIZABLE_FOO_ITEM_HPP

#include "common.hpp"
#include "RTC/Serializable.hpp"
#include <string>
#include <unordered_map>

using namespace RTC;

/**
 * FooItem.
 *
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
 * - Value (variable length).
 *
 * Given that Value Length field is the length of the Value field, the total
 * length of a FooItem can be between 2 and 17 bytes.
 *
 * It's NOT mandatory that the FooItem total length is multiple of 4 bytes and
 * it doesn't use padding bytes.
 */

// Forward declaration.
class FooPacket;

class FooItem : public Serializable
{
private:
	friend class FooPacket;

public:
	/**
	 * Item Id.
	 *
	 * @remarks
	 * Values other than the defined must be accepted as FooUnknownItem.
	 */
	enum class ItemId : uint8_t
	{
		NUMERIC = 0x1,
		TEXT    = 0x2
	};

	/**
	 * Struct of a the FooItem Header.
	 */
	struct ItemHeader
	{
#if defined(MS_LITTLE_ENDIAN)
		uint8_t flags : 4;
		ItemId id : 4;
#elif defined(MS_BIG_ENDIAN)
		ItemId id : 4;
		uint8_t flags : 4;
#endif
		uint8_t valueLength;
	};

public:
	static const size_t ItemHeaderLength{ 2u };

public:
	/**
	 * Whether given buffer could be a a valid FooItem.
	 *
	 * @param buffer
	 * @param bufferLength - Can be greater than real Chunk length.
	 * @param itemId - If given buffer is a valid FooItem then `itemId` is
	 *   rewritten to parsed ItemId.
	 * @param valueLength - If given buffer is a valid FooItem then `valueLength`
	 *   is rewritten to the value of the Value Length field.
	 */
	static bool IsFooItem(const uint8_t* buffer, size_t bufferLength, ItemId& itemId, uint8_t& valueLength);

	static const std::string& ItemId2String(ItemId id);

private:
	static std::unordered_map<ItemId, std::string> itemId2String;

protected:
	/**
	 * Constructor is protected because we only want to create FooItem instances
	 * via Parse() and Factory() in subclasses.
	 */
	FooItem(const uint8_t* buffer, size_t bufferLength);

public:
	virtual ~FooItem() override;

	/**
	 * NOTE: Should be overridden by each subclass.
	 */
	virtual void Dump() const override;

	/**
	 * Can be overridden by each subclass.
	 */
	virtual FooItem* Clone(uint8_t* buffer, size_t bufferLength) const override;

	virtual ItemId GetId() const final
	{
		return GetHeaderPointer()->id;
	}

	virtual bool HasUnknownId() const final
	{
		auto id = GetId();

		return id < ItemId::NUMERIC || id > ItemId::TEXT;
	}

	virtual uint8_t GetFlags() const final
	{
		return GetHeaderPointer()->flags;
	}

	virtual bool HasValue() const final
	{
		return GetValueLengthField() > 0u;
	}

	virtual uint8_t GetValueLength() const final
	{
		if (!HasValue())
		{
			return 0u;
		}

		return GetValueLengthField();
	}

protected:
	virtual void InitializeHeader(ItemId id, uint8_t flags, uint8_t valueLength) final;

	/**
	 * NOTE: Return ItemHeader* instead of const ItemHeader* since we may
	 * want to modify its fields.
	 */
	virtual ItemHeader* GetHeaderPointer() const final
	{
		return reinterpret_cast<ItemHeader*>(const_cast<uint8_t*>(GetBuffer()));
	}

	/**
	 * Private private because it returns the value of the Value Length field,
	 * which is not useful for the application.
	 */
	virtual uint8_t GetValueLengthField() const final
	{
		return GetHeaderPointer()->valueLength;
	}

	virtual void SetValueLengthField(uint8_t valueLength) final
	{
		GetHeaderPointer()->valueLength = valueLength;
	}

	virtual uint8_t* GetValuePointer() const final
	{
		return const_cast<uint8_t*>(GetBuffer()) + FooItem::ItemHeaderLength;
	}
};

#endif
