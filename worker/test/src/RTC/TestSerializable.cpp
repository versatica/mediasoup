#include "common.hpp"
#include "MediaSoupErrors.hpp"
#include "Utils.hpp"
#include "helpers.hpp"
#include "RTC/Serializable.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memcpy(), std::memset()

using namespace RTC;

/**
 * Foo Packet.
 *  0                   1                   2                   3
 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |      Type     |A|   Control   |          Body Length          |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                           Appendix                            |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                              Body                             |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                              ...              |    Padding    |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 * - Type (8 bits).
 * - A (1 bit): Whether the packet contains an Appendix field at the end.
 * - Control (7 bits).
 * - Body Length (16 bits): Length of the body excluding padding bytes.
 * - Appendix (32 bits).
 * - Body (variable length). It can have zero-length.
 * - Padding: Bytes of padding to make the packet size be multiple of 4 bytes.
 */

class Foo : public Serializable
{
public:
	/**
	 * Struct of a the Foo Header.
	 */
	struct Header
	{
		uint8_t type;
#if defined(MS_LITTLE_ENDIAN)
		uint8_t control : 7;
		uint8_t a : 1;
#elif defined(MS_BIG_ENDIAN)
		uint8_t a : 1;
		uint8_t control : 7;
#endif
		uint16_t bodyLength;
	};

public:
	static const size_t HeaderSize{ 4 };
	static const size_t AppendixSize{ 4 };

public:
	static Foo* Parse(const uint8_t* data, size_t len)
	{
		// No space for header.
		if (len < HeaderSize)
		{
			return nullptr;
		}

		uint8_t* ptr = const_cast<uint8_t*>(data);

		auto* foo = new Foo(ptr);

		// Move to the appendix.
		ptr += HeaderSize;

		if (foo->HasAppendix())
		{
			// No space for appendix.
			if (len - (ptr - data) < AppendixSize)
			{
				delete foo;
				return nullptr;
			}

			foo->SetAppendix(Utils::Byte::Get4Bytes(ptr, 0));

			// Move to the body.
			ptr += AppendixSize;
		}

		if (foo->HasBody())
		{
			// No space for body.
			if (len - (ptr - data) < foo->GetBodyLength())
			{
				delete foo;
				return nullptr;
			}

			foo->SetBody(ptr, foo->GetBodyLength());

			// Move to the body.
			ptr += foo->GetBodyLength();
		}

		size_t size       = ptr - data;
		size_t paddedSize = Utils::Byte::PadTo4Bytes(size);

		// No space for padding.
		if (len - (ptr - data) < paddedSize - size)
		{
			delete foo;
			return nullptr;
		}

		foo->SetInitialSize(paddedSize);

		return foo;
	}

public:
	Foo(uint8_t* buffer) : Serializable(buffer), header(reinterpret_cast<Header*>(buffer))
	{
	}

	/* Serializable class virtual methods. */

	void Dump() const override
	{
		// Nothing here.
	}

	size_t GetSize() const override
	{
		if (!NeedsSerialization())
		{
			return GetCurrentSize();
		}
		else
		{
			size_t size{ 0u };

			size += HeaderSize;

			if (HasAppendix())
			{
				size += AppendixSize;
			}

			if (HasBody())
			{
				size += GetBodyLength();
			}

			// Pad packet to 4 bytes.
			return Utils::Byte::PadTo4Bytes(size);
		}
	}

	void Serialize(uint8_t* buffer) override
	{
		auto size = GetSize();

		uint8_t* ptr = buffer;

		// Copy the header in the new buffer.
		std::memcpy(ptr, this->header, HeaderSize);

		// Update header pointer.
		this->header = reinterpret_cast<Header*>(ptr);

		// Move to the appendix field.
		ptr += HeaderSize;

		// Copy the appendix in the new buffer.
		if (HasAppendix())
		{
			Utils::Byte::Set4Bytes(ptr, 0u, this->appendix);

			// Move to body.
			ptr += AppendixSize;
		}

		// Copy the body in the new buffer.
		if (HasBody())
		{
			std::memcpy(ptr, this->body, GetBodyLength());

			// Update body pointer.
			this->body = ptr;

			// Move to padding.
			ptr += GetBodyLength();
		}

		auto padding = size - (ptr - buffer);

		// Fill padding bytes with zeroes.
		std::memset(ptr, 0x00, padding);

		Serialized(buffer, size);
	}

