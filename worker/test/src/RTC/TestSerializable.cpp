#include "common.hpp"
#include "MediaSoupErrors.hpp"
#include "Utils.hpp"
#include "helpers.hpp"
#include "RTC/Serializable.hpp"
#include "RTC/TestSerializable/FooItem.hpp"
#include "RTC/TestSerializable/FooNumericItem.hpp"
#include "RTC/TestSerializable/FooPacket.hpp"
#include "RTC/TestSerializable/FooTextItem.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memset()
#include <string>
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
		// FooItem 1: Id:1 (NUMERIC), Flags:0b0101, Length:2, Number: 0x1234
		0b00010101, 0x02, 0x12, 0x34,
		// FooItem 2: Id:2 (TEXT), Flags:0b0011, Length:5, , Text: 0xE282AC2B24 ("€+$")
		0b00100011, 0x05, 0xE2, 0x82,
		// 1 byte of padding.
		0xAC, 0x2B, 0x24, 0x00
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

	// auto& item1 = fooPacket->GetItem(0);

	// REQUIRE(item1);
	// REQUIRE(item1->GetBuffer() == buffer + 8);
	// REQUIRE(item1->GetBufferLength() == 4);
	// REQUIRE(item1->GetLength() == 4);
	// REQUIRE(item1->GetId() == FooItem::ItemId::NUMERIC);
	// REQUIRE(item1->GetFlags() == 0b0101);
	// REQUIRE(item1->GetNumber() == 0x1234);
	// REQUIRE(helpers::areBuffersEqual(item1->GetBuffer(), item1->GetLength(), buffer + 8, 4) == true);

	// auto& item2 = fooPacket->GetItem(1);

	// REQUIRE(item2);
	// REQUIRE(item2->GetBuffer() == buffer + 12);
	// // Buffer length in item 2 must be 7 since that's the remaining space from
	// // the first byte of item 2 until available packet length (padding excluded).
	// REQUIRE(item2->GetBufferLength() == 7);
	// REQUIRE(item2->GetLength() == 7);
	// REQUIRE(item2->GetId() == FooItem::ItemId::TEXT);
	// REQUIRE(item2->GetFlags() == 0b0011);
	// REQUIRE(item1->GetText() == 0xE282AC2B24);
	// REQUIRE(helpers::areBuffersEqual(item2->GetBuffer(), item2->GetLength(), buffer + 12, 7) == true);

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
		// FooItem 1: Id:1 (NUMERIC) Flags:0b0101, Length:2, Number: 0x1234
		0b00010101, 0x02, 0x12, 0x34,
		// FooItem 2: Id:2 (TEXT), Flags:0b0011, Length:5, , Text: 0xA987654321.
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

// SCENARIO("parse invalid FooPacket with FooItem with invalid id", "[rtc][serializable]")
// {
// 	// clang-format off
// 	uint8_t buffer[] =
// 	{
// 		// Type:1, A:0, Length:8
// 		0x01, 0b00000000, 0x00, 0x08,
// 		// FooItem 1: Id:0, Flags:0b0101, Length:2, Value: 0x1234
// 		0b00000101, 0x02, 0x12, 0x34,
// 	};
// 	// clang-format on

// 	auto fooPacket = FooPacket::Parse(buffer, sizeof(buffer));

// 	REQUIRE(sizeof(buffer) == 8);
// 	REQUIRE(!fooPacket);
// }

// SCENARIO("create and modify FooPacket", "[rtc][serializable]")
// {
// 	uint8_t buffer[256];
// 	uint8_t itemBuffer[17];

// 	std::memset(buffer, 0xFF, sizeof(buffer));
// 	std::memset(itemBuffer, 0xFF, sizeof(itemBuffer));

// 	auto fooPacket = FooPacket::Factory(buffer, sizeof(buffer), /*type*/ 55);

// 	REQUIRE(sizeof(buffer) == 256);
// 	REQUIRE(fooPacket);
// 	REQUIRE(fooPacket->GetBuffer() == buffer);
// 	REQUIRE(fooPacket->GetBufferLength() == 256);
// 	// Just the FooPacket header (4 bytes).
// 	REQUIRE(fooPacket->GetLength() == 4);
// 	REQUIRE(Utils::Byte::IsPaddedTo4Bytes(fooPacket->GetLength()) == true);
// 	REQUIRE(fooPacket->GetType() == 55);
// 	REQUIRE(fooPacket->HasAppendix() == false);
// 	REQUIRE(fooPacket->GetAppendix() == 0u);
// 	REQUIRE(fooPacket->HasItems() == false);
// 	REQUIRE(fooPacket->GetItemsCount() == 0);
// 	REQUIRE(helpers::areBuffersEqual(fooPacket->GetBuffer(), fooPacket->GetLength(), buffer, 4) == true);

