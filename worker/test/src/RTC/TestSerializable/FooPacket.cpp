#define MS_CLASS "RTC::TestSerializable:FooPacket"
#define MS_LOG_DEV_LEVEL 3

#include "RTC/TestSerializable/FooPacket.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "RTC/TestSerializable/FooNumericItem.hpp"
#include "RTC/TestSerializable/FooTextItem.hpp"
#include "RTC/TestSerializable/FooUnknownItem.hpp"
#include <cstring> // std::memcpy()
#include <utility> // std::move()

std::unique_ptr<FooPacket> FooPacket::Parse(const uint8_t* buffer, size_t length)
{
	MS_TRACE();

	// No space for header.
	if (length < FooPacket::HeaderLength)
	{
		MS_WARN_DEV("no space for Header");

		return nullptr;
	}

	// Pointer that starts at the beginning of the buffer and it's incremented
	// to point to different parts of the Packet.
	auto* ptr = buffer;

	// Pointer that points to the end of the buffer.
	auto* end = buffer + length;

	// NOTE: Here we are passing `length` as `bufferLength`. However we know
	// that, due to FooPacket nature, a FooPacket Packet must occupy the whole
	// given buffer.
	// NOTE: We are parsing so we don't want to initialize the header.
	// auto* fooPacket = new FooPacket(buffer, length, /*initializeHeader*/ false);
	auto fooPacket =
	  std::unique_ptr<FooPacket>(new FooPacket(buffer, length, /*initializeHeader*/ false));

	// Length field in FooPacket means total value of the packet (exluding
	// padding bytes).
	if (fooPacket->GetLengthField() > length)
	{
		MS_WARN_DEV("Length field is greater than given length");

		return nullptr;
	}

	// Move to the Appendix.
	if (fooPacket->HasAppendix())
	{
		ptr = fooPacket->GetAppendixPointer();

		// No space for Appendix.
		if (ptr + FooPacket::AppendixLength > end)
		{
			MS_WARN_DEV("no space for Appendix");

			return nullptr;
		}
	}

	// Move to items.
	if (fooPacket->HasItems())
	{
		ptr = fooPacket->GetItemsPointer();

		while (ptr < buffer + fooPacket->GetLengthField())
		{
			// Here we must anticipate the id of each item to use its appropriate.
			if (ptr + FooItem::ItemHeaderLength > end)
			{
				MS_WARN_DEV("no space for item");

				return nullptr;
			}

			const auto* itemHeader = reinterpret_cast<const FooItem::ItemHeader*>(ptr);

			auto itemId = itemHeader->id;

			// The remaining length in the buffer (excluding padding in the packet)
			// is the potential buffer length of the item.
			size_t itemBufferLength = fooPacket->GetLengthField() - (ptr - buffer);

			std::unique_ptr<FooItem> item;

			MS_DEBUG_DEV("parsing FooItem [ptr:%zu, id:%" PRIu8 "]", ptr - buffer, itemId);

			// TODO
			switch (itemId)
			{
				case FooItem::ItemId::NUMERIC:
				{
					item = FooNumericItem::Parse(ptr, itemBufferLength);

					if (!item)
					{
						MS_WARN_DEV("FooNumericItem parser failed");

						return nullptr;
					}

					break;
				}

				case FooItem::ItemId::TEXT:
				{
					item = FooTextItem::Parse(ptr, itemBufferLength);

					if (!item)
					{
						MS_WARN_DEV("FooTextItem parser failed");

						return nullptr;
					}

					break;
				}

				default:
				{
					item = FooUnknownItem::Parse(ptr, itemBufferLength);

					if (!item)
					{
						MS_WARN_DEV("FooUnknownItem parser failed");

						return nullptr;
					}
				}
			}

			// Let's fix item's buffer length. This is because we didn't know its
			// exact length when we called FooItem::Parse() so we passed the rest
			// of the Packet buffer as buffer length. Once item is parsed, and
			// given that it is part of the FooPacket buffer, we can fix its
			// buffer length by making it be equal to its real length.
			item->SetBufferLength(item->GetLength());

			// NOTE: We are gonna move item ownership in next line so must do
			// this before.
			ptr += item->GetLength();

			// Here we are parsing so we don't use AddItem() (that serializes the
			// Item into the Packet buffer, but AddParsedItem().
			// NOTE: We need to move ownership of the cloned FooItem unique pointer.
			fooPacket->AddParsedItem(std::move(item));
		}
	}

	// Move to the possible padding.
	ptr = fooPacket->GetPaddingPointer();

	const size_t computedLength = Utils::Byte::PadTo4Bytes(static_cast<size_t>(ptr - buffer));

	// Ensure computed length (padded to 4 bytes) matches the total given
	// length.
	if (computedLength != length)
	{
		MS_WARN_DEV("computed padded length != buffer length");

		return nullptr;
	}

	// It's mandatory to call SetLength() once we are done and we know the exact
	// length of the Packet (padding included).
	fooPacket->SetLength(computedLength);

	return fooPacket;
}