	/* Foo class methods. */

	uint8_t GetType() const
	{
		return this->header->type;
	}

	void SetType(uint8_t type)
	{
		this->header->type = type;
	}

	bool HasAppendix() const
	{
		return this->header->a;
	}

	uint32_t GetAppendix() const
	{
		return this->appendix;
	}

	void SetAppendix(uint32_t appendix)
	{
		auto hadAppendix = HasAppendix();

		this->header->a = true;
		this->appendix  = appendix;

		if (!hadAppendix)
		{
			SetSerializationNeeded(true);
		}
	}

	void RemoveAppendix()
	{
		auto hadAppendix = HasAppendix();

		this->header->a = false;
		this->appendix  = 0u;

		if (hadAppendix)
		{
			SetSerializationNeeded(true);
		}
	}

	uint8_t GetControl() const
	{
		return this->header->control;
	}

	void SetControl(uint8_t control)
	{
		this->header->control = control;
	}

	bool HasBody() const
	{
		return GetBodyLength() > 0u;
	}

	const uint8_t* GetBody() const
	{
		return this->body;
	}

	uint16_t GetBodyLength() const
	{
		return uint16_t{ ntohs(this->header->bodyLength) };
	}

	void SetBody(uint8_t* body, uint16_t bodyLength)
	{
		this->body               = body;
		this->header->bodyLength = uint16_t{ htons(bodyLength) };

		SetSerializationNeeded(true);
	}

private:
	// Pointer to header.
	Header* header{ nullptr };
	// Appendix value.
	uint32_t appendix{ 0u };
	// Pointer to body.
	uint8_t* body{ nullptr };
};

