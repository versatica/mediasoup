#include "common.hpp"
#include "helpers.hpp"
#include "RTC/Serializable.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memcpy()

using namespace RTC;

class Foo : public Serializable
{
public:
	Foo(const uint8_t* buffer, size_t size) : Serializable(buffer, size)
	{
	}

	void Dump() const override
	{
		// Nothing here.
	}

	size_t GetSize() const override
	{
		return this->size;
	}

	void Serialize(uint8_t* buffer, size_t size) override
	{
		std::memcpy(buffer, this->buffer, GetSize());

		this->buffer = buffer;
		this->size   = size;

		SetSerializationNeeded(false);
	}
};

SCENARIO("Serializable", "[rtc][Serializable]")
{
	// clang-format off
	uint8_t originalBuffer[] =
	{
		0x00, 0x11, 0x22, 0x33,
		0x44, 0x55, 0x66, 0x77,
		0x88, 0x99, 0xff, 0xff,
		0xff, 0xff, 0xff, 0xff,
		0xff, 0xff, 0xff, 0xff
	};
	// clang-format on
	size_t originalBufferSize = 10;

	Foo foo(originalBuffer, originalBufferSize);

	SECTION("serializable processing succeeds")
	{
		REQUIRE(foo.NeedsSerialization() == false);
		REQUIRE(foo.GetBuffer() == originalBuffer);
		REQUIRE(foo.GetSize() == originalBufferSize);
	}

	SECTION("Serialize() succeeds")
	{
		uint8_t newBuffer[10];

		foo.Serialize(newBuffer, originalBufferSize);

		REQUIRE(foo.NeedsSerialization() == false);
		REQUIRE(foo.GetBuffer() == newBuffer);
		REQUIRE(foo.GetSize() == originalBufferSize);
		REQUIRE(
		  helpers::areBuffersEqual(foo.GetBuffer(), foo.GetSize(), originalBuffer, originalBufferSize) ==
		  true);
	}

	// SECTION("set One-Byte header extensions")
	// {

	// 	std::unique_ptr<RtpPacket> packet{ RtpPacket::Parse(buffer, 28) };
	// 	std::vector<RTC::RtpPacket::GenericExtension> extensions;
	// 	uint8_t extenLen;
	// 	uint8_t* extenValue;

	// 	if (!packet)
	// 	{
	// 		FAIL("not a RTP packet");
	// 	}

	// 	REQUIRE(packet->GetSize() == 28);
	// 	REQUIRE(packet->HasHeaderExtension() == false);
	// 	REQUIRE(packet->GetHeaderExtensionId() == 0);
	// 	REQUIRE(packet->GetPayload()[0] == 0x11);
	// }
}
