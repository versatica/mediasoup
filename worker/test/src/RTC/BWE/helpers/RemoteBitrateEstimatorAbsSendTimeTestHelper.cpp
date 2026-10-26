#define MS_CLASS "test::bweHelpers::RemoteBitrateEstimatorAbsSendTimeTestHelper"
// #define MS_LOG_DEV_LEVEL 3

#include "test/include/RTC/BWE/helpers/RemoteBitrateEstimatorAbsSendTimeTestHelper.hpp"
#include "Logger.hpp"
#include "RTC/RTP/HeaderExtensionIds.hpp"
#include "RTC/RtpDictionaries.hpp"
#include "Utils.hpp"
#include "test/include/RTC/RTP/rtpCommon.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstdlib> // std::abs()
#include <limits>

namespace bweHelpers
{
	/* Static. */

	// How far off the expected bitrate an estimation may be (bps).
	static constexpr int64_t AcceptedBitrateErrorBps{ 50000 };
	// Packets needed before there is a valid estimation.
	static constexpr int NumInitialPackets{ 2 };
	// Instant the scenarios start at, which is only far from zero so that nothing
	// passes by sharing an origin with something else.
	static constexpr int64_t InitialTimeUs{ 100000000 };
	// Capacity of the link the scenarios start with (bps).
	static constexpr int64_t InitialCapacityBps{ 1000000 };
	// Id the scenarios give to the 'abs-send-time' extension.
	static constexpr uint8_t AbsSendTimeId{ 1 };

	/* Instance methods. */

	RemoteBitrateEstimatorAbsSendTimeTestHelper::RemoteBitrateEstimatorAbsSendTimeTestHelper()
	  : nowUs(InitialTimeUs),
	    linkSimulator(InitialCapacityBps, InitialTimeUs),
	    remoteBitrateEstimator(this)
	{
		MS_TRACE();
	}

	uint32_t RemoteBitrateEstimatorAbsSendTimeTestHelper::AbsSendTime(int64_t t, int64_t denom)
	{
		MS_TRACE();

		return (((t << 18) + (denom >> 1)) / denom) & 0x00FFFFFF;
	}

	uint32_t RemoteBitrateEstimatorAbsSendTimeTestHelper::AddAbsSendTime(uint32_t t1, uint32_t t2)
	{
		MS_TRACE();

		return (t1 + t2) & 0x00FFFFFF;
	}

	void RemoteBitrateEstimatorAbsSendTimeTestHelper::AddDefaultStream()
	{
		MS_TRACE();

		this->linkSimulator.AddStream(
		  std::make_unique<RtpStream>(
		    /*fps*/ 30,
		    /*bitrateBps*/ 300000,
		    RemoteBitrateEstimatorAbsSendTimeTestHelper::DefaultSsrc,
		    /*frequency*/ 90000,
		    /*rtpTimestampOffset*/ 0xFFFFF000));
	}

	void RemoteBitrateEstimatorAbsSendTimeTestHelper::IncomingPacket(
	  uint32_t ssrc, size_t payloadSize, int64_t arrivalTimeMs, uint32_t rtpTimestamp, uint32_t absSendTime)
	{
		MS_TRACE();

		std::unique_ptr<RTC::RTP::Packet> packet(
		  RTC::RTP::Packet::Factory(rtpCommon::FactoryBuffer, sizeof(rtpCommon::FactoryBuffer)));

		REQUIRE(packet);

		packet->SetSsrc(ssrc);
		// NOTE: The estimator never reads the RTP timestamp, it works off the
		// 'abs-send-time' extension alone. It is set because the scenarios below
		// move it around on purpose, wrapping it included.
		packet->SetTimestamp(rtpTimestamp);

		const std::vector<RTC::RTP::Packet::Extension> extensions{
			{ RTC::RtpHeaderExtensionUri::Type::ABS_SEND_TIME, AbsSendTimeId, /*len*/ 3, rtpCommon::DataBuffer }
		};

		packet->SetExtensions(RTC::RTP::Packet::ExtensionsType::OneByte, extensions);

		RTC::RTP::HeaderExtensionIds headerExtensionIds;

		headerExtensionIds.absSendTime = AbsSendTimeId;

		packet->AssignExtensionIds(headerExtensionIds);
		packet->SetPayloadLength(payloadSize);

		REQUIRE(packet->UpdateAbsSendTime(absSendTime));

		const int64_t arrivalTimeUs = (arrivalTimeMs + this->arrivalTimeOffsetMs) * 1000;

		this->remoteBitrateEstimator.ReceiveRtpPacket(packet.get(), arrivalTimeUs, this->nowUs);
	}

