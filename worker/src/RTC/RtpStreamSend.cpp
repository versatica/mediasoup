#define MS_CLASS "RTC::RtpStreamSend"
// #define MS_LOG_DEV

#include "RTC/RtpStreamSend.hpp"
#include "DepLibUV.hpp"
#include "Logger.hpp"
#include "Utils.hpp"

namespace RTC
{
	/* Static. */

	static constexpr uint32_t RtpSeqMod{ 1 << 16 };
	// Don't retransmit packets older than this (ms).
	static constexpr uint32_t MaxRetransmissionAge{ 1000 };
	static constexpr uint32_t DefaultRtt{ 100 };

	/* Instance methods. */

	RtpStreamSend::RtpStreamSend(RTC::RtpStream::Params& params, size_t bufferSize)
	  : RtpStream::RtpStream(params), storage(bufferSize)
	{
		MS_TRACE();
	}

	RtpStreamSend::~RtpStreamSend()
	{
		MS_TRACE();

		// Clear the RTP buffer.
		ClearRetransmissionBuffer();
	}

	bool RtpStreamSend::ReceivePacket(RTC::RtpPacket* packet)
	{
		MS_TRACE();

		// Call the parent method.
		if (!RtpStream::ReceivePacket(packet))
			return false;

		// If bufferSize was given, store the packet into the buffer.
		if (!this->storage.empty())
			StorePacket(packet);

		// Record current time and RTP timestamp.
		this->lastPacketTimeMs       = DepLibUV::GetTime();
		this->lastPacketRtpTimestamp = packet->GetTimestamp();

		return true;
	}

	void RtpStreamSend::ReceiveRtcpReceiverReport(RTC::RTCP::ReceiverReport* report)
	{
		MS_TRACE();

		/* Calculate RTT. */

		// Get the compact NTP representation of the current timestamp.
		Utils::Time::Ntp nowNtp{};
		Utils::Time::CurrentTimeNtp(nowNtp);
		uint32_t nowCompactNtp = (nowNtp.seconds & 0x0000FFFF) << 16;

		nowCompactNtp |= (nowNtp.fractions & 0xFFFF0000) >> 16;

		uint32_t lastSr = report->GetLastSenderReport();
		uint32_t dlsr   = report->GetDelaySinceLastSenderReport();
		// RTT in 1/2^16 seconds.
		uint32_t rtt = nowCompactNtp - dlsr - lastSr;

		// RTT in milliseconds.
		this->rtt = ((rtt >> 16) * 1000);
		this->rtt += (static_cast<float>(rtt & 0x0000FFFF) / 65536) * 1000;

		this->totalLost    = report->GetTotalLost();
		this->fractionLost = report->GetFractionLost();
		this->jitter       = report->GetJitter();
	}

