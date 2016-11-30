#define MS_CLASS "RTC::RtpBuffer"

#include "RTC/RtpBuffer.h"
#include "Logger.h"

namespace RTC
{
	/* Instance methods. */

	RtpBuffer::RtpBuffer(size_t capability) :
		items(capability)
	{
		MS_TRACE();
	}

	RtpBuffer::~RtpBuffer()
	{
		MS_TRACE();

		// Delete cloned packets.
		for (auto& item : this->items)
		{
			if (item.packet)
				delete item.packet;
		}
	}

	void RtpBuffer::Add(RTC::RtpPacket* packet)
	{
		MS_TRACE();

		Item* item = std::addressof(this->items[this->pos]);

		// Delete previous RTP packet (if any).
		if (item->packet)
			delete item->packet;

		// Clone the given RTP packet and store it.
		item->packet = packet->Clone(item->raw);

		this->pos = (this->pos + 1) % this->items.size();
    if (!this->full && this->pos == 0)
			this->full = true;
	}
}
