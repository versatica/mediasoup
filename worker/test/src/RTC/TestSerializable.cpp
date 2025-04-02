#include "common.hpp"
#include "MediaSoupErrors.hpp"
#include "Utils.hpp"
#include "helpers.hpp"
#include "RTC/Serializable.hpp"
#include "RTC/TestSerializable/FooDataItem.hpp"
#include "RTC/TestSerializable/FooItem.hpp"
#include "RTC/TestSerializable/FooPacket.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memset()
#include <utility> // std::move()
#include <vector>

using namespace RTC;

SCENARIO("parse FooPacket", "[rtc][serializable]")
{
	// clang-format off
	uint8_t buffer[] =
	{
		// Type:1, A:1, Length:19
		0x01, 0b10000000, 0x00, 0x13,
		// Appendix: 0x00BC614E
		0x00, 0xBC, 0x61, 0x4E,
		// FooItem 1: Id:1, Flags:0b0101, Length:2, Value: 0x1234
		0b00010101, 0x02, 0x12, 0x34,
		// FooItem 2: Id:2, Flags:0b0011, Length:5, , Value: 0xA987654321.
		0b00100011, 0x05, 0xA9, 0x87,
		// 1 byte of padding.
		0x65, 0x43, 0x21, 0x00
	};
	// clang-format on

	auto fooPacket = FooPacket::Parse(buffer, sizeof(buffer));

	REQUIRE(sizeof(buffer) == 20);
	REQUIRE(fooPacket);
	REQUIRE(fooPacket->GetBuffer() == buffer);
	REQUIRE(fooPacket->GetBufferLength() == 20);
	REQUIRE(fooPacket->GetLength() == 20);
	REQUIRE(Utils::Byte::IsPaddedTo4Bytes(fooPacket->GetLength()) == true);
	REQUIRE(fooPacket->GetType() == 1);
	REQUIRE(fooPacket->HasAppendix() == true);
	REQUIRE(fooPacket->GetAppendix() == 0x00BC614E);
	REQUIRE(fooPacket->HasItems() == true);
	REQUIRE(fooPacket->GetItemsCount() == 2);
	REQUIRE(
	  helpers::areBuffersEqual(fooPacket->GetBuffer(), fooPacket->GetLength(), buffer, 20) == true);

	auto& item1 = fooPacket->GetItem(0);

	REQUIRE(item1);
	REQUIRE(item1->GetBuffer() == buffer + 8);
	REQUIRE(item1->GetBufferLength() == 4);
	REQUIRE(item1->GetLength() == 4);
	REQUIRE(item1->GetId() == FooItem::ItemId::DATA);
	REQUIRE(item1->GetFlags() == 0b0101);
	REQUIRE(item1->GetValueLength() == 2);
	REQUIRE(item1->GetValue()[0] == 0x12);
	REQUIRE(item1->GetValue()[1] == 0x34);
	REQUIRE(helpers::areBuffersEqual(item1->GetBuffer(), item1->GetLength(), buffer + 8, 4) == true);

	auto& item2 = fooPacket->GetItem(1);

	REQUIRE(item2);
	REQUIRE(item2->GetBuffer() == buffer + 12);
	// Buffer length in item 2 must be 7 since that's the remaining space from
	// the first byte of item 2 until available packet length (padding excluded).
	REQUIRE(item2->GetBufferLength() == 7);
	REQUIRE(item2->GetLength() == 7);
	REQUIRE(item2->GetId() == FooItem::ItemId::EVENT);
	REQUIRE(item2->GetFlags() == 0b0011);
	REQUIRE(item2->GetValueLength() == 5);
	REQUIRE(item2->GetValue()[0] == 0xA9);
	REQUIRE(item2->GetValue()[1] == 0x87);
	REQUIRE(item2->GetValue()[2] == 0x65);
	REQUIRE(item2->GetValue()[3] == 0x43);
	REQUIRE(item2->GetValue()[4] == 0x21);
	REQUIRE(helpers::areBuffersEqual(item2->GetBuffer(), item2->GetLength(), buffer + 12, 7) == true);

	REQUIRE(!fooPacket->GetItem(2));
}

