#include "common.hpp"
#include "MediaSoupErrors.hpp"
#include "Utils.hpp"
#include "helpers.hpp"
#include "RTC/Serializable.hpp"
#include "RTC/TestSerializable/FooItem.hpp"
#include "RTC/TestSerializable/FooNumericItem.hpp"
#include "RTC/TestSerializable/FooPacket.hpp"
#include "RTC/TestSerializable/FooTextItem.hpp"
#include "RTC/TestSerializable/FooUnknownItem.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memset()
#include <string>

using namespace RTC;

SCENARIO("parse FooPacket", "[serializable]")
{
	// clang-format off
	uint8_t buffer[] =
	{
		// Type:1, A:1, Length:25
		0x01, 0b10000000, 0x00, 0x19,
		// Appendix: 0x00BC614E
		0x00, 0xBC, 0x61, 0x4E,
		// FooItem 1: Id:1 (NUMERIC), Flags:0b0101, Length:2, Number: 0x1234
		0b00010101, 0x02, 0x12, 0x34,
		// FooItem 2: Id:2 (TEXT), Flags:0b0011, Length:5, , Text: 0xE282AC2B24 ("€+$")
		0b00100011, 0x05, 0xE2, 0x82,
		// ... FooItem 3: Id:3 (UNKNOWN), Flags:0b1111, Length:4, Value:0x11223344
		0xAC, 0x2B, 0x24, 0b00111111,
		0x04, 0x11, 0x22, 0x33,
		// ... 3 bytes of padding
		0x44, 0x00, 0x00, 0x00
	};
	// clang-format on

	REQUIRE(FooPacket::IsFooPacket(buffer, sizeof(buffer)) == true);

	auto* fooPacket = FooPacket::Parse(buffer, sizeof(buffer));

	REQUIRE(sizeof(buffer) == 28);
	REQUIRE(fooPacket);
	REQUIRE(fooPacket->GetBuffer() == buffer);
	REQUIRE(fooPacket->GetBufferLength() == 28);
	REQUIRE(fooPacket->GetLength() == 28);
	REQUIRE(fooPacket->IsFrozen() == true);
	REQUIRE(Utils::Byte::IsPaddedTo4Bytes(fooPacket->GetLength()) == true);
	REQUIRE(fooPacket->GetType() == 1);
	REQUIRE(fooPacket->HasAppendix() == true);
	REQUIRE(fooPacket->GetAppendix() == 0x00BC614E);
	REQUIRE(fooPacket->HasItems() == true);
	REQUIRE(fooPacket->GetItemsCount() == 3);
	REQUIRE(
	  helpers::areBuffersEqual(fooPacket->GetBuffer(), fooPacket->GetLength(), buffer, 28) == true);

	auto* item1 = reinterpret_cast<const FooNumericItem*>(fooPacket->GetItemAt(0));

	REQUIRE(item1);
	REQUIRE(item1->GetBuffer() == buffer + 8);
	REQUIRE(item1->GetBufferLength() == 4);
	REQUIRE(item1->GetLength() == 4);
	REQUIRE(item1->IsFrozen() == true);
	REQUIRE(item1->GetId() == FooItem::ItemId::NUMERIC);
	REQUIRE(item1->HasUnknownId() == false);
	REQUIRE(item1->GetFlags() == 0b0101);
	REQUIRE(item1->GetValueLength() == 2);
	REQUIRE(item1->GetNumber() == 0x1234);
	REQUIRE(helpers::areBuffersEqual(item1->GetBuffer(), item1->GetLength(), buffer + 8, 4) == true);

	auto* item2 = reinterpret_cast<const FooTextItem*>(fooPacket->GetItemAt(1));

	REQUIRE(item2);
	REQUIRE(item2->GetBuffer() == buffer + 12);
	REQUIRE(item2->GetBufferLength() == 7);
	REQUIRE(item2->GetLength() == 7);
	REQUIRE(item2->IsFrozen() == true);
	REQUIRE(item2->GetId() == FooItem::ItemId::TEXT);
	REQUIRE(item2->HasUnknownId() == false);
	REQUIRE(item2->GetFlags() == 0b0011);
	REQUIRE(item2->GetValueLength() == 5);
	REQUIRE(item2->GetText() == "€+$");
	REQUIRE(helpers::areBuffersEqual(item2->GetBuffer(), item2->GetLength(), buffer + 12, 7) == true);

	auto* item3 = reinterpret_cast<const FooUnknownItem*>(fooPacket->GetItemAt(2));

	REQUIRE(item3);
	REQUIRE(item3->GetBuffer() == buffer + 19);
	REQUIRE(item3->GetBufferLength() == 6);
	REQUIRE(item3->GetLength() == 6);
	REQUIRE(item3->IsFrozen() == true);
	REQUIRE(item3->GetId() == static_cast<FooItem::ItemId>(3));
	REQUIRE(item3->HasUnknownId() == true);
	REQUIRE(item3->GetFlags() == 0b1111);
	REQUIRE(item3->GetValueLength() == 4);
	REQUIRE(item3->GetValue()[0] == 0x11);
	REQUIRE(item3->GetValue()[1] == 0x22);
	REQUIRE(item3->GetValue()[2] == 0x33);
	REQUIRE(item3->GetValue()[3] == 0x44);
	REQUIRE(helpers::areBuffersEqual(item3->GetBuffer(), item3->GetLength(), buffer + 19, 6) == true);

	REQUIRE(!fooPacket->GetItemAt(3));

	delete fooPacket;
}

