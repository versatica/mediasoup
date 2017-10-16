#define MS_CLASS "RTC::RtpStreamSend"
// #define MS_LOG_DEV

#include "RTC/RtpStreamSend.hpp"
#include "DepLibUV.hpp"
#include "Logger.hpp"
#include "Utils.hpp"
#include "RTC/SeqManager.hpp"

namespace RTC
{
	/* Static. */

	// Don't retransmit packets older than this (ms).
	// TODO: This must be tunned.
	static constexpr uint32_t MaxRetransmissionDelay{ 1000 };
	static constexpr uint32_t DefaultRtt{ 100 };
	static constexpr uint8_t MaxHealthLossPercentage{ 20 };

	/* Instance methods. */

	RtpStreamSend::RtpStreamSend(Listener* listener, RTC::RtpStream::Params& params, size_t bufferSize)
	  : RtpStream::RtpStream(params), listener(listener), storage(bufferSize)
	{
		MS_TRACE();
	}

	RtpStreamSend::~RtpStreamSend()
	{
		MS_TRACE();

		// Clear the RTP buffer.
		ClearRetransmissionBuffer();
	}

	Json::Value RtpStreamSend::GetStats()
	{
		static const std::string Type = "outbound-rtp";
		static const Json::StaticString JsonStringType{ "type" };
		static const Json::StaticString JsonStringRtt{ "roundTripTime" };

		Json::Value json = RtpStream::GetStats();

		json[JsonStringType] = Type;
		json[JsonStringRtt]  = Json::UInt{ this->rtt };

		return json;
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

		this->packetsLost  = report->GetTotalLost();
		this->fractionLost = report->GetFractionLost();
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

		uint16_t firstSeq = seq;
		uint16_t lastSeq  = firstSeq + MaxRequestedPackets - 1;

		// Number of requested packets cannot be greater than the container size - 1.
		MS_ASSERT(container.size() - 1 >= MaxRequestedPackets, "RtpPacket container is too small");

		auto bufferIt           = this->buffer.begin();
		auto bufferItReverse    = this->buffer.rbegin();
		uint16_t bufferFirstSeq = (*bufferIt).seq;
		uint16_t bufferLastSeq  = (*bufferItReverse).seq;

		// Requested packet range not found.
		if (
		  SeqManager<uint16_t>::IsSeqHigherThan(firstSeq, bufferLastSeq) ||
		  SeqManager<uint16_t>::IsSeqLowerThan(lastSeq, bufferFirstSeq))
		{
			MS_WARN_TAG(
			  rtx,
			  "requested packet range not in the buffer [seq:%" PRIu16 ", bufferFirstseq:%" PRIu16
			  ", bufferLastseq:%" PRIu16 "]",
			  seq,
			  bufferFirstSeq,
			  bufferLastSeq);

			return;
		}

		// Look for each requested packet.
		uint64_t now = DepLibUV::GetTime();
		uint16_t rtt = (this->rtt != 0u ? this->rtt : DefaultRtt);
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
					auto currentSeq = (*bufferIt).seq;

					// Found.
					if (currentSeq == seq)
					{
						auto currentPacket = (*bufferIt).packet;
						// Calculate how the elapsed time between the max timestampt seen and
						// the requested packet's timestampt (in ms).
						uint32_t diffTs = this->maxPacketTs - currentPacket->GetTimestamp();
						uint32_t diffMs = diffTs * 1000 / this->params.clockRate;

						// Just provide the packet if no older than MaxRetransmissionDelay ms.
						if (diffMs > MaxRetransmissionDelay)
						{
							if (!tooOldPacketFound)
							{
								// TODO: May we ask for a key frame in this case?

								MS_WARN_TAG(
								  rtx,
								  "ignoring retransmission for too old packet "
								  "[seq:%" PRIu16 ", max age:%" PRIu32 "ms, packet age:%" PRIu32 "ms]",
								  currentPacket->GetSequenceNumber(),
								  MaxRetransmissionDelay,
								  diffMs);

								tooOldPacketFound = true;
							}

							break;
						}

						// Don't resent the packet if it was resent in the last RTT ms.
						auto resentAtTime = (*bufferIt).resentAtTime;

						if ((resentAtTime != 0u) && now - resentAtTime <= static_cast<uint64_t>(rtt))
						{
							MS_DEBUG_TAG(
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
					if (SeqManager<uint16_t>::IsSeqHigherThan(currentSeq, seq))
						break;
				}
			}

			requested = (bitmask & 1) != 0;
			bitmask >>= 1;
			++seq;

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

		if (this->transmissionCounter.GetPacketCount() == 0u)
			return nullptr;

		auto report = new RTC::RTCP::SenderReport();
		report->SetPacketCount(this->transmissionCounter.GetPacketCount());
		report->SetOctetCount(this->transmissionCounter.GetBytes());

		// Calculate RTP timestamp diff between now and last received RTP packet.
		uint32_t diffMs = static_cast<uint32_t>(now - this->maxPacketMs);
		uint32_t diffTs = diffMs * this->params.clockRate / 1000;

		Utils::Time::Ntp ntp{};
		struct timeval unixTime;
		uint64_t unixTimeMs;

		gettimeofday(&unixTime, nullptr);

		// Convert unix time to millisecods.
		unixTimeMs = unixTime.tv_sec * 1000 + unixTime.tv_usec / 1000;

		if (now >= this->maxPacketMs)
		{
			unixTimeMs += diffMs;
			report->SetRtpTs(this->maxPacketTs + diffTs);
		}
		else
		{
			unixTimeMs -= diffMs;
			report->SetRtpTs(this->maxPacketTs - diffTs);
		}

		// Convert milliseconds to unix time.
		unixTime.tv_sec  = unixTimeMs / 1000;
		unixTime.tv_usec = (unixTimeMs % 1000) * 1000;

		// Convert unix time to NTP.
		Utils::Time::UnixTime2Ntp(&unixTime, ntp);

		report->SetNtpSec(ntp.seconds);
		report->SetNtpFrac(ntp.fractions);

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
		auto packetSeq = packet->GetSequenceNumber();
		BufferItem bufferItem;

		bufferItem.seq = packetSeq;

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
			auto currentSeq = (*bufferItReverse).seq;

			if (SeqManager<uint16_t>::IsSeqHigherThan(packetSeq, currentSeq))
			{
				// Get a forward iterator pointing to the same element.
				auto it = bufferItReverse.base();

				newBufferIt = this->buffer.insert(it, bufferItem);

				break;
			}

			// Packet is already stored.
			if (packetSeq == currentSeq)
				return;
		}

		// The packet was older than anything in the buffer, store it at the beginning.
		if (bufferItReverse == this->buffer.rend())
			newBufferIt = this->buffer.insert(this->buffer.begin(), bufferItem);

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

	void RtpStreamSend::CheckStatus()
	{
		MS_TRACE();

		auto lossPercentage = GetLossPercentage();

		if (lossPercentage >= MaxHealthLossPercentage)
		{
			if (this->notifyStatus || this->healthy)
			{
				MS_DEBUG_TAG(
				  rtp, "rtp stream packet loss [ssrc:%" PRIu32 ", %.2f%%]", GetSsrc(), lossPercentage);

				this->healthy = false;
				this->listener->OnRtpStreamUnhealthy(this);
			}
		}
		else if (this->notifyStatus || !this->healthy)
		{
			this->healthy = true;
			this->listener->OnRtpStreamHealthy(this);
		}

		this->notifyStatus = false;
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
