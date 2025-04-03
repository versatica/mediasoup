#define MS_CLASS "RTC::TestSerializable::FooTextItem"
#define MS_LOG_DEV_LEVEL 3

#include "RTC/TestSerializable/FooTextItem.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include <cstring> // std::memcpy()

using namespace RTC;

std::unique_ptr<FooTextItem> FooTextItem::Parse(const uint8_t* buffer, size_t bufferLength)
{
	MS_TRACE();

	FooItem::ItemId itemId;
	uint8_t valueLength;

	if (!FooItem::IsFooItem(buffer, bufferLength, itemId, valueLength))
	{
		MS_WARN_DEV("not a FooItem");

		return nullptr;
	}

	if (itemId != FooItem::ItemId::TEXT)
	{
		MS_WARN_DEV("invalid item id");

		return nullptr;
	}

	auto item = std::unique_ptr<FooTextItem>(new FooTextItem(buffer, bufferLength));

	// Must always invoke SetLength() after constructing a Serializable.
	item->SetLength(FooItem::ItemHeaderLength + valueLength);

	return item;
}

std::unique_ptr<FooTextItem> FooTextItem::Factory(
  uint8_t* buffer, size_t bufferLength, uint8_t flags, const std::string& text)
{
	MS_TRACE();

	// DooFataItem has fixed length.
	if (bufferLength < FooItem::ItemHeaderLength + text.size())
	{
		MS_THROW_TYPE_ERROR("too small buffer");
	}

	auto item = std::unique_ptr<FooTextItem>(new FooTextItem(buffer, bufferLength));

	item->InitializeHeader(FooItem::ItemId::TEXT, flags, text.size());
	item->SetText(text);

	// Must always invoke SetLength() after constructing a Serializable.
	item->SetLength(FooItem::ItemHeaderLength + text.size());

	return item;
}

FooTextItem::FooTextItem(const uint8_t* buffer, size_t bufferLength) : FooItem(buffer, bufferLength)
{
	MS_TRACE();
}

FooTextItem::~FooTextItem()
{
	MS_TRACE();
}

void FooTextItem::Dump() const
{
	MS_TRACE();

	MS_DUMP("<FooTextItem>");
	MS_DUMP("  length: %zu (buffer length: %zu)", GetLength(), GetBufferLength());
	MS_DUMP("  id: %" PRIu8 " (%s)", GetId(), FooItem::ItemId2String(GetId()).c_str());
	MS_DUMP("  flags: " MS_UINT8_4BITS_TO_BINARY_PATTERN, MS_UINT8_4BITS_TO_BINARY(GetFlags()));
	MS_DUMP(
	  "  value length field: %" PRIu8 " (computed value length: %" PRIu8 ")",
	  GetValueLengthField(),
	  GetValueLength());
	auto& text = GetText();
	MS_DUMP("  text: %.*s", static_cast<int>(text.size()), text.data());
	MS_DUMP("</FooTextItem>");
}

const std::string_view FooTextItem::GetText() const
{
	MS_TRACE();

	return std::string_view(reinterpret_cast<const char*>(GetValuePointer()), GetValueLength());
}

void FooTextItem::SetText(const std::string& text)
{
	MS_TRACE();

	auto previousValueLength = GetValueLength();

	// Let's call SetLength() on parent with the new computed item length.
	// NOTE: If there is no space in the buffer for it, it will throw.
	SetLength(GetLength() - previousValueLength + text.size());

	// Copy the given text into the buffer.
	std::memcpy(GetValuePointer(), text.data(), text.size());

	// Update the Value Length field.
	SetValueLengthField(text.size());
}