SCENARIO("parse invalid FooPacket with buffer not padded to 4 bytes", "[serializable]")
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

	auto* fooPacket = FooPacket::Parse(buffer, sizeof(buffer));

	REQUIRE(sizeof(buffer) == 23);
	REQUIRE(!fooPacket);
}

SCENARIO("create and modify FooPacket", "[serializable]")
{
	uint8_t buffer[256];
	uint8_t itemBuffer[17];

	std::memset(buffer, 0xFF, sizeof(buffer));
	std::memset(itemBuffer, 0xFF, sizeof(itemBuffer));

	auto* fooPacket = FooPacket::Factory(buffer, sizeof(buffer), /*type*/ 55);

	REQUIRE(sizeof(buffer) == 256);
	REQUIRE(fooPacket);
	REQUIRE(fooPacket->GetBuffer() == buffer);
	REQUIRE(fooPacket->GetBufferLength() == 256);
	// Just the FooPacket header (4 bytes).
	REQUIRE(fooPacket->GetLength() == 4);
	REQUIRE(fooPacket->IsFrozen() == false);
	REQUIRE(Utils::Byte::IsPaddedTo4Bytes(fooPacket->GetLength()) == true);
	REQUIRE(fooPacket->GetType() == 55);
	REQUIRE(fooPacket->HasAppendix() == false);
	REQUIRE(fooPacket->GetAppendix() == 0);
	REQUIRE(fooPacket->HasItems() == false);
	REQUIRE(fooPacket->GetItemsCount() == 0);
	REQUIRE(helpers::areBuffersEqual(fooPacket->GetBuffer(), fooPacket->GetLength(), buffer, 4) == true);

	/* Add Appendix. */

	fooPacket->SetAppendix(0x12345678);

	REQUIRE(fooPacket->GetBuffer() == buffer);
	REQUIRE(fooPacket->GetBufferLength() == 256);
	// Header (4 bytes) + Appendix (4 bytes).
	REQUIRE(fooPacket->GetLength() == 8);
	REQUIRE(fooPacket->IsFrozen() == false);
	REQUIRE(Utils::Byte::IsPaddedTo4Bytes(fooPacket->GetLength()) == true);
	REQUIRE(fooPacket->GetType() == 55);
	REQUIRE(fooPacket->HasAppendix() == true);
	REQUIRE(fooPacket->GetAppendix() == 0x12345678);
	REQUIRE(fooPacket->HasItems() == false);
	REQUIRE(fooPacket->GetItemsCount() == 0);
	REQUIRE(helpers::areBuffersEqual(fooPacket->GetBuffer(), fooPacket->GetLength(), buffer, 8) == true);

	/* Remove Appendix. */

	fooPacket->SetAppendix(0);

	REQUIRE(fooPacket->GetBuffer() == buffer);
	REQUIRE(fooPacket->GetBufferLength() == 256);
	// Header (4 bytes).
	REQUIRE(fooPacket->GetLength() == 4);
	REQUIRE(fooPacket->IsFrozen() == false);
	REQUIRE(Utils::Byte::IsPaddedTo4Bytes(fooPacket->GetLength()) == true);
	REQUIRE(fooPacket->GetType() == 55);
	REQUIRE(fooPacket->HasAppendix() == false);
	REQUIRE(fooPacket->GetAppendix() == 0);
	REQUIRE(fooPacket->HasItems() == false);
	REQUIRE(fooPacket->GetItemsCount() == 0);
	REQUIRE(helpers::areBuffersEqual(fooPacket->GetBuffer(), fooPacket->GetLength(), buffer, 4) == true);

	/* Add a FooItem. */

	// FooItem 1 (4 bytes).
	auto* item1 = FooNumericItem::Factory(
	  itemBuffer,
	  sizeof(itemBuffer),
	  /*flags*/ 0b1000,
	  /*number*/ 12345);

	REQUIRE(item1->IsFrozen() == false);

	fooPacket->AddItem(item1);

	// Original item remains frozen after calling `AddItem()` with it.
	REQUIRE(item1->IsFrozen() == false);

	// Delete the item since it's been cloned within the packet.
	delete item1;

	// FooItem 2 (6 bytes).
	auto* item2 = FooTextItem::Factory(
	  itemBuffer,
	  sizeof(itemBuffer),
	  /*flags*/ 0b1001,
	  "ABCD");

	REQUIRE(item2->IsFrozen() == false);

	fooPacket->AddItem(item2);

	// Original item remains frozen after calling `AddItem()` with it.
	REQUIRE(item2->IsFrozen() == false);

	// Delete the item since it's been cloned within the packet.
	delete item2;

	REQUIRE(fooPacket->GetBuffer() == buffer);
	REQUIRE(fooPacket->GetBufferLength() == 256);
	// Header (4 bytes) + items (10 bytes) + padding (2 bytes).
	REQUIRE(fooPacket->GetLength() == 16);
	REQUIRE(fooPacket->IsFrozen() == false);
	REQUIRE(Utils::Byte::IsPaddedTo4Bytes(fooPacket->GetLength()) == true);
	REQUIRE(fooPacket->GetType() == 55);
	REQUIRE(fooPacket->HasAppendix() == false);
	REQUIRE(fooPacket->GetAppendix() == 0);
	REQUIRE(fooPacket->HasItems() == true);
	REQUIRE(fooPacket->GetItemsCount() == 2);
	REQUIRE(
	  helpers::areBuffersEqual(fooPacket->GetBuffer(), fooPacket->GetLength(), buffer, 16) == true);

	auto* addedItem1 = reinterpret_cast<const FooNumericItem*>(fooPacket->GetItemAt(0));

	REQUIRE(addedItem1->GetBufferLength() == 4);
	REQUIRE(addedItem1->GetLength() == 4);
	// Internal items must always be frozen.
	REQUIRE(addedItem1->IsFrozen() == true);
	REQUIRE(addedItem1->GetId() == FooItem::ItemId::NUMERIC);
	REQUIRE(addedItem1->HasUnknownId() == false);
	REQUIRE(addedItem1->GetFlags() == 0b1000);
	REQUIRE(addedItem1->GetValueLength() == 2);
	REQUIRE(addedItem1->GetNumber() == 12345);
	REQUIRE(
	  helpers::areBuffersEqual(addedItem1->GetBuffer(), addedItem1->GetLength(), buffer + 4, 4) == true);

	auto* addedItem2 = reinterpret_cast<const FooTextItem*>(fooPacket->GetItemAt(1));

	REQUIRE(addedItem2->GetBufferLength() == 6);
	// Internal items must always be frozen.
	REQUIRE(addedItem2->IsFrozen() == true);
	REQUIRE(addedItem2->GetLength() == 6);
	REQUIRE(addedItem2->GetId() == FooItem::ItemId::TEXT);
	REQUIRE(addedItem2->HasUnknownId() == false);
	REQUIRE(addedItem2->GetFlags() == 0b1001);
	REQUIRE(addedItem2->GetValueLength() == 4);
	REQUIRE(addedItem2->GetText() == "ABCD");
	REQUIRE(
	  helpers::areBuffersEqual(addedItem2->GetBuffer(), addedItem2->GetLength(), buffer + 4 + 4, 6) ==
	  true);

	REQUIRE(!fooPacket->GetItemAt(2));

	// Must throw if we try to modify items within the packet because they are
	// always frozen.
	REQUIRE_THROWS_AS(const_cast<FooNumericItem*>(addedItem1)->SetNumber(9999), MediaSoupError);
	REQUIRE_THROWS_AS(const_cast<FooTextItem*>(addedItem2)->SetText("qweqwe"), MediaSoupError);

	/* Add Appendix. */

	fooPacket->SetAppendix(666);

	REQUIRE(fooPacket->GetBuffer() == buffer);
	REQUIRE(fooPacket->GetBufferLength() == 256);
	// Header (4 bytes) + appendix (4) + items (10 bytes) + padding (2 bytes).
	REQUIRE(fooPacket->GetLength() == 20);
	REQUIRE(fooPacket->IsFrozen() == false);
	REQUIRE(Utils::Byte::IsPaddedTo4Bytes(fooPacket->GetLength()) == true);
	REQUIRE(fooPacket->GetType() == 55);
	REQUIRE(fooPacket->HasAppendix() == true);
	REQUIRE(fooPacket->GetAppendix() == 666);
	REQUIRE(fooPacket->HasItems() == true);
	REQUIRE(fooPacket->GetItemsCount() == 2);
	REQUIRE(
	  helpers::areBuffersEqual(fooPacket->GetBuffer(), fooPacket->GetLength(), buffer, 20) == true);

	addedItem1 = reinterpret_cast<const FooNumericItem*>(fooPacket->GetItemAt(0));

	REQUIRE(addedItem1->GetBufferLength() == 4);
	REQUIRE(addedItem1->GetLength() == 4);
	REQUIRE(addedItem1->IsFrozen() == true);
	REQUIRE(addedItem1->GetId() == FooItem::ItemId::NUMERIC);
	REQUIRE(addedItem1->HasUnknownId() == false);
	REQUIRE(addedItem1->GetFlags() == 0b1000);
	REQUIRE(addedItem1->GetValueLength() == 2);
	REQUIRE(addedItem1->GetNumber() == 12345);
	REQUIRE(
	  helpers::areBuffersEqual(addedItem1->GetBuffer(), addedItem1->GetLength(), buffer + 4 + 4, 4) ==
	  true);

	addedItem2 = reinterpret_cast<const FooTextItem*>(fooPacket->GetItemAt(1));

	REQUIRE(addedItem2->GetBufferLength() == 6);
	REQUIRE(addedItem2->GetLength() == 6);
	REQUIRE(addedItem2->IsFrozen() == true);
	REQUIRE(addedItem2->GetId() == FooItem::ItemId::TEXT);
	REQUIRE(addedItem2->HasUnknownId() == false);
	REQUIRE(addedItem2->GetFlags() == 0b1001);
	REQUIRE(addedItem2->GetValueLength() == 4);
	REQUIRE(addedItem2->GetText() == "ABCD");
	REQUIRE(
	  helpers::areBuffersEqual(
	    addedItem2->GetBuffer(), addedItem2->GetLength(), buffer + 4 + 4 + 4, 6) == true);

	REQUIRE(!fooPacket->GetItemAt(2));

	/* Freeze FooPacket. */

	fooPacket->Freeze();

	REQUIRE(fooPacket->IsFrozen() == true);

	// Must throw if we try to modify the packet after freezing it.
	REQUIRE_THROWS_AS(fooPacket->SetAppendix(9999), MediaSoupError);
	REQUIRE_THROWS_AS(fooPacket->AddItem(addedItem1), MediaSoupError);
	REQUIRE_THROWS_AS(fooPacket->AddNumericItem(0b1100, 54321), MediaSoupError);
	REQUIRE_THROWS_AS(fooPacket->AddTextItem(0b0011, "hello"), MediaSoupError);

	/* Serialize FooPacket into another buffer. */

	uint8_t newBuffer1[256];

	std::memset(newBuffer1, 0xFF, sizeof(newBuffer1));

	// Must throw if buffer is too small.
	REQUIRE_THROWS_AS(fooPacket->Serialize(newBuffer1, fooPacket->GetLength() - 1), MediaSoupTypeError);

	fooPacket->Serialize(newBuffer1, sizeof(newBuffer1));

	// Compare new and old buffers.
	REQUIRE(helpers::areBuffersEqual(fooPacket->GetBuffer(), fooPacket->GetLength(), buffer, 20));

	// Once done fill the old buffer with 1s.
	std::memset(buffer, 0xFF, sizeof(buffer));

	REQUIRE(fooPacket->GetBuffer() == newBuffer1);
	REQUIRE(fooPacket->GetBufferLength() == 256);
	REQUIRE(fooPacket->GetLength() == 20);
	// After serializing, the packet must be unfrozen.
	REQUIRE(fooPacket->IsFrozen() == false);
	REQUIRE(Utils::Byte::IsPaddedTo4Bytes(fooPacket->GetLength()) == true);
	REQUIRE(fooPacket->GetType() == 55);
	REQUIRE(fooPacket->HasAppendix() == true);
	REQUIRE(fooPacket->GetAppendix() == 666);
	REQUIRE(fooPacket->HasItems() == true);
	REQUIRE(fooPacket->GetItemsCount() == 2);
	REQUIRE(
	  helpers::areBuffersEqual(fooPacket->GetBuffer(), fooPacket->GetLength(), newBuffer1, 20) == true);

	addedItem1 = reinterpret_cast<const FooNumericItem*>(fooPacket->GetItemAt(0));

	REQUIRE(addedItem1->GetBufferLength() == 4);
	REQUIRE(addedItem1->GetLength() == 4);
	// After serializing, items in the packet must remain frozen.
	REQUIRE(addedItem1->IsFrozen() == true);
	REQUIRE(addedItem1->GetId() == FooItem::ItemId::NUMERIC);
	REQUIRE(addedItem1->HasUnknownId() == false);
	REQUIRE(addedItem1->GetFlags() == 0b1000);
	REQUIRE(addedItem1->GetValueLength() == 2);
	REQUIRE(addedItem1->GetNumber() == 12345);
	REQUIRE(
	  helpers::areBuffersEqual(
	    addedItem1->GetBuffer(), addedItem1->GetLength(), newBuffer1 + 4 + 4, 4) == true);

	addedItem2 = reinterpret_cast<const FooTextItem*>(fooPacket->GetItemAt(1));

	REQUIRE(addedItem2->GetBufferLength() == 6);
	REQUIRE(addedItem2->GetLength() == 6);
	// After serializing, items in the packet must remain frozen.
	REQUIRE(addedItem2->IsFrozen() == true);
	REQUIRE(addedItem2->GetId() == FooItem::ItemId::TEXT);
	REQUIRE(addedItem2->HasUnknownId() == false);
	REQUIRE(addedItem2->GetFlags() == 0b1001);
	REQUIRE(addedItem2->GetValueLength() == 4);
	REQUIRE(addedItem2->GetText() == "ABCD");
	REQUIRE(
	  helpers::areBuffersEqual(
	    addedItem2->GetBuffer(), addedItem2->GetLength(), newBuffer1 + 4 + 4 + 4, 6) == true);

	REQUIRE(!fooPacket->GetItemAt(2));

	/* Add a new FooNumericItem and FooTextItem in place. */

	fooPacket->AddNumericItem(/*flags*/ 0b1100, /*number*/ 54321); // 4 bytes.
	fooPacket->AddTextItem(/*flags*/ 0b0011, /*number*/ "hello");  // 7 bytes.

	REQUIRE(fooPacket->GetBuffer() == newBuffer1);
	REQUIRE(fooPacket->GetBufferLength() == 256);
	REQUIRE(fooPacket->GetLength() == 32);
	REQUIRE(fooPacket->IsFrozen() == false);
	REQUIRE(Utils::Byte::IsPaddedTo4Bytes(fooPacket->GetLength()) == true);
	REQUIRE(fooPacket->GetType() == 55);
	REQUIRE(fooPacket->HasAppendix() == true);
	REQUIRE(fooPacket->GetAppendix() == 666);
	REQUIRE(fooPacket->HasItems() == true);
	REQUIRE(fooPacket->GetItemsCount() == 4);
	REQUIRE(
	  helpers::areBuffersEqual(fooPacket->GetBuffer(), fooPacket->GetLength(), newBuffer1, 32) == true);

	addedItem1 = reinterpret_cast<const FooNumericItem*>(fooPacket->GetItemAt(0));

	REQUIRE(addedItem1->GetBufferLength() == 4);
	REQUIRE(addedItem1->GetLength() == 4);
	REQUIRE(addedItem1->IsFrozen() == true);
	REQUIRE(addedItem1->GetId() == FooItem::ItemId::NUMERIC);
	REQUIRE(addedItem1->HasUnknownId() == false);
	REQUIRE(addedItem1->GetFlags() == 0b1000);
	REQUIRE(addedItem1->GetValueLength() == 2);
	REQUIRE(addedItem1->GetNumber() == 12345);
	REQUIRE(
	  helpers::areBuffersEqual(
	    addedItem1->GetBuffer(), addedItem1->GetLength(), newBuffer1 + 4 + 4, 4) == true);

	addedItem2 = reinterpret_cast<const FooTextItem*>(fooPacket->GetItemAt(1));

	REQUIRE(addedItem2->GetBufferLength() == 6);
	REQUIRE(addedItem2->GetLength() == 6);
	REQUIRE(addedItem2->IsFrozen() == true);
	REQUIRE(addedItem2->GetId() == FooItem::ItemId::TEXT);
	REQUIRE(addedItem2->HasUnknownId() == false);
	REQUIRE(addedItem2->GetFlags() == 0b1001);
	REQUIRE(addedItem2->GetValueLength() == 4);
	REQUIRE(addedItem2->GetText() == "ABCD");
	REQUIRE(
	  helpers::areBuffersEqual(
	    addedItem2->GetBuffer(), addedItem2->GetLength(), newBuffer1 + 4 + 4 + 4, 6) == true);

	auto* addedItem3 = reinterpret_cast<const FooNumericItem*>(fooPacket->GetItemAt(2));

	REQUIRE(addedItem3->GetBufferLength() == 4);
	REQUIRE(addedItem3->GetLength() == 4);
	REQUIRE(addedItem3->IsFrozen() == true);
	REQUIRE(addedItem3->GetId() == FooItem::ItemId::NUMERIC);
	REQUIRE(addedItem3->HasUnknownId() == false);
	REQUIRE(addedItem3->GetFlags() == 0b1100);
	REQUIRE(addedItem3->GetValueLength() == 2);
	REQUIRE(addedItem3->GetNumber() == 54321);
	REQUIRE(
	  helpers::areBuffersEqual(
	    addedItem3->GetBuffer(), addedItem3->GetLength(), newBuffer1 + 4 + 4 + 4 + 6, 4) == true);

	auto* addedItem4 = reinterpret_cast<const FooTextItem*>(fooPacket->GetItemAt(3));

	REQUIRE(addedItem4->GetBufferLength() == 7);
	REQUIRE(addedItem4->GetLength() == 7);
	REQUIRE(addedItem4->IsFrozen() == true);
	REQUIRE(addedItem4->GetId() == FooItem::ItemId::TEXT);
	REQUIRE(addedItem4->HasUnknownId() == false);
	REQUIRE(addedItem4->GetFlags() == 0b0011);
	REQUIRE(addedItem4->GetValueLength() == 5);
	REQUIRE(addedItem4->GetText() == "hello");
	REQUIRE(
	  helpers::areBuffersEqual(
	    addedItem4->GetBuffer(), addedItem4->GetLength(), newBuffer1 + 4 + 4 + 4 + 6 + 4, 7) == true);

	REQUIRE(!fooPacket->GetItemAt(4));

	/* Clone FooPacket into another buffer. */

	uint8_t newBuffer2[100];

	std::memset(newBuffer2, 0xFF, sizeof(newBuffer2));

	// Must throw if buffer is too small.
	REQUIRE_THROWS_AS(fooPacket->Clone(newBuffer2, fooPacket->GetLength() - 1), MediaSoupTypeError);

	auto* previousBuffer      = fooPacket->GetBuffer();
	auto previousBufferLength = fooPacket->GetBufferLength();
	auto* clonedFooPacket     = fooPacket->Clone(newBuffer2, sizeof(newBuffer2));

	// Compare the buffers of the original FooPacket and the cloned one.
	REQUIRE(
	  helpers::areBuffersEqual(
	    clonedFooPacket->GetBuffer(), clonedFooPacket->GetLength(), newBuffer1, fooPacket->GetLength()) ==
	  true);

	// Once done fill the original buffer with 1s (this is, we are ruining original
	// FooPacket despite it still exists since we have jsut cloned it).
	std::memset(const_cast<uint8_t*>(previousBuffer), 0xFF, previousBufferLength);

	// Freeze the original packet again.
	fooPacket->Freeze();

	REQUIRE(clonedFooPacket->GetBuffer() == newBuffer2);
	REQUIRE(clonedFooPacket->GetBufferLength() == 100);
	REQUIRE(clonedFooPacket->GetLength() == 32);
	// After cloning, the cloned packet must be unfrozen.
	REQUIRE(clonedFooPacket->IsFrozen() == false);
	REQUIRE(Utils::Byte::IsPaddedTo4Bytes(clonedFooPacket->GetLength()) == true);
	REQUIRE(clonedFooPacket->GetType() == 55);
	REQUIRE(clonedFooPacket->HasAppendix() == true);
	REQUIRE(clonedFooPacket->GetAppendix() == 666);
	REQUIRE(clonedFooPacket->HasItems() == true);
	REQUIRE(clonedFooPacket->GetItemsCount() == 4);

	addedItem1 = reinterpret_cast<const FooNumericItem*>(clonedFooPacket->GetItemAt(0));

	REQUIRE(addedItem1->GetBufferLength() == 4);
	REQUIRE(addedItem1->GetLength() == 4);
	// After cloning the packet, items in the packet must remain frozen.
	REQUIRE(addedItem1->IsFrozen() == true);
	REQUIRE(addedItem1->GetId() == FooItem::ItemId::NUMERIC);
	REQUIRE(addedItem1->HasUnknownId() == false);
	REQUIRE(addedItem1->GetFlags() == 0b1000);
	REQUIRE(addedItem1->GetValueLength() == 2);
	REQUIRE(addedItem1->GetNumber() == 12345);
	REQUIRE(
	  helpers::areBuffersEqual(
	    addedItem1->GetBuffer(), addedItem1->GetLength(), newBuffer2 + 4 + 4, 4) == true);

	addedItem2 = reinterpret_cast<const FooTextItem*>(clonedFooPacket->GetItemAt(1));

	REQUIRE(addedItem2->GetBufferLength() == 6);
	REQUIRE(addedItem2->GetLength() == 6);
	// After cloning the packet, items in the packet must remain frozen.
	REQUIRE(addedItem2->IsFrozen() == true);
	REQUIRE(addedItem2->GetId() == FooItem::ItemId::TEXT);
	REQUIRE(addedItem2->HasUnknownId() == false);
	REQUIRE(addedItem2->GetFlags() == 0b1001);
	REQUIRE(addedItem2->GetValueLength() == 4);
	REQUIRE(addedItem2->GetText() == "ABCD");
	REQUIRE(
	  helpers::areBuffersEqual(
	    addedItem2->GetBuffer(), addedItem2->GetLength(), newBuffer2 + 4 + 4 + 4, 6) == true);

	addedItem3 = reinterpret_cast<const FooNumericItem*>(clonedFooPacket->GetItemAt(2));

	REQUIRE(addedItem3->GetBufferLength() == 4);
	REQUIRE(addedItem3->GetLength() == 4);
	// After cloning the packet, items in the packet must remain frozen.
	REQUIRE(addedItem3->IsFrozen() == true);
	REQUIRE(addedItem3->GetId() == FooItem::ItemId::NUMERIC);
	REQUIRE(addedItem3->HasUnknownId() == false);
	REQUIRE(addedItem3->GetFlags() == 0b1100);
	REQUIRE(addedItem3->GetValueLength() == 2);
	REQUIRE(addedItem3->GetNumber() == 54321);
	REQUIRE(
	  helpers::areBuffersEqual(
	    addedItem3->GetBuffer(), addedItem3->GetLength(), newBuffer2 + 4 + 4 + 4 + 6, 4) == true);

	addedItem4 = reinterpret_cast<const FooTextItem*>(clonedFooPacket->GetItemAt(3));

	REQUIRE(addedItem4->GetBufferLength() == 7);
	REQUIRE(addedItem4->GetLength() == 7);
	// After serializing, items in the packet must remain frozen.
	REQUIRE(addedItem4->IsFrozen() == true);
	REQUIRE(addedItem4->GetId() == FooItem::ItemId::TEXT);
	REQUIRE(addedItem4->HasUnknownId() == false);
	REQUIRE(addedItem4->GetFlags() == 0b0011);
	REQUIRE(addedItem4->GetValueLength() == 5);
	REQUIRE(addedItem4->GetText() == "hello");
	REQUIRE(
	  helpers::areBuffersEqual(
	    addedItem4->GetBuffer(), addedItem4->GetLength(), newBuffer2 + 4 + 4 + 4 + 6 + 4, 7) == true);

	REQUIRE(!clonedFooPacket->GetItemAt(4));

	/* Clone a FooItem in the packet. */

	uint8_t newBuffer3[8];

	std::memset(newBuffer3, 0xFF, sizeof(newBuffer3));

	auto* clonedItem1 = addedItem1->Clone(newBuffer3, sizeof(newBuffer3));

	REQUIRE(clonedItem1->GetBufferLength() == 8);
	REQUIRE(clonedItem1->GetLength() == 4);
	// If we clone an item in the packet, the cloned item won't be frozen.
	REQUIRE(clonedItem1->IsFrozen() == false);
	REQUIRE(clonedItem1->GetId() == FooItem::ItemId::NUMERIC);
	REQUIRE(clonedItem1->HasUnknownId() == false);
	REQUIRE(clonedItem1->GetFlags() == 0b1000);
	REQUIRE(clonedItem1->GetValueLength() == 2);
	REQUIRE(clonedItem1->GetNumber() == 12345);
	REQUIRE(
	  helpers::areBuffersEqual(clonedItem1->GetBuffer(), clonedItem1->GetLength(), newBuffer3, 4) ==
	  true);

	delete fooPacket;
	delete clonedFooPacket;
	delete clonedItem1;
}