std::unique_ptr<FooPacket> FooPacket::Factory(uint8_t* buffer, size_t bufferLength, uint8_t type)
{
	MS_TRACE();

	size_t computedLength = FooPacket::HeaderLength;

	// No space for header.
	if (bufferLength < computedLength)
	{
		MS_THROW_TYPE_ERROR("no space for Header");
	}

	// We want to initialize the header since we are creating a Packet from
	// scratch.
	auto fooPacket =
	  std::unique_ptr<FooPacket>(new FooPacket(buffer, bufferLength, /*initializeHeader*/ true));

	fooPacket->SetType(type);

	// NOTE: No need to call fooPacket->SetLength() since the constructor did.

	return fooPacket;
}

FooPacket::FooPacket(const uint8_t* buffer, size_t bufferLength, bool initializeHeader)
  : Serializable(buffer, bufferLength)
{
	MS_TRACE();

	if (initializeHeader)
	{
		SetType(0u);
		SetAppendixFlag(false);
		SetUnusedField();
		SetLengthField(FooPacket::HeaderLength);

		// Update Serializable length.
		SetLength(FooPacket::HeaderLength);
	}
}

FooPacket::~FooPacket()
{
	MS_TRACE();
}

void FooPacket::Dump() const
{
	MS_TRACE();

	MS_DUMP("<FooPacket>");
	MS_DUMP("  length (padding included): %zu (buffer length: %zu)", GetLength(), GetBufferLength());
	MS_DUMP("  type: %" PRIu8, GetType());
	MS_DUMP("  length field: %" PRIu16, GetLengthField());
	MS_DUMP("  has appendix: %s", HasAppendix() ? "yes" : "no");
	MS_DUMP("  appendix: %" PRIu32, GetAppendix());
	MS_DUMP("  has items: %s", HasItems() ? "yes" : "no");
	MS_DUMP("  items count: %zu", GetItemsCount());
	for (const auto& item : this->items)
	{
		item->Dump();
	}
	MS_DUMP("</FooPacket>");
}

void FooPacket::Serialize(uint8_t* buffer, size_t bufferLength)
{
	MS_TRACE();

	if (bufferLength < GetLength())
	{
		MS_THROW_TYPE_ERROR(
		  "bufferLength (%zu bytes) is lower than current length (%zu bytes)", bufferLength, GetLength());
	}

	size_t itemsOffset   = GetItemsPointer() - GetBuffer();
	size_t paddingOffset = GetPaddingPointer() - GetBuffer();
	size_t padding       = GetLength() - (GetPaddingPointer() - GetBuffer());

	// Copy all bytes from beginning of the buffer until the position of the
	// items.
	std::memcpy(buffer, GetBuffer(), itemsOffset);

	// Serialize each item into the new buffer.
	auto* ptr = buffer + itemsOffset;

	for (const auto& item : this->items)
	{
		item->Serialize(ptr, item->GetLength());

		ptr += item->GetLength();
	}

	// Copy padding bytes.
	std::memcpy(buffer + paddingOffset, GetPaddingPointer(), padding);

	// Manually update buffer and buffer length.
	SetBuffer(buffer);
	SetBufferLength(bufferLength);
}