SCENARIO("parse invalid FooPacket with buffer not padded to 4 bytes", "[rtc][serializable]")
{
	// clang-format off
	uint8_t buffer[] =
	{
		// Type:1, A:1, Length:19
		0x01, 0b10000000, 0x00, 0x13,
		// Appendix: 0x00BC614E
		0x00, 0xBC, 0x61, 0x4E,
		// FooItem 1: Id:1 Flags:0b0101, Length:2, Value: 0x1234
		0b00010101, 0x02, 0x12, 0x34,
		// FooItem 2: Id:2, Flags:0b0011, Length:5, , Value: 0xA987654321.
		0b00100011, 0x05, 0xA9, 0x87,
		// 1 byte of padding.
		0x65, 0x43, 0x21, 0x00,
		// Extra bytes that make the packet invalid.
		0xFF, 0xFF, 0xFF
	};
	// clang-format on

	auto fooPacket = FooPacket::Parse(buffer, sizeof(buffer));

	REQUIRE(sizeof(buffer) == 23);
	REQUIRE(!fooPacket);
}

SCENARIO("parse invalid FooPacket with FooItem with invalid id NONE", "[rtc][serializable]")
{
	// clang-format off
	uint8_t buffer[] =
	{
		// Type:1, A:0, Length:8
		0x01, 0b00000000, 0x00, 0x08,
		// FooItem 1: Id:0 Flags:0b0101, Length:2, Value: 0x1234
		0b00000101, 0x02, 0x12, 0x34,
	};
	// clang-format on

	auto fooPacket = FooPacket::Parse(buffer, sizeof(buffer));

	REQUIRE(sizeof(buffer) == 8);
	REQUIRE(!fooPacket);
}