	// This method looks for the requested RTP packets and inserts them into the
	// given container (and set to null the next container position).
	void RtpStreamSend::RequestRtpRetransmission(
	  uint16_t seq, uint16_t bitmask, std::vector<RTC::RtpPacket*>& container)
	{
		MS_TRACE();

		// 17: 16 bit mask + the initial sequence number.
		static constexpr size_t MaxRequestedPackets{ 17 };

		// Ensure the container's first element is 0.
		container[0] = nullptr;

		// If NACK is not supported, exit.
		if (!this->params.useNack)
		{
			MS_WARN_TAG(rtx, "NACK not negotiated");

			return;
		}

		// If the buffer is empty just return.
		if (this->buffer.empty())
			return;

		// Convert the given sequence numbers to 32 bits.
		uint32_t firstSeq32 = uint32_t{ seq } + this->cycles;
		uint32_t lastSeq32  = firstSeq32 + MaxRequestedPackets - 1;

		// Number of requested packets cannot be greater than the container size - 1.
		MS_ASSERT(container.size() - 1 >= MaxRequestedPackets, "RtpPacket container is too small");

		auto bufferIt             = this->buffer.begin();
		auto bufferItReverse      = this->buffer.rbegin();
		uint32_t bufferFirstSeq32 = (*bufferIt).seq32;
		uint32_t bufferLastSeq32  = (*bufferItReverse).seq32;
		bool inRange              = true;

		// Requested packet range not found.
		if (firstSeq32 > bufferLastSeq32 || lastSeq32 < bufferFirstSeq32)
		{
			// Let's try with sequence numbers in the previous 16 cycle.
			if (this->cycles > 0)
			{
				firstSeq32 -= RtpSeqMod;
				lastSeq32 -= RtpSeqMod;

				// Try again.
				if (firstSeq32 > bufferLastSeq32 || lastSeq32 < bufferFirstSeq32)
				{
					firstSeq32 += RtpSeqMod;
					lastSeq32 += RtpSeqMod;
					inRange = false;
				}
			}
			// Otherwise just return.
			else
			{
				inRange = false;
			}
		}

		if (!inRange)
		{
			MS_WARN_TAG(
			  rtx,
			  "requested packet range not in the buffer [seq:%" PRIu16 ", seq32:%" PRIu32
			  ", bufferFirstSeq32:%" PRIu32 ", bufferLastSeq32:%" PRIu32 "]",
			  seq,
			  firstSeq32,
			  bufferFirstSeq32,
			  bufferLastSeq32);

			return;
		}

		// Look for each requested packet.
		uint64_t now   = DepLibUV::GetTime();
		uint32_t rtt   = (this->rtt != 0u ? this->rtt : DefaultRtt);
		uint32_t seq32 = firstSeq32;
		bool requested{ true };
		size_t containerIdx{ 0 };

		// Some variables for debugging.
		uint16_t origBitmask = bitmask;
		uint16_t sentBitmask{ 0b0000000000000000 };
		bool isFirstPacket{ true };
		bool firstPacketSent{ false };
		uint8_t bitmaskCounter{ 0 };
		bool tooOldPacketFound{ false };

		while (requested || bitmask != 0)
		{
			bool sent = false;

			if (requested)
			{
				for (; bufferIt != this->buffer.end(); ++bufferIt)
				{
					auto currentSeq32 = (*bufferIt).seq32;

					// Found.
					if (currentSeq32 == seq32)
					{
						auto currentPacket = (*bufferIt).packet;
						uint32_t diff =
						  (this->maxTimestamp - currentPacket->GetTimestamp()) * 1000 / this->params.clockRate;

						// Just provide the packet if no older than MaxRetransmissionAge ms.
						if (diff > MaxRetransmissionAge)
						{
							if (!tooOldPacketFound)
							{
								MS_WARN_TAG(
								  rtx,
								  "ignoring retransmission for too old packet "
								  "[seq:%" PRIu16 ", max age:%" PRIu32 "ms, packet age:%" PRIu32 "ms]",
								  currentPacket->GetSequenceNumber(),
								  MaxRetransmissionAge,
								  diff);

								tooOldPacketFound = true;
							}

							break;
						}

						// Don't resent the packet if it was resent in the last RTT ms.
						uint32_t resentAtTime = (*bufferIt).resentAtTime;

						if ((resentAtTime != 0u) && now - resentAtTime < static_cast<uint64_t>(rtt))
						{
							MS_WARN_TAG(
							  rtx,
							  "ignoring retransmission for a packet already resent in the last RTT ms "
							  "[seq:%" PRIu16 ", rtt:%" PRIu32 "]",
							  currentPacket->GetSequenceNumber(),
							  rtt);

							break;
						}

						// Store the packet in the container and then increment its index.
						container[containerIdx++] = currentPacket;

						// Save when this packet was resent.
						(*bufferIt).resentAtTime = now;

						sent = true;
						if (isFirstPacket)
							firstPacketSent = true;

						break;
					}
					// It can not be after this packet.
					if (currentSeq32 > seq32)
						break;
				}
			}

			requested = (bitmask & 1) != 0;
			bitmask >>= 1;
			++seq32;

			if (!isFirstPacket)
			{
				sentBitmask |= (sent ? 1 : 0) << bitmaskCounter;
				++bitmaskCounter;
			}
			else
			{
				isFirstPacket = false;
			}
		}

		// If not all the requested packets was sent, log it.
		if (!firstPacketSent || origBitmask != sentBitmask)
		{
			MS_DEBUG_TAG(
			  rtx,
			  "could not resend all packets [seq:%" PRIu16
			  ", first:%s, "
			  "bitmask:" MS_UINT16_TO_BINARY_PATTERN ", sent bitmask:" MS_UINT16_TO_BINARY_PATTERN "]",
			  seq,
			  firstPacketSent ? "yes" : "no",
			  MS_UINT16_TO_BINARY(origBitmask),
			  MS_UINT16_TO_BINARY(sentBitmask));
		}
		else
		{
			MS_DEBUG_TAG(
			  rtx,
			  "all packets resent [seq:%" PRIu16 ", bitmask:" MS_UINT16_TO_BINARY_PATTERN "]",
			  seq,
			  MS_UINT16_TO_BINARY(origBitmask));
		}

		// Set the next container element to null.
		container[containerIdx] = nullptr;
	}