SCENARIO("Serializable", "[rtc][serializable]")
{
	// clang-format off
	uint8_t buffer[] =
	{
		0x01, 0b10001111, 0x00, 0x02, // Type: 1, A: 1, Control: 15, Body Length: 2
		0x00, 0xBC, 0x61, 0x4E, // Appendix: 12345678
		0xF0, 0x0F, 0x00, 0x00 // Body and 2 bytes of padding.
	};
	// clang-format on

	size_t originalSize{ 12u };
	uint8_t originalType{ 1u };
	bool originalA{ true };
	uint8_t originalControl{ 15u };
	uint16_t originalBodyLength{ 2u };
	uint32_t originalAppendix{ 12345678u };

	auto* foo = Foo::Parse(buffer, sizeof(buffer));

	SECTION("verify parsed Foo")
	{
		REQUIRE(foo != nullptr);
		REQUIRE(foo->NeedsSerialization() == false);
		REQUIRE(foo->GetBuffer() == buffer);
		REQUIRE(foo->GetSize() == originalSize);
		REQUIRE(Utils::Byte::IsPaddedTo4Bytes(foo->GetSize()) == true);
		REQUIRE(foo->GetType() == originalType);
		REQUIRE(foo->HasAppendix() == originalA);
		REQUIRE(foo->GetControl() == originalControl);
		REQUIRE(foo->HasBody() == true);
		REQUIRE(foo->GetBodyLength() == originalBodyLength);
		REQUIRE(foo->GetAppendix() == originalAppendix);
		REQUIRE(helpers::areBuffersEqual(foo->GetBuffer(), foo->GetSize(), buffer, originalSize) == true);
		REQUIRE(
		  helpers::areBuffersEqual(
		    foo->GetBody(), foo->GetBodyLength(), buffer + 8, originalBodyLength) == true);

		delete foo;
	}

	SECTION("modify and serialize parsed Foo")
	{
		uint8_t newBuffer1[100];

		std::memset(newBuffer1, 0xFF, 100);

		foo->RemoveAppendix();

		// After changes, serialization is needed and GetBuffer() throws.
		REQUIRE(foo->NeedsSerialization() == true);
		REQUIRE_THROWS_AS(foo->GetBuffer(), MediaSoupError);

		REQUIRE(foo->GetSize() == originalSize - 4);
		REQUIRE(Utils::Byte::IsPaddedTo4Bytes(foo->GetSize()) == true);
		REQUIRE(foo->HasAppendix() == false);
		REQUIRE(foo->GetAppendix() == 0u);

		foo->Serialize(newBuffer1);

		REQUIRE(foo->NeedsSerialization() == false);
		REQUIRE(foo->GetBuffer() == static_cast<const void*>(newBuffer1));
		REQUIRE(foo->GetSize() == originalSize - 4);
		REQUIRE(Utils::Byte::IsPaddedTo4Bytes(foo->GetSize()) == true);
		REQUIRE(foo->GetType() == originalType);
		REQUIRE(foo->HasAppendix() == false);
		REQUIRE(foo->GetControl() == originalControl);
		REQUIRE(foo->HasBody() == true);
		REQUIRE(foo->GetBodyLength() == originalBodyLength);
		REQUIRE(foo->GetAppendix() == 0u);
		REQUIRE(
		  helpers::areBuffersEqual(foo->GetBuffer(), foo->GetSize(), newBuffer1, originalSize - 4) ==
		  true);
		REQUIRE(
		  helpers::areBuffersEqual(
		    foo->GetBody(), foo->GetBodyLength(), buffer + 8, originalBodyLength) == true);

		uint8_t newBuffer2[100];

		std::memset(newBuffer2, 0xFF, 100);

		uint8_t newBody[] = { 0x11, 0x22, 0x33, 0x44, 0x55 };

		foo->SetType(2u);

		REQUIRE(foo->NeedsSerialization() == false);
		REQUIRE(foo->GetType() == 2u);

		foo->SetControl(14u);

		REQUIRE(foo->NeedsSerialization() == false);
		REQUIRE(foo->GetControl() == 14u);

		foo->SetAppendix(987654321u);

		REQUIRE(foo->NeedsSerialization() == true);
		REQUIRE(foo->HasAppendix() == true);
		REQUIRE(foo->GetAppendix() == 987654321u);

		// New body is 5 bytes long so 3 extra bytes of padding will be generated.
		foo->SetBody(newBody, sizeof(newBody));

		REQUIRE(foo->NeedsSerialization() == true);
		REQUIRE(foo->GetBodyLength() == sizeof(newBody));
		REQUIRE(foo->HasBody() == true);
		REQUIRE(
		  helpers::areBuffersEqual(foo->GetBody(), foo->GetBodyLength(), newBody, sizeof(newBody)) ==
		  true);

		foo->Serialize(newBuffer2);

		REQUIRE(foo->NeedsSerialization() == false);
		REQUIRE(foo->GetBuffer() == static_cast<const void*>(newBuffer2));
		// New size is original size + 4 since new body is 5 bytes so 3 bytes of
		// padding are needed (and original size included 2 bytes of padding).
		REQUIRE(foo->GetSize() == originalSize + 4);
		REQUIRE(Utils::Byte::IsPaddedTo4Bytes(foo->GetSize()) == true);
		REQUIRE(foo->GetType() == 2);
		REQUIRE(foo->HasAppendix() == true);
		REQUIRE(foo->GetControl() == 14u);
		REQUIRE(foo->HasBody() == true);
		REQUIRE(foo->GetBodyLength() == 5u);
		REQUIRE(foo->GetAppendix() == 987654321u);
		REQUIRE(
		  helpers::areBuffersEqual(foo->GetBuffer(), foo->GetSize(), newBuffer2, originalSize + 4) ==
		  true);
		REQUIRE(
		  helpers::areBuffersEqual(foo->GetBody(), foo->GetBodyLength(), newBody, sizeof(newBody)) ==
		  true);

		delete foo;
	}
}
