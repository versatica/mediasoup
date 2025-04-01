#define MS_CLASS "RTC::TestSerializable"
#define MS_LOG_DEV_LEVEL 3

#include "common.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "Utils.hpp"
#include "helpers.hpp"
#include "RTC/Serializable.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memset(), std::memcpy()
#include <utility> // std::move()
#include <vector>

using namespace RTC;

/**
 * Foo Packet.
 *  0                   1                   2                   3
 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |      Type     |A|  (Unused)   |            Length             |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                           Appendix                            |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |            Item 1             |            Item 2             |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |            Item 2                             |    Padding    |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 * - Type (8 bits): Unsigned integer.
 * - A (1 bit): Whether the packet contains an Appendix field.
 * - (Unusued) (7 bits).
 * - Length (16 bits): Length of the packet excluding padding bytes.
 * - Appendix (32 bits): Unsigned integer. Only exists if flag A is set.
 * - Items (variable length): N items.
 * - Padding: Bytes of padding to make the packet length be multiple of 4 bytes.
 *
 * It's mandatory that the Foo packet total length is multiple of 4 bytes.
 */

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
	static std::unique_ptr<FooItem> Parse(const uint8_t* buffer, size_t bufferLength)
	{
		// No space for header.
		if (bufferLength < ItemHeaderLength)
		{
			MS_WARN_DEV("no space for Item Header");

			return nullptr;
		}

		// Pointer that starts at the beginning of the buffer and it's incremented
		// to point to different parts of the item.
		const uint8_t* ptr = buffer;

		// Pointer that points to the end of the buffer.
		const uint8_t* end = buffer + bufferLength;

		// NOTE: We are parsing so we don't want to initialize the header.
		auto item =
		  std::unique_ptr<FooItem>(new FooItem(buffer, bufferLength, /*initializeHeader*/ false));

		printf(
		  "FooItem::Parse() START [ptr+:%zu, Value Length:%" PRIu8 ", bufferLength:%zu]\n",
		  ptr - buffer,
		  item->GetValueLengthField(),
		  bufferLength);

		// Move to the value.
		if (item->HasValue())
		{
			ptr = item->GetValuePointer();

			printf("FooItem::Parse() has value [ptr+:%zu]\n", ptr - buffer);

			// No space for value.
			if (ptr + item->GetValueLength() > end)
			{
				MS_WARN_DEV("no space for Item Value");

				return nullptr;
			}
		}

		// Move to the end of the value.
		ptr = item->GetEndPointer();

		const size_t computedLength = ptr - buffer;

		printf("FooItem::Parse() END [ptr+:%zu, computedLength:%zu]\n", ptr - buffer, computedLength);

		// It's mandatory to call SetLength() once we are done and we know the
		// exact length of the item.
		item->SetLength(computedLength);

		return item;
	}

	static std::unique_ptr<FooItem> Factory(
	  const uint8_t* buffer,
	  size_t bufferLength,
	  uint8_t id,
	  uint8_t flags,
	  const uint8_t* value,
	  uint8_t valueLength)
	{
		const size_t computedLength = ItemHeaderLength + valueLength;

		// No space for header.
		if (bufferLength < computedLength)
		{
			MS_THROW_TYPE_ERROR("no space for Item Header");
		}

		printf(
		  "FooItem::Factory() [computedLength:%zu, bufferLength:%zu]\n", computedLength, bufferLength);

		// We want to initialize the header since we are creating an item from
		// scratch.
		auto item =
		  std::unique_ptr<FooItem>(new FooItem(buffer, bufferLength, /*initializeHeader*/ true));

		item->SetId(id);
		item->SetFlags(flags);
		item->SetValue(value, valueLength);

		// NOTE: No need to call item->SetLength() since item->SetValue() already
		// does it.

		return item;
	}

private:
	/**
	 * Constructor is private because we only want to create FooItem instances
	 * via Parse() and Factory().
	 */
	FooItem(const uint8_t* buffer, size_t bufferLength, bool initializeHeader)
	  : Serializable(buffer, bufferLength)
	{
		if (initializeHeader)
		{
			SetId(0u);
			SetFlags(0u);
			SetValueLengthField(0u);

			// Update Serializable length.
			SetLength(ItemHeaderLength);
		}
	}

public:
	~FooItem() override
	{
	}

	void Dump() const override
	{
		MS_TRACE();

		MS_DUMP("<FooItem>");
		MS_DUMP("  length: %zu (buffer length: %zu)", GetLength(), GetBufferLength());
		MS_DUMP("  id: %" PRIu8, GetId());
		MS_DUMP("  flags: " MS_UINT8_4BITS_TO_BINARY_PATTERN, MS_UINT8_4BITS_TO_BINARY(GetFlags()));
		MS_DUMP(
		  "  value length field: %" PRIu8 " (computed value length: %" PRIu8 ")",
		  GetValueLengthField(),
		  GetValueLength());
		MS_DUMP("  value:");
		MS_DUMP_DATA(GetValue(), GetValueLength());
		MS_DUMP("");
		MS_DUMP("</FooItem>");
	}

	std::unique_ptr<Serializable> Clone(const uint8_t* buffer, size_t bufferLength) const override
	{
		MS_TRACE();

		std::memcpy(const_cast<uint8_t*>(buffer), GetBuffer(), GetLength());

		auto clonedFooItem =
		  std::unique_ptr<FooItem>(new FooItem(buffer, bufferLength, /*initializeHeader*/ false));

		// Need to manually set Serializable length.
		clonedFooItem->SetLength(GetLength());

		return clonedFooItem;
	}

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

	void SetValue(const uint8_t* value, uint8_t valueLength)
	{
		auto previousValueLength = GetValueLength();

		// Update the Value Length field.
		SetValueLengthField(valueLength);

		// Copy the given value into the buffer.
		std::memcpy(const_cast<uint8_t*>(GetValuePointer()), value, valueLength);

		// Update Serializable length.
		SetLength(GetLength() - previousValueLength + valueLength);
	}

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

