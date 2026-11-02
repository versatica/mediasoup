#ifndef MS_RTC_NEW_RTCP_FIR_PACKET_HPP
#define MS_RTC_NEW_RTCP_FIR_PACKET_HPP

#include "common.hpp"
#include "RTC/NEW_RTCP/packet/AnyPayloadSpecificFeedbackPacket.hpp"
#include <vector>

namespace RTC
{
	namespace NEW_RTCP
	{
		/**
		 * Full Intra Request Payload-Specific Feedback Packet (PSFB 206, FMT 4).
		 *
		 * @see RFC 5104.
		 *
		 *  0                   1                   2                   3
		 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |V=2|P|  FMT=4  |  PT=PSFB=206  |             Length            |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |                     SSRC of packet sender                     |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |                      SSRC of media source                     |
		 * +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
		 * |                              SSRC                             |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * | Seq nr.       |    Reserved                                   |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * :                              ...                              :
		 *
		 * - SSRC (32 bits): The SSRC of the media sender that is requested to send
		 *   a decoder refresh point.
		 * - Seq nr. (8 bits): Command sequence number.
		 * - Reserved (24 bits): All set to 0.
		 *
		 * @remarks
		 * - The Feedback Control Information (FCI) contains at least one item.
		 */
		class FirPacket : public AnyPayloadSpecificFeedbackPacket
		{
			// We need that compound packet calls protected and private methods in
			// this class.
			friend class CompoundPacket;

		public:
			/**
			 * Length (in bytes) of an item.
			 */
			static constexpr size_t FirItemLength{ 8 };

		public:
			/**
			 * FIR item.
			 */
			struct Item
			{
				/**
				 * SSRC of the media sender requested to send a decoder refresh point.
				 */
				uint32_t ssrc;
				/**
				 * Command sequence number.
				 */
				uint8_t sequenceNumber;
			};

		public:
			/**
			 * Parse a FirPacket.
			 *
			 * @remarks
			 * - `bufferLength` may exceed the exact length of the packet.
			 */
			static FirPacket* Parse(const uint8_t* buffer, size_t bufferLength);

			/**
			 * Create a FirPacket.
			 *
			 * @remarks
			 * - `bufferLength` could be greater than the packet real length.
			 * - The packet is created with no items, so at least one must be added
			 *   with `AddItem()` before sending it.
			 *
			 * @throw MediaSoupTypeError - If given `bufferLength` is lower than
			 *   `FeedbackHeaderLength`.
			 */
			static FirPacket* Factory(uint8_t* buffer, size_t bufferLength);

		private:
			/**
			 * Parse a FirPacket.
			 *
			 * @remarks
			 * - To be used only by `CompoundPacket::Parse()`.
			 */
			static FirPacket* ParseStrict(
			  const uint8_t* buffer, size_t bufferLength, size_t packetLength, uint8_t padding);

		private:
			/**
			 * Only used by ParseStrict(), Factory(), Clone() and SoftClone() methods.
			 */
			FirPacket(uint8_t* buffer, size_t bufferLength);

		public:
			~FirPacket() override;

			void Dump(int indentation = 0) const final;

			FirPacket* Clone(uint8_t* buffer, size_t bufferLength) const final;

			size_t GetNumberOfItems() const
			{
				return (GetLength() - GetPaddingLength() - AnyFeedbackPacket::FeedbackHeaderLength) /
				       FirPacket::FirItemLength;
			}

			std::vector<Item> GetItems() const;

			/**
			 * Add an item at the end of the packet.
			 *
			 * @throw MediaSoupTypeError - If there is no space in the buffer.
			 */
			void AddItem(uint32_t ssrc, uint8_t sequenceNumber);

		private:
			FirPacket* SoftClone(const uint8_t* buffer) const final;

			uint8_t* GetItemsPointer() const
			{
				return const_cast<uint8_t*>(GetBuffer()) + AnyFeedbackPacket::FeedbackHeaderLength;
			}
		};
	} // namespace NEW_RTCP
} // namespace RTC

#endif
