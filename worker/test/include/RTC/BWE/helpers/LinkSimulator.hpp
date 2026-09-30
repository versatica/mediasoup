#ifndef MS_TEST_RTC_BWE_LINK_SIMULATOR_HPP
#define MS_TEST_RTC_BWE_LINK_SIMULATOR_HPP

#include "common.hpp"
#include "RTC/BWE/BweTypes.hpp"
#include <vector>

namespace bweHelpers
{
	// Largest packet a frame is split into (bytes).
	constexpr size_t Mtu{ 1200 };

	/**
	 * A packet as it left a stream and as it arrived, for whoever needs to know
	 * which stream it belongs to rather than just its feedback.
	 */
	struct Packet
	{
		uint32_t ssrc;
		uint32_t rtpTimestamp;
		int64_t sendTimeUs;
		int64_t arrivalTimeUs;
		size_t size;
	};

	/**
	 * A stream that produces frames at a given frame rate and bitrate, split into
	 * packets of at most one MTU.
	 */
	class RtpStream
	{
	public:
		RtpStream(
		  int fps,
		  int64_t bitrateBps,
		  uint32_t ssrc               = 0,
		  uint32_t frequency          = 90000,
		  uint32_t rtpTimestampOffset = 0);

		RtpStream& operator=(const RtpStream&) = delete;

		RtpStream(const RtpStream&) = delete;

		uint32_t GetSsrc() const
		{
			return this->ssrc;
		}

		void SetRtpTimestampOffset(uint32_t rtpTimestampOffset)
		{
			this->rtpTimestampOffset = rtpTimestampOffset;
		}

		/**
		 * Generate a frame and split it into packets, appending them to
		 * `packetResults`. Nothing is generated if it's called before the next frame
		 * is due.
		 *
		 * @returns The send time at which the next frame can be generated.
		 */
		int64_t GenerateFrame(
		  int64_t timeNowUs,
		  int64_t& nextSequenceNumber,
		  std::vector<RTC::BWE::Types::PacketResult>& packetResults);

		int64_t GetBitrateBps() const
		{
			return this->bitrateBps;
		}

		void SetBitrateBps(int64_t bitrateBps);

		/**
		 * Send time at which the next frame can be generated.
		 */
		int64_t GetNextRtpTimeUs() const
		{
			return this->nextRtpTimeUs;
		}

		/**
		 * Generate a frame as the other overload does, for whoever needs the packets
		 * themselves rather than the feedback they would produce. The arrival time
		 * is left for the link to fill in.
		 */
		int64_t GenerateFrame(int64_t timeNowUs, std::vector<Packet>& packets);

		/**
		 * Orders streams by which one is due to produce a frame first.
		 */
		static bool Compare(const std::unique_ptr<RtpStream>& lhs, const std::unique_ptr<RtpStream>& rhs);

	private:
		const int fps;
		const uint32_t ssrc;
		const uint32_t frequency;
		uint32_t rtpTimestampOffset;
		int64_t bitrateBps;
		int64_t nextRtpTimeUs{ 0 };
	};

	/**
	 * Pushes the frames of several streams through a link of a given capacity and
	 * decides when each packet arrives.
	 *
	 * The link has no queue limit: a packet takes as long to arrive as its size
	 * divided by the capacity, and packets that don't fit queue up behind the
	 * previous one for as long as needed.
	 */
	class LinkSimulator
	{
	public:
		LinkSimulator(int64_t capacityBps, int64_t timeNowUs);

		LinkSimulator& operator=(const LinkSimulator&) = delete;

		LinkSimulator(const LinkSimulator&) = delete;

		/**
		 * Add a stream, whose ownership is taken.
		 */
		void AddStream(std::unique_ptr<RtpStream> stream);

		void SetCapacityBps(int64_t capacityBps);

		/**
		 * Set the RTP timestamp offset of the stream with the given SSRC, which
		 * does nothing if no stream has it.
		 */
		void SetRtpTimestampOffset(uint32_t ssrc, uint32_t rtpTimestampOffset);

		/**
		 * Divide `bitrateBps` among the streams, keeping the ratios they had.
		 */
		void SetBitrateBps(int64_t bitrateBps);

		/**
		 * Generate a frame of whichever stream is due first and push its packets
		 * through the link, appending them to `packetResults` with the time at which
		 * each one arrives.
		 *
		 * @returns The send time at which the next frame can be generated.
		 */
		int64_t GenerateFrame(
		  int64_t timeNowUs,
		  int64_t& nextSequenceNumber,
		  std::vector<RTC::BWE::Types::PacketResult>& packetResults);

		/**
		 * Generate a frame as the other overload does, for whoever needs the packets
		 * themselves rather than the feedback they would produce.
		 */
		int64_t GenerateFrame(int64_t timeNowUs, std::vector<Packet>& packets);

	private:
		/**
		 * Time the link needs to put a packet of the given size on the wire, which
		 * is also how long the next one waits behind it.
		 */
		int64_t GetRequiredNetworkTimeUs(size_t size) const;

	private:
		// Capacity of the simulated link (bps).
		int64_t capacityBps;
		// Time at which the latest packet arrived.
		int64_t prevArrivalTimeUs;
		std::vector<std::unique_ptr<RtpStream>> streams;
	};
} // namespace bweHelpers

#endif
