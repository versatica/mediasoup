#define MS_CLASS "RTC::TestSerializable::FooNumericItem"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/TestSerializable/FooNumericItem.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "Utils.hpp"

namespace RTC
{
	/* Class methods. */

	FooNumericItem* FooNumericItem::Parse(const uint8_t* buffer, size_t bufferLength)
	{
		MS_TRACE();

		FooItem::ItemId itemId;
		uint8_t valueLength;

		if (!FooItem::IsFooItem(buffer, bufferLength, itemId, valueLength))
		{
			MS_WARN_DEV("not a FooItem");

			return nullptr;
		}

		if (itemId != FooItem::ItemId::NUMERIC)
		{
			MS_WARN_DEV("invalid item id");

			return nullptr;
		}

		if (valueLength != FooNumericItem::NumberLength)
		{
			MS_WARN_DEV("invalid value length");

			return nullptr;
		}

		auto* item = new FooNumericItem(buffer, bufferLength);

		// No need to invoke SetLength() since constructor invoked it with
		// FooNumericItem fixed length.

		// Mark the item as frozen since we are parsing.
		item->Freeze();

		return item;
	}

	FooNumericItem* FooNumericItem::Factory(
	  uint8_t* buffer, size_t bufferLength, uint8_t flags, uint16_t number)
	{
		MS_TRACE();

		// DooFataItem has fixed length.
		if (bufferLength < FooItem::ItemHeaderLength + FooNumericItem::NumberLength)
		{
			MS_THROW_TYPE_ERROR("too small buffer");
		}

		auto* item = new FooNumericItem(buffer, bufferLength);

		item->InitializeHeader(FooItem::ItemId::NUMERIC, flags, FooNumericItem::NumberLength);
		item->SetNumber(number);

		// No need to invoke SetLength() since constructor invoked it with
		// FooNumericItem fixed length.

		return item;
	}

	/* Instance methods. */

	FooNumericItem::FooNumericItem(const uint8_t* buffer, size_t bufferLength)
	  : FooItem(buffer, bufferLength)
	{
		MS_TRACE();

		SetLength(FooItem::ItemHeaderLength + FooNumericItem::NumberLength);
	}

	FooNumericItem::~FooNumericItem()
	{
		MS_TRACE();
	}

	void FooNumericItem::Dump() const
	{
		MS_TRACE();

		MS_DUMP("<FooNumericItem>");
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
		MS_DUMP("  number: %" PRIu16, GetNumber());
		MS_DUMP("</FooNumericItem>");
	}

	FooNumericItem* FooNumericItem::Clone(uint8_t* buffer, size_t bufferLength) const
	{
		MS_TRACE();

		auto* clonedItem = new FooNumericItem(buffer, bufferLength);

		CloneInto(clonedItem);

		return clonedItem;
	}

	uint16_t FooNumericItem::GetNumber() const
	{
		MS_TRACE();

		return Utils::Byte::Get2Bytes(GetValuePointer(), 0);
	}

	void FooNumericItem::SetNumber(uint16_t number)
	{
		MS_TRACE();

		AssertNotFrozen();

		Utils::Byte::Set2Bytes(GetValuePointer(), 0, number);

		// NOTE: FooNumericItem has fixed value so nothing else is needed here.
	}
} // namespace RTC
