#ifndef MS_RTC_SERIALIZABLE_FOO_PACKET_HPP
#define MS_RTC_SERIALIZABLE_FOO_PACKET_HPP

#include "common.hpp"
#include "Utils.hpp"
#include "RTC/Serializable.hpp"
#include "RTC/TestSerializable/FooItem.hpp"
#include <vector>

using namespace RTC;

/**
 * FooPacket.
 *
 *  0                   1                   2                   3
 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |      Type     |A|  (Unused)   |            Length             |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                           Appendix                            |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |            Item 1             |            Item 2             |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |            Item 2                             |    Padding    |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 * - Type (8 bits): Unsigned integer.
 * - A (1 bit): Whether the Packet contains an Appendix field.
 * - (Unusued) (7 bits).
 * - Length (16 bits): Length of the Packet excluding padding bytes.
 * - Appendix (32 bits): Unsigned integer. Only exists if flag A is set.
 * - Items (variable length): N items.
 * - Padding: Bytes of padding to make the Packet length be multiple of 4 bytes.
 *
 * It's mandatory that the FooPacket total length is multiple of 4 bytes.
 */

class FooPacket : public Serializable
{
public:
	/**
	 * Struct of a the FooPacket Header.
	 */
	struct Header
	{
		uint8_t type;
#if defined(MS_LITTLE_ENDIAN)
		uint8_t unused : 7;
		uint8_t a : 1;
#elif defined(MS_BIG_ENDIAN)
		uint8_t a : 1;
		uint8_t unused : 7;
#endif
		uint16_t length;
	};

public:
	static const size_t HeaderLength{ 4u };
	static const size_t AppendixLength{ 4u };

public:
	/**
	 * Parse a FooPacket.
	 *
	 * @remarks
	 * - `length` must be the exact length of the Packet.
	 */
	static std::unique_ptr<FooPacket> Parse(const uint8_t* buffer, size_t length);

	static std::unique_ptr<FooPacket> Factory(uint8_t* buffer, size_t bufferLength, uint8_t type);

private:
	/**
	 * Constructor is private because we only want to create FooPacket instances
	 * via Parse() and Factory().
	 */
	FooPacket(const uint8_t* buffer, size_t bufferLength, bool initializeHeader);

public:
	~FooPacket() override;

	void Dump() const override;

	void Serialize(uint8_t* buffer, size_t bufferLength) override;

	std::unique_ptr<Serializable> Clone(uint8_t* buffer, size_t bufferLength) const override;

	uint8_t GetType() const
	{
		return GetHeaderPointer()->type;
	}

	void SetType(uint8_t type)
	{
		GetHeaderPointer()->type = type;
	}

	bool HasAppendix() const
	{
		return GetHeaderPointer()->a;
	}

	uint32_t GetAppendix() const
	{
		if (!HasAppendix())
		{
			return 0u;
		}

		return Utils::Byte::Get4Bytes(GetAppendixPointer(), 0);
	}

	/**
	 * If given `appendix` is 0 then Appendix is removed.
	 */
	void SetAppendix(uint32_t appendix);

	bool HasItems() const
	{
		if (HasAppendix())
		{
			return GetLengthField() > FooPacket::HeaderLength + FooPacket::AppendixLength;
		}
		else
		{
			return GetLengthField() > FooPacket::HeaderLength;
		}
	}

	size_t GetItemsCount() const
	{
		return this->items.size();
	}

	/**
	 * Get the FooItem with index `idx` (starts at 0).
	 */
	const std::unique_ptr<FooItem>& GetItem(size_t idx) const;

	template<typename T>
	const T* GetItem(size_t idx) const;

	/**
	 * Serializes given FooItem into Packet's buffer.
	 *
	 * @remarks
	 * Once this method is called, the FooItem is owned by FooPacket instance.
	 */
	void AddItem(std::unique_ptr<FooItem> item);

private:
	/**
	 * NOTE: Return Header* instead of const Header* since we may want to
	 * modify its fields.
	 */
	Header* GetHeaderPointer() const
	{
		return reinterpret_cast<Header*>(const_cast<uint8_t*>(GetBuffer()));
	}

	/**
	 * NOTE: Private because it returns the value of the Length field, which is
	 * not useful for the application.
	 */
	uint16_t GetLengthField() const
	{
		return uint16_t{ ntohs(GetHeaderPointer()->length) };
	}

	void SetAppendixFlag(bool flag)
	{
		GetHeaderPointer()->a = flag;
	}

	void SetUnusedField()
	{
		GetHeaderPointer()->unused = 0u;
	}

	void SetLengthField(uint16_t length)
	{
		GetHeaderPointer()->length = uint16_t{ htons(length) };
	}

	/**
	 * NOTE: Return uint8_t* instead of const uint8_t* since we may want to
	 * modify its value.
	 */
	uint8_t* GetAppendixPointer() const
	{
		return const_cast<uint8_t*>(GetBuffer()) + FooPacket::HeaderLength;
	}

	const uint8_t* GetItemsPointer() const
	{
		auto* ptr = GetBuffer() + FooPacket::HeaderLength;

		if (HasAppendix())
		{
			ptr += FooPacket::AppendixLength;
		}

		return ptr;
	}

	uint8_t* GetPaddingPointer() const
	{
		return const_cast<uint8_t*>(GetBuffer()) + GetLengthField();
	}

	/**
	 * Must be used within Parse() static method (instead than AddItem()).
	 * This method doesn't serializa the given FooItem into Packet's buffer since
	 * it's already serialized (obviously since we are parsing a buffer).
	 */
	void AddParsedItem(std::unique_ptr<FooItem> item)
	{
		this->items.push_back(std::move(item));
	}

private:
	// FooItem instances.
	std::vector<std::unique_ptr<FooItem>> items;
};

#endif