// 	/* Add Type and Appendix. */

// 	fooPacket->SetType(125);
// 	fooPacket->SetAppendix(0x12345678);

// 	REQUIRE(fooPacket->GetBuffer() == buffer);
// 	REQUIRE(fooPacket->GetBufferLength() == 256);
// 	// Header (4 bytes) + Appendix (4 bytes).
// 	REQUIRE(fooPacket->GetLength() == 8);
// 	REQUIRE(Utils::Byte::IsPaddedTo4Bytes(fooPacket->GetLength()) == true);
// 	REQUIRE(fooPacket->GetType() == 125);
// 	REQUIRE(fooPacket->HasAppendix() == true);
// 	REQUIRE(fooPacket->GetAppendix() == 0x12345678);
// 	REQUIRE(fooPacket->HasItems() == false);
// 	REQUIRE(fooPacket->GetItemsCount() == 0);
// 	REQUIRE(helpers::areBuffersEqual(fooPacket->GetBuffer(), fooPacket->GetLength(), buffer, 8) == true);

// 	/* Remove Appendix. */

// 	fooPacket->SetAppendix(0u);

// 	REQUIRE(fooPacket->GetBuffer() == buffer);
// 	REQUIRE(fooPacket->GetBufferLength() == 256);
// 	// Header (4 bytes).
// 	REQUIRE(fooPacket->GetLength() == 4);
// 	REQUIRE(Utils::Byte::IsPaddedTo4Bytes(fooPacket->GetLength()) == true);
// 	REQUIRE(fooPacket->GetType() == 125);
// 	REQUIRE(fooPacket->HasAppendix() == false);
// 	REQUIRE(fooPacket->GetAppendix() == 0u);
// 	REQUIRE(fooPacket->HasItems() == false);
// 	REQUIRE(fooPacket->GetItemsCount() == 0);
// 	REQUIRE(helpers::areBuffersEqual(fooPacket->GetBuffer(), fooPacket->GetLength(), buffer, 4) == true);

// 	/* Add a FooItem. */

// 	uint8_t item1Value[] = { 0xAA, 0xBB, 0xCC };
// 	uint8_t item2Value[] = { 0xAB, 0xCD };

// 	// FooItem 1 (5 bytes).
// 	auto item1 = FooItem::Factory(
// 	  itemBuffer,
// 	  sizeof(itemBuffer),
// 	  /*id*/ FooItem::ItemId::NUMERIC,
// 	  /*flags*/ 0b1000,
// 	  item1Value,
// 	  sizeof(item1Value));

// 	// Hold item1 pointer for tests below.
// 	auto* item1Ptr = item1.get();

// 	fooPacket->AddItem(std::move(item1));

// 	// FooItem 2 (4 bytes).
// 	auto item2 = FooItem::Factory(
// 	  itemBuffer,
// 	  sizeof(itemBuffer),
// 	  /*id*/ FooItem::ItemId::TEXT,
// 	  /*flags*/ 0b1001,
// 	  item2Value,
// 	  sizeof(item2Value));

// 	// Hold item2 pointer for tests below.
// 	auto* item2Ptr = item2.get();

// 	fooPacket->AddItem(std::move(item2));

// 	auto foo2 = FooPacket::Parse(fooPacket->GetBuffer(), fooPacket->GetLength());

// 	REQUIRE(fooPacket->GetBuffer() == buffer);
// 	REQUIRE(fooPacket->GetBufferLength() == 256);
// 	// Header (4 bytes) + items (9 bytes) + padding (3 bytes).
// 	REQUIRE(fooPacket->GetLength() == 16);
// 	REQUIRE(Utils::Byte::IsPaddedTo4Bytes(fooPacket->GetLength()) == true);
// 	REQUIRE(fooPacket->GetType() == 125);
// 	REQUIRE(fooPacket->HasAppendix() == false);
// 	REQUIRE(fooPacket->GetAppendix() == 0u);
// 	REQUIRE(fooPacket->HasItems() == true);
// 	REQUIRE(fooPacket->GetItemsCount() == 2);
// 	REQUIRE(
// 	  helpers::areBuffersEqual(fooPacket->GetBuffer(), fooPacket->GetLength(), buffer, 16) == true);

// 	REQUIRE(item1Ptr == fooPacket->GetItem(0).get());
// 	// We know this will be same as item length.
// 	REQUIRE(item1Ptr->GetBufferLength() == 5);
// 	REQUIRE(item1Ptr->GetLength() == 5);
// 	REQUIRE(item1Ptr->GetId() == FooItem::ItemId::NUMERIC);
// 	REQUIRE(item1Ptr->GetFlags() == 0b1000);
// 	REQUIRE(item1Ptr->GetValueLength() == 3);
// 	REQUIRE(item1Ptr->GetValue()[0] == 0xAA);
// 	REQUIRE(item1Ptr->GetValue()[1] == 0xBB);
// 	REQUIRE(item1Ptr->GetValue()[2] == 0xCC);
// 	REQUIRE(
// 	  helpers::areBuffersEqual(item1Ptr->GetBuffer(), item1Ptr->GetLength(), buffer + 4, 5) == true);

