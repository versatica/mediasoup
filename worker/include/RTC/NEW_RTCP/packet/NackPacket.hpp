#ifndef MS_RTC_NEW_RTCP_NACK_PACKET_HPP
#define MS_RTC_NEW_RTCP_NACK_PACKET_HPP

#include "common.hpp"
#include "RTC/NEW_RTCP/packet/AnyTransportLayerFeedbackPacket.hpp"
#include <vector>

namespace RTC
{
	namespace NEW_RTCP
	{
		/**
		 * Generic NACK Transport Layer Feedback Packet (RTPFB 205, FMT 1).
		 *
		 * @see RFC 4585.
		 *
		 *  0                   1                   2                   3
		 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |V=2|P|  FMT=1  |  PT=RTPFB=205 |             Length            |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |                     SSRC of packet sender                     |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |                      SSRC of media source                     |
		 * +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
		 * |              PID              |              BLP              |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * :                              ...                              :
		 *
		 * - Packet ID (PID) (16 bits): The RTP sequence number of a lost packet.
		 * - Bitmask of following lost packets (BLP) (16 bits): Bit i set to 1
		 *   means that the packet with sequence number PID + i + 1 is also lost.
		 *
		 * @remarks
		 * - The Feedback Control Information (FCI) contains at least one item.
		 */
		class NackPacket : public AnyTransportLayerFeedbackPacket
		{
			// We need that compound packet calls protected and private methods in
			// this class.
			friend class CompoundPacket;

		public:
			/**
			 * Length (in bytes) of an item.
			 */
			static constexpr size_t NackItemLength{ 4 };

		public:
			/**
			 * NACK item.
			 */
			struct Item
			{
				/**
				 * Packet ID (PID).
				 */
				uint16_t packetId;
				/**
				 * Bitmask of following lost packets (BLP).
				 */
				uint16_t lostPacketBitmask;
			};

		public:
			/**
			 * Parse a NackPacket.
			 *
			 * @remarks
			 * - `bufferLength` may exceed the exact length of the packet.
			 */
			static NackPacket* Parse(const uint8_t* buffer, size_t bufferLength);

			/**
			 * Create a NackPacket.
			 *
			 * @remarks
			 * - `bufferLength` could be greater than the packet real length.
			 * - The packet is created with no items, so at least one must be added
			 *   with `AddItem()` before sending it.
			 *
			 * @throw MediaSoupTypeError - If given `bufferLength` is lower than
			 *   `FeedbackHeaderLength`.
			 */
			static NackPacket* Factory(uint8_t* buffer, size_t bufferLength);

			/**
			 * Length (in bytes) of a NackPacket with the given number of items.
			 */
			static size_t ComputeLengthFor(size_t numItems);

		private:
			/**
			 * Parse a NackPacket.
			 *
			 * @remarks
			 * - To be used only by `CompoundPacket::Parse()`.
			 */
			static NackPacket* ParseStrict(
			  const uint8_t* buffer, size_t bufferLength, size_t packetLength, uint8_t padding);

		private:
			/**
			 * Only used by ParseStrict(), Factory(), Clone() and SoftClone() methods.
			 */
			NackPacket(uint8_t* buffer, size_t bufferLength);

		public:
			~NackPacket() override;

			void Dump(int indentation = 0) const final;

			NackPacket* Clone(uint8_t* buffer, size_t bufferLength) const final;

			size_t GetNumberOfItems() const
			{
				return (GetLength() - GetPaddingLength() - AnyFeedbackPacket::FeedbackHeaderLength) /
				       NackPacket::NackItemLength;
			}

			std::vector<Item> GetItems() const;

			/**
			 * Add an item at the end of the packet.
			 *
			 * @throw MediaSoupTypeError - If there is no space in the buffer.
			 */
			void AddItem(uint16_t packetId, uint16_t lostPacketBitmask);

		private:
			NackPacket* SoftClone(const uint8_t* buffer) const final;

			uint8_t* GetItemsPointer() const
			{
				return const_cast<uint8_t*>(GetBuffer()) + AnyFeedbackPacket::FeedbackHeaderLength;
			}
		};
	} // namespace NEW_RTCP
} // namespace RTC

#endif
