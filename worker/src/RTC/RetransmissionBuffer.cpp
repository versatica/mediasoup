#define MS_CLASS "RTC::RetransmissionBuffer"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/RetransmissionBuffer.hpp"
#include "Logger.hpp"
#include "RTC/SeqManager.hpp"

namespace RTC
{
	/* Instance methods. */

	RetransmissionBuffer::RetransmissionBuffer(
	  uint16_t maxItems, uint32_t maxRetransmissionDelayMs, uint32_t clockRate)
	  : maxItems(maxItems), maxRetransmissionDelayMs(maxRetransmissionDelayMs), clockRate(clockRate)
	{
		MS_TRACE();

		MS_ASSERT(maxItems > 0u, "maxItems must be greater than 0");
	}

	RetransmissionBuffer::~RetransmissionBuffer()
	{
		MS_TRACE();

		Clear();
	}

	RetransmissionBuffer::Item* RetransmissionBuffer::Get(uint16_t seq) const
	{
		MS_TRACE();

		if (this->buffer.empty())
		{
			return nullptr;
		}

		if (RTC::SeqManager<uint16_t>::IsSeqLowerThan(seq, this->startSeq))
		{
			return nullptr;
		}

		auto idx = static_cast<uint16_t>(seq - this->startSeq);

		if (idx > static_cast<uint16_t>(this->buffer.size() - 1))
		{
			return nullptr;
		}

		return this->buffer.at(idx);
	}

