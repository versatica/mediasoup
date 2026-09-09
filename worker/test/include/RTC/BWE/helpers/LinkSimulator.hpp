#ifndef MS_TEST_RTC_BWE_LINK_SIMULATOR_HPP
#define MS_TEST_RTC_BWE_LINK_SIMULATOR_HPP

#include "common.hpp"
#include "RTC/BWE/BweTypes.hpp"

namespace bweHelpers
{
	/**
	 * A stream that produces frames at a given frame rate and bitrate, split into
	 * packets of at most one MTU.
	 */
	class RtpStream
	{
	public:
		/**
		 * Offset added to the send times so that they don't share the origin with
		 * the arrival ones, which would hide a mistake mixing both references.
		 */
		static constexpr int64_t SendSideOffsetUs{ 1000 * 1000 };

	public:
		RtpStream(int fps, int64_t bitrateBps);

		RtpStream& operator=(const RtpStream&) = delete;

		RtpStream(const RtpStream&) = delete;

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

		/**
		 * Send time at which the next frame can be generated.
		 */
		int64_t GetNextRtpTimeUs() const
		{
			return this->nextRtpTimeUs;
		}

		int64_t GetBitrateBps() const
		{
			return this->bitrateBps;
		}

		void SetBitrateBps(int64_t bitrateBps);

		/**
		 * Orders streams by which one is due to produce a frame first.
		 */
		static bool Compare(const std::unique_ptr<RtpStream>& lhs, const std::unique_ptr<RtpStream>& rhs);

	private:
		const int fps;
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

	private:
		// Capacity of the simulated link (bps).
		int64_t capacityBps;
		// Time at which the latest packet arrived.
		int64_t prevArrivalTimeUs;
		std::vector<std::unique_ptr<RtpStream>> streams;
	};
} // namespace bweHelpers

#endif
