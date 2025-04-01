// TODO: REMOVE
#define MS_CLASS "RTC::TestSerializable"
#define MS_LOG_DEV_LEVEL 3

#include "common.hpp"
// TODO: REMOVE
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "Utils.hpp"
#include "helpers.hpp"
#include "RTC/Serializable.hpp"
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
		// Item 1: Id:9, Flags:0b0101, Length:2, Value: 0x1234
		0b10010101, 0x02, 0x12, 0x34,
		// Item 2: Id:7, Flags:0b0011, Length:5, , Value: 0xA987654321.
		0b01110011, 0x05, 0xA9, 0x87,
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
	REQUIRE(item1->GetId() == 9);
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
	REQUIRE(item2->GetId() == 7);
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
		// Item 1: Id:9, Flags:0b0101, Length:2, Value: 0x1234
		0b10010101, 0x02, 0x12, 0x34,
		// Item 2: Id:7, Flags:0b0011, Length:5, , Value: 0xA987654321.
		0b01110011, 0x05, 0xA9, 0x87,
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

SCENARIO("create and modify FooPacket", "[rtc][serializable]")
{
	uint8_t buffer[256];
	uint8_t itemBuffer[17];

	std::memset(buffer, 0xFF, sizeof(buffer));
	std::memset(itemBuffer, 0xFF, sizeof(itemBuffer));

	auto fooPacket = FooPacket::Factory(buffer, sizeof(buffer), /*type*/ 55);

	MS_DEBUG_DEV("***** fooPacket, initial");
	fooPacket->Dump();

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

	MS_DEBUG_DEV("***** fooPacket, set type and add appendix");
	fooPacket->Dump();

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

	MS_DEBUG_DEV("***** fooPacket, remove appendix");
	fooPacket->Dump();

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

	/* Add an Item. */

	uint8_t item1Value[] = { 0xAA, 0xBB, 0xCC };
	uint8_t item2Value[] = { 0xAB, 0xCD };

	// Item 1 (5 bytes).
	auto item1 = FooItem::Factory(
	  itemBuffer, sizeof(itemBuffer), /*id*/ 1, /*flags*/ 0b1000, item1Value, sizeof(item1Value));

	// Hold the item1 pointer for tests below.
	auto* item1Ptr = item1.get();

	fooPacket->AddItem(std::move(item1));

	// Item 2 (4 bytes).
	auto item2 = FooItem::Factory(
	  itemBuffer, sizeof(itemBuffer), /*id*/ 2, /*flags*/ 0b1001, item2Value, sizeof(item2Value));

	// Hold the item2 pointer for tests below.
	auto* item2Ptr = item2.get();

	fooPacket->AddItem(std::move(item2));

	MS_DEBUG_DEV("***** fooPacket, add items");
	fooPacket->Dump();

	auto foo2 = FooPacket::Parse(fooPacket->GetBuffer(), fooPacket->GetLength());
	MS_DEBUG_DEV("*****  foo2, parsed from fooPacket");
	foo2->Dump();

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
	REQUIRE(item1Ptr->GetId() == 1);
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
	REQUIRE(item2Ptr->GetId() == 2);
	REQUIRE(item2Ptr->GetFlags() == 0b1001);
	REQUIRE(item2Ptr->GetValueLength() == 2);
	REQUIRE(item2Ptr->GetValue()[0] == 0xAB);
	REQUIRE(item2Ptr->GetValue()[1] == 0xCD);
	REQUIRE(
	  helpers::areBuffersEqual(item2Ptr->GetBuffer(), item2Ptr->GetLength(), buffer + 4 + 5, 4) == true);

	REQUIRE(!fooPacket->GetItem(2));

	/* Add Appendix. */

	fooPacket->SetAppendix(666u);

	MS_DEBUG_DEV("***** fooPacket, keep items and add appendix");
	fooPacket->Dump();

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
	REQUIRE(item1Ptr->GetId() == 1);
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
	REQUIRE(item2Ptr->GetId() == 2);
	REQUIRE(item2Ptr->GetFlags() == 0b1001);
	REQUIRE(item2Ptr->GetValueLength() == 2);
	REQUIRE(item2Ptr->GetValue()[0] == 0xAB);
	REQUIRE(item2Ptr->GetValue()[1] == 0xCD);
	REQUIRE(
	  helpers::areBuffersEqual(item2Ptr->GetBuffer(), item2Ptr->GetLength(), buffer + 4 + 4 + 5, 4) ==
	  true);

	REQUIRE(!fooPacket->GetItem(2));

	/* Remove Appendix and change flags of Item 2. */

	fooPacket->SetAppendix(0u);
	fooPacket->GetItem(1)->SetFlags(0b1111);

	MS_DEBUG_DEV("***** fooPacket, keep items and remove appendix");
	fooPacket->Dump();

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
	REQUIRE(item1Ptr->GetId() == 1);
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
	REQUIRE(item2Ptr->GetId() == 2);
	REQUIRE(item2Ptr->GetFlags() == 0b1111);
	REQUIRE(item2Ptr->GetValueLength() == 2);
	REQUIRE(item2Ptr->GetValue()[0] == 0xAB);
	REQUIRE(item2Ptr->GetValue()[1] == 0xCD);
	REQUIRE(
	  helpers::areBuffersEqual(item2Ptr->GetBuffer(), item2Ptr->GetLength(), buffer + 4 + 5, 4) == true);

	REQUIRE(!fooPacket->GetItem(2));

	/* Serialize FooPacket into another buffer. */

	uint8_t newBuffer[256];

	std::memset(newBuffer, 0xFF, sizeof(newBuffer));

	fooPacket->Serialize(newBuffer, sizeof(newBuffer));

	MS_DEBUG_DEV("***** serialize fooPacket");
	fooPacket->Dump();

	MS_DUMP_DATA(buffer, 16);
	MS_DUMP_DATA(newBuffer, 16);

	// Compare new and old buffers.
	REQUIRE(helpers::areBuffersEqual(newBuffer, 16, buffer, 16) == true);

	// Once done fill the old buffer with 1s.
	std::memset(buffer, 0xFF, sizeof(buffer));

	REQUIRE(fooPacket->GetBuffer() == newBuffer);
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
	  helpers::areBuffersEqual(fooPacket->GetBuffer(), fooPacket->GetLength(), newBuffer, 16) == true);

	REQUIRE(item1Ptr == fooPacket->GetItem(0).get());
	// We know this will be same as item length.
	REQUIRE(item1Ptr->GetBufferLength() == 5);
	REQUIRE(item1Ptr->GetLength() == 5);
	REQUIRE(item1Ptr->GetId() == 1);
	REQUIRE(item1Ptr->GetFlags() == 0b1000);
	REQUIRE(item1Ptr->GetValueLength() == 3);
	REQUIRE(item1Ptr->GetValue()[0] == 0xAA);
	REQUIRE(item1Ptr->GetValue()[1] == 0xBB);
	REQUIRE(item1Ptr->GetValue()[2] == 0xCC);
	REQUIRE(
	  helpers::areBuffersEqual(item1Ptr->GetBuffer(), item1Ptr->GetLength(), newBuffer + 4, 5) == true);

	REQUIRE(item2Ptr == fooPacket->GetItem(1).get());
	// We know this will be same as item length.
	REQUIRE(item2Ptr->GetBufferLength() == 4);
	REQUIRE(item2Ptr->GetLength() == 4);
	REQUIRE(item2Ptr->GetId() == 2);
	REQUIRE(item2Ptr->GetFlags() == 0b1111);
	REQUIRE(item2Ptr->GetValueLength() == 2);
	REQUIRE(item2Ptr->GetValue()[0] == 0xAB);
	REQUIRE(item2Ptr->GetValue()[1] == 0xCD);
	REQUIRE(
	  helpers::areBuffersEqual(item2Ptr->GetBuffer(), item2Ptr->GetLength(), newBuffer + 4 + 5, 4) ==
	  true);

	REQUIRE(!fooPacket->GetItem(2));
}