// 	REQUIRE(item2Ptr == fooPacket->GetItem(1).get());
// 	// We know this will be same as item length.
// 	REQUIRE(item2Ptr->GetBufferLength() == 4);
// 	REQUIRE(item2Ptr->GetLength() == 4);
// 	REQUIRE(item2Ptr->GetId() == FooItem::ItemId::TEXT);
// 	REQUIRE(item2Ptr->GetFlags() == 0b1001);
// 	REQUIRE(item2Ptr->GetValueLength() == 2);
// 	REQUIRE(item2Ptr->GetValue()[0] == 0xAB);
// 	REQUIRE(item2Ptr->GetValue()[1] == 0xCD);
// 	REQUIRE(
// 	  helpers::areBuffersEqual(item2Ptr->GetBuffer(), item2Ptr->GetLength(), buffer + 4 + 5, 4) == true);

// 	REQUIRE(!fooPacket->GetItem(2));

// 	/* Add Appendix. */

// 	fooPacket->SetAppendix(666u);

// 	REQUIRE(fooPacket->GetBuffer() == buffer);
// 	REQUIRE(fooPacket->GetBufferLength() == 256);
// 	// Header (4 bytes) + Appendix (4 bytes) + items (9 bytes) + padding (3
// 	// bytes);
// 	REQUIRE(fooPacket->GetLength() == 20);
// 	REQUIRE(Utils::Byte::IsPaddedTo4Bytes(fooPacket->GetLength()) == true);
// 	REQUIRE(fooPacket->GetType() == 125);
// 	REQUIRE(fooPacket->HasAppendix() == true);
// 	REQUIRE(fooPacket->GetAppendix() == 666u);
// 	REQUIRE(fooPacket->HasItems() == true);
// 	REQUIRE(fooPacket->GetItemsCount() == 2);
// 	REQUIRE(
// 	  helpers::areBuffersEqual(fooPacket->GetBuffer(), fooPacket->GetLength(), buffer, 20) == true);

// 	REQUIRE(item1Ptr == fooPacket->GetItem(0).get());
// 	// We know this will be same as item length.
// 	REQUIRE(item1Ptr->GetBufferLength() == 5);
// 	REQUIRE(item1Ptr->GetLength() == 5);
// 	REQUIRE(item1Ptr->GetId() == FooItem::ItemId::NUMERIC);
// 	REQUIRE(item1Ptr->GetFlags() == 0b1000);
// 	REQUIRE(item1Ptr->GetValueLength() == 3);
// 	REQUIRE(item1Ptr->GetValue()[0] == 0xAA);
// 	REQUIRE(item1Ptr->GetValue()[1] == 0xBB);
// 	REQUIRE(item1Ptr->GetValue()[2] == 0xCC);
// 	REQUIRE(
// 	  helpers::areBuffersEqual(item1Ptr->GetBuffer(), item1Ptr->GetLength(), buffer + 4 + 4, 5) == true);

// 	REQUIRE(item2Ptr == fooPacket->GetItem(1).get());
// 	// We know this will be same as item length.
// 	REQUIRE(item2Ptr->GetBufferLength() == 4);
// 	REQUIRE(item2Ptr->GetLength() == 4);
// 	REQUIRE(item2Ptr->GetId() == FooItem::ItemId::TEXT);
// 	REQUIRE(item2Ptr->GetFlags() == 0b1001);
// 	REQUIRE(item2Ptr->GetValueLength() == 2);
// 	REQUIRE(item2Ptr->GetValue()[0] == 0xAB);
// 	REQUIRE(item2Ptr->GetValue()[1] == 0xCD);
// 	REQUIRE(
// 	  helpers::areBuffersEqual(item2Ptr->GetBuffer(), item2Ptr->GetLength(), buffer + 4 + 4 + 5,
// 4) == 	  true);

// 	REQUIRE(!fooPacket->GetItem(2));

// 	/* Remove Appendix and change flags of FooItem 2. */

// 	fooPacket->SetAppendix(0u);
// 	fooPacket->GetItem(1)->SetFlags(0b1111);