	bool RemoteBitrateEstimatorAbsSendTimeTestHelper::GenerateAndProcessFrame(
	  uint32_t /*ssrc*/, int64_t bitrateBps)
	{
		MS_TRACE();

		REQUIRE(bitrateBps > 0);

		this->linkSimulator.SetBitrateBps(bitrateBps);

		std::vector<Packet> packets;

		const int64_t nextTimeUs = this->linkSimulator.GenerateFrame(this->nowUs, packets);
		bool overuse{ false };

		for (const auto& packet : packets)
		{
			this->updated = false;

			// The instant has to match the arrival time of the packet, since both are
			// used when it is fed in.
			this->nowUs = packet.arrivalTimeUs;

			IncomingPacket(
			  packet.ssrc,
			  packet.size,
			  (packet.arrivalTimeUs + 500) / 1000,
			  packet.rtpTimestamp,
			  RemoteBitrateEstimatorAbsSendTimeTestHelper::AbsSendTime(packet.sendTimeUs, 1000000));

			if (this->updated && this->latestBitrate < bitrateBps)
			{
				overuse = true;
			}
		}

		this->nowUs = nextTimeUs;

		return overuse;
	}

	int64_t RemoteBitrateEstimatorAbsSendTimeTestHelper::SteadyStateRun(
	  uint32_t ssrc,
	  int maxNumberOfFrames,
	  int64_t startBitrate,
	  int64_t minBitrate,
	  int64_t maxBitrate,
	  int64_t targetBitrate)
	{
		MS_TRACE();

		int64_t bitrateBps{ startBitrate };
		bool bitrateUpdateSeen{ false };

		for (int idx{ 0 }; idx < maxNumberOfFrames; ++idx)
		{
			const bool overuse = GenerateAndProcessFrame(ssrc, bitrateBps);

			if (overuse)
			{
				REQUIRE(this->latestBitrate < maxBitrate);
				REQUIRE(this->latestBitrate > minBitrate);

				bitrateBps        = this->latestBitrate;
				bitrateUpdateSeen = true;
			}
			else if (this->updated)
			{
				bitrateBps    = this->latestBitrate;
				this->updated = false;
			}

			if (bitrateUpdateSeen && bitrateBps > targetBitrate)
			{
				break;
			}
		}

		REQUIRE(bitrateUpdateSeen);

		return bitrateBps;
	}

	void RemoteBitrateEstimatorAbsSendTimeTestHelper::InitialBehaviorTestHelper(
	  int64_t expectedConvergeBitrate)
	{
		MS_TRACE();

		// 50 fps to avoid rounding errors.
		constexpr int Framerate{ 50 };
		constexpr int FrameIntervalMs{ 1000 / Framerate };

		const uint32_t frameIntervalAbsSendTime =
		  RemoteBitrateEstimatorAbsSendTimeTestHelper::AbsSendTime(1, Framerate);
		uint32_t rtpTimestamp{ 0 };
		uint32_t absSendTime{ 0 };

		REQUIRE(this->remoteBitrateEstimator.GetLatestEstimate() == 0);

		this->nowUs += 1000 * 1000;

		REQUIRE(this->remoteBitrateEstimator.GetLatestEstimate() == 0);
		REQUIRE(!this->updated);

		this->updated = false;
		this->nowUs += 1000 * 1000;

		// Inserting packets for 5 seconds to get a valid estimate.
		for (int idx{ 0 }; idx < (5 * Framerate) + 1 + NumInitialPackets; ++idx)
		{
			if (idx == NumInitialPackets)
			{
				REQUIRE(this->remoteBitrateEstimator.GetLatestEstimate() == 0);
				REQUIRE(!this->updated);

				this->updated = false;
			}

			IncomingPacket(
			  RemoteBitrateEstimatorAbsSendTimeTestHelper::DefaultSsrc,
			  Mtu,
			  Utils::Time::TimeUsToMs(this->nowUs),
			  rtpTimestamp,
			  absSendTime);

			this->nowUs += FrameIntervalMs * 1000;
			rtpTimestamp += 90 * FrameIntervalMs;

			absSendTime = RemoteBitrateEstimatorAbsSendTimeTestHelper::AddAbsSendTime(
			  absSendTime, frameIntervalAbsSendTime);
		}

		const int64_t bitrateBps = this->remoteBitrateEstimator.GetLatestEstimate();

		REQUIRE(std::abs(bitrateBps - expectedConvergeBitrate) <= AcceptedBitrateErrorBps);
		REQUIRE(this->updated);

		this->updated = false;

		REQUIRE(this->latestBitrate == bitrateBps);

		this->remoteBitrateEstimator.RemoveStream(
		  RemoteBitrateEstimatorAbsSendTimeTestHelper::DefaultSsrc);

		REQUIRE(this->remoteBitrateEstimator.GetLatestEstimate() == 0);
	}