class Foo : public Serializable
{
public:
	/**
	 * Struct of a the Foo Header.
	 */
	struct Header
	{
		uint8_t type;
#if defined(MS_LITTLE_ENDIAN)
		uint8_t unused : 7;
		uint8_t a : 1;
#elif defined(MS_BIG_ENDIAN)
		uint8_t a : 1;
		uint8_t unused : 7;
#endif
		uint16_t length;
	};

public:
	static const size_t HeaderLength{ 4u };
	static const size_t AppendixLength{ 4u };

public:
	/**
	 * Parse a Foo packet.
	 *
	 * @remarks
	 * - `length` must be the exact length of the packet.
	 */
	static std::unique_ptr<Foo> Parse(const uint8_t* buffer, size_t length)
	{
		// No space for header.
		if (length < HeaderLength)
		{
			MS_WARN_DEV("no space for Header");

			return nullptr;
		}

		// Pointer that starts at the beginning of the buffer and it's incremented
		// to point to different parts of the packet.
		const uint8_t* ptr = buffer;

		// Pointer that points to the end of the buffer.
		const uint8_t* end = buffer + length;

		// NOTE: Here we are passing `length` as `bufferLength`. However we know
		// that, due to Foo nature, a Foo packet must occupy the whole given buffer.
		// NOTE: We are parsing so we don't want to initialize the header.
		// auto* foo = new Foo(buffer, length, /*initializeHeader*/ false);
		auto foo = std::unique_ptr<Foo>(new Foo(buffer, length, /*initializeHeader*/ false));

		printf(
		  "Foo::Parse() START [ptr+:%zu, Length:%" PRIu16 ", length:%zu]\n",
		  ptr - buffer,
		  foo->GetLengthField(),
		  length);

		// Move to the Appendix.
		if (foo->HasAppendix())
		{
			ptr = foo->GetAppendixPointer();

			printf("Foo::Parse() has Appendix [ptr+:%zu]\n", ptr - buffer);

			// No space for Appendix.
			if (ptr + AppendixLength > end)
			{
				MS_WARN_DEV("no space for Appendix");

				return nullptr;
			}
		}

		// Move to items.
		if (foo->HasItems())
		{
			if (foo->GetLengthField() > length)
			{
				MS_WARN_DEV("no space for Items");

				return nullptr;
			}

			ptr = foo->GetItemsPointer();

			printf("Foo::Parse() has items [ptr+:%zu]\n", ptr - buffer);

			while (ptr < buffer + foo->GetLengthField())
			{
				printf("Foo::Parse() parsing item [ptr+:%zu]\n", ptr - buffer);

				auto item = FooItem::Parse(ptr, buffer + foo->GetLengthField() - ptr);

				if (item)
				{
					// Let's fix item's buffer length. This is because we didn't know its
					// exact length when we called FooItem::Parse() so we passed the rest
					// of the packet buffer as buffer length. Once item is parsed, and
					// given that it is part of the foo packet buffer, we can fix its
					// buffer length by making it be equal to its real length.
					item->SetBufferLength(item->GetLength());

					// NOTE: We are gonna move item ownership in next line so must do
					// this before.
					ptr += item->GetLength();

					// Here we are parsing so we don't use AddItem() (that serializes the
					// Item into the packet buffer, but AddParsedItem().
					// NOTE: We need to pass an unique_ptr so beed to use std::move() to
					// transfer ownership.
					foo->AddParsedItem(std::move(item));
				}
				else
				{
					MS_WARN_DEV("wrong Item");

					return nullptr;
				}
			}
		}

		// Move to the possible padding.
		ptr = foo->GetPaddingPointer();

		const size_t computedLength = Utils::Byte::PadTo4Bytes(static_cast<size_t>(ptr - buffer));

		printf("Foo::Parse() END [ptr+:%zu, computedLength:%zu]\n", ptr - buffer, computedLength);

		// Ensure computed length (padded to 4 bytes) matches the total given
		// length.
		if (computedLength != length)
		{
			MS_WARN_DEV("computed padded length != buffer length");

			return nullptr;
		}

		// It's mandatory to call SetLength() once we are done and we know the exact
		// length of the packet (padding included).
		foo->SetLength(computedLength);

		return foo;
	}

	static std::unique_ptr<Foo> Factory(const uint8_t* buffer, size_t bufferLength, uint8_t type)
	{
		size_t computedLength = HeaderLength;

		// No space for header.
		if (bufferLength < computedLength)
		{
			MS_THROW_TYPE_ERROR("no space for Header");
		}

		printf("Foo::Factory() [computedLength:%zu, bufferLength:%zu]\n", computedLength, bufferLength);

		// We want to initialize the header since we are creating a packet from
		// scratch.
		auto foo = std::unique_ptr<Foo>(new Foo(buffer, bufferLength, /*initializeHeader*/ true));

		foo->SetType(type);

		// NOTE: No need to call foo->SetLength() since the constructor did.

		return foo;
	}

private:
	/**
	 * Constructor is private because we only want to create Foo instances via
	 * Parse() and Factory().
	 */
	Foo(const uint8_t* buffer, size_t bufferLength, bool initializeHeader)
	  : Serializable(buffer, bufferLength)
	{
		if (initializeHeader)
		{
			SetType(0u);
			SetAppendixFlag(false);
			SetUnusedField();
			SetLengthField(HeaderLength);

			// Update Serializable length.
			SetLength(HeaderLength);
		}
	}

public:
	~Foo() override
	{
	}

	void Dump() const override
	{
		MS_TRACE();

		MS_DUMP("<Foo>");
		MS_DUMP("  length (padding included): %zu (buffer length: %zu)", GetLength(), GetBufferLength());
		MS_DUMP("  type: %" PRIu8, GetType());
		MS_DUMP("  length field: %" PRIu16, GetLengthField());
		MS_DUMP("  has appendix: %s", HasAppendix() ? "yes" : "no");
		MS_DUMP("  appendix: %" PRIu32, GetAppendix());
		MS_DUMP("  has items: %s", HasItems() ? "yes" : "no");
		MS_DUMP("  items count: %zu", GetItemsCount());
		for (auto& item : this->items)
		{
			item->Dump();
		}
		MS_DUMP("</Foo>");
	}

