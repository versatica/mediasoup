#define MS_CLASS "RTC::RtpStreamSend"
// #define MS_LOG_DEV

#include "RTC/RtpStreamSend.h"
#include "Logger.h"

#define RTP_SEQ_MOD (1<<16)
#define MAX_RETRANSMISSION_AGE 200 // Don't retransmit packets older than this (ms).

// Usage:
//   MS_DEBUG_DEV("Leading text "UINT16_TO_BINARY_PATTERN, UINT16_TO_BINARY(value));
#define UINT16_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c"
#define UINT16_TO_BINARY(value) \
	((value & 0x8000) ? '1' : '0'), \
	((value & 0x4000) ? '1' : '0'), \
	((value & 0x2000) ? '1' : '0'), \
	((value & 0x1000) ? '1' : '0'), \
	((value & 0x800) ? '1' : '0'), \
	((value & 0x400) ? '1' : '0'), \
	((value & 0x200) ? '1' : '0'), \
	((value & 0x100) ? '1' : '0'), \
	((value & 0x80) ? '1' : '0'), \
	((value & 0x40) ? '1' : '0'), \
	((value & 0x20) ? '1' : '0'), \
	((value & 0x10) ? '1' : '0'), \
	((value & 0x08) ? '1' : '0'), \
	((value & 0x04) ? '1' : '0'), \
	((value & 0x02) ? '1' : '0'), \
	((value & 0x01) ? '1' : '0')

namespace RTC
{
	/* Instance methods. */

	RtpStreamSend::RtpStreamSend(uint32_t clockRate, size_t bufferSize) :
		RtpStream::RtpStream(clockRate),
		storage(bufferSize)
	{
		MS_TRACE();
	}

	RtpStreamSend::~RtpStreamSend()
	{
		MS_TRACE();

		// Clear buffer.
		ClearBuffer();
	}

	bool RtpStreamSend::ReceivePacket(RTC::RtpPacket* packet)
	{
		MS_TRACE();

		// Call the parent method.
		if (!RtpStream::ReceivePacket(packet))
			return false;

		// If bufferSize was given, store the packet into the buffer.
		if (this->storage.size() > 0)
			StorePacket(packet);

		return true;
	}

	// This method looks for the requested RTP packets and inserts them into the
	// given container (and set to null the next container position).
	void RtpStreamSend::RequestRtpRetransmission(uint16_t seq, uint16_t bitmask, std::vector<RTC::RtpPacket*>& container)
	{
		MS_TRACE();

		// 17: 16 bit mask + the initial sequence number.
		static size_t maxRequestedPackets = 17;

		// Ensure the container's first element is 0.
		container[0] = nullptr;

		// If the buffer is empty just return.
		if (this->buffer.size() == 0)
			return;

		// Convert the given sequence numbers to 32 bits.
		uint32_t first_seq32 = (uint32_t)seq + this->cycles;
		uint32_t last_seq32 = first_seq32 + maxRequestedPackets - 1;

		// Number of requested packets cannot be greater than the container size - 1.
		MS_ASSERT(container.size() - 1 >= maxRequestedPackets, "RtpPacket container is too small");

		auto buffer_it = this->buffer.begin();
		auto buffer_it_r = this->buffer.rbegin();
		uint32_t buffer_first_seq32 = (*buffer_it).seq32;
		uint32_t buffer_last_seq32 = (*buffer_it_r).seq32;

		// Requested packet range not found.
		if (first_seq32 > buffer_last_seq32 || last_seq32 < buffer_first_seq32)
		{
			// Let's try with sequence numbers in the previous 16 cycle.
			if (this->cycles > 0)
			{
				first_seq32 -= RTP_SEQ_MOD;
				last_seq32 -= RTP_SEQ_MOD;

				// Try again.
				if (first_seq32 > buffer_last_seq32 || last_seq32 < buffer_first_seq32)
					return;
			}
			// Otherwise just return.
			else
			{
				MS_WARN_TAG(rtp, "requested packet range not in the buffer");

				return;
			}
		}

		// Look for each requested packet.
		uint32_t seq32 = first_seq32;
		bool requested = true;
		size_t container_idx = 0;

		// Some variables for debugging.
		uint16_t orig_bitmask = bitmask;
		// TODO: Should I set htons here?
		uint16_t sent_bitmask = 0b0000000000000000;
		bool is_first_packet = true;
		bool first_packet_sent = false;
		uint8_t bitmask_counter = 0;
		bool too_old_packet_found = false;

		// TODO: REMOVE
		// MS_WARN_TAG(rtcp, "loop [bitmask:" UINT16_TO_BINARY_PATTERN "]", UINT16_TO_BINARY(bitmask));

		while (requested || bitmask != 0)
		{
			// TODO: REMOVE
			// MS_WARN_TAG(rtcp, "while [bit:%" PRIu8 ", requested:%d]", bitmask_counter, requested);

			bool sent = false;

			if (requested)
			{
				for (; buffer_it != this->buffer.end(); ++buffer_it)
				{
					auto current_seq32 = (*buffer_it).seq32;

					// Found.
					if (current_seq32 == seq32)
					{
						auto current_packet = (*buffer_it).packet;
						uint32_t diff = (this->max_timestamp - current_packet->GetTimestamp()) * 1000 / this->clockRate;

						// Just provide the packet if no older than MAX_RETRANSMISSION_AGE ms.
						if (diff <= MAX_RETRANSMISSION_AGE)
						{
							// Store the packet in the container and then increment its index.
							container[container_idx++] = current_packet;

							sent = true;

							if (is_first_packet)
								first_packet_sent = true;
						}
						else if (!too_old_packet_found)
						{
							MS_WARN_TAG(rtp, "ignoring retransmission for too old packet [max_age:%" PRIu32 "ms, packet_age:%" PRIu32 "ms]", MAX_RETRANSMISSION_AGE, diff);

							too_old_packet_found = true;
						}

						// Exit the loop.
						break;
					}
				}
			}

			requested = (bitmask & 1) ? true : false;
			bitmask >>= 1;
			++seq32;

			if (!is_first_packet)
			{
				sent_bitmask |= (sent ? 1 : 0) << bitmask_counter;
				++bitmask_counter;
			}
			else
			{
				is_first_packet = false;
			}
		}

		// If the first requested packet in the NACK was sent but not all the others,
		// log it.
		if (first_packet_sent && orig_bitmask != sent_bitmask)
		{
			MS_WARN_TAG(rtcp, "first packet sent but not all the bitmask packets [bitmask:" UINT16_TO_BINARY_PATTERN ", sent:" UINT16_TO_BINARY_PATTERN "]",
				UINT16_TO_BINARY(orig_bitmask), UINT16_TO_BINARY(sent_bitmask));
		}
		// TODO: REMOVE
		else if (first_packet_sent && orig_bitmask && orig_bitmask == sent_bitmask)
		{
			MS_WARN_TAG(rtcp, "first packet sent and also all the bitmask packets [bitmask:" UINT16_TO_BINARY_PATTERN "]", UINT16_TO_BINARY(orig_bitmask));
		}

		// Set the next container element to null.
		container[container_idx] = nullptr;
	}