	/**
	 * This method tries to insert given packet into the buffer. Here we assume
	 * that packet seq number is legitimate according to the content of the buffer.
	 * We discard the packet if too old and also discard it if its timestamp does
	 * not properly fit (by ensuring that elements in the buffer are not only
	 * ordered by increasing seq but also that their timestamp are incremental).
	 */
	void RetransmissionBuffer::Insert(RTC::RtpPacket* packet, std::shared_ptr<RTC::RtpPacket>& sharedPacket)
	{
		MS_TRACE();

		auto ssrc      = packet->GetSsrc();
		auto seq       = packet->GetSequenceNumber();
		auto timestamp = packet->GetTimestamp();

		MS_DEBUG_DEV("packet [seq:%" PRIu16 ", timestamp:%" PRIu32 "]", seq, timestamp);

		// Buffer is empty, so just insert new item.
		if (this->buffer.empty())
		{
			MS_DEBUG_DEV("buffer empty [seq:%" PRIu16 ", timestamp:%" PRIu32 "]", seq, timestamp);

			auto* item = new Item();

			this->buffer.push_back(FillItem(item, packet, sharedPacket));

			// Packet's seq number becomes startSeq.
			this->startSeq = seq;

			return;
		}

		// Clear too old packets in the buffer.
		ClearTooOld();

		auto* oldestItem = GetOldest();
		auto* newestItem = GetNewest();

		MS_ASSERT(oldestItem != nullptr, "oldest item doesn't exist");
		MS_ASSERT(newestItem != nullptr, "newest item doesn't exist");

		// Packet arrived in order (its seq is higher than seq of the newest stored
		// packet) so will become the newest one in the buffer.
		if (RTC::SeqManager<uint16_t>::IsSeqHigherThan(seq, newestItem->sequenceNumber))
		{
			MS_DEBUG_DEV("packet in order [seq:%" PRIu16 ", timestamp:%" PRIu32 "]", seq, timestamp);

			// Ensure that the timestamp of the packet is equal or higher than the
			// timestamp of the newest stored packet.
			if (RTC::SeqManager<uint32_t>::IsSeqLowerThan(timestamp, newestItem->timestamp))
			{
				MS_WARN_TAG(
				  rtp,
				  "packet has higher seq but less timestamp than newest packet in the buffer, discarding it [ssrc:%" PRIu32
				  ", seq:%" PRIu16 ", timestamp:%" PRIu32 "]",
				  ssrc,
				  seq,
				  timestamp);

				return;
			}

			// Calculate how many blank slots it would be necessary to add when
			// pushing new item to the back of the buffer.
			auto numBlankSlots = static_cast<uint16_t>(seq - newestItem->sequenceNumber - 1);

			// We may have to remove oldest items not to exceed the maximum size of
			// the buffer.
			if (this->buffer.size() + numBlankSlots + 1 > this->maxItems)
			{
				auto numItemsToRemove =
				  static_cast<uint16_t>(this->buffer.size() + numBlankSlots + 1 - this->maxItems);

				// If num of items to be removed exceed buffer size minus one (needed to
				// allocate current packet) then we must clear the entire buffer.
				if (numItemsToRemove > this->buffer.size() - 1)
				{
					MS_WARN_TAG(
					  rtp,
					  "packet has too high seq and forces buffer emptying [ssrc:%" PRIu32 ", seq:%" PRIu16
					  ", timestamp:%" PRIu32 "]",
					  ssrc,
					  seq,
					  timestamp);

					numBlankSlots = 0u;
					Clear();
				}
				else
				{
					MS_DEBUG_DEV(
					  "calling RemoveOldest(%" PRIu16 ") [bufferSize:%zu, numBlankSlots:%" PRIu16
					  ", maxItems:%" PRIu16 "]",
					  numItemsToRemove,
					  this->buffer.size(),
					  numBlankSlots,
					  this->maxItems);

					RemoveOldest(numItemsToRemove);
				}
			}

			// Push blank slots to the back.
			for (uint16_t i{ 0u }; i < numBlankSlots; ++i)
			{
				this->buffer.push_back(nullptr);
			}

			// Push the packet, which becomes the newest one in the buffer.
			auto* item = new Item();

			this->buffer.push_back(FillItem(item, packet, sharedPacket));
		}
		// Packet arrived out order and its seq is less than seq of the oldest
		// stored packet, so will become the oldest one in the buffer.
		else if (RTC::SeqManager<uint16_t>::IsSeqLowerThan(seq, oldestItem->sequenceNumber))
		{
			MS_DEBUG_DEV(
			  "packet out of order and older than oldest packet in the buffer [seq:%" PRIu16
			  ", timestamp:%" PRIu32 "]",
			  seq,
			  timestamp);

			// Ensure that packet is not too old to be stored.
			if (IsTooOld(timestamp, newestItem->timestamp))
			{
				MS_WARN_DEV(
				  "packet too old, discarding it [seq:%" PRIu16 ", timestamp:%" PRIu32 "]", seq, timestamp);

				return;
			}

			// Ensure that the timestamp of the packet is equal or less than the
			// timestamp of the oldest stored packet.
			if (RTC::SeqManager<uint32_t>::IsSeqHigherThan(timestamp, oldestItem->timestamp))
			{
				MS_WARN_TAG(
				  rtp,
				  "packet has less seq but higher timestamp than oldest packet in the buffer, discarding it [ssrc:%" PRIu32
				  ", seq:%" PRIu16 ", timestamp:%" PRIu32 "]",
				  ssrc,
				  seq,
				  timestamp);

				return;
			}

			// Calculate how many blank slots it would be necessary to add when
			// pushing new item to the fton of the buffer.
			auto numBlankSlots = static_cast<uint16_t>(oldestItem->sequenceNumber - seq - 1);

			// If adding this packet (and needed blank slots) to the front makes the
			// buffer exceed its max size, discard this packet.
			if (this->buffer.size() + numBlankSlots + 1 > this->maxItems)
			{
				MS_WARN_TAG(
				  rtp,
				  "discarding received old packet to not exceed max buffer size [ssrc:%" PRIu32
				  ", seq:%" PRIu16 ", timestamp:%" PRIu32 "]",
				  ssrc,
				  seq,
				  timestamp);

				return;
			}

			// Push blank slots to the front.
			for (uint16_t i{ 0u }; i < numBlankSlots; ++i)
			{
				this->buffer.push_front(nullptr);
			}

			// Insert the packet, which becomes the oldest one in the buffer.
			auto* item = new Item();

			this->buffer.push_front(FillItem(item, packet, sharedPacket));

			// Packet's seq number becomes startSeq.
			this->startSeq = seq;
		}
		// Otherwise packet must be inserted between oldest and newest stored items
		// so there is already an allocated slot for it.
		else
		{
			MS_DEBUG_DEV(
			  "packet out of order and in between oldest and newest packets in the buffer [seq:%" PRIu16
			  ", timestamp:%" PRIu32 "]",
			  seq,
			  timestamp);

			// Let's check if an item already exist in same position. If so, assume
			// it's duplicated.
			auto* item = Get(seq);

			if (item)
			{
				MS_DEBUG_DEV(
				  "packet already in the buffer, discarding [seq:%" PRIu16 ", timestamp:%" PRIu32 "]",
				  seq,
				  timestamp);

				return;
			}

			// idx is the intended position of the received packet in the buffer.
			auto idx = static_cast<uint16_t>(seq - this->startSeq);

			// Validate that packet timestamp is equal or higher than the timestamp of
			// the immediate older packet (if any).
			for (auto idx2 = static_cast<int32_t>(idx - 1); idx2 >= 0; --idx2)
			{
				auto* olderItem = this->buffer.at(idx2);

				// Blank slot, continue.
				if (!olderItem)
				{
					continue;
				}

				// We are done.
				if (timestamp >= olderItem->timestamp)
				{
					break;
				}
				else
				{
					MS_WARN_TAG(
					  rtp,
					  "packet timestamp is less than timestamp of immediate older packet in the buffer, discarding it [ssrc:%" PRIu32
					  ", seq:%" PRIu16 ", timestamp:%" PRIu32 "]",
					  ssrc,
					  seq,
					  timestamp);

					return;
				}
			}

			// Validate that packet timestamp is equal or less than the timestamp of
			// the immediate newer packet (if any).
			for (auto idx2 = static_cast<size_t>(idx + 1); idx2 < this->buffer.size(); ++idx2)
			{
				auto* newerItem = this->buffer.at(idx2);

				// Blank slot, continue.
				if (!newerItem)
				{
					continue;
				}

				// We are done.
				if (timestamp <= newerItem->timestamp)
				{
					break;
				}
				else
				{
					MS_WARN_TAG(
					  rtp,
					  "packet timestamp is higher than timestamp of immediate newer packet in the buffer, discarding it [ssrc:%" PRIu32
					  ", seq:%" PRIu16 ", timestamp:%" PRIu32 "]",
					  ssrc,
					  seq,
					  timestamp);

					return;
				}
			}

			// Store the packet.
			item = new Item();

			this->buffer[idx] = FillItem(item, packet, sharedPacket);
		}

		MS_ASSERT(
		  this->buffer.size() <= this->maxItems,
		  "buffer contains %zu items (more than %" PRIu16 " max items)",
		  this->buffer.size(),
		  this->maxItems);
	}