SCENARIO("parse FooNumericItem", "[serializable]")
{
	// clang-format off
	uint8_t buffer[] =
	{
		// Id:1 (NUMERIC), Flags:0b1110, Value Length:2, Number: 0x12EF
		0b00011110, 0x02, 0x12, 0xEF
	};
	// clang-format on

	auto* item = FooNumericItem::Parse(buffer, sizeof(buffer));

	REQUIRE(sizeof(buffer) == 4);
	REQUIRE(item);
	REQUIRE(item->GetBuffer() == buffer);
	REQUIRE(item->GetBufferLength() == 4);
	REQUIRE(item->GetLength() == 4);
	REQUIRE(item->IsFrozen() == true);
	REQUIRE(item->GetId() == FooItem::ItemId::NUMERIC);
	REQUIRE(item->HasUnknownId() == false);
	REQUIRE(item->GetFlags() == 0b1110);
	REQUIRE(item->GetNumber() == 0x12EF);
	REQUIRE(helpers::areBuffersEqual(item->GetBuffer(), item->GetLength(), buffer, 4) == true);

	delete item;
}

SCENARIO("parse FooNumericItem by passing a buffer larger than the length of the item", "[serializable]")
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

	auto* item = FooNumericItem::Parse(buffer, sizeof(buffer));

	REQUIRE(sizeof(buffer) == 6);
	REQUIRE(item);
	REQUIRE(item->GetBuffer() == buffer);
	REQUIRE(item->GetBufferLength() == 6);
	REQUIRE(item->GetLength() == 4);
	REQUIRE(item->IsFrozen() == true);
	REQUIRE(item->GetId() == FooItem::ItemId::NUMERIC);
	REQUIRE(item->HasUnknownId() == false);
	REQUIRE(item->GetFlags() == 0b0000);
	REQUIRE(item->GetNumber() == 0xFFFF);
	REQUIRE(helpers::areBuffersEqual(item->GetBuffer(), item->GetLength(), buffer, 4) == true);

	delete item;
}

