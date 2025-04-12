#define MS_CLASS "RTC::TestSerializable:FooPacket"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/TestSerializable/FooPacket.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "RTC/TestSerializable/FooNumericItem.hpp"
#include "RTC/TestSerializable/FooTextItem.hpp"
#include "RTC/TestSerializable/FooUnknownItem.hpp"
#include <string>

namespace RTC
{
	/* Class methods. */

	bool FooPacket::IsFooPacket(const uint8_t* buffer, size_t bufferLength)
	{
		MS_TRACE();

		if (bufferLength < FooPacket::HeaderLength)
		{
			MS_WARN_DEV("no space for FooPacket header");

			return false;
		}

		auto* header = reinterpret_cast<const FooPacket::Header*>(buffer);
		auto length  = uint16_t{ ntohs(header->length) };

		// NOTE: We must cast to size_t, otherwise a maximum Length value of 65535
		// would generate a padded length of 0 bytes!
		if (Utils::Byte::PadTo4Bytes(size_t{ length }) != bufferLength)
		{
			MS_WARN_DEV("padded announced packet length does not match buffer length");

			return false;
		}

		if (header->a && length < FooPacket::HeaderLength + FooPacket::AppendixLength)
		{
			MS_WARN_DEV("no space for appendix");

			return false;
		}

		return true;
	}

	FooPacket* FooPacket::Parse(const uint8_t* buffer, size_t bufferLength)
	{
		MS_TRACE();

		if (!FooPacket::IsFooPacket(buffer, bufferLength))
		{
			MS_WARN_DEV("not a FooPacket");

			return nullptr;
		}

		auto* packet = new FooPacket(buffer, bufferLength);

		// Pointer that starts at the beginning of the buffer and it's incremented
		// to point to different parts of the Packet.
		auto* ptr = buffer;

		// Move to items.
		ptr = packet->GetItemsPointer();

		while (ptr < buffer + packet->GetLengthField())
		{
			// The remaining length in the buffer (excluding padding in the packet)
			// is the potential buffer length of the item.
			size_t itemMaxBufferLength = packet->GetPaddingPointer() - ptr;

			// Here we must anticipate the id of each item to use its appropriate
			// parser.
			FooItem::ItemId itemId;
			uint8_t valueLength;

			if (!FooItem::IsFooItem(ptr, itemMaxBufferLength, itemId, valueLength))
			{
				MS_WARN_DEV("not a FooItem");

				delete packet;
				return nullptr;
			}

			FooItem* item{ nullptr };

			MS_DEBUG_DEV("parsing FooItem [ptr:%zu, id:%" PRIu8 "]", ptr - buffer, itemId);

			switch (itemId)
			{
				case FooItem::ItemId::NUMERIC:
				{
					item = FooNumericItem::Parse(ptr, FooItem::ItemHeaderLength + valueLength);

					if (!item)
					{
						MS_WARN_DEV("FooNumericItem parser failed");

						delete packet;
						return nullptr;
					}

					break;
				}

				case FooItem::ItemId::TEXT:
				{
					item = FooTextItem::Parse(ptr, FooItem::ItemHeaderLength + valueLength);

					if (!item)
					{
						MS_WARN_DEV("FooTextItem parser failed");

						delete packet;
						return nullptr;
					}

					break;
				}

				default:
				{
					item = FooUnknownItem::Parse(ptr, FooItem::ItemHeaderLength + valueLength);

					if (!item)
					{
						MS_WARN_DEV("FooUnknownItem parser failed");

						delete packet;
						return nullptr;
					}
				}
			}

			// Here we are parsing so we don't use AddItem() (that clones the Item into
			// the Packet buffer) but AddParsedItem().
			packet->AddParsedItem(item);

			ptr += item->GetLength();
		}

		// We should be at the potential padding position.
		if (ptr != packet->GetPaddingPointer())
		{
			MS_WARN_DEV("we should be in the padding but we are not");

			delete packet;
			return nullptr;
		}

		const size_t computedLength = Utils::Byte::PadTo4Bytes(static_cast<size_t>(ptr - buffer));

		// Ensure computed length (padded to 4 bytes) matches the total given buffer
		// length.
		if (computedLength != bufferLength)
		{
			MS_WARN_DEV("computed padded length != buffer length");

			delete packet;
			return nullptr;
		}

		// It's mandatory to call SetLength() once we are done and we know the exact
		// length of the Packet (padding included).
		packet->SetLength(computedLength);

		// Mark the packet as frozen since we are parsing.
		packet->Freeze();

		return packet;
	}