// 	REQUIRE(fooPacket->GetBuffer() == buffer);
// 	REQUIRE(fooPacket->GetBufferLength() == 256);
// 	// Header (4 bytes) + items (9 bytes) + padding (3 bytes);
// 	REQUIRE(fooPacket->GetLength() == 16);
// 	REQUIRE(Utils::Byte::IsPaddedTo4Bytes(fooPacket->GetLength()) == true);
// 	REQUIRE(fooPacket->GetType() == 125);
// 	REQUIRE(fooPacket->HasAppendix() == false);
// 	REQUIRE(fooPacket->GetAppendix() == 0u);
// 	REQUIRE(fooPacket->HasItems() == true);
// 	REQUIRE(fooPacket->GetItemsCount() == 2);
// 	REQUIRE(
// 	  helpers::areBuffersEqual(fooPacket->GetBuffer(), fooPacket->GetLength(), buffer, 16) == true);

// 	REQUIRE(item1Ptr == fooPacket->GetItem(0).get());
// 	// We know this will be same as item length.
// 	REQUIRE(item1Ptr->GetBufferLength() == 5);
// 	REQUIRE(item1Ptr->GetLength() == 5);
// 	REQUIRE(item1Ptr->GetId() == FooItem::ItemId::NUMERIC);
// 	REQUIRE(item1Ptr->GetFlags() == 0b1000);
// 	REQUIRE(item1Ptr->GetValueLength() == 3);
// 	REQUIRE(item1Ptr->GetValue()[0] == 0xAA);
// 	REQUIRE(item1Ptr->GetValue()[1] == 0xBB);
// 	REQUIRE(item1Ptr->GetValue()[2] == 0xCC);
// 	REQUIRE(
// 	  helpers::areBuffersEqual(item1Ptr->GetBuffer(), item1Ptr->GetLength(), buffer + 4, 5) == true);

// 	REQUIRE(item2Ptr == fooPacket->GetItem(1).get());
// 	// We know this will be same as item length.
// 	REQUIRE(item2Ptr->GetBufferLength() == 4);
// 	REQUIRE(item2Ptr->GetLength() == 4);
// 	REQUIRE(item2Ptr->GetId() == FooItem::ItemId::TEXT);
// 	REQUIRE(item2Ptr->GetFlags() == 0b1111);
// 	REQUIRE(item2Ptr->GetValueLength() == 2);
// 	REQUIRE(item2Ptr->GetValue()[0] == 0xAB);
// 	REQUIRE(item2Ptr->GetValue()[1] == 0xCD);
// 	REQUIRE(
// 	  helpers::areBuffersEqual(item2Ptr->GetBuffer(), item2Ptr->GetLength(), buffer + 4 + 5, 4) == true);

// 	REQUIRE(!fooPacket->GetItem(2));

// 	/* Serialize FooPacket into another buffer. */

// 	uint8_t newBuffer1[256];

// 	std::memset(newBuffer1, 0xFF, sizeof(newBuffer1));
//
// 	// Must throw if buffer is too small.
//  REQUIRE_THROWS_AS(fooPacket->Serialize(newBuffer1, fooPacket->GetLength() - 1), MediaSoupTypeError);

// 	fooPacket->Serialize(newBuffer1, sizeof(newBuffer1));

// 	// Compare new and old buffers.
// 	REQUIRE(helpers::areBuffersEqual(fooPacket->GetBuffer(), fooPacket->GetLength(), buffer, 16));

// 	// Once done fill the old buffer with 1s.
// 	std::memset(buffer, 0xFF, sizeof(buffer));

// 	REQUIRE(fooPacket->GetBuffer() == newBuffer1);
// 	REQUIRE(fooPacket->GetBufferLength() == 256);
// 	// Header (4 bytes) + items (9 bytes) + padding (3 bytes);
// 	REQUIRE(fooPacket->GetLength() == 16);
// 	REQUIRE(Utils::Byte::IsPaddedTo4Bytes(fooPacket->GetLength()) == true);
// 	REQUIRE(fooPacket->GetType() == 125);
// 	REQUIRE(fooPacket->HasAppendix() == false);
// 	REQUIRE(fooPacket->GetAppendix() == 0u);
// 	REQUIRE(fooPacket->HasItems() == true);
// 	REQUIRE(fooPacket->GetItemsCount() == 2);
// 	REQUIRE(
// 	  helpers::areBuffersEqual(fooPacket->GetBuffer(), fooPacket->GetLength(), newBuffer1, 16) == true);