SCENARIO("parse invalid FooNumericItem with too small buffer", "[serializable]")
{
	// Item length should be 4 but given buffer is only 3 bytes.
	// clang-format off
	uint8_t buffer[] =
	{
		// Id:1 (NUMERIC), Flags:0b1111, Value Length:2, Number: 0xAB (wrong)
		0b00011111, 0x02, 0xAB
	};
	// clang-format on

	auto* item = FooNumericItem::Parse(buffer, sizeof(buffer));

	REQUIRE(sizeof(buffer) == 3);
	REQUIRE(!item);
}

SCENARIO("parse invalid FooNumericItem with wrong value length", "[serializable]")
{
	// clang-format off
	uint8_t buffer[] =
	{
		// Id:1 (NUMERIC), Flags:0b1111, Value Length:1, Number: 0xAB
		0b00011111, 0x03, 0xAB, 0xCD,
		0xEF
	};
	// clang-format on

	auto* item = FooNumericItem::Parse(buffer, sizeof(buffer));

	REQUIRE(sizeof(buffer) == 5);
	REQUIRE(!item);
}

SCENARIO("parse invalid FooNumericItem with wrong id", "[serializable]")
{
	// clang-format off
	uint8_t buffer[] =
	{
		// Id:2 (TEXT), Flags:0b1111, Value Length:2, Text: 0xABCD
		0b00101111, 0x02, 0xAB, 0xCD
	};
	// clang-format on

	auto* item = FooNumericItem::Parse(buffer, sizeof(buffer));

	REQUIRE(sizeof(buffer) == 4);
	REQUIRE(!item);
}