	void RemoteBitrateEstimatorAbsSendTimeTestHelper::RateIncreaseReorderingTestHelper(
	  int64_t expectedBitrate)
	{
		MS_TRACE();

		// 50 fps to avoid rounding errors.
		constexpr int Framerate{ 50 };
		constexpr int FrameIntervalMs{ 1000 / Framerate };

		const uint32_t frameIntervalAbsSendTime =
		  RemoteBitrateEstimatorAbsSendTimeTestHelper::AbsSendTime(1, Framerate);
		uint32_t rtpTimestamp{ 0 };
		uint32_t absSendTime{ 0 };

		// Inserting packets for five seconds to get a valid estimate.
		for (int idx{ 0 }; idx < (5 * Framerate) + 1 + NumInitialPackets; ++idx)
		{
			if (idx == NumInitialPackets)
			{
				// No valid estimate yet.
				REQUIRE(!this->updated);
			}

			IncomingPacket(
			  RemoteBitrateEstimatorAbsSendTimeTestHelper::DefaultSsrc,
			  Mtu,
			  Utils::Time::TimeUsToMs(this->nowUs),
			  rtpTimestamp,
			  absSendTime);

			this->nowUs += FrameIntervalMs * 1000;
			rtpTimestamp += 90 * FrameIntervalMs;

			absSendTime = RemoteBitrateEstimatorAbsSendTimeTestHelper::AddAbsSendTime(
			  absSendTime, frameIntervalAbsSendTime);
		}

		REQUIRE(this->updated);
		REQUIRE(std::abs(this->latestBitrate - expectedBitrate) <= AcceptedBitrateErrorBps);

		// The same stream with every pair of packets swapped, which the estimation
		// has to ride out unchanged.
		for (int idx{ 0 }; idx < 10; ++idx)
		{
			this->nowUs += 2 * FrameIntervalMs * 1000;
			rtpTimestamp += 2 * 90 * FrameIntervalMs;

			absSendTime = RemoteBitrateEstimatorAbsSendTimeTestHelper::AddAbsSendTime(
			  absSendTime, 2 * frameIntervalAbsSendTime);

			IncomingPacket(
			  RemoteBitrateEstimatorAbsSendTimeTestHelper::DefaultSsrc,
			  1000,
			  Utils::Time::TimeUsToMs(this->nowUs),
			  rtpTimestamp,
			  absSendTime);

			IncomingPacket(
			  RemoteBitrateEstimatorAbsSendTimeTestHelper::DefaultSsrc,
			  1000,
			  Utils::Time::TimeUsToMs(this->nowUs),
			  rtpTimestamp - (90 * FrameIntervalMs),
			  RemoteBitrateEstimatorAbsSendTimeTestHelper::AddAbsSendTime(
			    absSendTime, -static_cast<int>(frameIntervalAbsSendTime)));
		}

		REQUIRE(this->updated);
		REQUIRE(std::abs(this->latestBitrate - expectedBitrate) <= AcceptedBitrateErrorBps);
	}