	void RetransmissionBuffer::Clear()
	{
		MS_TRACE();

		for (auto* item : this->buffer)
		{
			if (!item)
			{
				continue;
			}

			// Reset the stored item (decrease RTP packet shared pointer counter).
			item->Reset();

			delete item;
		}

		this->buffer.clear();
		this->startSeq = 0u;
	}

	void RetransmissionBuffer::Dump() const
	{
		MS_TRACE();

		MS_DUMP("<RetransmissionBuffer>");
		MS_DUMP("  buffer [size:%zu, maxSize:%" PRIu16 "]", this->buffer.size(), this->maxItems);
		if (this->buffer.size() > 0)
		{
			const auto* oldestItem = GetOldest();
			const auto* newestItem = GetNewest();

			MS_DUMP(
			  "  oldest item [seq:%" PRIu16 ", timestamp:%" PRIu32 "]",
			  oldestItem->sequenceNumber,
			  oldestItem->timestamp);
			MS_DUMP(
			  "  newest item [seq:%" PRIu16 ", timestamp:%" PRIu32 "]",
			  newestItem->sequenceNumber,
			  newestItem->timestamp);
			MS_DUMP(
			  "  buffer window: %" PRIu32 "ms",
			  static_cast<uint32_t>(newestItem->timestamp * 1000 / this->clockRate) -
			    static_cast<uint32_t>(oldestItem->timestamp * 1000 / this->clockRate));
		}
		MS_DUMP("</RetransmissionBuffer>");
	}

	RetransmissionBuffer::Item* RetransmissionBuffer::GetOldest() const
	{
		MS_TRACE();

		return this->Get(this->startSeq);
	}

