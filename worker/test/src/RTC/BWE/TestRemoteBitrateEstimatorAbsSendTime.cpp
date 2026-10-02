#include "common.hpp"
#include "RTC/BWE/RemoteBitrateEstimatorAbsSendTime.hpp"
#include "RTC/RTP/HeaderExtensionIds.hpp"
#include "RTC/RTP/Packet.hpp"
#include "RTC/RtpDictionaries.hpp"
#include "Utils.hpp"
#include "test/include/RTC/BWE/helpers/RemoteBitrateEstimatorAbsSendTimeTestHelper.hpp"
#include "test/include/RTC/RTP/rtpCommon.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstdlib> // std::abs()
#include <vector>

SCENARIO("BWE RemoteBitrateEstimatorAbsSendTime", "[bwe][remotebitrateestimatorabssendtime]")
{
	class TestRemoteBitrateEstimatorAbsSendTimeListener
	  : public RTC::BWE::RemoteBitrateEstimatorAbsSendTime::Listener
	{
	public:
		void OnRemoteBitrateEstimatorAbsSendTimeBitrateChanged(
		  RTC::BWE::RemoteBitrateEstimatorAbsSendTime* /*remoteBitrateEstimator*/,
		  const std::vector<uint32_t>& ssrcs,
		  int64_t bitrate) override
		{
			this->updated       = true;
			this->latestBitrate = bitrate;
			this->latestSsrcs   = ssrcs;
		}

	public:
		bool updated{ false };
		int64_t latestBitrate{ 0 };
		std::vector<uint32_t> latestSsrcs;
	};

	constexpr uint32_t Ssrc{ 1111 };
	// Instant the scenarios below start at, which is irrelevant other than for
	// being far from zero.
	constexpr int64_t BaseTimeUs{ 1000 * 1000 };
	// Fewest packets a burst needs for it to be taken as one.
	constexpr int Probes{ 5 };

	// Builds a packet of the given size carrying the given instant in its
	// `abs-send-time` extension.
	//
	// NOTE: Every scenario reuses `rtpCommon::FactoryBuffer`, so the returned
	// packet is only valid until the next call.
	const auto buildPacket =
	  [](size_t payloadSize, int64_t sendTimeUs) -> std::unique_ptr<RTC::RTP::Packet>
	{
		std::unique_ptr<RTC::RTP::Packet> packet(
		  RTC::RTP::Packet::Factory(rtpCommon::FactoryBuffer, sizeof(rtpCommon::FactoryBuffer)));

		REQUIRE(packet);

		packet->SetSsrc(Ssrc);

		const auto absSendTimeId = static_cast<uint8_t>(RTC::RtpHeaderExtensionUri::Type::ABS_SEND_TIME);

		const std::vector<RTC::RTP::Packet::Extension> extensions{
			{ RTC::RtpHeaderExtensionUri::Type::ABS_SEND_TIME, absSendTimeId, /*len*/ 3, rtpCommon::DataBuffer }
		};

		packet->SetExtensions(RTC::RTP::Packet::ExtensionsType::OneByte, extensions);

		RTC::RTP::HeaderExtensionIds headerExtensionIds;

		headerExtensionIds.absSendTime = absSendTimeId;

		packet->AssignExtensionIds(headerExtensionIds);
		packet->SetPayloadLength(payloadSize);

		REQUIRE(packet->UpdateAbsSendTime(Utils::Time::TimeUsToAbsSendTime(sendTimeUs)));

		return packet;
	};

	// Feeds a packet that says it left at `sendTimeUs` and arrived at
	// `arrivalTimeUs`, which is also taken as the current instant.
	const auto feedPacket = [&buildPacket](
	                          RTC::BWE::RemoteBitrateEstimatorAbsSendTime& remoteBitrateEstimator,
	                          size_t payloadSize,
	                          int64_t sendTimeUs,
	                          int64_t arrivalTimeUs,
	                          int64_t nowUs) -> void
	{
		const auto packet = buildPacket(payloadSize, sendTimeUs);

		remoteBitrateEstimator.ReceiveRtpPacket(packet.get(), arrivalTimeUs, nowUs);
	};

	SECTION("a burst faster than the one before it raises the estimation")
	{
		TestRemoteBitrateEstimatorAbsSendTimeListener listener;
		RTC::BWE::RemoteBitrateEstimatorAbsSendTime remoteBitrateEstimator(std::addressof(listener));

		int64_t nowUs{ BaseTimeUs };

		// First burst sent at 8 * 1000 / 10 = 800 kbps.
		for (int i{ 0 }; i < Probes; ++i)
		{
			nowUs += 10 * 1000;

			feedPacket(remoteBitrateEstimator, 1000, nowUs, nowUs, nowUs);
		}

		// Second burst sent at 8 * 1000 / 5 = 1600 kbps.
		for (int i{ 0 }; i < Probes; ++i)
		{
			nowUs += 5 * 1000;

			feedPacket(remoteBitrateEstimator, 1000, nowUs, nowUs, nowUs);
		}

		REQUIRE(listener.updated);
		REQUIRE(listener.latestBitrate > 1500000);
	}

	SECTION("packets that were not paced do not spoil the burst")
	{
		TestRemoteBitrateEstimatorAbsSendTimeListener listener;
		RTC::BWE::RemoteBitrateEstimatorAbsSendTime remoteBitrateEstimator(std::addressof(listener));

		int64_t nowUs{ BaseTimeUs };

		// Burst sent at 8 * 1000 / 10 = 800 kbps, with a small packet in between
		// each pair of its packets.
		for (int i{ 0 }; i < Probes; ++i)
		{
			nowUs += 5 * 1000;

			const int64_t burstTimeUs = nowUs;

			feedPacket(remoteBitrateEstimator, 1000, burstTimeUs, burstTimeUs, nowUs);

			nowUs += 5 * 1000;

			feedPacket(remoteBitrateEstimator, 100, burstTimeUs, burstTimeUs, nowUs);
		}

		REQUIRE(listener.updated);
		REQUIRE(listener.latestBitrate > 800000);
	}

	SECTION("a burst that arrives dispersed is measured by how it arrived")
	{
		TestRemoteBitrateEstimatorAbsSendTimeListener listener;
		RTC::BWE::RemoteBitrateEstimatorAbsSendTime remoteBitrateEstimator(std::addressof(listener));

		int64_t nowUs{ BaseTimeUs };
		int64_t sendTimeUs{ BaseTimeUs };

		// First burst sent at 8 * 1000 / 10 = 800 kbps.
		for (int i{ 0 }; i < Probes; ++i)
		{
			nowUs += 10 * 1000;
			sendTimeUs += 10 * 1000;

			feedPacket(remoteBitrateEstimator, 1000, sendTimeUs, nowUs, nowUs);
		}

		// Second burst sent at 8 * 1000 / 5 = 1600 kbps but arriving at
		// 8 * 1000 / 8 = 1000 kbps, so the link did not take it.
		for (int i{ 0 }; i < Probes; ++i)
		{
			nowUs += 8 * 1000;
			sendTimeUs += 5 * 1000;

			feedPacket(remoteBitrateEstimator, 1000, sendTimeUs, nowUs, nowUs);
		}

		REQUIRE(listener.updated);
		REQUIRE(std::abs(listener.latestBitrate - 800000) <= 10000);
	}

	SECTION("a burst that arrives slightly faster than it was sent is taken")
	{
		TestRemoteBitrateEstimatorAbsSendTimeListener listener;
		RTC::BWE::RemoteBitrateEstimatorAbsSendTime remoteBitrateEstimator(std::addressof(listener));

		int64_t nowUs{ BaseTimeUs };
		int64_t sendTimeUs{ BaseTimeUs };

		// Sent at 8 * 1000 / 10 = 800 kbps, arriving at 8 * 1000 / 5 = 1600 kbps.
		for (int i{ 0 }; i < Probes; ++i)
		{
			nowUs += 5 * 1000;
			sendTimeUs += 10 * 1000;

			feedPacket(remoteBitrateEstimator, 1000, sendTimeUs, nowUs, nowUs);
		}

		REQUIRE(listener.updated);
		REQUIRE(listener.latestBitrate > 800000);
	}

	SECTION("a burst that arrives far faster than it was sent says nothing")
	{
		TestRemoteBitrateEstimatorAbsSendTimeListener listener;
		RTC::BWE::RemoteBitrateEstimatorAbsSendTime remoteBitrateEstimator(std::addressof(listener));

		int64_t nowUs{ BaseTimeUs };
		int64_t sendTimeUs{ BaseTimeUs };

		// Sent at 8 * 1000 / 10 = 800 kbps, arriving ten times faster, which no
		// link explains.
		for (int i{ 0 }; i < Probes; ++i)
		{
			nowUs += 1 * 1000;
			sendTimeUs += 10 * 1000;

			feedPacket(remoteBitrateEstimator, 1000, sendTimeUs, nowUs, nowUs);
		}

		REQUIRE(!listener.updated);
	}

	SECTION("a burst that arrives slower than it was sent is measured by how it arrived")
	{
		TestRemoteBitrateEstimatorAbsSendTimeListener listener;
		RTC::BWE::RemoteBitrateEstimatorAbsSendTime remoteBitrateEstimator(std::addressof(listener));

		int64_t nowUs{ BaseTimeUs };
		int64_t sendTimeUs{ BaseTimeUs };

		// Sent at 8 * 1000 / 5 = 1600 kbps, arriving at 8 * 1000 / 7 = 1142 kbps.
		for (int i{ 0 }; i < Probes; ++i)
		{
			nowUs += 7 * 1000;
			sendTimeUs += 5 * 1000;

			feedPacket(remoteBitrateEstimator, 1000, sendTimeUs, nowUs, nowUs);
		}

		REQUIRE(listener.updated);
		REQUIRE(std::abs(listener.latestBitrate - 1140000) <= 10000);
	}

	SECTION("a fast burst that arrives slower is measured by how it arrived")
	{
		TestRemoteBitrateEstimatorAbsSendTimeListener listener;
		RTC::BWE::RemoteBitrateEstimatorAbsSendTime remoteBitrateEstimator(std::addressof(listener));

		int64_t nowUs{ BaseTimeUs };
		int64_t sendTimeUs{ BaseTimeUs };

		// Sent at 8 * 1000 / 1 = 8000 kbps, arriving at 8 * 1000 / 2 = 4000 kbps.
		for (int i{ 0 }; i < Probes; ++i)
		{
			nowUs += 2 * 1000;
			sendTimeUs += 1 * 1000;

			feedPacket(remoteBitrateEstimator, 1000, sendTimeUs, nowUs, nowUs);
		}

		REQUIRE(listener.updated);
		REQUIRE(std::abs(listener.latestBitrate - 4000000) <= 10000);
	}

	SECTION("packets too small to have been paced are not taken as a burst")
	{
		TestRemoteBitrateEstimatorAbsSendTimeListener listener;
		RTC::BWE::RemoteBitrateEstimatorAbsSendTime remoteBitrateEstimator(std::addressof(listener));

		int64_t nowUs{ BaseTimeUs };

		// 200 bytes every 10 ms, which is not big enough to have been paced.
		for (int i{ 0 }; i < Probes; ++i)
		{
			nowUs += 10 * 1000;

			feedPacket(remoteBitrateEstimator, 200, nowUs, nowUs, nowUs);
		}

		REQUIRE(!listener.updated);

		// The same cadence with packets that are big enough is a burst.
		for (int i{ 0 }; i < Probes; ++i)
		{
			nowUs += 10 * 1000;

			feedPacket(remoteBitrateEstimator, 1000, nowUs, nowUs, nowUs);
		}

		REQUIRE(listener.updated);
		REQUIRE(std::abs(listener.latestBitrate - 800000) <= 10000);
	}

	// NOTE: What this pins is that a packet arriving after every stream has timed
	// out, and every packet after that one, never reach a null inter arrival or
	// overuse estimator.
	SECTION("a packet after every stream timed out is handled")
	{
		TestRemoteBitrateEstimatorAbsSendTimeListener listener;
		RTC::BWE::RemoteBitrateEstimatorAbsSendTime remoteBitrateEstimator(std::addressof(listener));

		int64_t nowUs{ BaseTimeUs };

		feedPacket(remoteBitrateEstimator, 1000, nowUs, nowUs, nowUs);

		// Longer than the two seconds a stream may be silent for.
		nowUs += 2001 * 1000;

		feedPacket(remoteBitrateEstimator, 1000, nowUs, nowUs, nowUs);

		nowUs += 1000 * 1000;

		feedPacket(remoteBitrateEstimator, 1000, nowUs, nowUs, nowUs);
	}

	SECTION("initial behavior")
	{
		bweHelpers::RemoteBitrateEstimatorAbsSendTimeTestHelper helper;

		helper.InitialBehaviorTestHelper(674840);
	}

	SECTION("a rate increase with reordered packets")
	{
		bweHelpers::RemoteBitrateEstimatorAbsSendTimeTestHelper helper;

		helper.RateIncreaseReorderingTestHelper(674840);
	}

	SECTION("a rate increase measured in rtp timestamps")
	{
		bweHelpers::RemoteBitrateEstimatorAbsSendTimeTestHelper helper;

		helper.RateIncreaseRtpTimestampsTestHelper(1237);
	}

	SECTION("a capacity drop with one stream")
	{
		bweHelpers::RemoteBitrateEstimatorAbsSendTimeTestHelper helper;

		helper.CapacityDropTestHelper(1, false, 633, 0);
	}

	SECTION("a capacity drop with the receiver clock moved forward")
	{
		bweHelpers::RemoteBitrateEstimatorAbsSendTimeTestHelper helper;

		helper.CapacityDropTestHelper(1, false, 267, 30000);
	}

	SECTION("a capacity drop with the receiver clock moved back")
	{
		bweHelpers::RemoteBitrateEstimatorAbsSendTimeTestHelper helper;

		helper.CapacityDropTestHelper(1, false, 267, -30000);
	}

	SECTION("a capacity drop with one stream whose timestamps wrap")
	{
		bweHelpers::RemoteBitrateEstimatorAbsSendTimeTestHelper helper;

		helper.CapacityDropTestHelper(1, true, 633, 0);
	}

	SECTION("a capacity drop with two streams whose timestamps wrap")
	{
		bweHelpers::RemoteBitrateEstimatorAbsSendTimeTestHelper helper;

		helper.CapacityDropTestHelper(2, true, 700, 0);
	}

	SECTION("a capacity drop with three streams whose timestamps wrap")
	{
		bweHelpers::RemoteBitrateEstimatorAbsSendTimeTestHelper helper;

		helper.CapacityDropTestHelper(3, true, 633, 0);
	}

	SECTION("a capacity drop with thirteen streams whose timestamps wrap")
	{
		bweHelpers::RemoteBitrateEstimatorAbsSendTimeTestHelper helper;

		helper.CapacityDropTestHelper(13, true, 667, 0);
	}

	SECTION("a capacity drop with nineteen streams whose timestamps wrap")
	{
		bweHelpers::RemoteBitrateEstimatorAbsSendTimeTestHelper helper;

		helper.CapacityDropTestHelper(19, true, 667, 0);
	}

	SECTION("a capacity drop with thirty streams whose timestamps wrap")
	{
		bweHelpers::RemoteBitrateEstimatorAbsSendTimeTestHelper helper;

		helper.CapacityDropTestHelper(30, true, 667, 0);
	}

	SECTION("packets sent very close together are taken as one group")
	{
		bweHelpers::RemoteBitrateEstimatorAbsSendTimeTestHelper helper;

		helper.TestTimestampGroupingTestHelper();
	}

	SECTION("a sender rejoining after a short silence, which wraps abs-send-time")
	{
		bweHelpers::RemoteBitrateEstimatorAbsSendTimeTestHelper helper;

		helper.TestWrappingHelper(35);
	}

	SECTION("a sender rejoining after a silence that leaves abs-send-time unchanged")
	{
		bweHelpers::RemoteBitrateEstimatorAbsSendTimeTestHelper helper;

		helper.TestWrappingHelper(10 * 64);
	}
}