// 	REQUIRE(item1Ptr == fooPacket->GetItem(0).get());
// 	// We know this will be same as item length.
// 	REQUIRE(item1Ptr->GetBufferLength() == 5);
// 	REQUIRE(item1Ptr->GetLength() == 5);
// 	REQUIRE(item1Ptr->GetId() == FooItem::ItemId::NUMERIC);
// 	REQUIRE(item1Ptr->GetFlags() == 0b1000);
// 	REQUIRE(item1Ptr->GetValueLength() == 3);
// 	REQUIRE(item1Ptr->GetValue()[0] == 0xAA);
// 	REQUIRE(item1Ptr->GetValue()[1] == 0xBB);
// 	REQUIRE(item1Ptr->GetValue()[2] == 0xCC);
// 	REQUIRE(
// 	  helpers::areBuffersEqual(item1Ptr->GetBuffer(), item1Ptr->GetLength(), newBuffer1 + 4, 5) == true);

// 	REQUIRE(item2Ptr == fooPacket->GetItem(1).get());
// 	// We know this will be same as item length.
// 	REQUIRE(item2Ptr->GetBufferLength() == 4);
// 	REQUIRE(item2Ptr->GetLength() == 4);
// 	REQUIRE(item2Ptr->GetId() == FooItem::ItemId::TEXT);
// 	REQUIRE(item2Ptr->GetFlags() == 0b1111);
// 	REQUIRE(item2Ptr->GetValueLength() == 2);
// 	REQUIRE(item2Ptr->GetValue()[0] == 0xAB);
// 	REQUIRE(item2Ptr->GetValue()[1] == 0xCD);
// 	REQUIRE(
// 	  helpers::areBuffersEqual(item2Ptr->GetBuffer(), item2Ptr->GetLength(), newBuffer1 + 4 + 5,
// 4) == 	  true);

// 	REQUIRE(!fooPacket->GetItem(2));

// 	/* Clone FooPacket into another buffer. */

// 	uint8_t newBuffer2[100];

// 	std::memset(newBuffer2, 0xFF, sizeof(newBuffer2));
//
// 	// Must throw if buffer is too small.
// REQUIRE_THROWS_AS(fooPacket->Clone(newBuffer2, fooPacket->GetLength() - 1), MediaSoupTypeError);

// 	auto* previousBuffer      = fooPacket->GetBuffer();
// 	auto previousBufferLength = fooPacket->GetBufferLength();

// 	// FooPacket::Clone() returns a unique_ptr<Serializable>. We need to release
// 	// its pointer, cast it to FooPacket*, and then create a unique_ptr<FooPacket>
// 	// with it.
// 	auto* clonedFooPacketPtr =
// 	  static_cast<FooPacket*>(fooPacket->Clone(newBuffer2, sizeof(newBuffer2)).release());
// 	auto clonedFooPacket = std::unique_ptr<FooPacket>(clonedFooPacketPtr);

// 	// Compare the buffers of the original FooPacket and the cloned one.
// 	REQUIRE(
// 	  helpers::areBuffersEqual(
// 	    clonedFooPacket->GetBuffer(), clonedFooPacket->GetLength(), newBuffer1,
// fooPacket->GetLength()) == 	  true);

// 	// Once done fill the original buffer with 1s (this is, we are ruining original
// 	// FooPacket despite it still exists since we have jsut cloned it).
// 	std::memset(const_cast<uint8_t*>(previousBuffer), 0xFF, previousBufferLength);

// 	REQUIRE(clonedFooPacket->GetBuffer() == newBuffer2);
// 	REQUIRE(clonedFooPacket->GetBufferLength() == 100);
// 	// Header (4 bytes) + items (9 bytes) + padding (3 bytes);
// 	REQUIRE(clonedFooPacket->GetLength() == 16);
// 	REQUIRE(Utils::Byte::IsPaddedTo4Bytes(clonedFooPacket->GetLength()) == true);
// 	REQUIRE(clonedFooPacket->GetType() == 125);
// 	REQUIRE(clonedFooPacket->HasAppendix() == false);
// 	REQUIRE(clonedFooPacket->GetAppendix() == 0u);
// 	REQUIRE(clonedFooPacket->HasItems() == true);
// 	REQUIRE(clonedFooPacket->GetItemsCount() == 2);

// 	auto& clonedItem1 = clonedFooPacket->GetItem(0);

// 	REQUIRE(clonedItem1->GetBufferLength() == 5);
// 	REQUIRE(clonedItem1->GetLength() == 5);
// 	REQUIRE(clonedItem1->GetId() == FooItem::ItemId::NUMERIC);
// 	REQUIRE(clonedItem1->GetFlags() == 0b1000);
// 	REQUIRE(clonedItem1->GetValueLength() == 3);
// 	REQUIRE(clonedItem1->GetValue()[0] == 0xAA);
// 	REQUIRE(clonedItem1->GetValue()[1] == 0xBB);
// 	REQUIRE(clonedItem1->GetValue()[2] == 0xCC);
// 	REQUIRE(
// 	  helpers::areBuffersEqual(
// 	    clonedItem1->GetBuffer(), clonedItem1->GetLength(), newBuffer2 + 4, 5) == true);