	void RemoteBitrateEstimatorAbsSendTimeTestHelper::RateIncreaseRtpTimestampsTestHelper(
	  int expectedIterations)
	{
		MS_TRACE();

		// This threshold corresponds approximately to increasing linearly with
		// bitrate(i) = 1.04 * bitrate(i-1) + 1000
		// until bitrate(i) > 500000, with bitrate(1) ~= 30000.
		int64_t bitrateBps{ 30000 };
		int iterations{ 0 };

		AddDefaultStream();

		// Feed the estimator with a stream of packets and verify that it reaches
		// 500 kbps at the expected time.
		while (bitrateBps < 500000)
		{
			const bool overuse = GenerateAndProcessFrame(
			  RemoteBitrateEstimatorAbsSendTimeTestHelper::DefaultSsrc, bitrateBps);

			if (overuse)
			{
				REQUIRE(this->latestBitrate > bitrateBps);

				bitrateBps    = this->latestBitrate;
				this->updated = false;
			}
			else if (this->updated)
			{
				bitrateBps    = this->latestBitrate;
				this->updated = false;
			}

			++iterations;

			REQUIRE(iterations <= expectedIterations);
		}

		REQUIRE(iterations == expectedIterations);
	}

	void RemoteBitrateEstimatorAbsSendTimeTestHelper::CapacityDropTestHelper(
	  int numberOfStreams,
	  bool wrapTimestamp,
	  int64_t expectedBitrateDropDeltaMs,
	  int64_t receiverClockOffsetChangeMs)
	{
		MS_TRACE();

		constexpr int Framerate{ 30 };
		constexpr int64_t StartBitrate{ 900000 };
		constexpr int64_t MinExpectedBitrate{ 800000 };
		constexpr int64_t MaxExpectedBitrate{ 1100000 };
		constexpr int64_t CapacityBps{ 1000000 };
		constexpr int64_t ReducedCapacityBps{ 500000 };

		int steadyStateTime{ 0 };

		if (numberOfStreams <= 1)
		{
			steadyStateTime = 10;

			AddDefaultStream();
		}
		else
		{
			steadyStateTime = 10 * numberOfStreams;

			int64_t bitrateSum{ 0 };
			const int64_t bitrateDenom = numberOfStreams * (numberOfStreams - 1);

			for (int idx{ 0 }; idx < numberOfStreams; ++idx)
			{
				// The first stream gets half of the bitrate and the rest share the
				// other half.
				int64_t bitrate = StartBitrate / 2;

				if (idx > 0)
				{
					bitrate = ((StartBitrate * idx) + (bitrateDenom / 2)) / bitrateDenom;
				}

				const auto mask = static_cast<uint32_t>(~0ULL << (32 - idx));

				this->linkSimulator.AddStream(
				  std::make_unique<RtpStream>(
				    Framerate,
				    bitrate,
				    RemoteBitrateEstimatorAbsSendTimeTestHelper::DefaultSsrc + idx,
				    /*frequency*/ 90000,
				    /*rtpTimestampOffset*/ 0xFFFFF000 ^ mask));

				bitrateSum += bitrate;
			}

			REQUIRE(bitrateSum == StartBitrate);
		}

		if (wrapTimestamp)
		{
			this->linkSimulator.SetRtpTimestampOffset(
			  RemoteBitrateEstimatorAbsSendTimeTestHelper::DefaultSsrc,
			  std::numeric_limits<uint32_t>::max() - (steadyStateTime * 90000));
		}

		// Run in steady state to make the estimator converge.
		this->linkSimulator.SetCapacityBps(CapacityBps);

		int64_t bitrateBps = SteadyStateRun(
		  RemoteBitrateEstimatorAbsSendTimeTestHelper::DefaultSsrc,
		  steadyStateTime * Framerate,
		  StartBitrate,
		  MinExpectedBitrate,
		  MaxExpectedBitrate,
		  CapacityBps);

		REQUIRE(bitrateBps >= (85 * CapacityBps) / 100);
		REQUIRE(bitrateBps <= (105 * CapacityBps) / 100);

		this->updated = false;

		// Move the receiver's clock to make sure the estimation copes with it.
		this->arrivalTimeOffsetMs += receiverClockOffsetChangeMs;

		// Reduce the capacity and see how long the estimation takes to follow.
		this->linkSimulator.SetCapacityBps(ReducedCapacityBps);

		const int64_t overuseStartTimeMs = Utils::Time::TimeUsToMs(this->nowUs);
		int64_t bitrateDropTimeMs{ -1 };

		for (int idx{ 0 }; idx < 100 * numberOfStreams; ++idx)
		{
			GenerateAndProcessFrame(RemoteBitrateEstimatorAbsSendTimeTestHelper::DefaultSsrc, bitrateBps);

			if (bitrateDropTimeMs == -1 && this->latestBitrate <= ReducedCapacityBps)
			{
				bitrateDropTimeMs = Utils::Time::TimeUsToMs(this->nowUs);
			}

			if (this->updated)
			{
				bitrateBps = this->latestBitrate;
			}
		}

		REQUIRE(std::abs((bitrateDropTimeMs - overuseStartTimeMs) - expectedBitrateDropDeltaMs) <= 33);

		// Remove the streams one by one.
		for (int idx{ 0 }; idx < numberOfStreams; ++idx)
		{
			REQUIRE(this->remoteBitrateEstimator.GetLatestEstimate() == bitrateBps);

			this->remoteBitrateEstimator.RemoveStream(
			  RemoteBitrateEstimatorAbsSendTimeTestHelper::DefaultSsrc + idx);
		}

		REQUIRE(this->remoteBitrateEstimator.GetLatestEstimate() == 0);
	}