SCENARIO("parse FooUnknownItem of maximum length", "[serializable]")
{
	// FooUnknownItem with:
	// - ItemHeaderLength: 2 bytes
	// - Maximum Value Length: 255 bytes
	// - Total Item length: 2 + 255 = 257 bytes
	// - Buffer length: 260 bytes
	uint8_t buffer[260];

	// Ininitalize every byte with 0x69.
	std::memset(buffer, 0x69, sizeof(buffer));

	// Id:0 (UNKNOWN), Flags:0b1111
	buffer[0] = 0b00001111;
	// Value Length:255
	buffer[1] = 0xFF;
	// First value byte:0x11
	buffer[2] = 0x11;
	// Second value byte:0x22
	buffer[3] = 0x22;
	// 3 remaining bytes
	buffer[260 - 3] = 0x00;
	buffer[260 - 2] = 0x00;
	buffer[260 - 1] = 0x00;

	auto* item = FooUnknownItem::Parse(buffer, sizeof(buffer));

	REQUIRE(sizeof(buffer) == 260);
	REQUIRE(item);
	REQUIRE(item->GetBuffer() == buffer);
	REQUIRE(item->GetBufferLength() == 260);
	REQUIRE(item->GetLength() == 257);
	REQUIRE(item->IsFrozen() == true);
	REQUIRE(item->GetId() == static_cast<FooItem::ItemId>(0));
	REQUIRE(item->HasUnknownId() == true);
	REQUIRE(item->GetFlags() == 0b1111);
	REQUIRE(item->GetValueLength() == 255);
	REQUIRE(item->GetValue()[0] == 0x11);
	REQUIRE(item->GetValue()[1] == 0x22);
	REQUIRE(item->GetValue()[2] == 0x69);
	REQUIRE(item->GetValue()[254] == 0x69);
	REQUIRE(item->GetBuffer()[257] == 0x00);
	REQUIRE(item->GetBuffer()[258] == 0x00);
	REQUIRE(item->GetBuffer()[259] == 0x00);
	REQUIRE(helpers::areBuffersEqual(item->GetBuffer(), item->GetLength(), buffer, 257) == true);

	delete item;
}

