// DOC: https://tools.ietf.org/html/rfc3550#appendix-A.1

#define MS_CLASS "RTC::RtpStream"
// #define MS_LOG_DEV

#include "RTC/RtpStream.h"
#include "DepLibUV.h"
#include "Logger.h"

#define MIN_SEQUENTIAL 2
#define MAX_DROPOUT 3000
#define MAX_MISORDER 100
#define RTP_SEQ_MOD (1<<16)

namespace RTC
{
	/* Instance methods. */

	RtpStream::RtpStream(uint32_t clockRate, size_t bufferSize) :
		clockRate(clockRate),
		storage(bufferSize)
	{
		MS_TRACE();
	}

	RtpStream::~RtpStream()
	{
		MS_TRACE();

		// Clear buffer.
		CleanBuffer();
	}

	bool RtpStream::ReceivePacket(RTC::RtpPacket* packet)
	{
		MS_TRACE();

		uint16_t seq = packet->GetSequenceNumber();

		// If this is the first packet seen, initialize stuff.
		if (!this->started)
		{
			this->started = true;

			InitSeq(seq);
			this->max_seq = seq - 1;
			this->probation = MIN_SEQUENTIAL;
		}

		// If not a valid packet ignore it.
		if (!UpdateSeq(seq))
		{
			MS_WARN_TAG(rtp, "invalid packet [seq:%" PRIu16 "]", packet->GetSequenceNumber());

			return false;
		}

		// If bufferSize was given, store the packet into the buffer.
		if (this->storage.size() > 0)
		{
			StorePacket(packet);
			// TODO: TMP
			// Dump();
		}

		// Calculate Jitter
		this->CalculateJitter(packet->GetTimestamp());

		return true;
	}