	void Serialize(const uint8_t* buffer, size_t bufferLength) override
	{
		size_t itemsOffset   = GetItemsPointer() - GetBuffer();
		size_t paddingOffset = GetPaddingPointer() - GetBuffer();
		size_t padding       = GetLength() - (GetPaddingPointer() - GetBuffer());

		// Copy all bytes from beginning of the buffer until the position of the
		// Items.
		std::memcpy(const_cast<uint8_t*>(buffer), GetBuffer(), itemsOffset);

		// Serialize each Item into the new buffer.
		uint8_t* ptr = const_cast<uint8_t*>(buffer) + itemsOffset;

		for (auto& item : this->items)
		{
			item->Serialize(ptr, item->GetLength());

			ptr += item->GetLength();
		}

		// Copy padding bytes.
		std::memcpy(const_cast<uint8_t*>(buffer) + paddingOffset, GetPaddingPointer(), padding);

		// Manually update buffer and buffer length.
		SetBuffer(buffer);
		SetBufferLength(bufferLength);
	}

	std::unique_ptr<Serializable> Clone(const uint8_t* buffer, size_t bufferLength) const override
	{
		MS_TRACE();

		auto clonedFoo = std::unique_ptr<Foo>(new Foo(buffer, bufferLength, /*initializeHeader*/ false));

		// TODO: Clone items.

		// Need to manually set Serializable length.
		clonedFoo->SetLength(GetLength());

		return clonedFoo;
	}

	uint8_t GetType() const
	{
		return GetHeaderPointer()->type;
	}

	void SetType(uint8_t type)
	{
		GetHeaderPointer()->type = type;
	}

	bool HasAppendix() const
	{
		return GetHeaderPointer()->a;
	}

	uint32_t GetAppendix() const
	{
		if (!HasAppendix())
		{
			return 0u;
		}

		return Utils::Byte::Get4Bytes(GetAppendixPointer(), 0);
	}

	/**
	 * If given `appendix` is 0 then Appendix is removed.
	 */
	void SetAppendix(uint32_t appendix)
	{
		auto hadAppendix = HasAppendix();

		// There was Appendix and we are just replacing it, so packet length
		// remains the same.
		if (hadAppendix && appendix)
		{
			Utils::Byte::Set4Bytes(const_cast<uint8_t*>(GetAppendixPointer()), 0, appendix);
		}
		// There wasn't Appendix and we are adding it, so need to move items.
		else if (!hadAppendix && appendix)
		{
			size_t previousLengthWithoutPadding = GetPaddingPointer() - GetBuffer();
			size_t lengthWithoutPadding         = previousLengthWithoutPadding + AppendixLength;

			// Update flag A and Length field. This will make `GetXxxxxPointer()`
			// return different values as if there was Appendix field.
			SetAppendixFlag(true);
			SetLengthField(lengthWithoutPadding);

			// Update Serializable length.
			SetLength(Utils::Byte::PadTo4Bytes(lengthWithoutPadding));

			for (auto it = this->items.rbegin(); it != this->items.rend(); ++it)
			{
				auto& item = *it;

				item->Serialize(item->GetBuffer() + AppendixLength, item->GetLength());
			}

			// Copy the given Appendix value.
			Utils::Byte::Set4Bytes(const_cast<uint8_t*>(GetAppendixPointer()), 0, appendix);
		}
		// There was Appendix and we are removing it, so need to move items.
		else if (hadAppendix && !appendix)
		{
			size_t previousLengthWithoutPadding = GetPaddingPointer() - GetBuffer();
			size_t lengthWithoutPadding         = previousLengthWithoutPadding - AppendixLength;

			// Update flag A and Length field. This will make `GetXxxxxPointer()`
			// return different values as if there was an Appendix field.
			SetAppendixFlag(false);
			SetLengthField(lengthWithoutPadding);

			// Update Serializable length.
			SetLength(Utils::Byte::PadTo4Bytes(lengthWithoutPadding));

			for (auto& item : this->items)
			{
				item->Serialize(item->GetBuffer() - AppendixLength, item->GetLength());
			}
		}
	}

	bool HasItems() const
	{
		if (HasAppendix())
		{
			return GetLengthField() > HeaderLength + AppendixLength;
		}
		else
		{
			return GetLengthField() > HeaderLength;
		}
	}

	size_t GetItemsCount() const
	{
		return this->items.size();
	}

	const std::unique_ptr<FooItem>& GetItem(size_t idx) const
	{
		if (idx >= this->items.size())
		{
			static std::unique_ptr<FooItem> nullItem;

			return nullItem;
		}

		return this->items.at(idx);
	}

	/**
	 * Serializes given Item into Foo's buffer.
	 *
	 * @remarks
	 * Once this method is called, the Item is owned by Foo instance and will be
	 * deallocated in Foo destructor.
	 */
	void AddItem(FooItem* item)
	{
		AddItem(std::unique_ptr<FooItem>(item));
	}

	// TODO: Remove this or the above.
	void AddItem(std::unique_ptr<FooItem> item)
	{
		size_t previousLengthWithoutPadding = GetPaddingPointer() - GetBuffer();
		size_t lengthWithoutPadding         = previousLengthWithoutPadding + item->GetLength();

		// Let's append the item at the end of existing items, this is, where the
		// padding would start.
		item->Serialize(GetPaddingPointer(), item->GetLength());

		// Update Length field.
		SetLengthField(lengthWithoutPadding);

		// Update Serializable length.
		SetLength(Utils::Byte::PadTo4Bytes(lengthWithoutPadding));

		this->items.push_back(std::move(item));
	}

private:
	Header* GetHeaderPointer() const
	{
		return reinterpret_cast<Header*>(const_cast<uint8_t*>(GetBuffer()));
	}