	RTC::RTCP::SenderReport* RtpStreamSend::GetRtcpSenderReport(uint64_t now)
	{
		MS_TRACE();

		if (this->counter.GetPacketCount() == 0u)
			return nullptr;

		auto report = new RTC::RTCP::SenderReport();

		report->SetPacketCount(this->counter.GetPacketCount());
		report->SetOctetCount(this->counter.GetBytes());

		Utils::Time::Ntp ntp{};
		Utils::Time::CurrentTimeNtp(ntp);

		report->SetNtpSec(ntp.seconds);
		report->SetNtpFrac(ntp.fractions);

		// Calculate RTP timestamp diff between now and last received RTP packet.
		uint32_t diffMs           = now - this->lastPacketTimeMs;
		uint32_t diffRtpTimestamp = diffMs * this->params.clockRate / 1000;

		report->SetRtpTs(this->lastPacketRtpTimestamp + diffRtpTimestamp);

		return report;
	}

	void RtpStreamSend::ClearRetransmissionBuffer()
	{
		MS_TRACE();

		// Delete cloned packets.
		for (auto& bufferItem : this->buffer)
		{
			delete bufferItem.packet;
		}

		// Clear list.
		this->buffer.clear();
	}

	inline void RtpStreamSend::StorePacket(RTC::RtpPacket* packet)
	{
		MS_TRACE();

		if (packet->GetSize() > RTC::MtuSize)
		{
			MS_WARN_TAG(
			  rtp,
			  "packet too big [ssrc:%" PRIu32 ", seq:%" PRIu16 ", size:%zu]",
			  packet->GetSsrc(),
			  packet->GetSequenceNumber(),
			  packet->GetSize());

			return;
		}

		// Sum the packet seq number and the number of 16 bits cycles.
		uint32_t packetSeq32 = packet->GetExtendedSequenceNumber();
		BufferItem bufferItem;

		bufferItem.seq32 = packetSeq32;

		// If empty do it easy.
		if (this->buffer.empty())
		{
			auto store = this->storage[0].store;

			bufferItem.packet = packet->Clone(store);
			this->buffer.push_back(bufferItem);

			return;
		}

		// Otherwise, do the stuff.

		Buffer::iterator newBufferIt;
		uint8_t* store{ nullptr };

		// Iterate the buffer in reverse order and find the proper place to store the
		// packet.
		auto bufferItReverse = this->buffer.rbegin();
		for (; bufferItReverse != this->buffer.rend(); ++bufferItReverse)
		{
			auto currentSeq32 = (*bufferItReverse).seq32;

			if (packetSeq32 > currentSeq32)
			{
				// Get a forward iterator pointing to the same element.
				auto it = bufferItReverse.base();

				newBufferIt = this->buffer.insert(it, bufferItem);

				// Exit the loop.
				break;
			}
		}
		// If the packet was older than anything in the buffer, just ignore it.
		// NOTE: This should never happen.
		if (bufferItReverse == this->buffer.rend())
		{
			MS_WARN_TAG(
			  rtp,
			  "ignoring packet older than anything in the buffer [ssrc:%" PRIu32 ", seq:%" PRIu16 "]",
			  packet->GetSsrc(),
			  packet->GetSequenceNumber());

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
			auto& firstBufferItem = *(this->buffer.begin());
			auto firstPacket      = firstBufferItem.packet;

			// Store points to the store used by the first packet.
			store = const_cast<uint8_t*>(firstPacket->GetData());
			// Free the first packet.
			delete firstPacket;
			// Remove the first element in the list.
			this->buffer.pop_front();
		}

		// Update the new buffer item so it points to the cloned packed.
		(*newBufferIt).packet = packet->Clone(store);
	}

	void RtpStreamSend::OnInitSeq()
	{
		MS_TRACE();

		// Clear the RTP buffer.
		ClearRetransmissionBuffer();
	}

	void RtpStreamSend::CheckHealth()
	{
		MS_TRACE();
	}

	void RtpStreamSend::SetRtx(uint8_t payloadType, uint32_t ssrc)
	{
		MS_TRACE();

		this->hasRtx         = true;
		this->rtxPayloadType = payloadType;
		this->rtxSsrc        = ssrc;
		this->rtxSeq         = Utils::Crypto::GetRandomUInt(0u, 0xFFFF);
	}

	void RtpStreamSend::RtxEncode(RTC::RtpPacket* packet)
	{
		MS_TRACE();

		MS_ASSERT(this->hasRtx, "RTX not enabled on this stream");

		packet->RtxEncode(this->rtxPayloadType, this->rtxSsrc, ++this->rtxSeq);
	}

} // namespace RTC
