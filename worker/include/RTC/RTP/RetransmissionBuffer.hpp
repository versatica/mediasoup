#ifndef MS_RTC_RTP_RETRANSMISSION_BUFFER_HPP
#define MS_RTC_RTP_RETRANSMISSION_BUFFER_HPP

#include "common.hpp"
#include "RTC/RTP/Codecs/PayloadDescriptorHandler.hpp"
#include "RTC/RTP/Packet.hpp"
#include <deque>

namespace RTC
{
	namespace RTP
	{
		// Special container that stores `Item`* elements addressable by their `uint16_t`
		// sequence number, while only taking as little memory as necessary to store
		// the range covering a maximum of `MaxRetransmissionDelayForVideoMs` or
		//  `MaxRetransmissionDelayForAudioMs` ms.
		class RetransmissionBuffer
		{
		public:
			struct Item
			{
				/**
				 * Original packet.
				 *
				 * @remarks
				 * - Insert() leaves it empty and returns the item so that the caller
				 *   fills it once the packet has been restored to its original state.
				 */
				std::shared_ptr<RTP::Packet> sharedPacket{ nullptr };
				/**
				 * Payload descriptor encoder, holding how the packet was rewritten so
				 * that its retransmissions are rewritten the very same way.
				 *
				 * @remarks
				 * - Insert() leaves it empty, as it does with the packet above. How the
				 *   packet was rewritten is up to the Consumer, since one packet is
				 *   given to all of them and each rewrites it its own way.
				 */
				std::unique_ptr<RTP::Codecs::PayloadDescriptor::Encoder> encoder{ nullptr };
				/**
				 * Correct SSRC since original packet may not have the same.
				 */
				uint32_t ssrc{ 0 };
				/**
				 * Correct sequence number since original packet may not have the same.
				 */
				uint16_t sequenceNumber{ 0 };
				/**
				 * Correct timestamp since original packet may not have the same.
				 */
				uint32_t timestamp{ 0 };
				/**
				 * Correct marker bit since original packet may not have the same.
				 */
				bool marker{ false };
				/**
				 * Time at which this packet was stored.
				 */
				int64_t storedAtMs{ 0 };
				/**
				 * Last time this packet was resent.
				 */
				int64_t resentAtMs{ 0 };
				/**
				 * Number of times this packet was resent.
				 */
				uint8_t sentTimes{ 0 };
			};

		private:
			static Item* CreateItem(RTP::Packet* packet, int64_t nowMs);

		public:
			RetransmissionBuffer(uint16_t maxItems, int64_t maxRetransmissionDelayMs, uint32_t clockRate);
			~RetransmissionBuffer();

			void Dump(int indentation = 0) const;
			Item* Get(uint16_t seq) const;
			/**
			 * Insert the given packet into the buffer.
			 *
			 * @returns The inserted item, whose `sharedPacket` is still empty, or
			 *   nullptr if the packet was not inserted.
			 *
			 * @remarks
			 * - The returned item is owned by the buffer and is only valid until the
			 *   next call that may evict items (Insert(), Clear() or the buffer
			 *   destruction), so the caller must fill it right away.
			 */
			Item* Insert(RTP::Packet* packet, int64_t nowMs);
			void Clear();
			/**
			 * Whether the given stored item was stored longer than the maximum
			 * retransmission delay ago.
			 */
			bool IsTooOld(const Item* item, int64_t nowMs) const;

		private:
			Item* GetOldest() const;
			Item* GetNewest() const;
			void RemoveOldest();
			void RemoveOldest(uint16_t numItems);
			bool ClearTooOldByTimestamp(uint32_t newestTimestamp);
			bool IsTooOldTimestamp(uint32_t timestamp, uint32_t newestTimestamp) const;

		protected:
			// Make buffer protected for testing purposes.
			std::deque<Item*> buffer;

		private:
			// Given as argument.
			uint16_t maxItems;
			int64_t maxRetransmissionDelayMs;
			uint32_t clockRate;
		};
	} // namespace RTP
} // namespace RTC

#endif