SCENARIO("create and modify FooNumericItem", "[serializable]")
{
	// Max length of a FooItem is 17 bytes.
	uint8_t buffer[17];

	// Let's fill the buffer with whatever (it should be overridden by
	// FooNumericItem:Factory()).
	std::memset(buffer, 0xFF, sizeof(buffer));

	auto* item = FooNumericItem::Factory(buffer, sizeof(buffer), /*flags*/ 0b1010, 1111);

	REQUIRE(sizeof(buffer) == 17);
	REQUIRE(item);
	REQUIRE(item->GetBuffer() == buffer);
	REQUIRE(item->GetBufferLength() == 17);
	REQUIRE(item->GetLength() == 4);
	REQUIRE(item->IsFrozen() == false);
	REQUIRE(item->GetId() == FooItem::ItemId::NUMERIC);
	REQUIRE(item->HasUnknownId() == false);
	REQUIRE(item->GetFlags() == 0b1010);
	REQUIRE(item->GetNumber() == 1111);
	REQUIRE(helpers::areBuffersEqual(item->GetBuffer(), item->GetLength(), buffer, 4) == true);

	/* Modify FooNumericItem. */

	item->SetNumber(2222);

	REQUIRE(item->GetBuffer() == buffer);
	REQUIRE(item->GetBufferLength() == 17);
	REQUIRE(item->GetLength() == 4);
	REQUIRE(item->IsFrozen() == false);
	REQUIRE(item->GetId() == FooItem::ItemId::NUMERIC);
	REQUIRE(item->HasUnknownId() == false);
	REQUIRE(item->GetFlags() == 0b1010);
	REQUIRE(item->GetNumber() == 2222);
	REQUIRE(helpers::areBuffersEqual(item->GetBuffer(), item->GetLength(), buffer, 4) == true);

	/* Freeze FooNumericItem. */

	item->Freeze();

	REQUIRE(item->IsFrozen() == true);
	// Must throw if we try to modify the item.
	REQUIRE_THROWS_AS(item->SetNumber(1122), MediaSoupError);

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
	// After serializing the item it must be unfrozen.
	REQUIRE(item->IsFrozen() == false);
	REQUIRE(item->GetId() == FooItem::ItemId::NUMERIC);
	REQUIRE(item->HasUnknownId() == false);
	REQUIRE(item->GetFlags() == 0b1010);
	REQUIRE(item->GetNumber() == 2222);
	REQUIRE(helpers::areBuffersEqual(item->GetBuffer(), item->GetLength(), newBuffer1, 4) == true);

	/* Clone FooNumericItem into another buffer. */

	uint8_t newBuffer2[100];

	std::memset(newBuffer2, 0xFF, sizeof(newBuffer2));

	// Must throw if buffer is too small.
	REQUIRE_THROWS_AS(item->Clone(newBuffer2, item->GetLength() - 1), MediaSoupTypeError);

	// Serialize the original item again just for testing.
	item->Freeze();

	auto* previousBuffer      = item->GetBuffer();
	auto previousBufferLength = item->GetBufferLength();
	auto* clonedItem          = item->Clone(newBuffer2, sizeof(newBuffer2));

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
	// Clone() must create an unfrozen item.
	REQUIRE(clonedItem->IsFrozen() == false);
	REQUIRE(clonedItem->GetId() == FooItem::ItemId::NUMERIC);
	REQUIRE(clonedItem->HasUnknownId() == false);
	REQUIRE(clonedItem->GetFlags() == 0b1010);
	REQUIRE(clonedItem->GetNumber() == 2222);
	REQUIRE(
	  helpers::areBuffersEqual(clonedItem->GetBuffer(), clonedItem->GetLength(), newBuffer2, 4) == true);

	delete item;
	delete clonedItem;
}

