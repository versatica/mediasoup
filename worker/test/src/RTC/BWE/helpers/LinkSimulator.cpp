#define MS_CLASS "test::bweHelpers::LinkSimulator"
// #define MS_LOG_DEV_LEVEL 3

#include "test/include/RTC/BWE/helpers/LinkSimulator.hpp"
#include "Logger.hpp"

namespace bweHelpers
{
	/* Static. */

	// Lowest capacity the link can be given, which is what keeps the transmission
	// time below from dividing by a capacity of zero bits per millisecond.
	static constexpr int64_t MinCapacityBps{ 1000 };
	// Offset added to the send times so that they don't share the origin with the
	// arrival ones, which would hide a mistake mixing both references.
	static constexpr int64_t SendSideOffsetUs{ 1000 * 1000 };

	/* Instance methods. */

	RtpStream::RtpStream(
	  int fps, int64_t bitrateBps, uint32_t ssrc, uint32_t frequency, uint32_t rtpTimestampOffset)
	  : fps(fps),
	    ssrc(ssrc),
	    frequency(frequency),
	    rtpTimestampOffset(rtpTimestampOffset),
	    bitrateBps(bitrateBps)
	{
		MS_TRACE();

		MS_ASSERT(fps > 0, "fps must be greater than zero [fps:%d]", fps);
	}

	void RtpStream::SetBitrateBps(int64_t bitrateBps)
	{
		MS_TRACE();

		MS_ASSERT(bitrateBps >= 0, "bitrate must not be negative [bitrate:%" PRIi64 "]", bitrateBps);

		this->bitrateBps = bitrateBps;
	}

	int64_t RtpStream::GenerateFrame(
	  int64_t timeNowUs,
	  int64_t& nextSequenceNumber,
	  std::vector<RTC::BWE::Types::PacketResult>& packetResults)
	{
		MS_TRACE();

		std::vector<Packet> packets;

		const int64_t nextRtpTimeUs = GenerateFrame(timeNowUs, packets);

		for (const auto& packet : packets)
		{
			RTC::BWE::Types::PacketResult packetResult;

			packetResult.sentPacket.sendTimeUs     = packet.sendTimeUs;
			packetResult.sentPacket.size           = packet.size;
			packetResult.sentPacket.sequenceNumber = nextSequenceNumber++;

			packetResults.push_back(packetResult);
		}

		return nextRtpTimeUs;
	}

	int64_t RtpStream::GenerateFrame(int64_t timeNowUs, std::vector<Packet>& packets)
	{
		MS_TRACE();

		if (timeNowUs < this->nextRtpTimeUs)
		{
			return this->nextRtpTimeUs;
		}

		const size_t bitsPerFrame = (this->bitrateBps + (this->fps / 2)) / this->fps;
		const size_t numPackets   = std::max<size_t>((bitsPerFrame + (4 * Mtu)) / (8 * Mtu), 1);
		const size_t payloadSize  = (bitsPerFrame + (4 * numPackets)) / (8 * numPackets);
		const int64_t sendTimeUs  = timeNowUs + SendSideOffsetUs;
		const auto rtpTimestamp =
		  this->rtpTimestampOffset +
		  static_cast<uint32_t>((((this->frequency / 1000) * sendTimeUs) + 500) / 1000);

		for (size_t idx{ 0 }; idx < numPackets; ++idx)
		{
			packets.push_back(
			  Packet{ .ssrc          = this->ssrc,
				        .rtpTimestamp  = rtpTimestamp,
				        .sendTimeUs    = sendTimeUs,
				        .arrivalTimeUs = 0,
				        .size          = payloadSize });
		}

		this->nextRtpTimeUs = timeNowUs + ((1000000 + (this->fps / 2)) / this->fps);

		return this->nextRtpTimeUs;
	}

	bool RtpStream::Compare(const std::unique_ptr<RtpStream>& lhs, const std::unique_ptr<RtpStream>& rhs)
	{
		MS_TRACE();

		return lhs->nextRtpTimeUs < rhs->nextRtpTimeUs;
	}

	LinkSimulator::LinkSimulator(int64_t capacityBps, int64_t timeNowUs)
	  : capacityBps(capacityBps), prevArrivalTimeUs(timeNowUs)
	{
		MS_TRACE();

		MS_ASSERT(
		  capacityBps >= MinCapacityBps,
		  "capacity must be at least one bit per millisecond [capacity:%" PRIi64 "]",
		  capacityBps);
	}

	void LinkSimulator::AddStream(std::unique_ptr<RtpStream> stream)
	{
		MS_TRACE();

		this->streams.push_back(std::move(stream));
	}

	void LinkSimulator::SetCapacityBps(int64_t capacityBps)
	{
		MS_TRACE();

		MS_ASSERT(
		  capacityBps >= MinCapacityBps,
		  "capacity must be at least one bit per millisecond [capacity:%" PRIi64 "]",
		  capacityBps);

		this->capacityBps = capacityBps;
	}