	RetransmissionBuffer::Item* RetransmissionBuffer::GetNewest() const
	{
		MS_TRACE();

		return this->Get(this->startSeq + this->buffer.size() - 1);
	}

	void RetransmissionBuffer::RemoveOldest()
	{
		MS_TRACE();

		if (this->buffer.empty())
		{
			return;
		}

		auto* item = this->buffer.at(0);

		// Reset the stored item (decrease RTP packet shared pointer counter).
		item->Reset();

		delete item;

		this->buffer.pop_front();
		this->startSeq++;

		MS_DEBUG_DEV("removed 1 item from the front");

		// Remove all nullptr elements from the beginning of the buffer.
		// NOTE: Calling front on an empty container is undefined.
		size_t numItemsRemoved{ 0u };

		while (!this->buffer.empty() && this->buffer.front() == nullptr)
		{
			this->buffer.pop_front();
			this->startSeq++;

			++numItemsRemoved;
		}

		if (numItemsRemoved)
		{
			MS_DEBUG_DEV("removed 1 blank slot from the front");
		}

		// If we emptied the full buffer, reset startSeq.
		if (this->buffer.empty())
		{
			this->startSeq = 0u;
		}
	}

	void RetransmissionBuffer::RemoveOldest(uint16_t numItems)
	{
		MS_TRACE();

		MS_ASSERT(
		  numItems <= this->buffer.size(),
		  "attempting to remove more items than current buffer size [numItems:%" PRIu16
		  ", bufferSize:%zu]",
		  numItems,
		  this->buffer.size());

		auto intendedBufferSize = this->buffer.size() - numItems;

		while (this->buffer.size() > intendedBufferSize)
		{
			RemoveOldest();
		}
	}

	void RetransmissionBuffer::ClearTooOld()
	{
		MS_TRACE();

		const auto* newestItem = GetNewest();

		if (!newestItem)
		{
			return;
		}

		RetransmissionBuffer::Item* oldestItem{ nullptr };

		// Go through all buffer items starting with the first and free all items
		// that contain too old packets.
		while ((oldestItem = GetOldest()))
		{
			if (IsTooOld(oldestItem->timestamp, newestItem->timestamp))
			{
				RemoveOldest();
			}
			// If current oldest stored packet is not too old, exit the loop since we
			// know that packets stored after it are guaranteed to be newer.
			else
			{
				break;
			}
		}
	}

	bool RetransmissionBuffer::IsTooOld(uint32_t timestamp, uint32_t newestTimestamp) const
	{
		MS_TRACE();

		if (RTC::SeqManager<uint32_t>::IsSeqHigherThan(timestamp, newestTimestamp))
		{
			return false;
		}

		const int64_t diffTs = newestTimestamp - timestamp;

		return static_cast<uint32_t>(diffTs * 1000 / this->clockRate) > this->maxRetransmissionDelayMs;
	}

	RetransmissionBuffer::Item* RetransmissionBuffer::FillItem(
	  RetransmissionBuffer::Item* item,
	  RTC::RtpPacket* packet,
	  std::shared_ptr<RTC::RtpPacket>& sharedPacket) const
	{
		MS_TRACE();

		// Store original packet into the item. Only clone once and only if
		// necessary.
		//
		// NOTE: This must be done BEFORE assigning item->packet = sharedPacket,
		// otherwise the value being copied in item->packet will remain nullptr.
		// This is because we are copying an **empty** shared_ptr into another
		// shared_ptr (item->packet), so future value assigned via reset() in the
		// former doesn't update the value in the copy.
		if (!sharedPacket.get())
		{
			sharedPacket.reset(packet->Clone());
		}

		// Store original packet and some extra info into the item.
		item->packet         = sharedPacket;
		item->ssrc           = packet->GetSsrc();
		item->sequenceNumber = packet->GetSequenceNumber();
		item->timestamp      = packet->GetTimestamp();

		return item;
	}

	void RetransmissionBuffer::Item::Reset()
	{
		MS_TRACE();

		this->packet.reset();
		this->ssrc           = 0u;
		this->sequenceNumber = 0u;
		this->timestamp      = 0u;
		this->resentAtMs     = 0u;
		this->sentTimes      = 0u;
	}
} // namespace RTC