	// We make this method private because it returns the value of the Length
	// field, which is not useful for the application.
	uint16_t GetLengthField() const
	{
		return uint16_t{ ntohs(GetHeaderPointer()->length) };
	}

	void SetAppendixFlag(bool flag)
	{
		GetHeaderPointer()->a = flag;
	}

	void SetUnusedField()
	{
		GetHeaderPointer()->unused = 0u;
	}

	void SetLengthField(uint16_t length)
	{
		GetHeaderPointer()->length = uint16_t{ htons(length) };
	}

	const uint8_t* GetAppendixPointer() const
	{
		return GetBuffer() + HeaderLength;
	}

	const uint8_t* GetItemsPointer() const
	{
		auto* ptr = GetBuffer() + HeaderLength;

		if (HasAppendix())
		{
			ptr += AppendixLength;
		}

		return ptr;
	}

	const uint8_t* GetPaddingPointer() const
	{
		return GetBuffer() + GetLengthField();
	}

	/**
	 * Must be used within Parse() static method (instead than AddItem()).
	 * This method doesn't serializa the given Item into Foo's buffer since it's
	 * already serialized (obviously since we are parsing a buffer).
	 */
	void AddParsedItem(std::unique_ptr<FooItem> item)
	{
		this->items.push_back(std::move(item));
	}

private:
	// FooItem instances.
	std::vector<std::unique_ptr<FooItem>> items;
};

TEST_CASE("parse Foo packet", "[rtc][serializable]")
{
	// clang-format off
	uint8_t buffer[] =
	{
		// Type:1, A:1, Length:19
		0x01, 0b10000000, 0x00, 0x13,
		// Appendix: 0x00BC614E
		0x00, 0xBC, 0x61, 0x4E,
		// Item 1: Id:9, Flags:0b0101, Length:2, Value: 0x1234
		0b10010101, 0x02, 0x12, 0x34,
		// Item 2: Id:7, Flags:0b0011, Length:5, , Value: 0xA987654321.
		0b01110011, 0x05, 0xA9, 0x87,
		// 1 byte of padding.
		0x65, 0x43, 0x21, 0x00
	};
	// clang-format on

	auto foo = Foo::Parse(buffer, sizeof(buffer));

	REQUIRE(sizeof(buffer) == 20);
	REQUIRE(foo);
	REQUIRE(foo->GetBuffer() == buffer);
	REQUIRE(foo->GetBufferLength() == 20);
	REQUIRE(foo->GetLength() == 20);
	REQUIRE(Utils::Byte::IsPaddedTo4Bytes(foo->GetLength()) == true);
	REQUIRE(foo->GetType() == 1);
	REQUIRE(foo->HasAppendix() == true);
	REQUIRE(foo->GetAppendix() == 0x00BC614E);
	REQUIRE(foo->HasItems() == true);
	REQUIRE(foo->GetItemsCount() == 2);
	REQUIRE(helpers::areBuffersEqual(foo->GetBuffer(), foo->GetLength(), buffer, 20) == true);

	auto& item1 = foo->GetItem(0);

	REQUIRE(item1);
	REQUIRE(item1->GetBuffer() == buffer + 8);
	REQUIRE(item1->GetBufferLength() == 4);
	REQUIRE(item1->GetLength() == 4);
	REQUIRE(item1->GetId() == 9);
	REQUIRE(item1->GetFlags() == 0b0101);
	REQUIRE(item1->GetValueLength() == 2);
	REQUIRE(item1->GetValue()[0] == 0x12);
	REQUIRE(item1->GetValue()[1] == 0x34);
	REQUIRE(helpers::areBuffersEqual(item1->GetBuffer(), item1->GetLength(), buffer + 8, 4) == true);

	auto& item2 = foo->GetItem(1);

	REQUIRE(item2);
	REQUIRE(item2->GetBuffer() == buffer + 12);
	// Buffer length in item 2 must be 7 since that's the remaining space from
	// the first byte of item 2 until available packet length (padding excluded).
	REQUIRE(item2->GetBufferLength() == 7);
	REQUIRE(item2->GetLength() == 7);
	REQUIRE(item2->GetId() == 7);
	REQUIRE(item2->GetFlags() == 0b0011);
	REQUIRE(item2->GetValueLength() == 5);
	REQUIRE(item2->GetValue()[0] == 0xA9);
	REQUIRE(item2->GetValue()[1] == 0x87);
	REQUIRE(item2->GetValue()[2] == 0x65);
	REQUIRE(item2->GetValue()[3] == 0x43);
	REQUIRE(item2->GetValue()[4] == 0x21);
	REQUIRE(helpers::areBuffersEqual(item2->GetBuffer(), item2->GetLength(), buffer + 12, 7) == true);

	REQUIRE(!foo->GetItem(2));
}

TEST_CASE("parse invalid Foo packet with buffer not padded to 4 bytes", "[rtc][serializable]")
{
	// clang-format off
	uint8_t buffer[] =
	{
		// Type:1, A:1, Length:19
		0x01, 0b10000000, 0x00, 0x13,
		// Appendix: 0x00BC614E
		0x00, 0xBC, 0x61, 0x4E,
		// Item 1: Id:9, Flags:0b0101, Length:2, Value: 0x1234
		0b10010101, 0x02, 0x12, 0x34,
		// Item 2: Id:7, Flags:0b0011, Length:5, , Value: 0xA987654321.
		0b01110011, 0x05, 0xA9, 0x87,
		// 1 byte of padding.
		0x65, 0x43, 0x21, 0x00,
		// Extra bytes that make the packet invalid.
		0xFF, 0xFF, 0xFF
	};
	// clang-format on

	auto foo = Foo::Parse(buffer, sizeof(buffer));

	REQUIRE(sizeof(buffer) == 23);
	REQUIRE(!foo);
}

