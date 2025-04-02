#define MS_CLASS "RTC::TestSerializable::FooItem"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/TestSerializable/FooItem.hpp"
#include "common.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "Utils.hpp"
#include "helpers.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memcpy()
#include <vector>

using namespace RTC;

/* Class variables. */

// clang-format off
std::unordered_map<FooItem::ItemId, std::string> FooItem::itemId2String =
{
	{ FooItem::ItemId::NONE,    "NONE"    },
	{ FooItem::ItemId::DATA,    "DATA"    },
	{ FooItem::ItemId::EVENT,   "EVENT"   },
	{ FooItem::ItemId::CONTROL, "CONTROL" }
};
// clang-format on

std::unique_ptr<FooItem> FooItem::Parse(const uint8_t* buffer, size_t bufferLength)
{
	MS_TRACE();

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
	auto item = std::unique_ptr<FooItem>(new FooItem(buffer, bufferLength, /*initializeHeader*/ false));

	// Move to the value.
	if (item->HasValue())
	{
		ptr = item->GetValuePointer();

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

	// It's mandatory to call SetLength() once we are done and we know the
	// exact length of the item.
	item->SetLength(computedLength);

	return item;
}

std::unique_ptr<FooItem> FooItem::Factory(
  const uint8_t* buffer,
  size_t bufferLength,
  ItemId id,
  uint8_t flags,
  const uint8_t* value,
  uint8_t valueLength)
{
	MS_TRACE();

	const size_t computedLength = ItemHeaderLength + valueLength;

	// No space for header.
	if (bufferLength < computedLength)
	{
		MS_THROW_TYPE_ERROR("no space for Item Header");
	}

	// We want to initialize the header since we are creating an item from
	// scratch.
	auto item = std::unique_ptr<FooItem>(new FooItem(buffer, bufferLength, /*initializeHeader*/ true));

	item->SetId(id);
	item->SetFlags(flags);
	item->SetValue(value, valueLength);

	// NOTE: No need to call item->SetLength() since item->SetValue() already
	// does it.

	return item;
}

const std::string& FooItem::ItemId2String(ItemId id)
{
	MS_TRACE();

	static const std::string Unknown("UNKNOWN");

	auto it = FooItem::itemId2String.find(id);

	if (it == FooItem::itemId2String.end())
	{
		return Unknown;
	}

	return it->second;
}

FooItem::FooItem(const uint8_t* buffer, size_t bufferLength, bool initializeHeader)
  : Serializable(buffer, bufferLength)
{
	MS_TRACE();

	if (initializeHeader)
	{
		SetId(ItemId::NONE);
		SetFlags(0u);
		SetValueLengthField(0u);

		// Update Serializable length.
		SetLength(ItemHeaderLength);
	}
}

FooItem::~FooItem()
{
	MS_TRACE();
}

void FooItem::Dump() const
{
	MS_TRACE();

	MS_DUMP("<FooItem>");
	MS_DUMP("  length: %zu (buffer length: %zu)", GetLength(), GetBufferLength());
	MS_DUMP("  id: %" PRIu8 " (%s)", GetId(), FooItem::ItemId2String(GetId()).c_str());
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

std::unique_ptr<Serializable> FooItem::Clone(const uint8_t* buffer, size_t bufferLength) const
{
	MS_TRACE();

	std::memcpy(const_cast<uint8_t*>(buffer), GetBuffer(), GetLength());

	auto clonedFooItem =
	  std::unique_ptr<FooItem>(new FooItem(buffer, bufferLength, /*initializeHeader*/ false));

	// Need to manually set Serializable length.
	clonedFooItem->SetLength(GetLength());

	return clonedFooItem;
}

void FooItem::SetValue(const uint8_t* value, uint8_t valueLength)
{
	MS_TRACE();

	auto previousValueLength = GetValueLength();

	// Update the Value Length field.
	SetValueLengthField(valueLength);

	// Copy the given value into the buffer.
	std::memcpy(const_cast<uint8_t*>(GetValuePointer()), value, valueLength);

	// Update Serializable length.
	SetLength(GetLength() - previousValueLength + valueLength);
}