	void RemoteBitrateEstimatorAbsSendTimeTestHelper::TestTimestampGroupingTestHelper()
	{
		MS_TRACE();

		// 50 fps to avoid rounding errors.
		constexpr int Framerate{ 50 };
		constexpr int FrameIntervalMs{ 1000 / Framerate };
		// How many frames go out with a single timestamp tick between them, which
		// the estimator has to take as one group.
		constexpr int TimestampGroupLength{ 15 };

		const uint32_t frameIntervalAbsSendTime =
		  RemoteBitrateEstimatorAbsSendTimeTestHelper::AbsSendTime(1, Framerate);
		uint32_t rtpTimestamp{ 0 };
		// Started near the top of the 24 bits so that it definitely wraps during
		// the run.
		uint32_t absSendTime = RemoteBitrateEstimatorAbsSendTimeTestHelper::AddAbsSendTime(
		  1 << 24, -static_cast<int>(50 * frameIntervalAbsSendTime));

		// Initial set of frames to increase the bitrate, over six seconds so that
		// there is time for the first estimation to come out.
		for (int idx{ 0 }; idx <= 6 * Framerate; ++idx)
		{
			IncomingPacket(
			  RemoteBitrateEstimatorAbsSendTimeTestHelper::DefaultSsrc,
			  1000,
			  Utils::Time::TimeUsToMs(this->nowUs),
			  rtpTimestamp,
			  absSendTime);

			this->nowUs += FrameIntervalMs * 1000;
			rtpTimestamp += 90 * FrameIntervalMs;

			absSendTime = RemoteBitrateEstimatorAbsSendTimeTestHelper::AddAbsSendTime(
			  absSendTime, frameIntervalAbsSendTime);
		}

		REQUIRE(this->updated);
		REQUIRE(this->latestBitrate >= 400000);

		const uint32_t timestampGroupLengthAbsSendTime =
		  RemoteBitrateEstimatorAbsSendTimeTestHelper::AbsSendTime(TimestampGroupLength, 90000);
		const uint32_t singleRtpTickAbsSendTime =
		  RemoteBitrateEstimatorAbsSendTimeTestHelper::AbsSendTime(1, 90000);

		// Batches of frames sent very close together, with the gap between batches
		// stretched so that the link looks overused.
		for (int idx{ 0 }; idx < 100; ++idx)
		{
			for (int groupIdx{ 0 }; groupIdx < TimestampGroupLength; ++groupIdx)
			{
				IncomingPacket(
				  RemoteBitrateEstimatorAbsSendTimeTestHelper::DefaultSsrc,
				  100,
				  Utils::Time::TimeUsToMs(this->nowUs),
				  rtpTimestamp,
				  absSendTime);

				this->nowUs += (FrameIntervalMs / TimestampGroupLength) * 1000;
				rtpTimestamp += 1;

				absSendTime = RemoteBitrateEstimatorAbsSendTimeTestHelper::AddAbsSendTime(
				  absSendTime, singleRtpTickAbsSendTime);
			}

			this->nowUs += 10 * 1000;
			rtpTimestamp += (90 * FrameIntervalMs) - TimestampGroupLength;

			absSendTime = RemoteBitrateEstimatorAbsSendTimeTestHelper::AddAbsSendTime(
			  absSendTime,
			  RemoteBitrateEstimatorAbsSendTimeTestHelper::AddAbsSendTime(
			    frameIntervalAbsSendTime, -static_cast<int>(timestampGroupLengthAbsSendTime)));
		}

		REQUIRE(this->updated);
		// It should have brought the estimation down.
		REQUIRE(this->latestBitrate < 400000);
	}

