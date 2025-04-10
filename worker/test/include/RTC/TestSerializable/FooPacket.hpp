#ifndef MS_RTC_SERIALIZABLE_FOO_PACKET_HPP
#define MS_RTC_SERIALIZABLE_FOO_PACKET_HPP

#include "common.hpp"
#include "Utils.hpp"
#include "RTC/Serializable.hpp"
#include "RTC/TestSerializable/FooItem.hpp"
#include <string>
#include <vector>

namespace RTC
{
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
	 * - Length (16 bits): Unsigned integer. Length of the Packet excluding padding
	 *   bytes.
	 * - Appendix (32 bits): Unsigned integer. Only exists if flag A is set.
	 * - Items (variable length): N items.
	 * - Padding: Bytes of padding to make the Packet length be multiple of 4
	 *   bytes.
	 *
	 * It's mandatory that the FooPacket total length is multiple of 4 bytes.
	 */

	class FooPacket : public Serializable
	{
	public:
		using ItemsIterator = typename std::vector<FooItem*>::const_iterator;

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
		 * Whether given buffer could be a a valid FooPacket.
		 *
		 * @remarks
		 * - `bufferLength` must be the exact length of the FooPacket.
		 */
		static bool IsFooPacket(const uint8_t* buffer, size_t bufferLength);

		/**
		 * Parse a FooPacket.
		 *
		 * @remarks
		 * - `bufferLength` must be the exact length of the Packet.
		 */
		static FooPacket* Parse(const uint8_t* buffer, size_t bufferLength);

		/**
		 * Create a FooPacket.
		 *
		 * @remarks
		 * - `bufferLength` could be greater than the Packet real length.
		 */
		static FooPacket* Factory(uint8_t* buffer, size_t bufferLength, uint8_t type);

	private:
		/**
		 * Constructor is private because we only want to create FooPacket instances
		 * via Parse() and Factory().
		 */
		FooPacket(const uint8_t* buffer, size_t bufferLength);

	public:
		~FooPacket() override;

		virtual void Dump() const override final;

		virtual void Serialize(uint8_t* buffer, size_t bufferLength) override final;

		virtual FooPacket* Clone(uint8_t* buffer, size_t bufferLength) const override final;

		uint8_t GetType() const
		{
			return GetHeaderPointer()->type;
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

		ItemsIterator ItemsBegin() const
		{
			return this->items.begin();
		}

		ItemsIterator ItemsEnd() const
		{
			return this->items.end();
		}

		const FooItem* GetItemAt(size_t idx) const
		{
			if (idx >= this->items.size())
			{
				return nullptr;
			}

			return this->items[idx];
		}

		/**
		 * Clone given FooItem into Packet's buffer.
		 *
		 * @remarks
		 * Once this method is called, the caller may want to free the original given
		 * FooItem.
		 */
		void AddItem(const FooItem* item);

		/**
		 * Create and add a FooNumericItem in the packet.
		 */
		void AddNumericItem(uint8_t flags, uint16_t number);

		/**
		 * Create and add a FooTextItem in the packet.
		 */
		void AddTextItem(uint8_t flags, const std::string& text);

	private:
		void InitializeHeader(uint8_t type, uint16_t length);

		Header* GetHeaderPointer() const
		{
			return reinterpret_cast<Header*>(const_cast<uint8_t*>(GetBuffer()));
		}

		void SetAppendixFlag(bool flag)
		{
			GetHeaderPointer()->a = flag;
		}

		/**
		 * NOTE: Private because it returns the value of the Length field, which is
		 * not useful for the application.
		 */
		uint16_t GetLengthField() const
		{
			return uint16_t{ ntohs(GetHeaderPointer()->length) };
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
		void AddParsedItem(FooItem* item);

	private:
		// FooItem instances.
		std::vector<FooItem*> items;
	};
} // namespace RTC

#endif