TEST_CASE("create and modify Foo packet", "[rtc][serializable]")
{
	uint8_t buffer[256];
	uint8_t itemBuffer[17];

	std::memset(buffer, 0xFF, sizeof(buffer));
	std::memset(itemBuffer, 0xFF, sizeof(itemBuffer));

	auto foo = Foo::Factory(buffer, sizeof(buffer), /*type*/ 55);

	MS_DEBUG_DEV("***** foo, initial");
	foo->Dump();

	REQUIRE(sizeof(buffer) == 256);
	REQUIRE(foo);
	REQUIRE(foo->GetBuffer() == buffer);
	REQUIRE(foo->GetBufferLength() == 256);
	// Just the Foo header (4 bytes).
	REQUIRE(foo->GetLength() == 4);
	REQUIRE(Utils::Byte::IsPaddedTo4Bytes(foo->GetLength()) == true);
	REQUIRE(foo->GetType() == 55);
	REQUIRE(foo->HasAppendix() == false);
	REQUIRE(foo->GetAppendix() == 0u);
	REQUIRE(foo->HasItems() == false);
	REQUIRE(foo->GetItemsCount() == 0);
	REQUIRE(helpers::areBuffersEqual(foo->GetBuffer(), foo->GetLength(), buffer, 4) == true);

	/* Add Type and Appendix. */

	foo->SetType(125);
	foo->SetAppendix(0x12345678);

	MS_DEBUG_DEV("***** foo, set type and add appendix");
	foo->Dump();

	REQUIRE(foo->GetBuffer() == buffer);
	REQUIRE(foo->GetBufferLength() == 256);
	// Header (4 bytes) + Appendix (4 bytes).
	REQUIRE(foo->GetLength() == 8);
	REQUIRE(Utils::Byte::IsPaddedTo4Bytes(foo->GetLength()) == true);
	REQUIRE(foo->GetType() == 125);
	REQUIRE(foo->HasAppendix() == true);
	REQUIRE(foo->GetAppendix() == 0x12345678);
	REQUIRE(foo->HasItems() == false);
	REQUIRE(foo->GetItemsCount() == 0);
	REQUIRE(helpers::areBuffersEqual(foo->GetBuffer(), foo->GetLength(), buffer, 8) == true);

	/* Remove Appendix. */

	foo->SetAppendix(0u);

	MS_DEBUG_DEV("***** foo, remove appendix");
	foo->Dump();

	REQUIRE(foo->GetBuffer() == buffer);
	REQUIRE(foo->GetBufferLength() == 256);
	// Header (4 bytes).
	REQUIRE(foo->GetLength() == 4);
	REQUIRE(Utils::Byte::IsPaddedTo4Bytes(foo->GetLength()) == true);
	REQUIRE(foo->GetType() == 125);
	REQUIRE(foo->HasAppendix() == false);
	REQUIRE(foo->GetAppendix() == 0u);
	REQUIRE(foo->HasItems() == false);
	REQUIRE(foo->GetItemsCount() == 0);
	REQUIRE(helpers::areBuffersEqual(foo->GetBuffer(), foo->GetLength(), buffer, 4) == true);

	/* Add an Item. */

	uint8_t item1Value[] = { 0xAA, 0xBB, 0xCC };
	uint8_t item2Value[] = { 0xAB, 0xCD };

	// Item 1 (5 bytes).
	auto item1 = FooItem::Factory(
	  itemBuffer, sizeof(itemBuffer), /*id*/ 1, /*flags*/ 0b1000, item1Value, sizeof(item1Value));

	// Hold the item1 pointer for tests below.
	auto* item1Ptr = item1.get();

	foo->AddItem(std::move(item1));

	// Item 2 (4 bytes).
	auto item2 = FooItem::Factory(
	  itemBuffer, sizeof(itemBuffer), /*id*/ 2, /*flags*/ 0b1001, item2Value, sizeof(item2Value));

	// Hold the item2 pointer for tests below.
	auto* item2Ptr = item2.get();

	foo->AddItem(std::move(item2));

	MS_DEBUG_DEV("***** foo, add items");
	foo->Dump();

	auto foo2 = Foo::Parse(foo->GetBuffer(), foo->GetLength());
	MS_DEBUG_DEV("*****  foo2, parsed from foo");
	foo2->Dump();

	REQUIRE(foo->GetBuffer() == buffer);
	REQUIRE(foo->GetBufferLength() == 256);
	// Header (4 bytes) + items (9 bytes) + padding (3 bytes).
	REQUIRE(foo->GetLength() == 16);
	REQUIRE(Utils::Byte::IsPaddedTo4Bytes(foo->GetLength()) == true);
	REQUIRE(foo->GetType() == 125);
	REQUIRE(foo->HasAppendix() == false);
	REQUIRE(foo->GetAppendix() == 0u);
	REQUIRE(foo->HasItems() == true);
	REQUIRE(foo->GetItemsCount() == 2);
	REQUIRE(helpers::areBuffersEqual(foo->GetBuffer(), foo->GetLength(), buffer, 16) == true);

	REQUIRE(item1Ptr == foo->GetItem(0).get());
	// We know this will be same as item length.
	REQUIRE(item1Ptr->GetBufferLength() == 5);
	REQUIRE(item1Ptr->GetLength() == 5);
	REQUIRE(item1Ptr->GetId() == 1);
	REQUIRE(item1Ptr->GetFlags() == 0b1000);
	REQUIRE(item1Ptr->GetValueLength() == 3);
	REQUIRE(item1Ptr->GetValue()[0] == 0xAA);
	REQUIRE(item1Ptr->GetValue()[1] == 0xBB);
	REQUIRE(item1Ptr->GetValue()[2] == 0xCC);
	REQUIRE(
	  helpers::areBuffersEqual(item1Ptr->GetBuffer(), item1Ptr->GetLength(), buffer + 4, 5) == true);

	REQUIRE(item2Ptr == foo->GetItem(1).get());
	// We know this will be same as item length.
	REQUIRE(item2Ptr->GetBufferLength() == 4);
	REQUIRE(item2Ptr->GetLength() == 4);
	REQUIRE(item2Ptr->GetId() == 2);
	REQUIRE(item2Ptr->GetFlags() == 0b1001);
	REQUIRE(item2Ptr->GetValueLength() == 2);
	REQUIRE(item2Ptr->GetValue()[0] == 0xAB);
	REQUIRE(item2Ptr->GetValue()[1] == 0xCD);
	REQUIRE(
	  helpers::areBuffersEqual(item2Ptr->GetBuffer(), item2Ptr->GetLength(), buffer + 4 + 5, 4) == true);

	REQUIRE(!foo->GetItem(2));

	/* Add Appendix. */

	foo->SetAppendix(666u);

	MS_DEBUG_DEV("***** foo, keep items and add appendix");
	foo->Dump();

	REQUIRE(foo->GetBuffer() == buffer);
	REQUIRE(foo->GetBufferLength() == 256);
	// Header (4 bytes) + Appendix (4 bytes) + items (9 bytes) + padding (3
	// bytes);
	REQUIRE(foo->GetLength() == 20);
	REQUIRE(Utils::Byte::IsPaddedTo4Bytes(foo->GetLength()) == true);
	REQUIRE(foo->GetType() == 125);
	REQUIRE(foo->HasAppendix() == true);
	REQUIRE(foo->GetAppendix() == 666u);
	REQUIRE(foo->HasItems() == true);
	REQUIRE(foo->GetItemsCount() == 2);
	REQUIRE(helpers::areBuffersEqual(foo->GetBuffer(), foo->GetLength(), buffer, 20) == true);

	REQUIRE(item1Ptr == foo->GetItem(0).get());
	// We know this will be same as item length.
	REQUIRE(item1Ptr->GetBufferLength() == 5);
	REQUIRE(item1Ptr->GetLength() == 5);
	REQUIRE(item1Ptr->GetId() == 1);
	REQUIRE(item1Ptr->GetFlags() == 0b1000);
	REQUIRE(item1Ptr->GetValueLength() == 3);
	REQUIRE(item1Ptr->GetValue()[0] == 0xAA);
	REQUIRE(item1Ptr->GetValue()[1] == 0xBB);
	REQUIRE(item1Ptr->GetValue()[2] == 0xCC);
	REQUIRE(
	  helpers::areBuffersEqual(item1Ptr->GetBuffer(), item1Ptr->GetLength(), buffer + 4 + 4, 5) == true);

	REQUIRE(item2Ptr == foo->GetItem(1).get());
	// We know this will be same as item length.
	REQUIRE(item2Ptr->GetBufferLength() == 4);
	REQUIRE(item2Ptr->GetLength() == 4);
	REQUIRE(item2Ptr->GetId() == 2);
	REQUIRE(item2Ptr->GetFlags() == 0b1001);
	REQUIRE(item2Ptr->GetValueLength() == 2);
	REQUIRE(item2Ptr->GetValue()[0] == 0xAB);
	REQUIRE(item2Ptr->GetValue()[1] == 0xCD);
	REQUIRE(
	  helpers::areBuffersEqual(item2Ptr->GetBuffer(), item2Ptr->GetLength(), buffer + 4 + 4 + 5, 4) ==
	  true);

	REQUIRE(!foo->GetItem(2));

	/* Remove Appendix and change flags of Item 2. */

	foo->SetAppendix(0u);
	foo->GetItem(1)->SetFlags(0b1111);

	MS_DEBUG_DEV("***** foo, keep items and remove appendix");
	foo->Dump();

	REQUIRE(foo->GetBuffer() == buffer);
	REQUIRE(foo->GetBufferLength() == 256);
	// Header (4 bytes) + items (9 bytes) + padding (3 bytes);
	REQUIRE(foo->GetLength() == 16);
	REQUIRE(Utils::Byte::IsPaddedTo4Bytes(foo->GetLength()) == true);
	REQUIRE(foo->GetType() == 125);
	REQUIRE(foo->HasAppendix() == false);
	REQUIRE(foo->GetAppendix() == 0u);
	REQUIRE(foo->HasItems() == true);
	REQUIRE(foo->GetItemsCount() == 2);
	REQUIRE(helpers::areBuffersEqual(foo->GetBuffer(), foo->GetLength(), buffer, 16) == true);

	REQUIRE(item1Ptr == foo->GetItem(0).get());
	// We know this will be same as item length.
	REQUIRE(item1Ptr->GetBufferLength() == 5);
	REQUIRE(item1Ptr->GetLength() == 5);
	REQUIRE(item1Ptr->GetId() == 1);
	REQUIRE(item1Ptr->GetFlags() == 0b1000);
	REQUIRE(item1Ptr->GetValueLength() == 3);
	REQUIRE(item1Ptr->GetValue()[0] == 0xAA);
	REQUIRE(item1Ptr->GetValue()[1] == 0xBB);
	REQUIRE(item1Ptr->GetValue()[2] == 0xCC);
	REQUIRE(
	  helpers::areBuffersEqual(item1Ptr->GetBuffer(), item1Ptr->GetLength(), buffer + 4, 5) == true);

	REQUIRE(item2Ptr == foo->GetItem(1).get());
	// We know this will be same as item length.
	REQUIRE(item2Ptr->GetBufferLength() == 4);
	REQUIRE(item2Ptr->GetLength() == 4);
	REQUIRE(item2Ptr->GetId() == 2);
	REQUIRE(item2Ptr->GetFlags() == 0b1111);
	REQUIRE(item2Ptr->GetValueLength() == 2);
	REQUIRE(item2Ptr->GetValue()[0] == 0xAB);
	REQUIRE(item2Ptr->GetValue()[1] == 0xCD);
	REQUIRE(
	  helpers::areBuffersEqual(item2Ptr->GetBuffer(), item2Ptr->GetLength(), buffer + 4 + 5, 4) == true);

	REQUIRE(!foo->GetItem(2));

	/* Serialize Foo packet into another buffer. */

	uint8_t newBuffer[256];

	std::memset(newBuffer, 0xFF, sizeof(newBuffer));

	foo->Serialize(newBuffer, sizeof(newBuffer));

	MS_DEBUG_DEV("***** serialize foo");
	foo->Dump();

	MS_DUMP_DATA(buffer, 16);
	MS_DUMP_DATA(newBuffer, 16);

	// Compare new and old buffers.
	REQUIRE(helpers::areBuffersEqual(newBuffer, 16, buffer, 16) == true);

	// Once done fill the old buffer with 1s.
	std::memset(buffer, 0xFF, sizeof(buffer));

	REQUIRE(foo->GetBuffer() == newBuffer);
	REQUIRE(foo->GetBufferLength() == 256);
	// Header (4 bytes) + items (9 bytes) + padding (3 bytes);
	REQUIRE(foo->GetLength() == 16);
	REQUIRE(Utils::Byte::IsPaddedTo4Bytes(foo->GetLength()) == true);
	REQUIRE(foo->GetType() == 125);
	REQUIRE(foo->HasAppendix() == false);
	REQUIRE(foo->GetAppendix() == 0u);
	REQUIRE(foo->HasItems() == true);
	REQUIRE(foo->GetItemsCount() == 2);
	REQUIRE(helpers::areBuffersEqual(foo->GetBuffer(), foo->GetLength(), newBuffer, 16) == true);

	REQUIRE(item1Ptr == foo->GetItem(0).get());
	// We know this will be same as item length.
	REQUIRE(item1Ptr->GetBufferLength() == 5);
	REQUIRE(item1Ptr->GetLength() == 5);
	REQUIRE(item1Ptr->GetId() == 1);
	REQUIRE(item1Ptr->GetFlags() == 0b1000);
	REQUIRE(item1Ptr->GetValueLength() == 3);
	REQUIRE(item1Ptr->GetValue()[0] == 0xAA);
	REQUIRE(item1Ptr->GetValue()[1] == 0xBB);
	REQUIRE(item1Ptr->GetValue()[2] == 0xCC);
	REQUIRE(
	  helpers::areBuffersEqual(item1Ptr->GetBuffer(), item1Ptr->GetLength(), newBuffer + 4, 5) == true);

	REQUIRE(item2Ptr == foo->GetItem(1).get());
	// We know this will be same as item length.
	REQUIRE(item2Ptr->GetBufferLength() == 4);
	REQUIRE(item2Ptr->GetLength() == 4);
	REQUIRE(item2Ptr->GetId() == 2);
	REQUIRE(item2Ptr->GetFlags() == 0b1111);
	REQUIRE(item2Ptr->GetValueLength() == 2);
	REQUIRE(item2Ptr->GetValue()[0] == 0xAB);
	REQUIRE(item2Ptr->GetValue()[1] == 0xCD);
	REQUIRE(
	  helpers::areBuffersEqual(item2Ptr->GetBuffer(), item2Ptr->GetLength(), newBuffer + 4 + 5, 4) ==
	  true);

	REQUIRE(!foo->GetItem(2));
}

