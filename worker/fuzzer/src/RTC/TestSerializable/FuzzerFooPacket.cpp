#include "RTC/TestSerializable/FuzzerFooPacket.hpp"
#include "RTC/TestSerializable/FooNumericItem.hpp"
#include "RTC/TestSerializable/FooPacket.hpp"
#include "RTC/TestSerializable/FooTextItem.hpp"

thread_local static uint8_t FooPacketSerializeBuffer[65536];
thread_local static uint8_t FooPacketCloneBuffer[65536];
thread_local static uint8_t FooItemFactoryBuffer[65536];

void Fuzzer::RTC::FooPacket::Fuzz(const uint8_t* data, size_t len)
{
	::RTC::FooPacket* fooPacket = ::RTC::FooPacket::Parse(data, len);

	if (!fooPacket)
	{
		return;
	}

	fooPacket->GetType();
	fooPacket->HasAppendix();
	fooPacket->GetAppendix();
	fooPacket->GetItemsCount();
	fooPacket->ItemsBegin();
	fooPacket->ItemsEnd();
	fooPacket->GetItemAt(0);
	fooPacket->GetItemAt(1);
	fooPacket->GetItemAt(666);

	fooPacket->Serialize(FooPacketSerializeBuffer, sizeof(FooPacketSerializeBuffer));

	fooPacket->SetAppendix(0x12345678);
	fooPacket->SetAppendix(0u);
	fooPacket->SetAppendix(0x87654321);

	// FooItem 1 (4 bytes).
	auto* item1 = ::RTC::FooNumericItem::Factory(
	  FooItemFactoryBuffer,
	  sizeof(FooItemFactoryBuffer),
	  /*flags*/ 0b1000,
	  /*number*/ 12345);

	fooPacket->AddItem(item1);

	// Delete the item since it's been cloned within the packet.
	delete item1;

	// FooItem 2 (6 bytes).
	auto* item2 = ::RTC::FooTextItem::Factory(
	  FooItemFactoryBuffer,
	  sizeof(FooItemFactoryBuffer),
	  /*flags*/ 0b1001,
	  "ABCD");

	fooPacket->AddItem(item2);

	// Delete the item since it's been cloned within the packet.
	delete item2;

	fooPacket->AddNumericItem(0b1100, 54321);
	fooPacket->AddTextItem(0b0011, "œæ€å∫∂∑©Ω∑©å∫∂å∫∂");

	fooPacket->SetAppendix(0x12345678);
	fooPacket->SetAppendix(0u);
	fooPacket->SetAppendix(0x87654321);

	auto* clonedFooPacket = fooPacket->Clone(FooPacketCloneBuffer, sizeof(FooPacketCloneBuffer));

	clonedFooPacket->SetAppendix(0x12345678);
	clonedFooPacket->SetAppendix(0u);
	clonedFooPacket->SetAppendix(0x87654321);
	clonedFooPacket->AddNumericItem(0b1111, 12345);
	clonedFooPacket->AddTextItem(0b0000, "∂ƒ®¥ƒ ƒ™ƒ™ƒ∑©∫#¢");
	clonedFooPacket->SetAppendix(0x12345678);
	clonedFooPacket->SetAppendix(0u);
	clonedFooPacket->SetAppendix(0x87654321);

	clonedFooPacket->Serialize(FooPacketSerializeBuffer, sizeof(FooPacketSerializeBuffer));

	delete fooPacket;
	delete clonedFooPacket;
}