// 	auto& clonedItem2 = clonedFooPacket->GetItem(1);

// 	REQUIRE(clonedItem2->GetBufferLength() == 4);
// 	REQUIRE(clonedItem2->GetLength() == 4);
// 	REQUIRE(clonedItem2->GetId() == FooItem::ItemId::TEXT);
// 	REQUIRE(clonedItem2->GetFlags() == 0b1111);
// 	REQUIRE(clonedItem2->GetValueLength() == 2);
// 	REQUIRE(clonedItem2->GetValue()[0] == 0xAB);
// 	REQUIRE(clonedItem2->GetValue()[1] == 0xCD);
// 	REQUIRE(
// 	  helpers::areBuffersEqual(
// 	    clonedItem2->GetBuffer(), clonedItem2->GetLength(), newBuffer2 + 4 + 5, 4) == true);

// 	REQUIRE(!clonedFooPacket->GetItem(2));
// }

SCENARIO("parse FooNumericItem", "[rtc][serializable]")
{
	// clang-format off
	uint8_t buffer[] =
	{
		// Id:1 (NUMERIC), Flags:0b1110, Value Length:2, Number: 0x12EF
		0b00011110, 0x02, 0x12, 0xEF
	};
	// clang-format on

	auto item = FooNumericItem::Parse(buffer, sizeof(buffer));

	REQUIRE(sizeof(buffer) == 4);
	REQUIRE(item);
	REQUIRE(item->GetBuffer() == buffer);
	REQUIRE(item->GetBufferLength() == 4);
	REQUIRE(item->GetLength() == 4);
	REQUIRE(item->GetId() == FooItem::ItemId::NUMERIC);
	REQUIRE(item->GetFlags() == 0b1110);
	REQUIRE(item->GetNumber() == 0x12EF);
	REQUIRE(helpers::areBuffersEqual(item->GetBuffer(), item->GetLength(), buffer, 4) == true);
}

SCENARIO("parse FooNumericItem by passing a buffer larger than the length of the item", "[rtc][serializable]")
{
	// Item length is 4 but given buffer is 6 bytes. Not a problem.
	// clang-format off
	uint8_t buffer[] =
	{
		// Id:1 (NUMERIC), Flags:0b0000, Value Length:2, Number: 0xFFFF
		0b00010000, 0x02, 0xFF, 0xFF,
		0x00, 0x00
	};
	// clang-format on

	auto item = FooNumericItem::Parse(buffer, sizeof(buffer));

	REQUIRE(sizeof(buffer) == 6);
	REQUIRE(item);
	REQUIRE(item->GetBuffer() == buffer);
	REQUIRE(item->GetBufferLength() == 6);
	REQUIRE(item->GetLength() == 4);
	REQUIRE(item->GetId() == FooItem::ItemId::NUMERIC);
	REQUIRE(item->GetFlags() == 0b0000);
	REQUIRE(item->GetNumber() == 0xFFFF);
	REQUIRE(helpers::areBuffersEqual(item->GetBuffer(), item->GetLength(), buffer, 4) == true);
}

SCENARIO("parse invalid FooNumericItem with too small buffer", "[rtc][serializable]")
{
	// Item length should be 4 but given buffer is only 3 bytes.
	// clang-format off
	uint8_t buffer[] =
	{
		// Id:1 (NUMERIC), Flags:0b1111, Value Length:2, Number: 0xAB (wrong)
		0b00011111, 0x02, 0xAB
	};
	// clang-format on

	auto item = FooNumericItem::Parse(buffer, sizeof(buffer));

	REQUIRE(sizeof(buffer) == 3);
	REQUIRE(!item);
}

SCENARIO("parse invalid FooNumericItem with wrong value length", "[rtc][serializable]")
{
	// clang-format off
	uint8_t buffer[] =
	{
		// Id:1 (NUMERIC), Flags:0b1111, Value Length:1, Number: 0xAB
		0b00011111, 0x03, 0xAB, 0xCD,
		0xEF
	};
	// clang-format on

	auto item = FooNumericItem::Parse(buffer, sizeof(buffer));

	REQUIRE(sizeof(buffer) == 5);
	REQUIRE(!item);
}

SCENARIO("parse invalid FooNumericItem with wrong id", "[rtc][serializable]")
{
	// clang-format off
	uint8_t buffer[] =
	{
		// Id:2 (TEXT), Flags:0b1111, Value Length:2, Text: 0xABCD
		0b00101111, 0x02, 0xAB, 0xCD
	};
	// clang-format on

	auto item = FooNumericItem::Parse(buffer, sizeof(buffer));

	REQUIRE(sizeof(buffer) == 4);
	REQUIRE(!item);
}