std::unique_ptr<Serializable> FooPacket::Clone(uint8_t* buffer, size_t bufferLength) const
{
	MS_TRACE();

	if (bufferLength < GetLength())
	{
		MS_THROW_TYPE_ERROR(
		  "bufferLength (%zu bytes) is lower than current length (%zu bytes)", bufferLength, GetLength());
	}

	size_t itemsOffset   = GetItemsPointer() - GetBuffer();
	size_t paddingOffset = GetPaddingPointer() - GetBuffer();
	size_t padding       = GetLength() - (GetPaddingPointer() - GetBuffer());

	// Copy all bytes from beginning of the buffer until the position of the
	// items.
	std::memcpy(buffer, GetBuffer(), itemsOffset);

	auto clonedFooPacket =
	  std::unique_ptr<FooPacket>(new FooPacket(buffer, bufferLength, /*initializeHeader*/ false));

	// Clone each item into the new buffer.
	auto* ptr = buffer + itemsOffset;

	for (const auto& item : this->items)
	{
		// FooItem::Clone() returns a unique_ptr<Serializable>. We need to release
		// its pointer, cast it to FooItem*, and then create a unique_ptr<FooItem>
		// with it.
		auto* clonedItemPtr = static_cast<FooItem*>(item->Clone(ptr, item->GetLength()).release());
		auto clonedItem     = std::unique_ptr<FooItem>(clonedItemPtr);

		// NOTE: We need to move ownership of the cloned FooItem unique pointer.
		clonedFooPacket->items.push_back(std::move(clonedItem));

		ptr += item->GetLength();
	}

	// Copy padding bytes.
	std::memcpy(buffer + paddingOffset, GetPaddingPointer(), padding);

	// Need to manually set Serializable length.
	clonedFooPacket->SetLength(GetLength());

	return clonedFooPacket;
}

/**
 * If given `appendix` is 0 then Appendix is removed.
 */
void FooPacket::SetAppendix(uint32_t appendix)
{
	MS_TRACE();

	auto hadAppendix = HasAppendix();

	// There was Appendix and we are just replacing it, so Packet length
	// remains the same.
	if (hadAppendix && appendix)
	{
		Utils::Byte::Set4Bytes(GetAppendixPointer(), 0, appendix);
	}
	// There wasn't Appendix and we are adding it, so need to move items.
	else if (!hadAppendix && appendix)
	{
		size_t previousLengthWithoutPadding = GetPaddingPointer() - GetBuffer();
		size_t lengthWithoutPadding         = previousLengthWithoutPadding + FooPacket::AppendixLength;

		// Update flag A and Length field. This will make `GetXxxxxPointer()`
		// return different values as if there was Appendix field.
		SetAppendixFlag(true);
		SetLengthField(lengthWithoutPadding);

		// Update Serializable length.
		SetLength(Utils::Byte::PadTo4Bytes(lengthWithoutPadding));

		for (auto it = this->items.rbegin(); it != this->items.rend(); ++it)
		{
			const auto& item = *it;

			item->Serialize(
			  const_cast<uint8_t*>(item->GetBuffer()) + FooPacket::FooPacket::AppendixLength,
			  item->GetLength());
		}

		// Copy the given Appendix value.
		Utils::Byte::Set4Bytes(GetAppendixPointer(), 0, appendix);
	}
	// There was Appendix and we are removing it, so need to move items.
	else if (hadAppendix && !appendix)
	{
		size_t previousLengthWithoutPadding = GetPaddingPointer() - GetBuffer();
		size_t lengthWithoutPadding         = previousLengthWithoutPadding - FooPacket::AppendixLength;

		// Update flag A and Length field. This will make `GetXxxxxPointer()`
		// return different values as if there was an Appendix field.
		SetAppendixFlag(false);
		SetLengthField(lengthWithoutPadding);

		// Update Serializable length.
		SetLength(Utils::Byte::PadTo4Bytes(lengthWithoutPadding));

		for (const auto& item : this->items)
		{
			item->Serialize(
			  const_cast<uint8_t*>(item->GetBuffer()) - FooPacket::AppendixLength, item->GetLength());
		}
	}
}

const std::unique_ptr<FooItem>& FooPacket::GetItem(size_t idx) const
{
	MS_TRACE();

	if (idx >= this->items.size())
	{
		static std::unique_ptr<FooItem> nullItem;

		return nullItem;
	}

	return this->items.at(idx);
}

/**
 * Serializes given Item into Packet's buffer.
 *
 * @remarks
 * Once this method is called, the Item is owned by FooPacket instance.
 */
void FooPacket::AddItem(std::unique_ptr<FooItem> item)
{
	MS_TRACE();

	size_t previousLengthWithoutPadding = GetPaddingPointer() - GetBuffer();
	size_t lengthWithoutPadding         = previousLengthWithoutPadding + item->GetLength();

	// Let's append the item at the end of existing items, this is, where the
	// padding would start.
	item->Serialize(GetPaddingPointer(), item->GetLength());

	// Update Length field.
	SetLengthField(lengthWithoutPadding);

	// Update Serializable length.
	SetLength(Utils::Byte::PadTo4Bytes(lengthWithoutPadding));

	// NOTE: We need to move ownership of the cloned FooItem unique pointer.
	this->items.push_back(std::move(item));
}