SCENARIO("create and modify FooPacket", "[rtc][serializable]")
{
	uint8_t buffer[256];
	uint8_t itemBuffer[17];

	std::memset(buffer, 0xFF, sizeof(buffer));
	std::memset(itemBuffer, 0xFF, sizeof(itemBuffer));

	auto fooPacket = FooPacket::Factory(buffer, sizeof(buffer), /*type*/ 55);

	REQUIRE(sizeof(buffer) == 256);
	REQUIRE(fooPacket);
	REQUIRE(fooPacket->GetBuffer() == buffer);
	REQUIRE(fooPacket->GetBufferLength() == 256);
	// Just the FooPacket header (4 bytes).
	REQUIRE(fooPacket->GetLength() == 4);
	REQUIRE(Utils::Byte::IsPaddedTo4Bytes(fooPacket->GetLength()) == true);
	REQUIRE(fooPacket->GetType() == 55);
	REQUIRE(fooPacket->HasAppendix() == false);
	REQUIRE(fooPacket->GetAppendix() == 0u);
	REQUIRE(fooPacket->HasItems() == false);
	REQUIRE(fooPacket->GetItemsCount() == 0);
	REQUIRE(helpers::areBuffersEqual(fooPacket->GetBuffer(), fooPacket->GetLength(), buffer, 4) == true);

	/* Add Type and Appendix. */

	fooPacket->SetType(125);
	fooPacket->SetAppendix(0x12345678);

	REQUIRE(fooPacket->GetBuffer() == buffer);
	REQUIRE(fooPacket->GetBufferLength() == 256);
	// Header (4 bytes) + Appendix (4 bytes).
	REQUIRE(fooPacket->GetLength() == 8);
	REQUIRE(Utils::Byte::IsPaddedTo4Bytes(fooPacket->GetLength()) == true);
	REQUIRE(fooPacket->GetType() == 125);
	REQUIRE(fooPacket->HasAppendix() == true);
	REQUIRE(fooPacket->GetAppendix() == 0x12345678);
	REQUIRE(fooPacket->HasItems() == false);
	REQUIRE(fooPacket->GetItemsCount() == 0);
	REQUIRE(helpers::areBuffersEqual(fooPacket->GetBuffer(), fooPacket->GetLength(), buffer, 8) == true);

	/* Remove Appendix. */

	fooPacket->SetAppendix(0u);

	REQUIRE(fooPacket->GetBuffer() == buffer);
	REQUIRE(fooPacket->GetBufferLength() == 256);
	// Header (4 bytes).
	REQUIRE(fooPacket->GetLength() == 4);
	REQUIRE(Utils::Byte::IsPaddedTo4Bytes(fooPacket->GetLength()) == true);
	REQUIRE(fooPacket->GetType() == 125);
	REQUIRE(fooPacket->HasAppendix() == false);
	REQUIRE(fooPacket->GetAppendix() == 0u);
	REQUIRE(fooPacket->HasItems() == false);
	REQUIRE(fooPacket->GetItemsCount() == 0);
	REQUIRE(helpers::areBuffersEqual(fooPacket->GetBuffer(), fooPacket->GetLength(), buffer, 4) == true);

	/* Add a FooItem. */

	uint8_t item1Value[] = { 0xAA, 0xBB, 0xCC };
	uint8_t item2Value[] = { 0xAB, 0xCD };

	// FooItem 1 (5 bytes).
	auto item1 = FooItem::Factory(
	  itemBuffer,
	  sizeof(itemBuffer),
	  /*id*/ FooItem::ItemId::DATA,
	  /*flags*/ 0b1000,
	  item1Value,
	  sizeof(item1Value));

	// Hold item1 pointer for tests below.
	auto* item1Ptr = item1.get();

	fooPacket->AddItem(std::move(item1));

	// FooItem 2 (4 bytes).
	auto item2 = FooItem::Factory(
	  itemBuffer,
	  sizeof(itemBuffer),
	  /*id*/ FooItem::ItemId::EVENT,
	  /*flags*/ 0b1001,
	  item2Value,
	  sizeof(item2Value));

	// Hold item2 pointer for tests below.
	auto* item2Ptr = item2.get();

	fooPacket->AddItem(std::move(item2));

	auto foo2 = FooPacket::Parse(fooPacket->GetBuffer(), fooPacket->GetLength());

	REQUIRE(fooPacket->GetBuffer() == buffer);
	REQUIRE(fooPacket->GetBufferLength() == 256);
	// Header (4 bytes) + items (9 bytes) + padding (3 bytes).
	REQUIRE(fooPacket->GetLength() == 16);
	REQUIRE(Utils::Byte::IsPaddedTo4Bytes(fooPacket->GetLength()) == true);
	REQUIRE(fooPacket->GetType() == 125);
	REQUIRE(fooPacket->HasAppendix() == false);
	REQUIRE(fooPacket->GetAppendix() == 0u);
	REQUIRE(fooPacket->HasItems() == true);
	REQUIRE(fooPacket->GetItemsCount() == 2);
	REQUIRE(
	  helpers::areBuffersEqual(fooPacket->GetBuffer(), fooPacket->GetLength(), buffer, 16) == true);

	REQUIRE(item1Ptr == fooPacket->GetItem(0).get());
	// We know this will be same as item length.
	REQUIRE(item1Ptr->GetBufferLength() == 5);
	REQUIRE(item1Ptr->GetLength() == 5);
	REQUIRE(item1Ptr->GetId() == FooItem::ItemId::DATA);
	REQUIRE(item1Ptr->GetFlags() == 0b1000);
	REQUIRE(item1Ptr->GetValueLength() == 3);
	REQUIRE(item1Ptr->GetValue()[0] == 0xAA);
	REQUIRE(item1Ptr->GetValue()[1] == 0xBB);
	REQUIRE(item1Ptr->GetValue()[2] == 0xCC);
	REQUIRE(
	  helpers::areBuffersEqual(item1Ptr->GetBuffer(), item1Ptr->GetLength(), buffer + 4, 5) == true);

	REQUIRE(item2Ptr == fooPacket->GetItem(1).get());
	// We know this will be same as item length.
	REQUIRE(item2Ptr->GetBufferLength() == 4);
	REQUIRE(item2Ptr->GetLength() == 4);
	REQUIRE(item2Ptr->GetId() == FooItem::ItemId::EVENT);
	REQUIRE(item2Ptr->GetFlags() == 0b1001);
	REQUIRE(item2Ptr->GetValueLength() == 2);
	REQUIRE(item2Ptr->GetValue()[0] == 0xAB);
	REQUIRE(item2Ptr->GetValue()[1] == 0xCD);
	REQUIRE(
	  helpers::areBuffersEqual(item2Ptr->GetBuffer(), item2Ptr->GetLength(), buffer + 4 + 5, 4) == true);

	REQUIRE(!fooPacket->GetItem(2));

	/* Add Appendix. */

	fooPacket->SetAppendix(666u);

	REQUIRE(fooPacket->GetBuffer() == buffer);
	REQUIRE(fooPacket->GetBufferLength() == 256);
	// Header (4 bytes) + Appendix (4 bytes) + items (9 bytes) + padding (3
	// bytes);
	REQUIRE(fooPacket->GetLength() == 20);
	REQUIRE(Utils::Byte::IsPaddedTo4Bytes(fooPacket->GetLength()) == true);
	REQUIRE(fooPacket->GetType() == 125);
	REQUIRE(fooPacket->HasAppendix() == true);
	REQUIRE(fooPacket->GetAppendix() == 666u);
	REQUIRE(fooPacket->HasItems() == true);
	REQUIRE(fooPacket->GetItemsCount() == 2);
	REQUIRE(
	  helpers::areBuffersEqual(fooPacket->GetBuffer(), fooPacket->GetLength(), buffer, 20) == true);

	REQUIRE(item1Ptr == fooPacket->GetItem(0).get());
	// We know this will be same as item length.
	REQUIRE(item1Ptr->GetBufferLength() == 5);
	REQUIRE(item1Ptr->GetLength() == 5);
	REQUIRE(item1Ptr->GetId() == FooItem::ItemId::DATA);
	REQUIRE(item1Ptr->GetFlags() == 0b1000);
	REQUIRE(item1Ptr->GetValueLength() == 3);
	REQUIRE(item1Ptr->GetValue()[0] == 0xAA);
	REQUIRE(item1Ptr->GetValue()[1] == 0xBB);
	REQUIRE(item1Ptr->GetValue()[2] == 0xCC);
	REQUIRE(
	  helpers::areBuffersEqual(item1Ptr->GetBuffer(), item1Ptr->GetLength(), buffer + 4 + 4, 5) == true);

	REQUIRE(item2Ptr == fooPacket->GetItem(1).get());
	// We know this will be same as item length.
	REQUIRE(item2Ptr->GetBufferLength() == 4);
	REQUIRE(item2Ptr->GetLength() == 4);
	REQUIRE(item2Ptr->GetId() == FooItem::ItemId::EVENT);
	REQUIRE(item2Ptr->GetFlags() == 0b1001);
	REQUIRE(item2Ptr->GetValueLength() == 2);
	REQUIRE(item2Ptr->GetValue()[0] == 0xAB);
	REQUIRE(item2Ptr->GetValue()[1] == 0xCD);
	REQUIRE(
	  helpers::areBuffersEqual(item2Ptr->GetBuffer(), item2Ptr->GetLength(), buffer + 4 + 4 + 5, 4) ==
	  true);

	REQUIRE(!fooPacket->GetItem(2));

	/* Remove Appendix and change flags of FooItem 2. */

	fooPacket->SetAppendix(0u);
	fooPacket->GetItem(1)->SetFlags(0b1111);

	REQUIRE(fooPacket->GetBuffer() == buffer);
	REQUIRE(fooPacket->GetBufferLength() == 256);
	// Header (4 bytes) + items (9 bytes) + padding (3 bytes);
	REQUIRE(fooPacket->GetLength() == 16);
	REQUIRE(Utils::Byte::IsPaddedTo4Bytes(fooPacket->GetLength()) == true);
	REQUIRE(fooPacket->GetType() == 125);
	REQUIRE(fooPacket->HasAppendix() == false);
	REQUIRE(fooPacket->GetAppendix() == 0u);
	REQUIRE(fooPacket->HasItems() == true);
	REQUIRE(fooPacket->GetItemsCount() == 2);
	REQUIRE(
	  helpers::areBuffersEqual(fooPacket->GetBuffer(), fooPacket->GetLength(), buffer, 16) == true);

	REQUIRE(item1Ptr == fooPacket->GetItem(0).get());
	// We know this will be same as item length.
	REQUIRE(item1Ptr->GetBufferLength() == 5);
	REQUIRE(item1Ptr->GetLength() == 5);
	REQUIRE(item1Ptr->GetId() == FooItem::ItemId::DATA);
	REQUIRE(item1Ptr->GetFlags() == 0b1000);
	REQUIRE(item1Ptr->GetValueLength() == 3);
	REQUIRE(item1Ptr->GetValue()[0] == 0xAA);
	REQUIRE(item1Ptr->GetValue()[1] == 0xBB);
	REQUIRE(item1Ptr->GetValue()[2] == 0xCC);
	REQUIRE(
	  helpers::areBuffersEqual(item1Ptr->GetBuffer(), item1Ptr->GetLength(), buffer + 4, 5) == true);

	REQUIRE(item2Ptr == fooPacket->GetItem(1).get());
	// We know this will be same as item length.
	REQUIRE(item2Ptr->GetBufferLength() == 4);
	REQUIRE(item2Ptr->GetLength() == 4);
	REQUIRE(item2Ptr->GetId() == FooItem::ItemId::EVENT);
	REQUIRE(item2Ptr->GetFlags() == 0b1111);
	REQUIRE(item2Ptr->GetValueLength() == 2);
	REQUIRE(item2Ptr->GetValue()[0] == 0xAB);
	REQUIRE(item2Ptr->GetValue()[1] == 0xCD);
	REQUIRE(
	  helpers::areBuffersEqual(item2Ptr->GetBuffer(), item2Ptr->GetLength(), buffer + 4 + 5, 4) == true);

	REQUIRE(!fooPacket->GetItem(2));

	/* Serialize FooPacket into another buffer. */

	uint8_t newBuffer1[256];

	std::memset(newBuffer1, 0xFF, sizeof(newBuffer1));

	fooPacket->Serialize(newBuffer1, sizeof(newBuffer1));

	// Compare new and old buffers.
	REQUIRE(helpers::areBuffersEqual(fooPacket->GetBuffer(), fooPacket->GetLength(), buffer, 16));

	// Once done fill the old buffer with 1s.
	std::memset(buffer, 0xFF, sizeof(buffer));

	REQUIRE(fooPacket->GetBuffer() == newBuffer1);
	REQUIRE(fooPacket->GetBufferLength() == 256);
	// Header (4 bytes) + items (9 bytes) + padding (3 bytes);
	REQUIRE(fooPacket->GetLength() == 16);
	REQUIRE(Utils::Byte::IsPaddedTo4Bytes(fooPacket->GetLength()) == true);
	REQUIRE(fooPacket->GetType() == 125);
	REQUIRE(fooPacket->HasAppendix() == false);
	REQUIRE(fooPacket->GetAppendix() == 0u);
	REQUIRE(fooPacket->HasItems() == true);
	REQUIRE(fooPacket->GetItemsCount() == 2);
	REQUIRE(
	  helpers::areBuffersEqual(fooPacket->GetBuffer(), fooPacket->GetLength(), newBuffer1, 16) == true);

	REQUIRE(item1Ptr == fooPacket->GetItem(0).get());
	// We know this will be same as item length.
	REQUIRE(item1Ptr->GetBufferLength() == 5);
	REQUIRE(item1Ptr->GetLength() == 5);
	REQUIRE(item1Ptr->GetId() == FooItem::ItemId::DATA);
	REQUIRE(item1Ptr->GetFlags() == 0b1000);
	REQUIRE(item1Ptr->GetValueLength() == 3);
	REQUIRE(item1Ptr->GetValue()[0] == 0xAA);
	REQUIRE(item1Ptr->GetValue()[1] == 0xBB);
	REQUIRE(item1Ptr->GetValue()[2] == 0xCC);
	REQUIRE(
	  helpers::areBuffersEqual(item1Ptr->GetBuffer(), item1Ptr->GetLength(), newBuffer1 + 4, 5) == true);

	REQUIRE(item2Ptr == fooPacket->GetItem(1).get());
	// We know this will be same as item length.
	REQUIRE(item2Ptr->GetBufferLength() == 4);
	REQUIRE(item2Ptr->GetLength() == 4);
	REQUIRE(item2Ptr->GetId() == FooItem::ItemId::EVENT);
	REQUIRE(item2Ptr->GetFlags() == 0b1111);
	REQUIRE(item2Ptr->GetValueLength() == 2);
	REQUIRE(item2Ptr->GetValue()[0] == 0xAB);
	REQUIRE(item2Ptr->GetValue()[1] == 0xCD);
	REQUIRE(
	  helpers::areBuffersEqual(item2Ptr->GetBuffer(), item2Ptr->GetLength(), newBuffer1 + 4 + 5, 4) ==
	  true);

	REQUIRE(!fooPacket->GetItem(2));

	/* Clone FooPacket into another buffer. */

	uint8_t newBuffer2[100];

	std::memset(newBuffer2, 0xFF, sizeof(newBuffer2));

	auto* previousBuffer      = fooPacket->GetBuffer();
	auto previousBufferLength = fooPacket->GetBufferLength();

	// FooPacket::Clone() returns a unique_ptr<Serializable>. We need to release
	// its pointer, cast it to FooPacket*, and then create a unique_ptr<FooPacket>
	// with it.
	auto* clonedFooPacketPtr =
	  static_cast<FooPacket*>(fooPacket->Clone(newBuffer2, sizeof(newBuffer2)).release());
	auto clonedFooPacket = std::unique_ptr<FooPacket>(clonedFooPacketPtr);

	// Compare the buffers of the original FooPacket and the cloned one.
	REQUIRE(
	  helpers::areBuffersEqual(
	    clonedFooPacket->GetBuffer(), clonedFooPacket->GetLength(), newBuffer1, fooPacket->GetLength()) ==
	  true);

	// Once done fill the original buffer with 1s (this is, we are running original
	// FooPacket despite it still exists since we have jsut cloned it).
	std::memset(const_cast<uint8_t*>(previousBuffer), 0xFF, previousBufferLength);

	REQUIRE(clonedFooPacket->GetBuffer() == newBuffer2);
	REQUIRE(clonedFooPacket->GetBufferLength() == 100);
	// Header (4 bytes) + items (9 bytes) + padding (3 bytes);
	REQUIRE(clonedFooPacket->GetLength() == 16);
	REQUIRE(Utils::Byte::IsPaddedTo4Bytes(clonedFooPacket->GetLength()) == true);
	REQUIRE(clonedFooPacket->GetType() == 125);
	REQUIRE(clonedFooPacket->HasAppendix() == false);
	REQUIRE(clonedFooPacket->GetAppendix() == 0u);
	REQUIRE(clonedFooPacket->HasItems() == true);
	REQUIRE(clonedFooPacket->GetItemsCount() == 2);

	auto& clonedItem1 = clonedFooPacket->GetItem(0);

	REQUIRE(clonedItem1->GetBufferLength() == 5);
	REQUIRE(clonedItem1->GetLength() == 5);
	REQUIRE(clonedItem1->GetId() == FooItem::ItemId::DATA);
	REQUIRE(clonedItem1->GetFlags() == 0b1000);
	REQUIRE(clonedItem1->GetValueLength() == 3);
	REQUIRE(clonedItem1->GetValue()[0] == 0xAA);
	REQUIRE(clonedItem1->GetValue()[1] == 0xBB);
	REQUIRE(clonedItem1->GetValue()[2] == 0xCC);
	REQUIRE(
	  helpers::areBuffersEqual(
	    clonedItem1->GetBuffer(), clonedItem1->GetLength(), newBuffer2 + 4, 5) == true);

	auto& clonedItem2 = clonedFooPacket->GetItem(1);

	REQUIRE(clonedItem2->GetBufferLength() == 4);
	REQUIRE(clonedItem2->GetLength() == 4);
	REQUIRE(clonedItem2->GetId() == FooItem::ItemId::EVENT);
	REQUIRE(clonedItem2->GetFlags() == 0b1111);
	REQUIRE(clonedItem2->GetValueLength() == 2);
	REQUIRE(clonedItem2->GetValue()[0] == 0xAB);
	REQUIRE(clonedItem2->GetValue()[1] == 0xCD);
	REQUIRE(
	  helpers::areBuffersEqual(
	    clonedItem2->GetBuffer(), clonedItem2->GetLength(), newBuffer2 + 4 + 5, 4) == true);

	REQUIRE(!clonedFooPacket->GetItem(2));
}

