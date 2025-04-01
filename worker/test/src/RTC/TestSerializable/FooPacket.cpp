#define MS_CLASS "RTC::TestSerializable:FooPacket"
#define MS_LOG_DEV_LEVEL 3

#include "RTC/TestSerializable/FooPacket.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "Utils.hpp"
#include "helpers.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memcpy()
#include <utility> // std::move()
#include <vector>

std::unique_ptr<FooPacket> FooPacket::Parse(const uint8_t* buffer, size_t length)
{
	// No space for header.
	if (length < HeaderLength)
	{
		MS_WARN_DEV("no space for Header");

		return nullptr;
	}

	// Pointer that starts at the beginning of the buffer and it's incremented
	// to point to different parts of the Packet.
	const uint8_t* ptr = buffer;

	// Pointer that points to the end of the buffer.
	const uint8_t* end = buffer + length;

	// NOTE: Here we are passing `length` as `bufferLength`. However we know
	// that, due to FooPacket nature, a FooPacket Packet must occupy the whole
	// given buffer.
	// NOTE: We are parsing so we don't want to initialize the header.
	// auto* fooPacket = new FooPacket(buffer, length, /*initializeHeader*/ false);
	auto fooPacket =
	  std::unique_ptr<FooPacket>(new FooPacket(buffer, length, /*initializeHeader*/ false));

	printf(
	  "FooPacket::Parse() START [ptr+:%zu, Length:%" PRIu16 ", length:%zu]\n",
	  ptr - buffer,
	  fooPacket->GetLengthField(),
	  length);

	// Move to the Appendix.
	if (fooPacket->HasAppendix())
	{
		ptr = fooPacket->GetAppendixPointer();

		printf("FooPacket::Parse() has Appendix [ptr+:%zu]\n", ptr - buffer);

		// No space for Appendix.
		if (ptr + AppendixLength > end)
		{
			MS_WARN_DEV("no space for Appendix");

			return nullptr;
		}
	}

	// Move to items.
	if (fooPacket->HasItems())
	{
		if (fooPacket->GetLengthField() > length)
		{
			MS_WARN_DEV("no space for Items");

			return nullptr;
		}

		ptr = fooPacket->GetItemsPointer();

		printf("FooPacket::Parse() has items [ptr+:%zu]\n", ptr - buffer);

		while (ptr < buffer + fooPacket->GetLengthField())
		{
			printf("FooPacket::Parse() parsing item [ptr+:%zu]\n", ptr - buffer);

			auto item = FooItem::Parse(ptr, buffer + fooPacket->GetLengthField() - ptr);

			if (item)
			{
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
				// NOTE: We need to pass an unique_ptr so beed to use std::move() to
				// transfer ownership.
				fooPacket->AddParsedItem(std::move(item));
			}
			else
			{
				MS_WARN_DEV("wrong Item");

				return nullptr;
			}
		}
	}

	// Move to the possible padding.
	ptr = fooPacket->GetPaddingPointer();

	const size_t computedLength = Utils::Byte::PadTo4Bytes(static_cast<size_t>(ptr - buffer));

	printf("FooPacket::Parse() END [ptr+:%zu, computedLength:%zu]\n", ptr - buffer, computedLength);

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

std::unique_ptr<FooPacket> FooPacket::Factory(const uint8_t* buffer, size_t bufferLength, uint8_t type)
{
	size_t computedLength = HeaderLength;

	// No space for header.
	if (bufferLength < computedLength)
	{
		MS_THROW_TYPE_ERROR("no space for Header");
	}

	printf(
	  "FooPacket::Factory() [computedLength:%zu, bufferLength:%zu]\n", computedLength, bufferLength);

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

FooPacket::~FooPacket()
{
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
	for (auto& item : this->items)
	{
		item->Dump();
	}
	MS_DUMP("</FooPacket>");
}

void FooPacket::Serialize(const uint8_t* buffer, size_t bufferLength)
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

std::unique_ptr<Serializable> FooPacket::Clone(const uint8_t* buffer, size_t bufferLength) const
{
	MS_TRACE();

	auto clonedFooPacket =
	  std::unique_ptr<FooPacket>(new FooPacket(buffer, bufferLength, /*initializeHeader*/ false));

	// TODO: Clone items.

	// Need to manually set Serializable length.
	clonedFooPacket->SetLength(GetLength());

	return clonedFooPacket;
}

/**
 * If given `appendix` is 0 then Appendix is removed.
 */
void FooPacket::SetAppendix(uint32_t appendix)
{
	auto hadAppendix = HasAppendix();

	// There was Appendix and we are just replacing it, so Packet length
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

const std::unique_ptr<FooItem>& FooPacket::GetItem(size_t idx) const
{
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