	void RtpStreamSend::ClearBuffer()
	{
		MS_TRACE();

		// Delete cloned packets.
		for (auto& buffer_item : this->buffer)
		{
			delete buffer_item.packet;
		}

		// Clear list.
		this->buffer.clear();
	}

	inline
	void RtpStreamSend::StorePacket(RTC::RtpPacket* packet)
	{
		MS_TRACE();

		// Sum the packet seq number and the number of 16 bits cycles.
		uint32_t packet_seq32 = (uint32_t)packet->GetSequenceNumber() + this->cycles;
		BufferItem buffer_item;

		buffer_item.seq32 = packet_seq32;

		// If empty do it easy.
		if (this->buffer.size() == 0)
		{
			auto store = this->storage[0].store;

			buffer_item.packet = packet->Clone(store);
			this->buffer.push_back(buffer_item);

			return;
		}

		// Otherwise, do the stuff.

		Buffer::iterator new_buffer_it;
		uint8_t* store = nullptr;

		// Iterate the buffer in reverse order and find the proper place to store the
		// packet.
		auto buffer_it_r = this->buffer.rbegin();
		for (; buffer_it_r != this->buffer.rend(); ++buffer_it_r)
		{
			auto current_seq32 = (*buffer_it_r).seq32;

			if (packet_seq32 > current_seq32)
			{
				// Get a forward iterator pointing to the same element.
				auto it = buffer_it_r.base();

				new_buffer_it = this->buffer.insert(it, buffer_item);

				// Exit the loop.
				break;
			}
		}
		// If the packet was older than anything in the buffer, just ignore it.
		// NOTE: This should never happen.
		if (buffer_it_r == this->buffer.rend())
		{
			MS_WARN_TAG(rtp, "packet is older than anything in the buffer, ignoring it");

			return;
		}

		// If the buffer is not full use the next free storage item.
		if (this->buffer.size() - 1 < this->storage.size())
		{
			store = this->storage[this->buffer.size() - 1].store;
		}
		// Otherwise remove the first packet of the buffer and replace its storage area.
		else
		{
			auto& first_buffer_item = *(this->buffer.begin());
			auto first_packet = first_buffer_item.packet;

			// Store points to the store used by the first packet.
			store = (uint8_t*)first_packet->GetData();
			// Free the first packet.
			delete first_packet;
			// Remove the first element in the list.
			this->buffer.pop_front();
		}

		// Update the new buffer item so it points to the cloned packed.
		(*new_buffer_it).packet = packet->Clone(store);
	}

	// TODO: TMP
	void RtpStreamSend::Dump()
	{
		MS_TRACE();

		MS_DEBUG_TAG(rtp, "<RtpStreamSend>");

		MS_DEBUG_TAG(rtp, "  [buffer.size:%zu, storage.size:%zu]", this->buffer.size(), this->storage.size());

		for (auto& buffer_item : this->buffer)
		{
			auto packet = buffer_item.packet;

			MS_DEBUG_TAG(rtp, "  packet [seq:%" PRIu16 ", seq32:%" PRIu32 "]", packet->GetSequenceNumber(), buffer_item.seq32);
		}

		MS_DEBUG_TAG(rtp, "</RtpStreamSend>");
	}
}