SCENARIO("parse FooItem", "[rtc][serializable]")
{
	// clang-format off
	uint8_t buffer[] =
	{
		// Id:1, Flags:0b1110, Value Length:2, Value: 0x12EF
		0b00011110, 0x02, 0x12, 0xEF
	};
	// clang-format on

	auto item = FooItem::Parse(buffer, sizeof(buffer));

	REQUIRE(sizeof(buffer) == 4);
	REQUIRE(item);
	REQUIRE(item->GetBuffer() == buffer);
	REQUIRE(item->GetBufferLength() == 4);
	REQUIRE(item->GetLength() == 4);
	REQUIRE(item->GetId() == FooItem::ItemId::DATA);
	REQUIRE(item->GetFlags() == 0b1110);
	REQUIRE(item->GetValueLength() == 2);
	REQUIRE(item->GetValue()[0] == 0x12);
	REQUIRE(item->GetValue()[1] == 0xEF);
	REQUIRE(helpers::areBuffersEqual(item->GetBuffer(), item->GetLength(), buffer, 4) == true);

	/* Cast to DataFooItem (we know that item has id DATA. */

	// We need to transfer ownership of the underlying FooItem* pointer to the
	// new FooDataItem unique pointer, so we must use item.release() rather than
	// item.get() (which would produce SEGFAULT due to double free).
	auto dataItem = std::unique_ptr<FooDataItem>(static_cast<FooDataItem*>(item.release()));

	// item unique pointer no longer holds the pointer.
	REQUIRE(!item);
	REQUIRE(dataItem);
	REQUIRE(dataItem->GetBuffer() == buffer);
	REQUIRE(dataItem->GetBufferLength() == 4);
	REQUIRE(dataItem->GetLength() == 4);
	REQUIRE(dataItem->GetId() == FooItem::ItemId::DATA);
	REQUIRE(dataItem->GetFlags() == 0b1110);
	REQUIRE(dataItem->GetValueLength() == 2);
	REQUIRE(dataItem->GetValue()[0] == 0x12);
	REQUIRE(dataItem->GetValue()[1] == 0xEF);
	REQUIRE(dataItem->GetNumber() == 0x12EF);
	REQUIRE(helpers::areBuffersEqual(dataItem->GetBuffer(), dataItem->GetLength(), buffer, 4) == true);
}