SCENARIO("create and modify FooNumericItem", "[rtc][serializable]")
{
	// Max length of a FooItem is 17 bytes.
	uint8_t buffer[17];

	// Let's fill the buffer with whatever (it should be overridden by
	// FooNumericItem:Factory()).
	std::memset(buffer, 0xFF, sizeof(buffer));

	auto item = FooNumericItem::Factory(buffer, sizeof(buffer), /*flags*/ 0b1010, 1111);

	REQUIRE(sizeof(buffer) == 17);
	REQUIRE(item);
	REQUIRE(item->GetBuffer() == buffer);
	REQUIRE(item->GetBufferLength() == 17);
	REQUIRE(item->GetLength() == 4);
	REQUIRE(item->GetId() == FooItem::ItemId::NUMERIC);
	REQUIRE(item->GetFlags() == 0b1010);
	REQUIRE(item->GetNumber() == 1111);
	REQUIRE(helpers::areBuffersEqual(item->GetBuffer(), item->GetLength(), buffer, 4) == true);

	/* Modify FooNumericItem. */

	item->SetNumber(2222);

	REQUIRE(item->GetBuffer() == buffer);
	REQUIRE(item->GetBufferLength() == 17);
	REQUIRE(item->GetLength() == 4);
	REQUIRE(item->GetId() == FooItem::ItemId::NUMERIC);
	REQUIRE(item->GetFlags() == 0b1010);
	REQUIRE(item->GetNumber() == 2222);
	REQUIRE(helpers::areBuffersEqual(item->GetBuffer(), item->GetLength(), buffer, 4) == true);

	/* Serialize FooNumericItem into another buffer. */

	uint8_t newBuffer1[17];

	std::memset(newBuffer1, 0xFF, sizeof(newBuffer1));

	// Must throw if buffer is too small.
	REQUIRE_THROWS_AS(item->Serialize(newBuffer1, item->GetLength() - 1), MediaSoupTypeError);

	item->Serialize(newBuffer1, sizeof(newBuffer1));

	// Compare new and old buffers.
	REQUIRE(helpers::areBuffersEqual(item->GetBuffer(), item->GetLength(), buffer, 4));

	// Once done fill the old buffer with 1s.
	std::memset(buffer, 0xFF, sizeof(buffer));

	REQUIRE(item->GetBuffer() == newBuffer1);
	REQUIRE(item->GetBufferLength() == 17);
	REQUIRE(item->GetLength() == 4);
	REQUIRE(item->GetId() == FooItem::ItemId::NUMERIC);
	REQUIRE(item->GetFlags() == 0b1010);
	REQUIRE(item->GetNumber() == 2222);
	REQUIRE(helpers::areBuffersEqual(item->GetBuffer(), item->GetLength(), newBuffer1, 4) == true);

	/* Clone FooNumericItem into another buffer. */

	uint8_t newBuffer2[100];

	std::memset(newBuffer2, 0xFF, sizeof(newBuffer2));

	// Must throw if buffer is too small.
	REQUIRE_THROWS_AS(item->Clone(newBuffer2, item->GetLength() - 1), MediaSoupTypeError);

	auto* previousBuffer      = item->GetBuffer();
	auto previousBufferLength = item->GetBufferLength();

	// FooNumericItem::Clone() returns a unique_ptr<Serializable>. We need to release
	// its pointer, cast it to FooNumericItem*, and then create a
	// unique_ptr<FooNumericItem> with it.
	auto* clonedItemPtr =
	  static_cast<FooNumericItem*>(item->Clone(newBuffer2, sizeof(newBuffer2)).release());
	auto clonedItem = std::unique_ptr<FooNumericItem>(clonedItemPtr);

	// Compare the buffers of the original FooNumericItem and the cloned one.
	REQUIRE(
	  helpers::areBuffersEqual(
	    clonedItem->GetBuffer(), clonedItem->GetLength(), newBuffer1, item->GetLength()) == true);

	// Once done fill the original buffer with 1s (this is, we are ruining original
	// FooNumericItem despite it still exists since we have jsut cloned it).
	std::memset(const_cast<uint8_t*>(previousBuffer), 0xFF, previousBufferLength);

	REQUIRE(clonedItem->GetBuffer() == newBuffer2);
	REQUIRE(clonedItem->GetBufferLength() == 100);
	REQUIRE(clonedItem->GetLength() == 4);
	REQUIRE(clonedItem->GetId() == FooItem::ItemId::NUMERIC);
	REQUIRE(clonedItem->GetFlags() == 0b1010);
	REQUIRE(clonedItem->GetNumber() == 2222);
	REQUIRE(
	  helpers::areBuffersEqual(clonedItem->GetBuffer(), clonedItem->GetLength(), newBuffer2, 4) == true);
}

