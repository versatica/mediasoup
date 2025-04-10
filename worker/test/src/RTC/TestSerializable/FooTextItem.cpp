#define MS_CLASS "RTC::TestSerializable::FooTextItem"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/TestSerializable/FooTextItem.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "Utils.hpp"

namespace RTC
{
	/* Class methods. */

	FooTextItem* FooTextItem::Parse(const uint8_t* buffer, size_t bufferLength)
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

		auto* item = new FooTextItem(buffer, bufferLength);

		// Must always invoke SetLength() after constructing a Serializable.
		item->SetLength(FooItem::ItemHeaderLength + valueLength);

		// Mark the item as frozen since we are parsing.
		item->Freeze();

		return item;
	}

	FooTextItem* FooTextItem::Factory(
	  uint8_t* buffer, size_t bufferLength, uint8_t flags, const std::string& text)
	{
		MS_TRACE();

		// DooFataItem has fixed length.
		if (bufferLength < FooItem::ItemHeaderLength + text.size())
		{
			MS_THROW_TYPE_ERROR("too small buffer");
		}

		auto* item = new FooTextItem(buffer, bufferLength);

		item->InitializeHeader(FooItem::ItemId::TEXT, flags, 0u);
		item->SetText(text);

		// No need to invoke SetLength() since constructor invoked it with
		// minimum FooTextItem length and SetText() updated it.

		return item;
	}

	/* Instance methods. */

	FooTextItem::FooTextItem(const uint8_t* buffer, size_t bufferLength)
	  : FooItem(buffer, bufferLength)
	{
		MS_TRACE();

		SetLength(FooItem::ItemHeaderLength);
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
		MS_DUMP(
		  "  id: %" PRIu8 " (%s) (unknown:%s)",
		  static_cast<uint8_t>(GetId()),
		  FooItem::ItemId2String(GetId()).c_str(),
		  HasUnknownId() ? "yes" : "no");
		MS_DUMP("  flags: " MS_UINT8_4BITS_TO_BINARY_PATTERN, MS_UINT8_4BITS_TO_BINARY(GetFlags()));
		MS_DUMP(
		  "  value length field: %" PRIu8 " (computed value length: %" PRIu8 ")",
		  GetValueLengthField(),
		  GetValueLength());
		auto& text = GetText();
		MS_DUMP("  text: %.*s", static_cast<int>(text.size()), text.data());
		MS_DUMP("</FooTextItem>");
	}

	FooTextItem* FooTextItem::Clone(uint8_t* buffer, size_t bufferLength) const
	{
		MS_TRACE();

		auto* clonedItem = new FooTextItem(buffer, bufferLength);

		CloneInto(clonedItem);

		return clonedItem;
	}

	const std::string_view FooTextItem::GetText() const
	{
		MS_TRACE();

		return std::string_view(reinterpret_cast<const char*>(GetValuePointer()), GetValueLength());
	}

	void FooTextItem::SetText(const std::string& text)
	{
		MS_TRACE();

		AssertNotFrozen();

		auto previousValueLength = GetValueLength();

		// Let's call SetLength() on parent with the new computed item length.
		// NOTE: If there is no space in the buffer for it, it will throw.
		SetLength(GetLength() - previousValueLength + text.size());

		// Copy the given text into the buffer.
		Utils::Buffer::MemcpyOrMemmove(
		  GetValuePointer(), reinterpret_cast<const uint8_t*>(text.data()), text.size());

		// Update the Value Length field.
		SetValueLengthField(text.size());
	}
} // namespace RTC