SCENARIO("parse FooItem by passing to it a buffer larger than the length of the item", "[rtc][serializable]")
{
	// Item length is 7 but given buffer is 8 bytes. Not a problem.
	// clang-format off
	uint8_t buffer[] =
	{
		// Id:1, Flags:0b0000, Value Length:5, Value: 0xFFFFFFFFFF
		0b00010000, 0x05, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0x00
	};
	// clang-format on

	auto item = FooItem::Parse(buffer, sizeof(buffer));

	REQUIRE(sizeof(buffer) == 8);
	REQUIRE(item);
	REQUIRE(item->GetBuffer() == buffer);
	REQUIRE(item->GetBufferLength() == 8);
	REQUIRE(item->GetLength() == 7);
	REQUIRE(item->GetId() == FooItem::ItemId::DATA);
	REQUIRE(item->GetFlags() == 0b0000);
	REQUIRE(item->GetValueLength() == 5);
	REQUIRE(item->GetValue()[0] == 0xFF);
	REQUIRE(item->GetValue()[1] == 0xFF);
	REQUIRE(item->GetValue()[2] == 0xFF);
	REQUIRE(item->GetValue()[3] == 0xFF);
	REQUIRE(item->GetValue()[4] == 0xFF);
	REQUIRE(helpers::areBuffersEqual(item->GetBuffer(), item->GetLength(), buffer, 7) == true);
}

