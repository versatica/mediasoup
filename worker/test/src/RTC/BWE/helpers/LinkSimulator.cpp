#define MS_CLASS "test::bweHelpers::LinkSimulator"
// #define MS_LOG_DEV_LEVEL 3

#include "test/include/RTC/BWE/helpers/LinkSimulator.hpp"
#include "Logger.hpp"

namespace bweHelpers
{
	/* Static. */

	// Largest packet a frame is split into (bytes).
	static constexpr size_t Mtu{ 1200 };

	/* Instance methods. */

	RtpStream::RtpStream(int fps, int64_t bitrateBps) : fps(fps), bitrateBps(bitrateBps)
	{
		MS_TRACE();

		MS_ASSERT(fps > 0, "fps must be greater than zero [fps:%d]", fps);
	}

	int64_t RtpStream::GenerateFrame(
	  int64_t timeNowUs,
	  int64_t& nextSequenceNumber,
	  std::vector<RTC::BWE::Types::PacketResult>& packetResults)
	{
		MS_TRACE();

		if (timeNowUs < this->nextRtpTimeUs)
		{
			return this->nextRtpTimeUs;
		}

		const size_t bitsPerFrame = (this->bitrateBps + (this->fps / 2)) / this->fps;
		const size_t numPackets   = std::max<size_t>((bitsPerFrame + (4 * Mtu)) / (8 * Mtu), 1);
		const size_t payloadSize  = (bitsPerFrame + (4 * numPackets)) / (8 * numPackets);

		for (size_t idx{ 0 }; idx < numPackets; ++idx)
		{
			RTC::BWE::Types::PacketResult packetResult;

			packetResult.sentPacket.sendTimeUs     = timeNowUs + SendSideOffsetUs;
			packetResult.sentPacket.size           = payloadSize;
			packetResult.sentPacket.sequenceNumber = nextSequenceNumber++;

			packetResults.push_back(packetResult);
		}

		this->nextRtpTimeUs = timeNowUs + ((1000000 + (this->fps / 2)) / this->fps);

		return this->nextRtpTimeUs;
	}

	void RtpStream::SetBitrateBps(int64_t bitrateBps)
	{
		MS_TRACE();

		MS_ASSERT(bitrateBps >= 0, "bitrate must not be negative [bitrate:%" PRIi64 "]", bitrateBps);

		this->bitrateBps = bitrateBps;
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
		  capacityBps > 0, "capacity must be greater than zero [capacity:%" PRIi64 "]", capacityBps);

		this->capacityBps = capacityBps;
	}

	void LinkSimulator::SetBitrateBps(int64_t bitrateBps)
	{
		MS_TRACE();

		int64_t totalBitrateBefore{ 0 };

		for (const auto& stream : this->streams)
		{
			totalBitrateBefore += stream->GetBitrateBps();
		}

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
		  this->capacityBps > 0,
		  "capacity must be greater than zero [capacity:%" PRIi64 "]",
		  this->capacityBps);

		auto it = std::ranges::min_element(this->streams, RtpStream::Compare);

		(*it)->GenerateFrame(timeNowUs, nextSequenceNumber, packetResults);

		for (auto& packetResult : packetResults)
		{
			// Time the link needs to put the packet on the wire.
			const int64_t capacityBpUs = this->capacityBps / 1000;
			const int64_t requiredNetworkTimeUs =
			  ((8 * 1000 * static_cast<int64_t>(packetResult.sentPacket.size)) + (capacityBpUs / 2)) /
			  capacityBpUs;

			// A packet arrives once the link is free again, so it queues behind the
			// previous one whenever the stream is sending above the capacity.
			this->prevArrivalTimeUs =
			  std::max(timeNowUs + requiredNetworkTimeUs, this->prevArrivalTimeUs + requiredNetworkTimeUs);

			packetResult.receiveTimeUs = this->prevArrivalTimeUs;
		}

		it = std::ranges::min_element(this->streams, RtpStream::Compare);

		return std::max((*it)->GetNextRtpTimeUs(), timeNowUs);
	}
} // namespace bweHelpers
