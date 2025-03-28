#include "common.hpp"
#include "MediaSoupErrors.hpp"
#include "Utils.hpp"
#include "helpers.hpp"
#include "RTC/Serializable.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memcpy()
#include <stdexcept>

using namespace RTC;

class Foo : public Serializable
{
public:
	static Foo* Parse(const uint8_t* data, size_t len, uint8_t padding)
	{
		auto* foo = new Foo(const_cast<uint8_t*>(data));

		foo->SetInitialContentSize(len);
		foo->SetInitialPadding(padding);

		return foo;
	}

public:
	Foo(uint8_t* buffer) : Serializable(buffer)
	{
	}

	void Dump() const override
	{
		// Nothing here.
	}

	size_t ComputeContentSize() const override
	{
		if (!NeedsSerialization())
		{
			return GetCurrentContentSize();
		}

		// Yeah, this could be better.
		return GetCurrentContentSize() + this->additionalContentSize;
	}

	void Serialize(uint8_t* buffer, bool padTo4Bytes) override
	{
		auto contentSize = ComputeContentSize();
		uint8_t padding{ 0u };

		std::memcpy(buffer, GetCurrentBuffer(), contentSize);

		if (padTo4Bytes)
		{
			auto paddedContentSize = Utils::Byte::PadTo4Bytes(static_cast<uint32_t>(contentSize));

			padding = static_cast<uint8_t>(paddedContentSize - contentSize);
		}

		Serialized(buffer, contentSize, padding);
	}

	void SetAdditionalContent(size_t additionalContentSize)
	{
		this->additionalContentSize = additionalContentSize;

		SetSerializationNeeded();
	}

private:
	size_t additionalContentSize{ 0u };
};

SCENARIO("Serializable with no padding", "[rtc][Serializable]")
{
	// clang-format off
	uint8_t buffer[] =
	{
		0x00, 0x11, 0x22, 0x33,
		0x44, 0x55, 0x66, 0x77,
		0x88, 0x99
	};
	// clang-format on
	size_t contentSize = 10u;

	auto* foo = Foo::Parse(buffer, contentSize, 0u);

	SECTION("NeedsSerialization() works as expected")
	{
		// Initially serialization is not needed.
		REQUIRE(foo->NeedsSerialization() == false);
		REQUIRE(foo->GetBuffer() == buffer);
		REQUIRE(foo->GetSize() == contentSize);

		foo->SetAdditionalContent(3u);

		// After changes, serialization is needed and cannot call some methods.
		REQUIRE(foo->NeedsSerialization() == true);
		REQUIRE_THROWS_AS(foo->GetBuffer(), MediaSoupError);
		REQUIRE_THROWS_AS(foo->GetSize(), MediaSoupError);

		delete foo;
	}

	SECTION("Serialize() without padding succeeds")
	{
		uint8_t newBuffer[contentSize];

		std::memset(newBuffer, 0xFF, contentSize);

		foo->Serialize(newBuffer, /*padTo4Bytes*/ false);

		REQUIRE(foo->NeedsSerialization() == false);
		REQUIRE(foo->GetBuffer() == static_cast<const void*>(newBuffer));
		REQUIRE(foo->GetSize() == contentSize);
		REQUIRE(helpers::areBuffersEqual(foo->GetBuffer(), foo->GetSize(), buffer, contentSize) == true);

		delete foo;
	}

	SECTION("Serialize() with padding succeeds")
	{
		auto paddedContentSize = Utils::Byte::PadTo4Bytes(static_cast<uint32_t>(contentSize));

		REQUIRE(paddedContentSize != contentSize);

		uint8_t newBuffer[paddedContentSize];

		std::memset(newBuffer, 0xFF, paddedContentSize);

		foo->Serialize(newBuffer, /*padTo4Bytes*/ true);

		REQUIRE(foo->NeedsSerialization() == false);
		REQUIRE(foo->GetBuffer() == static_cast<const void*>(newBuffer));
		REQUIRE(foo->GetSize() == paddedContentSize);
		REQUIRE(helpers::areBuffersEqual(foo->GetBuffer(), contentSize, buffer, contentSize) == true);

		auto padding    = paddedContentSize - contentSize;
		uint8_t zeros[] = { 0x00, 0x00, 0x00, 0x00 };

		REQUIRE(helpers::areBuffersEqual(foo->GetBuffer() + contentSize, padding, zeros, padding) == true);

		delete foo;
	}
}

SCENARIO("Serializable with padding", "[rtc][Serializable]")
{
	// clang-format off
	uint8_t buffer[] =
	{
		0x00, 0x11, 0x22, 0x33,
		0x44, 0x55, 0x66, 0x77,
		0x88, 0x99, 0x00, 0x00
	};
	// clang-format on
	size_t contentSize = 10u;
	size_t padding     = 2u;

	auto* foo = Foo::Parse(buffer, contentSize, padding);

	SECTION("NeedsSerialization() works as expected")
	{
		// Initially serialization is not needed.
		REQUIRE(foo->NeedsSerialization() == false);
		REQUIRE(foo->GetBuffer() == buffer);
		REQUIRE(foo->GetSize() == contentSize + padding);

		foo->SetAdditionalContent(3u);

		// After changes, serialization is needed and cannot call some methods.
		REQUIRE(foo->NeedsSerialization() == true);
		REQUIRE_THROWS_AS(foo->GetBuffer(), MediaSoupError);
		REQUIRE_THROWS_AS(foo->GetSize(), MediaSoupError);

		delete foo;
	}

	SECTION("Serialize() without padding succeeds")
	{
		uint8_t newBuffer[contentSize];

		std::memset(newBuffer, 0xFF, contentSize);

		foo->Serialize(newBuffer, /*padTo4Bytes*/ false);

		REQUIRE(foo->NeedsSerialization() == false);
		REQUIRE(foo->GetBuffer() == static_cast<const void*>(newBuffer));
		REQUIRE(foo->GetSize() == contentSize);
		REQUIRE(helpers::areBuffersEqual(foo->GetBuffer(), foo->GetSize(), buffer, contentSize) == true);

		delete foo;
	}

	SECTION("Serialize() with padding succeeds")
	{
		uint8_t newBuffer[contentSize + padding];

		std::memset(newBuffer, 0xFF, contentSize + padding);

		foo->Serialize(newBuffer, /*padTo4Bytes*/ true);

		REQUIRE(foo->NeedsSerialization() == false);
		REQUIRE(foo->GetBuffer() == static_cast<const void*>(newBuffer));
		REQUIRE(foo->GetSize() == contentSize + padding);
		REQUIRE(
		  helpers::areBuffersEqual(
		    foo->GetBuffer(), contentSize + padding, buffer, contentSize + padding) == true);

		delete foo;
	}
}
