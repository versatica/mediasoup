#define MS_CLASS "RTC::TestSerializable::FooDataItem"
#define MS_LOG_DEV_LEVEL 3

#include "RTC/TestSerializable/FooDataItem.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"

using namespace RTC;

std::unique_ptr<FooDataItem> FooDataItem::Parse(const uint8_t* buffer, size_t bufferLength)
{
	MS_TRACE();

	FooItem::ItemId itemId;
	uint8_t valueLength;

	if (!FooItem::IsFooItem(buffer, bufferLength, itemId, valueLength))
	{
		MS_WARN_DEV("not a FooItem");

		return nullptr;
	}

	if (itemId != FooItem::ItemId::DATA)
	{
		MS_WARN_DEV("invalid item id");

		return nullptr;
	}

	if (valueLength != FooDataItem::NumberLength)
	{
		MS_WARN_DEV("invalid value length");

		return nullptr;
	}

	// NOTE: We are parsing so we don't want to initialize the header.
	return std::unique_ptr<FooDataItem>(
	  new FooDataItem(buffer, bufferLength, /*initializeHeader*/ false));
}

std::unique_ptr<FooDataItem> FooDataItem::Factory(
  const uint8_t* buffer, size_t bufferLength, uint8_t flags, uint16_t number)
{
	MS_TRACE();

	// DooFataItem has fixed length.
	if (bufferLength < FooDataItem::Length)
	{
		MS_THROW_TYPE_ERROR("too small buffer");
	}

	// We want to initialize the header since we are creating an item from
	// scratch.
	auto item =
	  std::unique_ptr<FooDataItem>(new FooDataItem(buffer, bufferLength, /*initializeHeader*/ true));

	item->SetFlags(flags);
	item->SetNumber(number);

	return item;
}

FooDataItem::FooDataItem(const uint8_t* buffer, size_t bufferLength, bool initializeHeader)
  : FooItem(buffer, bufferLength, initializeHeader)
{
	MS_TRACE();

	if (initializeHeader)
	{
		SetId(FooItem::ItemId::DATA);

		// FooDataItem value length is fixed.
		SetValueLengthField(NumberLength);
	}

	// FooDataItem length is fixed.
	SetLength(FooDataItem::Length);
}

FooDataItem::~FooDataItem()
{
	MS_TRACE();
}

void FooDataItem::Dump() const
{
	MS_TRACE();

	MS_DUMP("<FooDataItem>");
	MS_DUMP("  length: %zu (buffer length: %zu)", GetLength(), GetBufferLength());
	MS_DUMP("  id: %" PRIu8 " (%s)", GetId(), FooItem::ItemId2String(GetId()).c_str());
	MS_DUMP("  flags: " MS_UINT8_4BITS_TO_BINARY_PATTERN, MS_UINT8_4BITS_TO_BINARY(GetFlags()));
	MS_DUMP(
	  "  value length field: %" PRIu8 " (computed value length: %" PRIu8 ")",
	  GetValueLengthField(),
	  GetValueLength());
	MS_DUMP("  number: %" PRIu16, GetNumber());
	MS_DUMP("</FooDataItem>");
}
