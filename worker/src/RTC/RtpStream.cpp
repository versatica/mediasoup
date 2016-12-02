// TODO: https://tools.ietf.org/html/rfc3550#appendix-A.1

#define MS_CLASS "RTC::RtpStream"

#include "RTC/RtpStream.h"
#include "Logger.h"

namespace RTC
{
	/* Instance methods. */

	RtpStream::RtpStream(size_t bufferSize) :
		storage(bufferSize)
	{
		MS_TRACE();
	}

	RtpStream::~RtpStream()
	{
		MS_TRACE();

		// Delete cloned packets.
		for (auto& packet : this->packets)
		{
			delete packet;
		}
	}

	bool RtpStream::AddPacket(RTC::RtpPacket* packet)
	{
		MS_TRACE();

		MS_WARN("BEGIN | seq_number: %" PRIu16, packet->GetSequenceNumber());

		// If empty do it easy.
		if (this->packets.size() == 0)
		{
			MS_WARN("  empty packet list");

			auto store = this->storage[0].store;
			auto cloned_packet = packet->Clone(store);

			this->packets.push_back(cloned_packet);

			MS_WARN("END   | seq_number: %" PRIu16, packet->GetSequenceNumber());

			return true;
		}

		// Otherwise, do the stuff.

		Packets::iterator new_it;
		uint8_t* store = nullptr;

		auto it_r = this->packets.rbegin();
		for (; it_r != this->packets.rend(); it_r++)
		{
			auto current_packet = *it_r;

			if (packet->GetSequenceNumber() > current_packet->GetSequenceNumber())
			{
				// Get a forward iterator pointing to the same element.
				auto it = it_r.base();

				new_it = this->packets.insert(it, nullptr);

				// Exit the loop.
				break;
			}
		}
		// If the packet was older than anything in the storate, just ignore it.
		if (it_r == this->packets.rend())
		{
			// TODO: Wrong, this will drop all the received packets once sequence_number
			// reaches 65535

			MS_WARN("  packet is older than anything in the storage, ignoring it");
			MS_WARN("END   | seq_number: %" PRIu16, packet->GetSequenceNumber());

			return false;
		}

		MS_WARN("  packets.size:%zu, storage.size:%zu", this->packets.size(), this->storage.size());

		// If the storage is not full use the next free area.
		if (this->packets.size() - 1 < this->storage.size())
		{
			MS_WARN("  storage not full");

			// Store points to the next free mem area.
			store = this->storage[this->packets.size() - 1].store;
		}
		// Otherwise remove the first packet list entry and replace its storage area.
		else
		{
			auto first_packet = *(this->packets.begin());

			MS_WARN("  storage full, deleting first list packet with seq_number: %" PRIu16, first_packet->GetSequenceNumber());

			// Store points to the store used by the first packet.
			store = (uint8_t*)first_packet->GetRaw();
			// Free the first packet.
			delete first_packet;
			// Remove the first element in the list.
			this->packets.pop_front();
		}

		// Update the new list entry so it points to the cloned packed.
		*new_it = packet->Clone(store);

		MS_WARN("END   | seq_number: %" PRIu16, packet->GetSequenceNumber());

		MS_WARN("<DUMP>");
		for (auto& packet : this->packets)
		{
			MS_WARN("  - seq_number: %" PRIu16, packet->GetSequenceNumber());
		}
		MS_WARN("</DUMP>\n");

		return true;
	}
}