SCENARIO("create and modify FooTextItem", "[serializable]")
{
	uint8_t buffer[40];
	std::string text = "Iñaki"; // 6 bytes.

	// Let's fill the buffer with whatever (it should be overridden by
	// FooTextItem:Factory()).
	std::memset(buffer, 0xFF, sizeof(buffer));

	auto* item = FooTextItem::Factory(buffer, sizeof(buffer), /*flags*/ 0b1010, text);

	REQUIRE(sizeof(buffer) == 40);
	REQUIRE(item);
	REQUIRE(item->GetBuffer() == buffer);
	REQUIRE(item->GetBufferLength() == 40);
	REQUIRE(item->GetLength() == 8);
	REQUIRE(item->IsFrozen() == false);
	REQUIRE(item->GetId() == FooItem::ItemId::TEXT);
	REQUIRE(item->HasUnknownId() == false);
	REQUIRE(item->GetFlags() == 0b1010);
	REQUIRE(item->GetText() == text);
	REQUIRE(helpers::areBuffersEqual(item->GetBuffer(), item->GetLength(), buffer, 8) == true);

	/* Modify FooTextItem. */

	std::string newText = "œæ€å∫∂"; // 15 bytes.

	item->SetText(newText);

	REQUIRE(item->GetBuffer() == buffer);
	REQUIRE(item->GetBufferLength() == 40);
	REQUIRE(item->GetLength() == 17);
	REQUIRE(item->IsFrozen() == false);
	REQUIRE(item->GetId() == FooItem::ItemId::TEXT);
	REQUIRE(item->HasUnknownId() == false);
	REQUIRE(item->GetFlags() == 0b1010);
	REQUIRE(item->GetText() == newText);
	REQUIRE(helpers::areBuffersEqual(item->GetBuffer(), item->GetLength(), buffer, 17) == true);

	/* Freeze FooTextItem. */

	item->Freeze();

	REQUIRE(item->IsFrozen() == true);
	// Must throw if we try to modify the item.
	REQUIRE_THROWS_AS(item->SetText("zzz!!!"), MediaSoupError);

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
	// After serializing the item it must be unfrozen.
	REQUIRE(item->IsFrozen() == false);
	REQUIRE(item->GetId() == FooItem::ItemId::TEXT);
	REQUIRE(item->HasUnknownId() == false);
	REQUIRE(item->GetFlags() == 0b1010);
	REQUIRE(item->GetText() == newText);
	REQUIRE(helpers::areBuffersEqual(item->GetBuffer(), item->GetLength(), newBuffer1, 17) == true);

	/* Clone FooTextItem into another buffer. */

	uint8_t newBuffer2[100];

	std::memset(newBuffer2, 0xFF, sizeof(newBuffer2));

	// Must throw if buffer is too small.
	REQUIRE_THROWS_AS(item->Clone(newBuffer2, item->GetLength() - 1), MediaSoupTypeError);

	// Serialize the original item again just for testing.
	item->Freeze();

	auto* previousBuffer      = item->GetBuffer();
	auto previousBufferLength = item->GetBufferLength();
	auto* clonedItem          = item->Clone(newBuffer2, sizeof(newBuffer2));

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
	// Clone() must create an unfrozen item.
	REQUIRE(clonedItem->IsFrozen() == false);
	REQUIRE(clonedItem->GetId() == FooItem::ItemId::TEXT);
	REQUIRE(clonedItem->HasUnknownId() == false);
	REQUIRE(clonedItem->GetFlags() == 0b1010);
	REQUIRE(clonedItem->GetText() == newText);
	REQUIRE(
	  helpers::areBuffersEqual(clonedItem->GetBuffer(), clonedItem->GetLength(), newBuffer2, 17) ==
	  true);

	delete item;
	delete clonedItem;
}