SCENARIO("parse invalid FooItem with buffer too small", "[rtc][serializable]")
{
	// Item length should be 7 but given buffer is only 6 bytes.
	// clang-format off
	uint8_t buffer[] =
	{
		// Id:1, Flags:0b0000, Value Length:5
		0b00010000, 0x05, 0xFF, 0xFF,
		0xFF, 0xFF
	};
	// clang-format on

	auto item = FooItem::Parse(buffer, sizeof(buffer));

	REQUIRE(sizeof(buffer) == 6);
	REQUIRE(!item);
}

SCENARIO("parse invalid DataFooItem with too small buffer", "[rtc][serializable]")
{
	// clang-format off
	uint8_t buffer[] =
	{
		// Id:1, Flags:0b1111, Value Length:2, Value: 0xAB
		0b00011111, 0x02, 0xAB
	};
	// clang-format on

	auto item = FooDataItem::Parse(buffer, sizeof(buffer));

	REQUIRE(sizeof(buffer) == 3);
	REQUIRE(!item);
}

SCENARIO("parse invalid DataFooItem with wrong value length", "[rtc][serializable]")
{
	// clang-format off
	uint8_t buffer[] =
	{
		// Id:1, Flags:0b1111, Value Length:1, Value: 0xAB
		0b00011111, 0x03, 0xAB, 0xCD,
		0xEF
	};
	// clang-format on

	auto item = FooDataItem::Parse(buffer, sizeof(buffer));

	REQUIRE(sizeof(buffer) == 5);
	REQUIRE(!item);
}

