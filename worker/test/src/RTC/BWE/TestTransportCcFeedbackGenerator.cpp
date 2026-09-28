#include "common.hpp"
#include "RTC/BWE/TransportCcFeedbackGenerator.hpp"
#include "RTC/Consts.hpp"
#include "RTC/RTP/HeaderExtensionIds.hpp"
#include "RTC/RTP/Packet.hpp"
#include "RTC/RtpDictionaries.hpp"
#include "test/include/RTC/RTP/rtpCommon.hpp"
#include "mocks/include/MockShared.hpp"
#include <catch2/catch_test_macros.hpp>
#include <deque>
#include <string_view>
#include <vector>

SCENARIO("BWE TransportCcFeedbackGenerator", "[bwe][transportccfeedbackgenerator]")
{
	struct TestInput
	{
		uint16_t wideSeqNumber;
		int64_t arrivalTimeUs;
	};

	struct TestPacketStatus
	{
		uint16_t wideSeqNumber;
		bool received;
		int64_t timestampUs;
	};

	using TestResults = std::deque<std::vector<TestPacketStatus>>;

	class TestTransportCcFeedbackGeneratorListener
	  : public RTC::BWE::TransportCcFeedbackGenerator::Listener
	{
	public:
		void OnTransportCcFeedbackGeneratorSendRtcpPacket(
		  RTC::BWE::TransportCcFeedbackGenerator* /*transportCcFeedbackGenerator*/,
		  RTC::RTCP::FeedbackRtpTransportPacket* packet) override
		{
			std::vector<TestPacketStatus> statuses;

			for (const auto& packetStatus : packet->GetPacketStatuses())
			{
				// The reconstructed times sit in the frame of the reference time,
				// which is shifted by a whole wrap period so that it's positive
				// regardless of the sign of the reference time on the wire.
				statuses.push_back(
				  TestPacketStatus{ .wideSeqNumber = packetStatus.sequenceNumber,
					                  .received      = packetStatus.received,
					                  .timestampUs = packetStatus.received
					                                   ? packetStatus.receivedAtUs -
					                                       RTC::RTCP::FeedbackRtpTransportPacket::TimeWrapPeriodUs
					                                   : 0 });
			}

			this->baseSequenceNumbers.push_back(packet->GetBaseSequenceNumber());
			this->feedbacks.push_back(statuses);
		}

	public:
		std::vector<uint16_t> baseSequenceNumbers;
		std::vector<std::vector<TestPacketStatus>> feedbacks;
	};

	constexpr uint32_t Ssrc{ 1111 };
	constexpr uint8_t TransportWideCc01Id{ 5 };
	// Instant the scenarios below start at, which is irrelevant other than for
	// being far from zero.
	constexpr int64_t BaseTimeUs{ 1000 * 1000 };
	// Size used by the scenarios that don't care about it.
	constexpr size_t DefaultPayloadSize{ 1000 };
	constexpr std::string_view TimerLabel{ "transport-cc-feedback-generator-send" };

	int64_t nowUs{ BaseTimeUs };

	mocks::MockShared shared(
	  [&nowUs]() -> int64_t
	  {
		  return nowUs;
	  });

	// Builds a packet of the given size carrying the given transport wide
	// sequence number.
	//
	// NOTE: Every scenario reuses `rtpCommon::FactoryBuffer`, so the returned
	// packet is only valid until the next call.
	auto buildPacket = [](size_t payloadSize, uint16_t wideSeqNumber) -> std::unique_ptr<RTC::RTP::Packet>
	{
		std::unique_ptr<RTC::RTP::Packet> packet(
		  RTC::RTP::Packet::Factory(rtpCommon::FactoryBuffer, sizeof(rtpCommon::FactoryBuffer)));

		REQUIRE(packet);

		packet->SetSsrc(Ssrc);

		const std::vector<RTC::RTP::Packet::Extension> extensions{
			{ RTC::RtpHeaderExtensionUri::Type::TRANSPORT_WIDE_CC_01,
			 TransportWideCc01Id, /*len*/ 2,
			 rtpCommon::DataBuffer }
		};

		packet->SetExtensions(RTC::RTP::Packet::ExtensionsType::OneByte, extensions);

		RTC::RTP::HeaderExtensionIds headerExtensionIds;

		headerExtensionIds.transportWideCc01 = TransportWideCc01Id;

		packet->AssignExtensionIds(headerExtensionIds);

		packet->SetPayloadLength(payloadSize);

		REQUIRE(packet->UpdateTransportWideCc01(wideSeqNumber));

		return packet;
	};

	// Feeds a packet that arrived at the given instant, which is also taken as
	// the current one since the meter of incoming data reads the clock itself.
	auto feedPacket = [&buildPacket, &nowUs](
	                    RTC::BWE::TransportCcFeedbackGenerator& transportCcFeedbackGenerator,
	                    uint16_t wideSeqNumber,
	                    int64_t arrivalTimeUs,
	                    size_t payloadSize) -> void
	{
		nowUs = arrivalTimeUs;

		const auto packet = buildPacket(payloadSize, wideSeqNumber);

		transportCcFeedbackGenerator.IncomingPacket(arrivalTimeUs, packet.get());
	};

	// Checks that the feedback packets the listener got are the expected ones,
	// status by status.
	auto checkFeedbacks = [](
	                        const TestTransportCcFeedbackGeneratorListener& listener,
	                        const TestResults& expectedResults) -> void
	{
		REQUIRE(listener.feedbacks.size() == expectedResults.size());

		for (size_t idx{ 0 }; idx < expectedResults.size(); ++idx)
		{
			const auto& statuses         = listener.feedbacks.at(idx);
			const auto& expectedStatuses = expectedResults.at(idx);

			REQUIRE(statuses.size() == expectedStatuses.size());

			for (size_t statusIdx{ 0 }; statusIdx < expectedStatuses.size(); ++statusIdx)
			{
				const auto& status         = statuses.at(statusIdx);
				const auto& expectedStatus = expectedStatuses.at(statusIdx);

				REQUIRE(status.wideSeqNumber == expectedStatus.wideSeqNumber);
				REQUIRE(status.received == expectedStatus.received);

				if (status.received)
				{
					REQUIRE(status.timestampUs == expectedStatus.timestampUs);
				}
			}
		}
	};

	// Feeds the given packets, emitting a feedback every time 100 ms have gone by
	// since the previous one, and checks what came out.
	auto validate = [&shared, &feedPacket, &checkFeedbacks](
	                  const std::vector<TestInput>& inputs, const TestResults& expectedResults) -> void
	{
		TestTransportCcFeedbackGeneratorListener listener;
		RTC::BWE::TransportCcFeedbackGenerator transportCcFeedbackGenerator(
		  std::addressof(listener), std::addressof(shared), RTC::Consts::MtuSize);

		static constexpr int64_t FeedbackSendIntervalUs{ 100 * 1000 };

		int64_t startTsUs = inputs.at(0).arrivalTimeUs;

		for (const auto& input : inputs)
		{
			if (input.arrivalTimeUs - startTsUs >= FeedbackSendIntervalUs)
			{
				transportCcFeedbackGenerator.FillAndSendFeedback();

				startTsUs = input.arrivalTimeUs;
			}

			feedPacket(
			  transportCcFeedbackGenerator, input.wideSeqNumber, input.arrivalTimeUs, DefaultPayloadSize);
		}

		transportCcFeedbackGenerator.FillAndSendFeedback();

		checkFeedbacks(listener, expectedResults);
	};

	SECTION("normal time and sequence")
	{
		const std::vector<TestInput> inputs{
			{ .wideSeqNumber = 1, .arrivalTimeUs = 1000000 },
			{ .wideSeqNumber = 2, .arrivalTimeUs = 1050000 },
			{ .wideSeqNumber = 3, .arrivalTimeUs = 1100000 },
			{ .wideSeqNumber = 4, .arrivalTimeUs = 1150000 },
			{ .wideSeqNumber = 5, .arrivalTimeUs = 1200000 },
		};

		const TestResults expectedResults{
			{
			 { .wideSeqNumber = 1, .received = true, .timestampUs = 1000000 },
			 { .wideSeqNumber = 2, .received = true, .timestampUs = 1050000 },
			 },
			{
			 { .wideSeqNumber = 3, .received = true, .timestampUs = 1100000 },
			 { .wideSeqNumber = 4, .received = true, .timestampUs = 1150000 },
			 },
			{
			 { .wideSeqNumber = 5, .received = true, .timestampUs = 1200000 },
			 },
		};

		validate(inputs, expectedResults);
	}

	SECTION("lost packets")
	{
		const std::vector<TestInput> inputs{
			{ .wideSeqNumber = 1, .arrivalTimeUs = 1000000 },
			{ .wideSeqNumber = 3, .arrivalTimeUs = 1050000 },
			{ .wideSeqNumber = 5, .arrivalTimeUs = 1100000 },
			{ .wideSeqNumber = 6, .arrivalTimeUs = 1150000 },
		};

		const TestResults expectedResults{
			{
			 { .wideSeqNumber = 1, .received = true, .timestampUs = 1000000 },
			 { .wideSeqNumber = 2, .received = false, .timestampUs = 0 },
			 { .wideSeqNumber = 3, .received = true, .timestampUs = 1050000 },
			 },
			{
			 { .wideSeqNumber = 4, .received = false, .timestampUs = 0 },
			 { .wideSeqNumber = 5, .received = true, .timestampUs = 1100000 },
			 { .wideSeqNumber = 6, .received = true, .timestampUs = 1150000 },
			 },
		};

		validate(inputs, expectedResults);
	}

	SECTION("duplicate packets")
	{
		const std::vector<TestInput> inputs{
			{ .wideSeqNumber = 1, .arrivalTimeUs = 1000000 },
			{ .wideSeqNumber = 1, .arrivalTimeUs = 1050000 },
			{ .wideSeqNumber = 2, .arrivalTimeUs = 1100000 },
			{ .wideSeqNumber = 3, .arrivalTimeUs = 1150000 },
			{ .wideSeqNumber = 3, .arrivalTimeUs = 1200000 },
			{ .wideSeqNumber = 4, .arrivalTimeUs = 1250000 },
		};

		const TestResults expectedResults{
			{
			 { .wideSeqNumber = 1, .received = true, .timestampUs = 1000000 },
			 },
			{
			 { .wideSeqNumber = 2, .received = true, .timestampUs = 1100000 },
			 { .wideSeqNumber = 3, .received = true, .timestampUs = 1150000 },
			 },
			{
			 { .wideSeqNumber = 4, .received = true, .timestampUs = 1250000 },
			 },
		};

		validate(inputs, expectedResults);
	}

	SECTION("packets arrive out of order")
	{
		const std::vector<TestInput> inputs{
			{ .wideSeqNumber = 1, .arrivalTimeUs = 1000000 },
			{ .wideSeqNumber = 2, .arrivalTimeUs = 1050000 },
			{ .wideSeqNumber = 4, .arrivalTimeUs = 1100000 },
			{ .wideSeqNumber = 5, .arrivalTimeUs = 1150000 },
			// Out of order.
			{ .wideSeqNumber = 3, .arrivalTimeUs = 1200000 },
			{ .wideSeqNumber = 6, .arrivalTimeUs = 1250000 },
		};

		const TestResults expectedResults{
			{
			 { .wideSeqNumber = 1, .received = true, .timestampUs = 1000000 },
			 { .wideSeqNumber = 2, .received = true, .timestampUs = 1050000 },
			 },
			{
			 { .wideSeqNumber = 3, .received = false, .timestampUs = 0 },
			 { .wideSeqNumber = 4, .received = true, .timestampUs = 1100000 },
			 { .wideSeqNumber = 5, .received = true, .timestampUs = 1150000 },
			 },
			{
			 { .wideSeqNumber = 3, .received = true, .timestampUs = 1200000 },
			 { .wideSeqNumber = 4, .received = true, .timestampUs = 1100000 },
			 { .wideSeqNumber = 5, .received = true, .timestampUs = 1150000 },
			 { .wideSeqNumber = 6, .received = true, .timestampUs = 1250000 },
			 },
		};

		validate(inputs, expectedResults);
	}

	SECTION("arrival times older than the window are forgotten")
	{
		TestTransportCcFeedbackGeneratorListener listener;
		RTC::BWE::TransportCcFeedbackGenerator transportCcFeedbackGenerator(
		  std::addressof(listener), std::addressof(shared), RTC::Consts::MtuSize);

		feedPacket(transportCcFeedbackGenerator, 12, 1000000, DefaultPayloadSize);

		transportCcFeedbackGenerator.FillAndSendFeedback();

		// Exactly the 500 ms of the window later, so packet 12 is forgotten here.
		feedPacket(transportCcFeedbackGenerator, 13, 1500000, DefaultPayloadSize);

		transportCcFeedbackGenerator.FillAndSendFeedback();

		// Below the sequence the next feedback starts at, so that they are reported
		// along with whatever is still known.
		feedPacket(transportCcFeedbackGenerator, 10, 1499000, DefaultPayloadSize);
		feedPacket(transportCcFeedbackGenerator, 11, 1499500, DefaultPayloadSize);

		transportCcFeedbackGenerator.FillAndSendFeedback();

		const TestResults expectedResults{
			{
			 { .wideSeqNumber = 12, .received = true, .timestampUs = 1000000 },
			 },
			{
			 { .wideSeqNumber = 13, .received = true, .timestampUs = 1500000 },
			 },
			{
			 { .wideSeqNumber = 10, .received = true, .timestampUs = 1499000 },
			 { .wideSeqNumber = 11, .received = true, .timestampUs = 1499500 },
			 // Forgotten above, so no longer known to have arrived.
			  { .wideSeqNumber = 12, .received = false, .timestampUs = 0 },
			 { .wideSeqNumber = 13, .received = true, .timestampUs = 1500000 },
			 },
		};

		checkFeedbacks(listener, expectedResults);

		REQUIRE(listener.baseSequenceNumbers.at(0) == 12);
		REQUIRE(listener.baseSequenceNumbers.at(1) == 13);
		REQUIRE(listener.baseSequenceNumbers.at(2) == 10);
	}

	SECTION("a sequence number that wrapped backwards is reported first")
	{
		TestTransportCcFeedbackGeneratorListener listener;
		RTC::BWE::TransportCcFeedbackGenerator transportCcFeedbackGenerator(
		  std::addressof(listener), std::addressof(shared), RTC::Consts::MtuSize);

		feedPacket(transportCcFeedbackGenerator, 10, 1000000, DefaultPayloadSize);
		// Ahead of the one above by more than half the sequence space, so it goes
		// before it rather than after.
		feedPacket(transportCcFeedbackGenerator, 62762, 1001000, DefaultPayloadSize);

		transportCcFeedbackGenerator.FillAndSendFeedback();

		REQUIRE(listener.baseSequenceNumbers.at(0) == 62762);

		// Everything in between is reported as missing, so only the two that did
		// arrive are looked at.
		std::vector<TestPacketStatus> receivedStatuses;

		for (const auto& statuses : listener.feedbacks)
		{
			for (const auto& status : statuses)
			{
				if (status.received)
				{
					receivedStatuses.push_back(status);
				}
			}
		}

		REQUIRE(receivedStatuses.size() == 2);
		REQUIRE(receivedStatuses.at(0).wideSeqNumber == 62762);
		REQUIRE(receivedStatuses.at(0).timestampUs == 1001000);
		REQUIRE(receivedStatuses.at(1).wideSeqNumber == 10);
		REQUIRE(receivedStatuses.at(1).timestampUs == 1000000);
	}

	SECTION("a delta too large to encode splits the report in two")
	{
		TestTransportCcFeedbackGeneratorListener listener;
		RTC::BWE::TransportCcFeedbackGenerator transportCcFeedbackGenerator(
		  std::addressof(listener), std::addressof(shared), RTC::Consts::MtuSize);

		feedPacket(transportCcFeedbackGenerator, 1, 1000000, DefaultPayloadSize);
		// Nine seconds later, which no delta of a feedback packet can express.
		feedPacket(transportCcFeedbackGenerator, 2, 10000000, DefaultPayloadSize);

		transportCcFeedbackGenerator.FillAndSendFeedback();

		const TestResults expectedResults{
			{
			 { .wideSeqNumber = 1, .received = true, .timestampUs = 1000000 },
			 },
			{
			 { .wideSeqNumber = 2, .received = true, .timestampUs = 10000000 },
			 },
		};

		checkFeedbacks(listener, expectedResults);

		REQUIRE(listener.baseSequenceNumbers.at(0) == 1);
		REQUIRE(listener.baseSequenceNumbers.at(1) == 2);
	}

	SECTION("the send interval follows the bitrate it reports on")
	{
		// 80000 * 0.05 = 68 bytes * 8 bits * 1000 ms / 136 ms.
		REQUIRE(RTC::BWE::TransportCcFeedbackGenerator::ComputeSendIntervalMs(80000) == 136);
		// Reporting on this much would fit in less than the shortest interval, so
		// the shortest one stands.
		REQUIRE(RTC::BWE::TransportCcFeedbackGenerator::ComputeSendIntervalMs(300000) == 50);
		// Too little coming in for the reporting to be spread any thinner than the
		// longest interval.
		REQUIRE(RTC::BWE::TransportCcFeedbackGenerator::ComputeSendIntervalMs(20000) == 250);
		// Nothing at all, which is what the guard against dividing by the bitrate
		// is there for.
		REQUIRE(RTC::BWE::TransportCcFeedbackGenerator::ComputeSendIntervalMs(0) == 250);
	}

	SECTION("with nothing coming in the send interval stays at its default")
	{
		TestTransportCcFeedbackGeneratorListener listener;
		RTC::BWE::TransportCcFeedbackGenerator transportCcFeedbackGenerator(
		  std::addressof(listener), std::addressof(shared), RTC::Consts::MtuSize);

		auto* timer = shared.GetTimer(TimerLabel);

		REQUIRE(timer);
		REQUIRE(timer->GetRepeatMs() == 100);

		nowUs = BaseTimeUs + (100 * 1000);

		REQUIRE(timer->EvaluateHasExpired());

		REQUIRE(transportCcFeedbackGenerator.GetSendIntervalMs() == 100);
		REQUIRE(timer->GetRepeatMs() == 100);
	}

	SECTION("plenty coming in brings the send interval down to its shortest")
	{
		TestTransportCcFeedbackGeneratorListener listener;
		RTC::BWE::TransportCcFeedbackGenerator transportCcFeedbackGenerator(
		  std::addressof(listener), std::addressof(shared), RTC::Consts::MtuSize);

		// A thousand bytes every 20 ms is around 400 kbps, well past what the
		// shortest interval covers.
		for (uint16_t wideSeqNumber{ 1 }; wideSeqNumber <= 50; ++wideSeqNumber)
		{
			feedPacket(
			  transportCcFeedbackGenerator, wideSeqNumber, BaseTimeUs + (wideSeqNumber * 20 * 1000), 1000);
		}

		auto* timer = shared.GetTimer(TimerLabel);

		REQUIRE(timer);
		REQUIRE(timer->EvaluateHasExpired());

		REQUIRE(transportCcFeedbackGenerator.GetSendIntervalMs() == 50);
		REQUIRE(timer->GetTimeoutMs() == 50);
		REQUIRE(timer->GetRepeatMs() == 50);
	}

	SECTION("little coming in pushes the send interval up to its longest")
	{
		TestTransportCcFeedbackGeneratorListener listener;
		RTC::BWE::TransportCcFeedbackGenerator transportCcFeedbackGenerator(
		  std::addressof(listener), std::addressof(shared), RTC::Consts::MtuSize);

		// Two hundred and fifty bytes every 100 ms is around 20 kbps, well below
		// what the longest interval covers.
		for (uint16_t wideSeqNumber{ 1 }; wideSeqNumber <= 50; ++wideSeqNumber)
		{
			feedPacket(
			  transportCcFeedbackGenerator, wideSeqNumber, BaseTimeUs + (wideSeqNumber * 100 * 1000), 250);
		}

		auto* timer = shared.GetTimer(TimerLabel);

		REQUIRE(timer);
		REQUIRE(timer->EvaluateHasExpired());

		REQUIRE(transportCcFeedbackGenerator.GetSendIntervalMs() == 250);
		REQUIRE(timer->GetTimeoutMs() == 250);
		REQUIRE(timer->GetRepeatMs() == 250);
	}
}