TEST_CASE("parse FooItem item", "[rtc][serializable]")
{
	// clang-format off
	uint8_t buffer[] =
	{
		// Item 1: Id:10, Flags:0b1111, Value Length:1, Value: 0xEE
		0b10101111, 0x01, 0xEE
	};
	// clang-format on

	auto item = FooItem::Parse(buffer, sizeof(buffer));

	REQUIRE(sizeof(buffer) == 3);
	REQUIRE(item);
	REQUIRE(item->GetBuffer() == buffer);
	REQUIRE(item->GetBufferLength() == 3);
	REQUIRE(item->GetLength() == 3);
	REQUIRE(item->GetId() == 10);
	REQUIRE(item->GetFlags() == 0b1111);
	REQUIRE(item->GetValueLength() == 1);
	REQUIRE(item->GetValue()[0] == 0xEE);
	REQUIRE(helpers::areBuffersEqual(item->GetBuffer(), item->GetLength(), buffer, 3) == true);
}

TEST_CASE(
  "parse FooItem by passing to it a buffer larger than the length of the item", "[rtc][serializable]")
{
	// Item length is 7 but given buffer is 8 bytes. Not a problem.
	//
	// clang-format off
	uint8_t buffer[] =
	{
		// Item 1: Id:1, Flags:0b0000, Value Length:5, Value: 0xFFFFFFFFFF
		0b00010000, 0x05, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0x00
	};
	// clang-format on

	auto item = FooItem::Parse(buffer, sizeof(buffer));

	REQUIRE(sizeof(buffer) == 8);
	REQUIRE(item);
	REQUIRE(item->GetBuffer() == buffer);
	REQUIRE(item->GetBufferLength() == 8);
	REQUIRE(item->GetLength() == 7);
	REQUIRE(item->GetId() == 1);
	REQUIRE(item->GetFlags() == 0b0000);
	REQUIRE(item->GetValueLength() == 5);
	REQUIRE(item->GetValue()[0] == 0xFF);
	REQUIRE(item->GetValue()[1] == 0xFF);
	REQUIRE(item->GetValue()[2] == 0xFF);
	REQUIRE(item->GetValue()[3] == 0xFF);
	REQUIRE(item->GetValue()[4] == 0xFF);
	REQUIRE(helpers::areBuffersEqual(item->GetBuffer(), item->GetLength(), buffer, 7) == true);
}

