#define MS_CLASS "RTC::TestSerializable::FooItem"
#define MS_LOG_DEV_LEVEL 3

#include "RTC/TestSerializable/FooItem.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include <cstring> // std::memcpy()

using namespace RTC;

/* Class variables. */

// clang-format off
std::unordered_map<FooItem::ItemId, std::string> FooItem::itemId2String =
{
	{ FooItem::ItemId::NUMERIC, "NUMERIC" },
	{ FooItem::ItemId::TEXT,    "TEXT"    }
};
// clang-format on

bool FooItem::IsFooItem(const uint8_t* buffer, size_t bufferLength, ItemId& itemId, uint8_t& valueLength)
{
	MS_TRACE();

	MS_DUMP("*** bufferLength:%zu", bufferLength);
	;

	if (bufferLength < FooItem::ItemHeaderLength)
	{
		MS_WARN_DEV("no space for FooItem header");

		return false;
	}

	const auto* itemHeader = reinterpret_cast<const FooItem::ItemHeader*>(buffer);

	MS_DUMP("*** id:%" PRIu8 ", valueLength:%" PRIu8, itemHeader->id, itemHeader->valueLength);

	if (bufferLength < FooItem::ItemHeaderLength + itemHeader->valueLength)
	{
		MS_WARN_DEV("no space for FooItem value");

		return false;
	}

	MS_DUMP(
	  "*** GOOD !!! => id:%" PRIu8 ", valueLength:%" PRIu8, itemHeader->id, itemHeader->valueLength);

	itemId      = itemHeader->id;
	valueLength = itemHeader->valueLength;

	return true;
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

FooItem::FooItem(const uint8_t* buffer, size_t bufferLength) : Serializable(buffer, bufferLength)
{
	MS_TRACE();
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
	MS_DUMP_DATA(GetValuePointer(), GetValueLength());
	MS_DUMP("</FooItem>");
}

std::unique_ptr<Serializable> FooItem::Clone(uint8_t* buffer, size_t bufferLength) const
{
	MS_TRACE();

	if (bufferLength < GetLength())
	{
		MS_THROW_TYPE_ERROR(
		  "bufferLength (%zu bytes) is lower than current length (%zu bytes)", bufferLength, GetLength());
	}

	std::memcpy(buffer, GetBuffer(), GetLength());

	auto clonedFooItem = std::unique_ptr<FooItem>(new FooItem(buffer, bufferLength));

	// Need to manually set Serializable length.
	clonedFooItem->SetLength(GetLength());

	return clonedFooItem;
}
