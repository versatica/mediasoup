#define MS_CLASS "RTC::RtpStreamSend"
// #define MS_LOG_DEV

#include "RTC/RtpStreamSend.hpp"
#include "Logger.hpp"
#include "DepLibUV.hpp"
#include "Utils.hpp"

#define RTP_SEQ_MOD (1<<16)
#define MAX_RETRANSMISSION_AGE 1000 // Don't retransmit packets older than this (ms).
#define DEFAULT_RTT 100 // Default RTT if not set (in ms).

namespace RTC
{
	/* Instance methods. */

	RtpStreamSend::RtpStreamSend(RTC::RtpStream::Params& params, size_t bufferSize) :
		RtpStream::RtpStream(params),
		storage(bufferSize)
	{
		MS_TRACE();
	}

	RtpStreamSend::~RtpStreamSend()
	{
		MS_TRACE();

		// Clear the RTP buffer.
		ClearBuffer();
	}

	Json::Value RtpStreamSend::toJson() const
	{
		MS_TRACE();

		static const Json::StaticString k_params("params");
		static const Json::StaticString k_received("received");
		static const Json::StaticString k_maxTimestamp("maxTimestamp");
		static const Json::StaticString k_receivedBytes("receivedBytes");
		static const Json::StaticString k_rtt("rtt");

		Json::Value json(Json::objectValue);

		json[k_params] = this->params.toJson();
		json[k_received] = (Json::UInt)this->received;
		json[k_maxTimestamp] = (Json::UInt)this->max_timestamp;
		json[k_receivedBytes] = (Json::UInt)this->receivedBytes;
		json[k_rtt] = (Json::UInt)this->rtt;

		return json;
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

		// Increase packet counters.
		this->receivedBytes += packet->GetPayloadLength();

		// Record current time and RTP timestamp.
		this->lastPacketTimeMs = DepLibUV::GetTime();
		this->lastPacketRtpTimestamp = packet->GetTimestamp();

		return true;
	}