SCENARIO("parse FooItem item", "[rtc][serializable]")
{
	// clang-format off
	uint8_t buffer[] =
	{
		// Item 1: Id:10, Flags:0b1111, Value Length:1, Value: 0xEE
		0b10101111, 0x01, 0xEE
	};
	// clang-format on

	auto item = FooItem::Parse(buffer, sizeof(buffer));

	REQUIRE(sizeof(buffer) == 3);
	REQUIRE(item);
	REQUIRE(item->GetBuffer() == buffer);
	REQUIRE(item->GetBufferLength() == 3);
	REQUIRE(item->GetLength() == 3);
	REQUIRE(item->GetId() == 10);
	REQUIRE(item->GetFlags() == 0b1111);
	REQUIRE(item->GetValueLength() == 1);
	REQUIRE(item->GetValue()[0] == 0xEE);
	REQUIRE(helpers::areBuffersEqual(item->GetBuffer(), item->GetLength(), buffer, 3) == true);
}

SCENARIO("parse FooItem by passing to it a buffer larger than the length of the item", "[rtc][serializable]")
{
	// Item length is 7 but given buffer is 8 bytes. Not a problem.
	//
	// clang-format off
	uint8_t buffer[] =
	{
		// Item 1: Id:1, Flags:0b0000, Value Length:5, Value: 0xFFFFFFFFFF
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
	REQUIRE(item->GetId() == 1);
	REQUIRE(item->GetFlags() == 0b0000);
	REQUIRE(item->GetValueLength() == 5);
	REQUIRE(item->GetValue()[0] == 0xFF);
	REQUIRE(item->GetValue()[1] == 0xFF);
	REQUIRE(item->GetValue()[2] == 0xFF);
	REQUIRE(item->GetValue()[3] == 0xFF);
	REQUIRE(item->GetValue()[4] == 0xFF);
	REQUIRE(helpers::areBuffersEqual(item->GetBuffer(), item->GetLength(), buffer, 7) == true);
}

SCENARIO("parse invalid FooItem item", "[rtc][serializable]")
{
	SECTION("buffer too small")
	{
		// Item length should be 7 but given buffer is only 6 bytes.

		// clang-format off
		uint8_t buffer[] =
		{
			// Item 1: Id:1, Flags:0b0000, Value Length:5
			0b00010000, 0x05, 0xFF, 0xFF,
			0xFF, 0xFF
		};
		// clang-format on

		auto item = FooItem::Parse(buffer, sizeof(buffer));

		REQUIRE(sizeof(buffer) == 6);
		REQUIRE(!item);
	}
}

SCENARIO("create and modify FooItem item", "[rtc][serializable]")
{
	// Max length of a FooItem is 17 bytes.
	uint8_t buffer[17];
	uint8_t value[] = { 0x11, 0x22, 0x33 };

	// Let's fill the buffer with whatever (it should be overriden by
	// FooItem:Factory()).
	std::memset(buffer, 0xFF, sizeof(buffer));

	auto item =
	  FooItem::Factory(buffer, sizeof(buffer), /*id*/ 9, /*flags*/ 0b1010, value, sizeof(value));

	REQUIRE(sizeof(buffer) == 17);
	REQUIRE(sizeof(value) == 3);
	REQUIRE(item);
	REQUIRE(item->GetBuffer() == buffer);
	REQUIRE(item->GetBufferLength() == 17);
	REQUIRE(item->GetLength() == 5);
	REQUIRE(item->GetId() == 9);
	REQUIRE(item->GetFlags() == 0b1010);
	REQUIRE(item->GetValueLength() == 3);
	REQUIRE(item->GetValue()[0] == 0x11);
	REQUIRE(item->GetValue()[1] == 0x22);
	REQUIRE(item->GetValue()[2] == 0x33);
	REQUIRE(helpers::areBuffersEqual(item->GetBuffer(), item->GetLength(), buffer, 5) == true);
	REQUIRE(helpers::areBuffersEqual(item->GetValue(), item->GetValueLength(), value, 3) == true);

	/* Modify Item. */

	uint8_t newValue[] = { 0xFF, 0xEE, 0xDD, 0xCC };

	item->SetId(14);
	item->SetFlags(0b1111);
	item->SetValue(newValue, sizeof(newValue));

	REQUIRE(sizeof(newValue) == 4);
	REQUIRE(item->GetBuffer() == buffer);
	REQUIRE(item->GetBufferLength() == 17);
	REQUIRE(item->GetLength() == 6);
	REQUIRE(item->GetId() == 14);
	REQUIRE(item->GetFlags() == 0b1111);
	REQUIRE(item->GetValueLength() == 4);
	REQUIRE(item->GetValue()[0] == 0xFF);
	REQUIRE(item->GetValue()[1] == 0xEE);
	REQUIRE(item->GetValue()[2] == 0xDD);
	REQUIRE(item->GetValue()[3] == 0xCC);
	REQUIRE(helpers::areBuffersEqual(item->GetBuffer(), item->GetLength(), buffer, 6) == true);
	REQUIRE(helpers::areBuffersEqual(item->GetValue(), item->GetValueLength(), newValue, 4) == true);

	/* Serialize Item into another buffer. */

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
	REQUIRE(item->GetId() == 14);
	REQUIRE(item->GetFlags() == 0b1111);
	REQUIRE(item->GetValueLength() == 4);
	REQUIRE(item->GetValue()[0] == 0xFF);
	REQUIRE(item->GetValue()[1] == 0xEE);
	REQUIRE(item->GetValue()[2] == 0xDD);
	REQUIRE(item->GetValue()[3] == 0xCC);
	REQUIRE(helpers::areBuffersEqual(item->GetBuffer(), item->GetLength(), newBuffer1, 6) == true);
	REQUIRE(helpers::areBuffersEqual(item->GetValue(), item->GetValueLength(), newValue, 4) == true);

	/* Clone Item into another buffer. */

	uint8_t newBuffer2[100];

	std::memset(newBuffer2, 0xFF, sizeof(newBuffer2));

	auto* previousBuffer      = item->GetBuffer();
	auto previousBufferLength = item->GetBufferLength();

	std::unique_ptr<Serializable> genericClonedItem = item->Clone(newBuffer2, sizeof(newBuffer2));
	std::unique_ptr<FooItem> clonedItem =
	  std::unique_ptr<FooItem>(static_cast<FooItem*>(genericClonedItem.release()));

	// Compare the buffers of the original item and the cloned one.
	REQUIRE(
	  helpers::areBuffersEqual(
	    clonedItem->GetBuffer(), clonedItem->GetLength(), newBuffer1, item->GetLength()) == true);

	// Once done fill the original buffer with 1s (this is, we are running original
	// Item despite it still exists since we have jsut cloned it).
	std::memset(const_cast<uint8_t*>(previousBuffer), 0xFF, previousBufferLength);

	REQUIRE(clonedItem->GetBuffer() == newBuffer2);
	REQUIRE(clonedItem->GetBufferLength() == 100);
	REQUIRE(clonedItem->GetLength() == 6);
	REQUIRE(clonedItem->GetId() == 14);
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
