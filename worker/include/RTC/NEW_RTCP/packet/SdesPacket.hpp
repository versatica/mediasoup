#ifndef MS_RTC_NEW_RTCP_SDES_PACKET_HPP
#define MS_RTC_NEW_RTCP_SDES_PACKET_HPP

#include "common.hpp"
#include "RTC/NEW_RTCP/packet/Packet.hpp"
#include <ankerl/unordered_dense.h>
#include <initializer_list>
#include <string>
#include <string_view>
#include <vector>

namespace RTC
{
	namespace NEW_RTCP
	{
		/**
		 * SDES RTCP Packet (202).
		 *
		 * @see RFC 3550.
		 *
		 *  0                   1                   2                   3
		 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |V=2|P|    SC   |  PT=SDES=202  |             Length            |
		 * +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
		 * |                          SSRC/CSRC_1                          |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |                           SDES items                          |
		 * |                              ...                              |
		 * +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
		 * |                          SSRC/CSRC_2                          |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |                           SDES items                          |
		 * |                              ...                              |
		 * +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
		 *
		 * - Packet type (8 bits): 202.
		 * - Source count (SC) (5 bits): The number of SSRC/CSRC chunks contained
		 *   in this SDES packet.
		 * - Chunk: A SSRC/CSRC identifier followed by a list of zero or more
		 *   items. Each chunk starts on a 32-bit boundary.
		 *
		 * Each SDES item:
		 *
		 *  0                   1                   2                   3
		 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |     Type      |    Length     |         Value              ...
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 *
		 * - Type (8 bits): Item type.
		 * - Length (8 bits): Number of bytes of the value.
		 * - Value (variable length): Item value, with no padding.
		 *
		 * The list of items in each chunk is terminated by a null octet (which is
		 * interpreted as an item type of 0), followed by null octets to pad the
		 * chunk until the next 32-bit boundary.
		 *
		 * @remarks
		 * - The value of every item is exposed as a sequence of bytes, so the
		 *   prefix of the PRIV item is not interpreted.
		 */
		class SdesPacket : public Packet
		{
			// We need that compound packet calls protected and private methods in
			// this class.
			friend class CompoundPacket;

		public:
			/**
			 * SDES Item Type.
			 */
			enum class ItemType : uint8_t
			{
				CNAME = 1,
				NAME  = 2,
				EMAIL = 3,
				PHONE = 4,
				LOC   = 5,
				TOOL  = 6,
				NOTE  = 7,
				PRIV  = 8,
			};

			/**
			 * SDES item and the SSRC/CSRC of the chunk it belongs to.
			 */
			struct Item
			{
				/**
				 * SSRC/CSRC of the chunk.
				 */
				uint32_t ssrc;
				/**
				 * Item type.
				 */
				ItemType type;
				/**
				 * Item value.
				 */
				std::string_view value;
			};

		public:
			/**
			 * Maximum number of chunks in a SDES packet (the SC field has 5 bits).
			 */
			static constexpr size_t MaxChunks{ 31 };

		public:
			static const std::string& ItemTypeToString(ItemType itemType);

			/**
			 * Parse a SdesPacket.
			 *
			 * @remarks
			 * - `bufferLength` may exceed the exact length of the packet.
			 */
			static SdesPacket* Parse(const uint8_t* buffer, size_t bufferLength);

			/**
			 * Create a SdesPacket.
			 *
			 * @remarks
			 * - `bufferLength` could be greater than the packet real length.
			 */
			static SdesPacket* Factory(uint8_t* buffer, size_t bufferLength);

			/**
			 * Length (in bytes) of a SdesPacket carrying the given chunks.
			 *
			 * @param chunksLength - Sum of what `ComputeChunkLengthFor()` returns for
			 *   each chunk the packet will carry.
			 */
			static size_t ComputeLengthFor(size_t chunksLength);

			/**
			 * Length (in bytes) of a chunk carrying items with values of the given
			 * lengths, padding included.
			 */
			static size_t ComputeChunkLengthFor(std::initializer_list<size_t> itemValueLengths);

		private:
			/**
			 * Parse a SdesPacket.
			 *
			 * @remarks
			 * - To be used only by `CompoundPacket::Parse()`.
			 */
			static SdesPacket* ParseStrict(
			  const uint8_t* buffer, size_t bufferLength, size_t packetLength, uint8_t padding);

		private:
			static const ankerl::unordered_dense::map<ItemType, std::string> ItemType2String;

		private:
			/**
			 * Only used by ParseStrict(), Factory(), Clone() and SoftClone() methods.
			 */
			SdesPacket(uint8_t* buffer, size_t bufferLength);

		public:
			~SdesPacket() override;

			void Dump(int indentation = 0) const final;

			SdesPacket* Clone(uint8_t* buffer, size_t bufferLength) const final;

			/**
			 * Value of the SC field.
			 */
			uint8_t GetNumberOfChunks() const
			{
				return GetCount();
			}

			/**
			 * All the items of all the chunks, in order.
			 *
			 * @remarks
			 * - A chunk with no items doesn't contribute any entry.
			 */
			std::vector<Item> GetItems() const;

			/**
			 * Add a chunk with no items at the end of the packet.
			 *
			 * @throw MediaSoupTypeError - If the packet already has `MaxChunks`
			 *   chunks or if there is no space in the buffer.
			 */
			void AddChunk(uint32_t ssrc);

			/**
			 * Add an item to the last chunk of the packet.
			 *
			 * @throw MediaSoupTypeError - If the packet has no chunks, if `type` is
			 *   0, if `value` is longer than 255 bytes or if there is no space in the
			 *   buffer.
			 */
			void AddItem(ItemType type, std::string_view value);

		private:
			SdesPacket* SoftClone(const uint8_t* buffer) const final;

			/**
			 * Pointer to the null octet that terminates the list of items of the
			 * last chunk.
			 *
			 * @remarks
			 * - The packet must have at least one chunk.
			 */
			uint8_t* GetLastChunkItemsEndPointer() const;
		};
	} // namespace NEW_RTCP
} // namespace RTC

#endif