	void RtpStreamSend::ReceiveRtcpReceiverReport(RTC::RTCP::ReceiverReport* report)
	{
		MS_TRACE();

		/* Calculate RTT. */

		// Get the compact NTP representation of the current timestamp.
		Utils::Time::Ntp nowNtp;
		Utils::Time::CurrentTimeNtp(nowNtp);
		uint32_t nowCompactNtp = (nowNtp.seconds & 0x0000FFFF) << 16;

		nowCompactNtp |= (nowNtp.fractions & 0xFFFF0000) >> 16;

		uint32_t lastSr = report->GetLastSenderReport();
		uint32_t dlsr = report->GetDelaySinceLastSenderReport();

		// RTT in 1/2^16 seconds.
		uint32_t rtt = nowCompactNtp - dlsr - lastSr;

		// RTT in milliseconds.
		this->rtt = ((rtt >> 16) * 1000);
		this->rtt += (static_cast<float>(rtt & 0x0000FFFF) / 65536) * 1000;
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

		// If NACK is not supported, exit.
		if (!this->params.useNack)
		{
			MS_WARN_TAG(rtx, "NACK not negotiated");

			return;
		}

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
				{
					MS_WARN_TAG(rtx, "requested packet range not in the buffer");

					return;
				}
			}
			// Otherwise just return.
			else
			{
				MS_WARN_TAG(rtx, "requested packet range not in the buffer");

				return;
			}
		}

		// Look for each requested packet.
		uint64_t now = DepLibUV::GetTime();
		uint32_t rtt = (this->rtt ? this->rtt : DEFAULT_RTT);
		uint32_t seq32 = first_seq32;
		bool requested = true;
		size_t container_idx = 0;

		// Some variables for debugging.
		uint16_t orig_bitmask = bitmask;
		uint16_t sent_bitmask = 0b0000000000000000;
		bool is_first_packet = true;
		bool first_packet_sent = false;
		uint8_t bitmask_counter = 0;
		bool too_old_packet_found = false;

		MS_DEBUG_DEV("loop [bitmask:" MS_UINT16_TO_BINARY_PATTERN "]", MS_UINT16_TO_BINARY(bitmask));

		while (requested || bitmask != 0)
		{
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
						uint32_t diff = (this->max_timestamp - current_packet->GetTimestamp()) * 1000 / this->params.clockRate;

						// Just provide the packet if no older than MAX_RETRANSMISSION_AGE ms.
						if (diff > MAX_RETRANSMISSION_AGE)
						{
							if (!too_old_packet_found)
							{
								MS_DEBUG_TAG(rtx,
									"ignoring retransmission for too old packet [seq:%" PRIu16 ", max_age:%" PRIu32 "ms, packet_age:%" PRIu32 "ms]",
									current_packet->GetSequenceNumber(), MAX_RETRANSMISSION_AGE, diff);

								too_old_packet_found = true;
							}

							break;
						}

						// Don't resent the packet if it was resent in the last RTT ms.
						uint32_t resent_at_time = (*buffer_it).resent_at_time;

						if (
							resent_at_time &&
							now - resent_at_time < static_cast<uint64_t>(rtt))
						{
							MS_DEBUG_TAG(rtx,
								"ignoring retransmission for a packet already resent in the last RTT ms [seq:%" PRIu16 ", rtt:%" PRIu32 "]",
								current_packet->GetSequenceNumber(), rtt);

							break;
						}

						// Store the packet in the container and then increment its index.
						container[container_idx++] = current_packet;

						// Save when this packet was resent.
						(*buffer_it).resent_at_time = now;

						sent = true;
						if (is_first_packet)
							first_packet_sent = true;

						break;
					}
					// It can not be after this packet.
					else if (current_seq32 > seq32)
					{
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

		// If not all the requested packets was sent, log it.
		if (!first_packet_sent || orig_bitmask != sent_bitmask)
		{
			MS_WARN_TAG(rtx, "could not resend all packets [seq:%" PRIu16 ", first:%s, bitmask:" MS_UINT16_TO_BINARY_PATTERN ", sent_bitmask:" MS_UINT16_TO_BINARY_PATTERN "]",
				seq, first_packet_sent ? "yes" : "no",
				MS_UINT16_TO_BINARY(orig_bitmask), MS_UINT16_TO_BINARY(sent_bitmask));
		}
		else
		{
			MS_DEBUG_TAG(rtx, "all packets resent [seq:%" PRIu16 ", bitmask:" MS_UINT16_TO_BINARY_PATTERN "]",
				seq, MS_UINT16_TO_BINARY(orig_bitmask));
		}

		// Set the next container element to null.
		container[container_idx] = nullptr;
	}

	RTC::RTCP::SenderReport* RtpStreamSend::GetRtcpSenderReport(uint64_t now)
	{
		MS_TRACE();

		if (!received)
			return nullptr;

		RTC::RTCP::SenderReport* report = new RTC::RTCP::SenderReport();

		report->SetPacketCount(this->received);
		report->SetOctetCount(this->receivedBytes);

		Utils::Time::Ntp ntp;
		Utils::Time::CurrentTimeNtp(ntp);

		report->SetNtpSec(ntp.seconds);
		report->SetNtpFrac(ntp.fractions);

		// Calculate RTP timestamp diff between now and last received RTP packet.
		uint32_t diffMs = now - this->lastPacketTimeMs;
		uint32_t diffRtpTimestamp = diffMs * this->params.clockRate / 1000;

		report->SetRtpTs(this->lastPacketRtpTimestamp + diffRtpTimestamp);

		return report;
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
			MS_WARN_TAG(rtp, "ignoring packet older than anything in the buffer [ssrc:%" PRIu32 ", seq:%" PRIu16 "]", packet->GetSsrc(), packet->GetSequenceNumber());

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

	void RtpStreamSend::onInitSeq()
	{
		MS_TRACE();

		// Clear the RTP buffer.
		ClearBuffer();
	}
}