	void LinkSimulator::SetRtpTimestampOffset(uint32_t ssrc, uint32_t rtpTimestampOffset)
	{
		MS_TRACE();

		for (auto& stream : this->streams)
		{
			if (stream->GetSsrc() == ssrc)
			{
				stream->SetRtpTimestampOffset(rtpTimestampOffset);

				break;
			}
		}
	}

	void LinkSimulator::SetBitrateBps(int64_t bitrateBps)
	{
		MS_TRACE();

		int64_t totalBitrateBefore{ 0 };

		for (const auto& stream : this->streams)
		{
			totalBitrateBefore += stream->GetBitrateBps();
		}

		// The ratios each stream keeps are taken from what they add up to, so there
		// has to be something to take them from.
		MS_ASSERT(
		  totalBitrateBefore > 0,
		  "the streams add up to no bitrate at all [streams:%zu]",
		  this->streams.size());

		int64_t bitrateBefore{ 0 };
		int64_t totalBitrateAfter{ 0 };

		for (const auto& stream : this->streams)
		{
			bitrateBefore += stream->GetBitrateBps();

			const int64_t bitrateAfter =
			  ((bitrateBefore * bitrateBps) + (totalBitrateBefore / 2)) / totalBitrateBefore;

			stream->SetBitrateBps(bitrateAfter - totalBitrateAfter);

			totalBitrateAfter += stream->GetBitrateBps();
		}

		MS_ASSERT(
		  totalBitrateAfter == bitrateBps,
		  "the bitrate was not fully distributed [expected:%" PRIi64 ", distributed:%" PRIi64 "]",
		  bitrateBps,
		  totalBitrateAfter);
	}

	int64_t LinkSimulator::GenerateFrame(
	  int64_t timeNowUs,
	  int64_t& nextSequenceNumber,
	  std::vector<RTC::BWE::Types::PacketResult>& packetResults)
	{
		MS_TRACE();

		MS_ASSERT(packetResults.empty(), "the given vector is not empty");
		MS_ASSERT(
		  this->capacityBps >= MinCapacityBps,
		  "capacity must be at least one bit per millisecond [capacity:%" PRIi64 "]",
		  this->capacityBps);

		auto it = std::ranges::min_element(this->streams, RtpStream::Compare);

		auto& dueStream = *it;

		dueStream->GenerateFrame(timeNowUs, nextSequenceNumber, packetResults);

		for (auto& packetResult : packetResults)
		{
			const int64_t requiredNetworkTimeUs = GetRequiredNetworkTimeUs(packetResult.sentPacket.size);

			// A packet arrives once the link is free again, so it queues behind the
			// previous one whenever the stream is sending above the capacity.
			this->prevArrivalTimeUs =
			  std::max(timeNowUs + requiredNetworkTimeUs, this->prevArrivalTimeUs + requiredNetworkTimeUs);

			packetResult.receiveTimeUs = this->prevArrivalTimeUs;
		}

		it = std::ranges::min_element(this->streams, RtpStream::Compare);

		auto& nextDueStream = *it;

		return std::max(nextDueStream->GetNextRtpTimeUs(), timeNowUs);
	}

	int64_t LinkSimulator::GenerateFrame(int64_t timeNowUs, std::vector<Packet>& packets)
	{
		MS_TRACE();

		MS_ASSERT(packets.empty(), "the given vector is not empty");
		MS_ASSERT(
		  this->capacityBps >= MinCapacityBps,
		  "capacity must be at least one bit per millisecond [capacity:%" PRIi64 "]",
		  this->capacityBps);

		auto it = std::ranges::min_element(this->streams, RtpStream::Compare);

		auto& dueStream = *it;

		dueStream->GenerateFrame(timeNowUs, packets);

		for (auto& packet : packets)
		{
			const int64_t requiredNetworkTimeUs = GetRequiredNetworkTimeUs(packet.size);

			// A packet arrives once the link is free again, so it queues behind the
			// previous one whenever the stream is sending above the capacity.
			this->prevArrivalTimeUs =
			  std::max(timeNowUs + requiredNetworkTimeUs, this->prevArrivalTimeUs + requiredNetworkTimeUs);

			packet.arrivalTimeUs = this->prevArrivalTimeUs;
		}

		it = std::ranges::min_element(this->streams, RtpStream::Compare);

		auto& nextDueStream = *it;

		return std::max(nextDueStream->GetNextRtpTimeUs(), timeNowUs);
	}

	int64_t LinkSimulator::GetRequiredNetworkTimeUs(size_t size) const
	{
		MS_TRACE();

		// Time the link needs to put a packet of that size on the wire.
		const int64_t capacityBpUs = this->capacityBps / 1000;

		return ((8 * 1000 * static_cast<int64_t>(size)) + (capacityBpUs / 2)) / capacityBpUs;
	}
} // namespace bweHelpers