	// This method looks for the requested RTP packets and inserts them into the
	// given container (and set to null the next container position).
	void RtpStream::RequestRtpRetransmission(uint16_t seq, uint16_t bitmask, std::vector<RTC::RtpPacket*>& container)
	{
		MS_TRACE();

		// 17: 16 bit mask + the initial sequence number.
		static size_t maxRequestedPackets = 17;

		// If the buffer is empty, just return.
		if (this->buffer.size() == 0)
			return;

		// Convert the given sequence numbers to 32 bits.
		uint32_t first_seq32 = (uint32_t)seq + this->cycles;
		uint32_t last_seq32 = first_seq32 + maxRequestedPackets - 1;
		size_t container_idx = 0;

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
				return;
			}
		}

		// Look for each requested packet.
		uint32_t seq32 = first_seq32;
		bool requested = true;

		do
		{
			if (requested)
			{
				for (; buffer_it != this->buffer.end(); ++buffer_it)
				{
					auto current_seq32 = (*buffer_it).seq32;

					// Found.
					if (current_seq32 == seq32)
					{
						auto current_packet = (*buffer_it).packet;

						// Store the packet in the container and then increment its index.
						container[container_idx++] = current_packet;
						// Exit the loop.
						break;
					}
				}
			}

			requested = (bitmask & 1) ? true : false;
			bitmask >>= 1;
			++seq32;
		}
		while (bitmask != 0);

		// Set the next container element to null.
		container[container_idx] = nullptr;
	}

	void RtpStream::InitSeq(uint16_t seq)
	{
		MS_TRACE();

		// Initialize/reset RTP counters.
		this->base_seq = seq;
		this->max_seq = seq;
		this->bad_seq = RTP_SEQ_MOD + 1; // So seq == bad_seq is false.
		this->cycles = 0;
		this->received = 0;
		this->received_prior = 0;
		this->expected_prior = 0;

		// Clean buffer.
		CleanBuffer();
	}

	bool RtpStream::UpdateSeq(uint16_t seq)
	{
		MS_TRACE();

		uint16_t udelta = seq - this->max_seq;

		/*
		 * Source is not valid until MIN_SEQUENTIAL packets with
		 * sequential sequence numbers have been received.
		 */
		if (this->probation)
		{
			// Packet is in sequence.
			if (seq == this->max_seq + 1)
			{
				this->probation--;
				this->max_seq = seq;

				if (this->probation == 0)
				{
					InitSeq(seq);
					this->received++;

					return true;
				}
			}
			else
			{
				this->probation = MIN_SEQUENTIAL - 1;
				this->max_seq = seq;
			}

			return false;
		}
		else if (udelta < MAX_DROPOUT)
		{
			// In order, with permissible gap.
			if (seq < this->max_seq)
			{
				// Sequence number wrapped: count another 64K cycle.
				this->cycles += RTP_SEQ_MOD;
			}

			this->max_seq = seq;
		}
		else if (udelta <= RTP_SEQ_MOD - MAX_MISORDER)
		{
			// The sequence number made a very large jump.
			if (seq == this->bad_seq)
			{
				/*
				 * Two sequential packets -- assume that the other side
				 * restarted without telling us so just re-sync
				 * (i.e., pretend this was the first packet).
				 */
				InitSeq(seq);
			}
			else
			{
				this->bad_seq = (seq + 1) & (RTP_SEQ_MOD - 1);

				return false;
			}
		}
		else
		{
			// Duplicate or reordered packet.
			// NOTE: This would never happen because libsrtp rejects duplicated packets.
		}

		this->received++;

		return true;
	}

	void RtpStream::CleanBuffer()
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
	void RtpStream::StorePacket(RTC::RtpPacket* packet)
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

	void RtpStream::CalculateJitter(uint32_t rtpTimestamp)
	{
		if (!this->clockRate)
			return;

		int transit = DepLibUV::GetTime() - (rtpTimestamp*1000/this->clockRate);
		int d = transit - this->transit;
		this->transit = transit;
		if (d < 0) d = -d;
		this->jitter += (1./16.) * ((double)d - this->jitter);
	}

	RTC::RTCP::ReceiverReport* RtpStream::GetRtcpReceiverReport()
	{
		RTC::RTCP::ReceiverReport* report = new RTC::RTCP::ReceiverReport();

		// Calculate Packets Expected and Lost
		uint32_t expected = (this->cycles + this->max_seq) - this->base_seq + 1;
		int32_t total_lost = expected - this->received;

		report->SetTotalLost(total_lost);

		// Calculate Fraction Lost
		uint32_t expected_interval = expected - this->expected_prior;
		this->expected_prior = expected;
		uint32_t received_interval = this->received - this->received_prior;
		this->received_prior = this->received;
		uint32_t lost_interval = expected_interval - received_interval;
		uint8_t fraction_lost;
		if (expected_interval == 0 || lost_interval <= 0)
			fraction_lost = 0;
		else
			fraction_lost = (lost_interval << 8) / expected_interval;

		report->SetFractionLost(fraction_lost);

		// Fill the rest of the report
		report->SetLastSeq((uint32_t)this->max_seq + this->cycles);
		report->SetJitter(this->jitter);

		if (this->last_sr_received)
		{
			uint32_t dlsr = (DepLibUV::GetTime() - this->last_sr_received) / 1000 * 65536;
			report->SetDelaySinceLastSenderReport(dlsr);
			report->SetLastSenderReport(this->last_sr_timestamp);
		}
		else {
			report->SetDelaySinceLastSenderReport(0);
			report->SetLastSenderReport(0);
		}

		return report;
	}

	void RtpStream::ReceiveRtcpSenderReport(RTC::RTCP::SenderReport* report)
	{
		this->last_sr_received = DepLibUV::GetTime();

		this->last_sr_timestamp = htonl(report->GetNtpSec()) << 16;
		this->last_sr_timestamp += htonl(report->GetNtpFrac()) >> 16;
	}

	// TODO: TMP
	void RtpStream::Dump()
	{
		MS_TRACE();

		MS_DEBUG_TAG(rtp, "<RtpStream>");

		MS_DEBUG_TAG(rtp, "  [buffer.size:%zu, storage.size:%zu]", this->buffer.size(), this->storage.size());

		for (auto& buffer_item : this->buffer)
		{
			auto packet = buffer_item.packet;

			MS_DEBUG_TAG(rtp, "  packet [seq:%" PRIu16 ", seq32:%" PRIu32 "]", packet->GetSequenceNumber(), buffer_item.seq32);
		}

		MS_DEBUG_TAG(rtp, "</RtpStream>");
	}
}