SCENARIO("parse invalid DataFooItem with wrong id", "[rtc][serializable]")
{
	// clang-format off
	uint8_t buffer[] =
	{
		// Id:3, Flags:0b1111, Value Length:1, Value: 0xABCD
		0b00111111, 0x02, 0xAB, 0xCD
	};
	// clang-format on

	auto item = FooDataItem::Parse(buffer, sizeof(buffer));

	REQUIRE(sizeof(buffer) == 4);
	REQUIRE(!item);
}

SCENARIO("create and modify FooItem", "[rtc][serializable]")
{
	// Max length of a FooItem is 17 bytes.
	uint8_t buffer[17];
	uint8_t value[] = { 0x11, 0x22, 0x33 };

	// Let's fill the buffer with whatever (it should be overriden by
	// FooItem:Factory()).
	std::memset(buffer, 0xFF, sizeof(buffer));

	auto item = FooItem::Factory(
	  buffer, sizeof(buffer), /*id*/ FooItem::ItemId::DATA, /*flags*/ 0b1010, value, sizeof(value));

	REQUIRE(sizeof(buffer) == 17);
	REQUIRE(sizeof(value) == 3);
	REQUIRE(item);
	REQUIRE(item->GetBuffer() == buffer);
	REQUIRE(item->GetBufferLength() == 17);
	REQUIRE(item->GetLength() == 5);
	REQUIRE(item->GetId() == FooItem::ItemId::DATA);
	REQUIRE(item->GetFlags() == 0b1010);
	REQUIRE(item->GetValueLength() == 3);
	REQUIRE(item->GetValue()[0] == 0x11);
	REQUIRE(item->GetValue()[1] == 0x22);
	REQUIRE(item->GetValue()[2] == 0x33);
	REQUIRE(helpers::areBuffersEqual(item->GetBuffer(), item->GetLength(), buffer, 5) == true);
	REQUIRE(helpers::areBuffersEqual(item->GetValue(), item->GetValueLength(), value, 3) == true);

	/* Modify FooItem. */

	uint8_t newValue[] = { 0xFF, 0xEE, 0xDD, 0xCC };

	item->SetId(FooItem::ItemId::CONTROL);
	item->SetFlags(0b1111);
	item->SetValue(newValue, sizeof(newValue));

	REQUIRE(sizeof(newValue) == 4);
	REQUIRE(item->GetBuffer() == buffer);
	REQUIRE(item->GetBufferLength() == 17);
	REQUIRE(item->GetLength() == 6);
	REQUIRE(item->GetId() == FooItem::ItemId::CONTROL);
	REQUIRE(item->GetFlags() == 0b1111);
	REQUIRE(item->GetValueLength() == 4);
	REQUIRE(item->GetValue()[0] == 0xFF);
	REQUIRE(item->GetValue()[1] == 0xEE);
	REQUIRE(item->GetValue()[2] == 0xDD);
	REQUIRE(item->GetValue()[3] == 0xCC);
	REQUIRE(helpers::areBuffersEqual(item->GetBuffer(), item->GetLength(), buffer, 6) == true);
	REQUIRE(helpers::areBuffersEqual(item->GetValue(), item->GetValueLength(), newValue, 4) == true);

	/* Serialize FooItem into another buffer. */

	uint8_t newBuffer1[17];

	std::memset(newBuffer1, 0xFF, sizeof(newBuffer1));

	item->Serialize(newBuffer1, sizeof(newBuffer1));

	// Compare new and old buffers.
	REQUIRE(helpers::areBuffersEqual(item->GetBuffer(), item->GetLength(), buffer, 6));

	// Once done fill the old buffer with 1s.
	std::memset(buffer, 0xFF, sizeof(buffer));

	REQUIRE(item->GetBuffer() == newBuffer1);
	REQUIRE(item->GetBufferLength() == 17);
	REQUIRE(item->GetLength() == 6);
	REQUIRE(item->GetId() == FooItem::ItemId::CONTROL);
	REQUIRE(item->GetFlags() == 0b1111);
	REQUIRE(item->GetValueLength() == 4);
	REQUIRE(item->GetValue()[0] == 0xFF);
	REQUIRE(item->GetValue()[1] == 0xEE);
	REQUIRE(item->GetValue()[2] == 0xDD);
	REQUIRE(item->GetValue()[3] == 0xCC);
	REQUIRE(helpers::areBuffersEqual(item->GetBuffer(), item->GetLength(), newBuffer1, 6) == true);
	REQUIRE(helpers::areBuffersEqual(item->GetValue(), item->GetValueLength(), newValue, 4) == true);

	/* Clone FooItem into another buffer. */

	uint8_t newBuffer2[100];

	std::memset(newBuffer2, 0xFF, sizeof(newBuffer2));

	auto* previousBuffer      = item->GetBuffer();
	auto previousBufferLength = item->GetBufferLength();

	// FooItem::Clone() returns a unique_ptr<Serializable>. We need to release
	// its pointer, cast it to FooItem*, and then create a unique_ptr<FooItem>
	// with it.
	auto* clonedItemPtr = static_cast<FooItem*>(item->Clone(newBuffer2, sizeof(newBuffer2)).release());
	auto clonedItem = std::unique_ptr<FooItem>(clonedItemPtr);

	// Compare the buffers of the original FooItem and the cloned one.
	REQUIRE(
	  helpers::areBuffersEqual(
	    clonedItem->GetBuffer(), clonedItem->GetLength(), newBuffer1, item->GetLength()) == true);

	// Once done fill the original buffer with 1s (this is, we are running original
	// FooItem despite it still exists since we have jsut cloned it).
	std::memset(const_cast<uint8_t*>(previousBuffer), 0xFF, previousBufferLength);

	REQUIRE(clonedItem->GetBuffer() == newBuffer2);
	REQUIRE(clonedItem->GetBufferLength() == 100);
	REQUIRE(clonedItem->GetLength() == 6);
	REQUIRE(clonedItem->GetId() == FooItem::ItemId::CONTROL);
	REQUIRE(clonedItem->GetFlags() == 0b1111);
	REQUIRE(clonedItem->GetValueLength() == 4);
	REQUIRE(clonedItem->GetValue()[0] == 0xFF);
	REQUIRE(clonedItem->GetValue()[1] == 0xEE);
	REQUIRE(clonedItem->GetValue()[2] == 0xDD);
	REQUIRE(clonedItem->GetValue()[3] == 0xCC);
	REQUIRE(
	  helpers::areBuffersEqual(clonedItem->GetBuffer(), clonedItem->GetLength(), newBuffer2, 6) == true);
	REQUIRE(
	  helpers::areBuffersEqual(clonedItem->GetValue(), clonedItem->GetValueLength(), newValue, 4) ==
	  true);
}
