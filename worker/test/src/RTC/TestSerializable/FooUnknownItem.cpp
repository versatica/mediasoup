#define MS_CLASS "RTC::TestSerializable::FooUnknownItem"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/TestSerializable/FooUnknownItem.hpp"
#include "Logger.hpp"

using namespace RTC;

FooUnknownItem* FooUnknownItem::Parse(const uint8_t* buffer, size_t bufferLength)
{
	MS_TRACE();

	FooItem::ItemId itemId;
	uint8_t valueLength;

	if (!FooItem::IsFooItem(buffer, bufferLength, itemId, valueLength))
	{
		MS_WARN_DEV("not a FooItem");

		return nullptr;
	}

	auto* item = new FooUnknownItem(buffer, bufferLength);

	// Must always invoke SetLength() after constructing a Serializable.
	item->SetLength(FooItem::ItemHeaderLength + valueLength);

	// Mark the item as frozen since we are parsing.
	item->Freeze();

	return item;
}

FooUnknownItem::FooUnknownItem(const uint8_t* buffer, size_t bufferLength)
  : FooItem(buffer, bufferLength)
{
	MS_TRACE();
}

FooUnknownItem::~FooUnknownItem()
{
	MS_TRACE();
}

void FooUnknownItem::Dump() const
{
	MS_TRACE();

	MS_DUMP("<FooUnknownItem>");
	MS_DUMP("  length: %zu (buffer length: %zu)", GetLength(), GetBufferLength());
	MS_DUMP("  id: %" PRIu8 " (%s)", GetId(), FooItem::ItemId2String(GetId()).c_str());
	MS_DUMP("  flags: " MS_UINT8_4BITS_TO_BINARY_PATTERN, MS_UINT8_4BITS_TO_BINARY(GetFlags()));
	MS_DUMP(
	  "  value length field: %" PRIu8 " (computed value length: %" PRIu8 ")",
	  GetValueLengthField(),
	  GetValueLength());
	MS_DUMP("</FooUnknownItem>");
}

FooUnknownItem* FooUnknownItem::Clone(uint8_t* buffer, size_t bufferLength) const
{
	MS_TRACE();

	return static_cast<FooUnknownItem*>(FooItem::Clone(buffer, bufferLength));
}

const uint8_t* FooUnknownItem::GetValue() const
{
	MS_TRACE();

	if (!HasValue())
	{
		return nullptr;
	}

	return GetValuePointer();
}