	void RemoteBitrateEstimatorAbsSendTimeTestHelper::TestWrappingHelper(int silenceTimeS)
	{
		MS_TRACE();

		constexpr int Framerate{ 100 };
		constexpr int FrameIntervalMs{ 1000 / Framerate };

		const uint32_t frameIntervalAbsSendTime =
		  RemoteBitrateEstimatorAbsSendTimeTestHelper::AbsSendTime(1, Framerate);
		uint32_t rtpTimestamp{ 0 };
		uint32_t absSendTime{ 0 };

		for (size_t idx{ 0 }; idx < 3000; ++idx)
		{
			IncomingPacket(
			  RemoteBitrateEstimatorAbsSendTimeTestHelper::DefaultSsrc,
			  1000,
			  Utils::Time::TimeUsToMs(this->nowUs),
			  rtpTimestamp,
			  absSendTime);

			rtpTimestamp += FrameIntervalMs;
			this->nowUs += FrameIntervalMs * 1000;

			absSendTime = RemoteBitrateEstimatorAbsSendTimeTestHelper::AddAbsSendTime(
			  absSendTime, frameIntervalAbsSendTime);
		}

		const int64_t bitrateBefore = this->remoteBitrateEstimator.GetLatestEstimate();

		// The sender goes quiet for long enough to make the 24 bits wrap.
		this->nowUs += static_cast<int64_t>(silenceTimeS) * 1000 * 1000;

		absSendTime = RemoteBitrateEstimatorAbsSendTimeTestHelper::AddAbsSendTime(
		  absSendTime, RemoteBitrateEstimatorAbsSendTimeTestHelper::AbsSendTime(silenceTimeS, 1));

		for (size_t idx{ 0 }; idx < 21; ++idx)
		{
			IncomingPacket(
			  RemoteBitrateEstimatorAbsSendTimeTestHelper::DefaultSsrc,
			  1000,
			  Utils::Time::TimeUsToMs(this->nowUs),
			  rtpTimestamp,
			  absSendTime);

			rtpTimestamp += FrameIntervalMs;
			this->nowUs += 2 * FrameIntervalMs * 1000;

			absSendTime = RemoteBitrateEstimatorAbsSendTimeTestHelper::AddAbsSendTime(
			  absSendTime, frameIntervalAbsSendTime);
		}

		const int64_t bitrateAfter = this->remoteBitrateEstimator.GetLatestEstimate();

		REQUIRE(bitrateAfter < bitrateBefore);
	}

	void RemoteBitrateEstimatorAbsSendTimeTestHelper::OnRemoteBitrateEstimatorAbsSendTimeBitrateChanged(
	  RTC::BWE::RemoteBitrateEstimatorAbsSendTime* /*remoteBitrateEstimator*/,
	  const std::vector<uint32_t>& /*ssrcs*/,
	  int64_t bitrate)
	{
		MS_TRACE();

		this->latestBitrate = bitrate;
		this->updated       = true;
	}
} // namespace bweHelpers