	FooPacket* FooPacket::Factory(uint8_t* buffer, size_t bufferLength, uint8_t type)
	{
		MS_TRACE();

		size_t computedLength = FooPacket::HeaderLength;

		// No space for header.
		if (bufferLength < computedLength)
		{
			MS_THROW_TYPE_ERROR("no space for header");
		}

		auto* packet = new FooPacket(buffer, bufferLength);

		packet->InitializeHeader(type, computedLength);

		// No need to invoke SetLength() since constructor invoked it with
		// minimum FooPacket length.

		return packet;
	}

	/* Instance methods. */

	FooPacket::FooPacket(const uint8_t* buffer, size_t bufferLength)
	  : Serializable(buffer, bufferLength)
	{
		MS_TRACE();

		SetLength(FooPacket::HeaderLength);
	}

	FooPacket::~FooPacket()
	{
		MS_TRACE();

		for (auto* item : this->items)
		{
			delete item;
		}
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
		for (const auto* item : this->items)
		{
			item->Dump();
		}
		MS_DUMP("</FooPacket>");
	}

	void FooPacket::Serialize(uint8_t* buffer, size_t bufferLength)
	{
		MS_TRACE();

		const auto* previousBuffer = GetBuffer();

		// Invoke the parent method to copy the whole buffer.
		Serializable::Serialize(buffer, bufferLength);

		// Reassign pointers.
		for (auto* item : this->items)
		{
			size_t offset = item->GetBuffer() - previousBuffer;

			item->SetBuffer(buffer + offset);
		}
	}

	FooPacket* FooPacket::Clone(uint8_t* buffer, size_t bufferLength) const
	{
		MS_TRACE();

		auto* clonedPacket = new FooPacket(buffer, bufferLength);

		CloneInto(clonedPacket);

		// Add a new parsed item for each item in this packet and make it pointer
		// point to its position in the new buffer.
		for (const auto* item : this->items)
		{
			size_t offset = item->GetBuffer() - GetBuffer();

			FooItem* clonedItem{ nullptr };

			switch (item->GetId())
			{
				case FooItem::ItemId::NUMERIC:
				{
					clonedItem = new FooNumericItem(buffer + offset, item->GetBufferLength());

					break;
				}

				case FooItem::ItemId::TEXT:
				{
					clonedItem = new FooTextItem(buffer + offset, item->GetBufferLength());

					break;
				}

				default:
				{
					clonedItem = new FooUnknownItem(buffer + offset, item->GetBufferLength());
				}
			}

			try
			{
				clonedItem->SetLength(item->GetLength());
			}
			catch (const MediaSoupError& error)
			{
				delete clonedItem;

				throw;
			}

			clonedPacket->AddParsedItem(clonedItem);
		}

		return clonedPacket;
	}

	/**
	 * If given `appendix` is 0 then Appendix is removed.
	 */
	void FooPacket::SetAppendix(uint32_t appendix)
	{
		MS_TRACE();

		AssertNotFrozen();

		auto hadAppendix = HasAppendix();

		// There was Appendix and we are just replacing it, so Packet length remains
		// the same.
		if (hadAppendix && appendix)
		{
			Utils::Byte::Set4Bytes(GetAppendixPointer(), 0, appendix);
		}
		// There wasn't Appendix and we are adding it, so need to move items.
		else if (!hadAppendix && appendix)
		{
			size_t previousLengthWithoutPadding = GetPaddingPointer() - GetBuffer();
			size_t lengthWithoutPadding = previousLengthWithoutPadding + FooPacket::AppendixLength;

			// Update flag A and Length field. This will make `GetXxxxxPointer()`
			// return different values as if there was Appendix field.
			SetAppendixFlag(true);
			SetLengthField(lengthWithoutPadding);

			// Update Serializable length.
			SetLength(Utils::Byte::PadTo4Bytes(lengthWithoutPadding));

			for (auto it = this->items.rbegin(); it != this->items.rend(); ++it)
			{
				auto* item = *it;

				item->Serialize(
				  const_cast<uint8_t*>(item->GetBuffer()) + FooPacket::FooPacket::AppendixLength,
				  item->GetLength());

				// After calling `Serialize()` on the item, its `frozen` flag is reverted
				// to false, but we want it to remain set because it's an item within the
				// packet.
				item->Freeze();
			}

			// Copy the given Appendix value.
			Utils::Byte::Set4Bytes(GetAppendixPointer(), 0, appendix);
		}
		// There was Appendix and we are removing it, so need to move items.
		else if (hadAppendix && !appendix)
		{
			size_t previousLengthWithoutPadding = GetPaddingPointer() - GetBuffer();
			size_t lengthWithoutPadding = previousLengthWithoutPadding - FooPacket::AppendixLength;

			// Update flag A and Length field. This will make `GetXxxxxPointer()`
			// return different values as if there was an Appendix field.
			SetAppendixFlag(false);
			SetLengthField(lengthWithoutPadding);

			// Update Serializable length.
			SetLength(Utils::Byte::PadTo4Bytes(lengthWithoutPadding));

			for (auto* item : this->items)
			{
				item->Serialize(
				  const_cast<uint8_t*>(item->GetBuffer()) - FooPacket::AppendixLength, item->GetLength());

				// After calling `Serialize()` on the item, its `frozen` flag is reverted
				// to false, but we want it to remain set because it's an item within the
				// packet.
				item->Freeze();
			}
		}
	}