SCENARIO("create and modify FooTextItem", "[rtc][serializable]")
{
	uint8_t buffer[40];
	std::string text = "Iñaki"; // 6 bytes.

	// Let's fill the buffer with whatever (it should be overridden by
	// FooTextItem:Factory()).
	std::memset(buffer, 0xFF, sizeof(buffer));

	auto item = FooTextItem::Factory(buffer, sizeof(buffer), /*flags*/ 0b1010, text);

	REQUIRE(sizeof(buffer) == 40);
	REQUIRE(item);
	REQUIRE(item->GetBuffer() == buffer);
	REQUIRE(item->GetBufferLength() == 40);
	REQUIRE(item->GetLength() == 8);
	REQUIRE(item->GetId() == FooItem::ItemId::TEXT);
	REQUIRE(item->GetFlags() == 0b1010);
	REQUIRE(item->GetText() == text);
	REQUIRE(helpers::areBuffersEqual(item->GetBuffer(), item->GetLength(), buffer, 8) == true);

	/* Modify FooTextItem. */

	std::string newText = "œæ€å∫∂"; // 15 bytes.

	item->SetText(newText);

	REQUIRE(item->GetBuffer() == buffer);
	REQUIRE(item->GetBufferLength() == 40);
	REQUIRE(item->GetLength() == 17);
	REQUIRE(item->GetId() == FooItem::ItemId::TEXT);
	REQUIRE(item->GetFlags() == 0b1010);
	REQUIRE(item->GetText() == newText);
	REQUIRE(helpers::areBuffersEqual(item->GetBuffer(), item->GetLength(), buffer, 17) == true);

	/* Serialize FooTextItem into another buffer. */

	uint8_t newBuffer1[22];

	std::memset(newBuffer1, 0xFF, sizeof(newBuffer1));

	// Must throw if buffer is too small.
	REQUIRE_THROWS_AS(item->Serialize(newBuffer1, item->GetLength() - 1), MediaSoupTypeError);

	item->Serialize(newBuffer1, sizeof(newBuffer1));

	// Compare new and old buffers.
	REQUIRE(helpers::areBuffersEqual(item->GetBuffer(), item->GetLength(), buffer, 17));

	// Once done fill the old buffer with 1s.
	std::memset(buffer, 0xFF, sizeof(buffer));

	REQUIRE(item->GetBuffer() == newBuffer1);
	REQUIRE(item->GetBufferLength() == 22);
	REQUIRE(item->GetLength() == 17);
	REQUIRE(item->GetId() == FooItem::ItemId::TEXT);
	REQUIRE(item->GetFlags() == 0b1010);
	REQUIRE(item->GetText() == newText);
	REQUIRE(helpers::areBuffersEqual(item->GetBuffer(), item->GetLength(), newBuffer1, 17) == true);

	/* Clone FooTextItem into another buffer. */

	uint8_t newBuffer2[100];

	std::memset(newBuffer2, 0xFF, sizeof(newBuffer2));

	// Must throw if buffer is too small.
	REQUIRE_THROWS_AS(item->Clone(newBuffer2, item->GetLength() - 1), MediaSoupTypeError);

	auto* previousBuffer      = item->GetBuffer();
	auto previousBufferLength = item->GetBufferLength();

	// FooTextItem::Clone() returns a unique_ptr<Serializable>. We need to release
	// its pointer, cast it to FooTextItem*, and then create a
	// unique_ptr<FooTextItem> with it.
	auto* clonedItemPtr =
	  static_cast<FooTextItem*>(item->Clone(newBuffer2, sizeof(newBuffer2)).release());
	auto clonedItem = std::unique_ptr<FooTextItem>(clonedItemPtr);

	// Compare the buffers of the original FooTextItem and the cloned one.
	REQUIRE(
	  helpers::areBuffersEqual(
	    clonedItem->GetBuffer(), clonedItem->GetLength(), newBuffer1, item->GetLength()) == true);

	// Once done fill the original buffer with 1s (this is, we are ruining original
	// FooTextItem despite it still exists since we have jsut cloned it).
	std::memset(const_cast<uint8_t*>(previousBuffer), 0xFF, previousBufferLength);

	REQUIRE(clonedItem->GetBuffer() == newBuffer2);
	REQUIRE(clonedItem->GetBufferLength() == 100);
	REQUIRE(clonedItem->GetLength() == 17);
	REQUIRE(clonedItem->GetId() == FooItem::ItemId::TEXT);
	REQUIRE(clonedItem->GetFlags() == 0b1010);
	REQUIRE(clonedItem->GetText() == newText);
	REQUIRE(
	  helpers::areBuffersEqual(clonedItem->GetBuffer(), clonedItem->GetLength(), newBuffer2, 17) ==
	  true);
}