TEST_CASE("parse invalid FooItem item", "[rtc][serializable]")
{
	SECTION("buffer too small")
	{
		// Item length should be 7 but given buffer is only 6 bytes.

		// clang-format off
		uint8_t buffer[] =
		{
			// Item 1: Id:1, Flags:0b0000, Value Length:5
			0b00010000, 0x05, 0xFF, 0xFF,
			0xFF, 0xFF
		};
		// clang-format on

		auto item = FooItem::Parse(buffer, sizeof(buffer));

		REQUIRE(sizeof(buffer) == 6);
		REQUIRE(!item);
	}
}

TEST_CASE("create and modify FooItem item", "[rtc][serializable]")
{
	// Max length of a FooItem is 17 bytes.
	uint8_t buffer[17];
	uint8_t value[] = { 0x11, 0x22, 0x33 };

	// Let's fill the buffer with whatever (it should be overriden by
	// FooItem:Factory()).
	std::memset(buffer, 0xFF, sizeof(buffer));

	auto item =
	  FooItem::Factory(buffer, sizeof(buffer), /*id*/ 9, /*flags*/ 0b1010, value, sizeof(value));

	REQUIRE(sizeof(buffer) == 17);
	REQUIRE(sizeof(value) == 3);
	REQUIRE(item);
	REQUIRE(item->GetBuffer() == buffer);
	REQUIRE(item->GetBufferLength() == 17);
	REQUIRE(item->GetLength() == 5);
	REQUIRE(item->GetId() == 9);
	REQUIRE(item->GetFlags() == 0b1010);
	REQUIRE(item->GetValueLength() == 3);
	REQUIRE(item->GetValue()[0] == 0x11);
	REQUIRE(item->GetValue()[1] == 0x22);
	REQUIRE(item->GetValue()[2] == 0x33);
	REQUIRE(helpers::areBuffersEqual(item->GetBuffer(), item->GetLength(), buffer, 5) == true);
	REQUIRE(helpers::areBuffersEqual(item->GetValue(), item->GetValueLength(), value, 3) == true);

	/* Modify Item. */

	uint8_t newValue[] = { 0xFF, 0xEE, 0xDD, 0xCC };

	item->SetId(14);
	item->SetFlags(0b1111);
	item->SetValue(newValue, sizeof(newValue));

	REQUIRE(sizeof(newValue) == 4);
	REQUIRE(item->GetBuffer() == buffer);
	REQUIRE(item->GetBufferLength() == 17);
	REQUIRE(item->GetLength() == 6);
	REQUIRE(item->GetId() == 14);
	REQUIRE(item->GetFlags() == 0b1111);
	REQUIRE(item->GetValueLength() == 4);
	REQUIRE(item->GetValue()[0] == 0xFF);
	REQUIRE(item->GetValue()[1] == 0xEE);
	REQUIRE(item->GetValue()[2] == 0xDD);
	REQUIRE(item->GetValue()[3] == 0xCC);
	REQUIRE(helpers::areBuffersEqual(item->GetBuffer(), item->GetLength(), buffer, 6) == true);
	REQUIRE(helpers::areBuffersEqual(item->GetValue(), item->GetValueLength(), newValue, 4) == true);

	/* Serialize Item into another buffer. */

	uint8_t newBuffer1[17];

	std::memset(newBuffer1, 0xFF, sizeof(newBuffer1));

	item->Serialize(newBuffer1, sizeof(newBuffer1));

	// Compare new and old buffers.
	REQUIRE(helpers::areBuffersEqual(item->GetBuffer(), item->GetLength(), buffer, 6));

	// Once done fill the old buffer with 1s.
	std::memset(buffer, 0xFF, sizeof(buffer));

	REQUIRE(item->GetBuffer() == newBuffer1);
	REQUIRE(item->GetBufferLength() == 17);
	REQUIRE(item->GetLength() == 6);
	REQUIRE(item->GetId() == 14);
	REQUIRE(item->GetFlags() == 0b1111);
	REQUIRE(item->GetValueLength() == 4);
	REQUIRE(item->GetValue()[0] == 0xFF);
	REQUIRE(item->GetValue()[1] == 0xEE);
	REQUIRE(item->GetValue()[2] == 0xDD);
	REQUIRE(item->GetValue()[3] == 0xCC);
	REQUIRE(helpers::areBuffersEqual(item->GetBuffer(), item->GetLength(), newBuffer1, 6) == true);
	REQUIRE(helpers::areBuffersEqual(item->GetValue(), item->GetValueLength(), newValue, 4) == true);

	/* Clone Item into another buffer. */

	uint8_t newBuffer2[100];

	std::memset(newBuffer2, 0xFF, sizeof(newBuffer2));

	auto* previousBuffer      = item->GetBuffer();
	auto previousBufferLength = item->GetBufferLength();

	std::unique_ptr<Serializable> genericClonedItem = item->Clone(newBuffer2, sizeof(newBuffer2));
	std::unique_ptr<FooItem> clonedItem =
	  std::unique_ptr<FooItem>(static_cast<FooItem*>(genericClonedItem.release()));

	// Compare the buffers of the original item and the cloned one.
	REQUIRE(
	  helpers::areBuffersEqual(
	    clonedItem->GetBuffer(), clonedItem->GetLength(), newBuffer1, item->GetLength()) == true);

	// Once done fill the original buffer with 1s (this is, we are running original
	// Item despite it still exists since we have jsut cloned it).
	std::memset(const_cast<uint8_t*>(previousBuffer), 0xFF, previousBufferLength);

	REQUIRE(clonedItem->GetBuffer() == newBuffer2);
	REQUIRE(clonedItem->GetBufferLength() == 100);
	REQUIRE(clonedItem->GetLength() == 6);
	REQUIRE(clonedItem->GetId() == 14);
	REQUIRE(clonedItem->GetFlags() == 0b1111);
	REQUIRE(clonedItem->GetValueLength() == 4);
	REQUIRE(clonedItem->GetValue()[0] == 0xFF);
	REQUIRE(clonedItem->GetValue()[1] == 0xEE);
	REQUIRE(clonedItem->GetValue()[2] == 0xDD);
	REQUIRE(clonedItem->GetValue()[3] == 0xCC);
	REQUIRE(
	  helpers::areBuffersEqual(clonedItem->GetBuffer(), clonedItem->GetLength(), newBuffer2, 6) == true);
	REQUIRE(
	  helpers::areBuffersEqual(clonedItem->GetValue(), clonedItem->GetValueLength(), newValue, 4) ==
	  true);
}