	void FooPacket::AddItem(const FooItem* item)
	{
		MS_TRACE();

		AssertNotFrozen();

		size_t previousLengthWithoutPadding = GetPaddingPointer() - GetBuffer();
		size_t lengthWithoutPadding         = previousLengthWithoutPadding + item->GetLength();

		// Let's append the item at the end of existing items, this is, where the
		// padding would start.
		auto* clonedItem = item->Clone(GetPaddingPointer(), item->GetLength());

		// Freeze the cloned item.
		clonedItem->Freeze();

		this->items.push_back(clonedItem);

		// Update Length field.
		SetLengthField(lengthWithoutPadding);

		// Update Serializable length.
		SetLength(Utils::Byte::PadTo4Bytes(lengthWithoutPadding));
	}

	void FooPacket::AddNumericItem(uint8_t flags, uint16_t number)
	{
		MS_TRACE();

		AssertNotFrozen();

		const size_t bufferLength = GetBufferLength() - (GetPaddingPointer() - GetBuffer());

		auto* item = FooNumericItem::Factory(GetPaddingPointer(), bufferLength, flags, number);

		// Let's fix item's buffer length by making it match the item real length.
		item->SetBufferLength(item->GetLength());

		size_t previousLengthWithoutPadding = GetPaddingPointer() - GetBuffer();
		size_t lengthWithoutPadding         = previousLengthWithoutPadding + item->GetLength();

		// Freeze the item.
		item->Freeze();

		this->items.push_back(item);

		// Update Length field.
		SetLengthField(lengthWithoutPadding);

		// Update Serializable length.
		SetLength(Utils::Byte::PadTo4Bytes(lengthWithoutPadding));
	}

	void FooPacket::AddTextItem(uint8_t flags, const std::string& text)
	{
		MS_TRACE();

		AssertNotFrozen();

		const size_t bufferLength = GetBufferLength() - (GetPaddingPointer() - GetBuffer());

		auto* item = FooTextItem::Factory(GetPaddingPointer(), bufferLength, flags, text);

		// Let's fix item's buffer length by making it match the item real length.
		item->SetBufferLength(item->GetLength());

		size_t previousLengthWithoutPadding = GetPaddingPointer() - GetBuffer();
		size_t lengthWithoutPadding         = previousLengthWithoutPadding + item->GetLength();

		// Freeze the item.
		item->Freeze();

		this->items.push_back(item);

		// Update Length field.
		SetLengthField(lengthWithoutPadding);

		// Update Serializable length.
		SetLength(Utils::Byte::PadTo4Bytes(lengthWithoutPadding));
	}

	void FooPacket::InitializeHeader(uint8_t type, uint16_t length)
	{
		MS_TRACE();

		GetHeaderPointer()->type = type;
		SetAppendixFlag(false);
		GetHeaderPointer()->unused = 0u;
		SetLengthField(FooPacket::HeaderLength);
	}

	void FooPacket::AddParsedItem(FooItem* item)
	{
		MS_TRACE();

		// Freeze the item.
		item->Freeze();

		this->items.push_back(item);
	}
} // namespace RTC
