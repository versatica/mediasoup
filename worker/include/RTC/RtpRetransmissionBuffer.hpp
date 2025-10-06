#ifndef MS_RTC_RTP_RETRANSMISSION_BUFFER_HPP
#define MS_RTC_RTP_RETRANSMISSION_BUFFER_HPP

#include "common.hpp"
#include "RTC/Codecs/PayloadDescriptorHandler.hpp"
#include "RTC/RtpPacket.hpp"
#include "RTC/SharedRtpPacket.hpp"
#include <deque>

namespace RTC
{
	// Special container that stores `Item`* elements addressable by their `uint16_t`
	// sequence number, while only taking as little memory as necessary to store
	// the range covering a maximum of `MaxRetransmissionDelayForVideoMs` or
	//  `MaxRetransmissionDelayForAudioMs` ms.
	class RtpRetransmissionBuffer
	{
	public:
		struct Item
		{
			void Reset();

			// Original packet.
			RTC::SharedRtpPacket sharedPacket{ nullptr };
			// Payload descriptor encoder.
			std::unique_ptr<RTC::Codecs::PayloadDescriptor::Encoder> encoder{ nullptr };
			// Correct SSRC since original packet may not have the same.
			uint32_t ssrc{ 0u };
			// Correct sequence number since original packet may not have the same.
			uint16_t sequenceNumber{ 0u };
			// Correct timestamp since original packet may not have the same.
			uint32_t timestamp{ 0u };
			// Correct marker bit since original packet may not have the same.
			bool marker{ false };
			// Last time this packet was resent.
			uint64_t resentAtMs{ 0u };
			// Number of times this packet was resent.
			uint8_t sentTimes{ 0u };
		};

	private:
		static Item* FillItem(Item* item, RTC::RtpPacket* packet, const RTC::SharedRtpPacket& sharedPacket);

	public:
		RtpRetransmissionBuffer(uint16_t maxItems, uint32_t maxRetransmissionDelayMs, uint32_t clockRate);
		~RtpRetransmissionBuffer();

		void Dump(int indentation = 0) const;
		Item* Get(uint16_t seq) const;
		bool Insert(RTC::RtpPacket* packet, const RTC::SharedRtpPacket& sharedPacket);
		void Clear();

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
		uint32_t maxRetransmissionDelayMs;
		uint32_t clockRate;
	};
} // namespace RTC

#endif
