#include "common.hpp"
#include "RTC/BWE/FeedbackAdapter.hpp"
#include "RTC/BWE/SendPacketHistory.hpp"
#include "RTC/RTCP/FeedbackRtpTransport.hpp"
#include <catch2/catch_test_macros.hpp>

SCENARIO("BWE FeedbackAdapter", "[bwe][feedbackadapter]")
{
	// The clock starts well away from zero so that a mistake taking a time for a
	// duration doesn't go unnoticed.
	constexpr int64_t InitialTimeUs{ 100000000 };
	// The remote clock has nothing to do with ours, and is a whole number of base
	// time ticks so that the arrival times survive the feedback untouched.
	constexpr int64_t RemoteTimeUs{ 1000000000000 };
	constexpr size_t PacketSize{ 1000 };
	constexpr uint32_t Ssrc{ 1111 };
	constexpr uint32_t SenderSsrc{ 2222 };
	constexpr uint32_t MediaSsrc{ 3333 };
	constexpr size_t RtcpMtu{ 1200 };

	// Builds the feedback a receiver would send, reporting the given arrival
	// times. The sequence numbers left out of them are reported as lost.
	auto createFeedback = [SenderSsrc, MediaSsrc](
	                        uint16_t baseSequenceNumber,
	                        int64_t baseTimeUs,
	                        const std::vector<std::pair<uint16_t, int64_t>>& receivedPackets)
	  -> std::unique_ptr<RTC::RTCP::FeedbackRtpTransportPacket>
	{
		auto feedback = std::make_unique<RTC::RTCP::FeedbackRtpTransportPacket>(SenderSsrc, MediaSsrc);

		feedback->SetBase(baseSequenceNumber, baseTimeUs);

		for (const auto& [sequenceNumber, receivedAtUs] : receivedPackets)
		{
			feedback->AddPacket(sequenceNumber, receivedAtUs, RtcpMtu);
		}

		feedback->Finish();

		return feedback;
	};

	SECTION("a feedback is resolved into the packets that were sent")
	{
		RTC::BWE::SendPacketHistory sendPacketHistory;
		RTC::BWE::FeedbackAdapter feedbackAdapter(&sendPacketHistory);

		sendPacketHistory.AddPacket(Ssrc, 100, PacketSize, false, InitialTimeUs);
		sendPacketHistory.AddPacket(Ssrc, 101, PacketSize, false, InitialTimeUs + 10000);
		sendPacketHistory.AddPacket(Ssrc, 102, PacketSize, false, InitialTimeUs + 20000);

		const auto feedback = createFeedback(
		  0,
		  RemoteTimeUs,
		  {
		    { 0, RemoteTimeUs         },
        { 1, RemoteTimeUs + 10000 },
        { 2, RemoteTimeUs + 20000 }
    });

		const auto processedFeedback =
		  feedbackAdapter.ProcessTransportFeedback(feedback.get(), InitialTimeUs + 50000);

		REQUIRE(processedFeedback.has_value());

		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		const auto& result = processedFeedback.value();

		REQUIRE(result.feedbackTimeUs == InitialTimeUs + 50000);
		REQUIRE(result.packetFeedbacks.size() == 3);

		int64_t expectedSequenceNumber{ 0 };

		for (const auto& packetResult : result.packetFeedbacks)
		{
			REQUIRE(packetResult.sentPacket.sequenceNumber == expectedSequenceNumber);
			REQUIRE(packetResult.sentPacket.sendTimeUs == InitialTimeUs + (expectedSequenceNumber * 10000));
			REQUIRE(packetResult.sentPacket.size == PacketSize);
			REQUIRE(packetResult.IsReceived());

			++expectedSequenceNumber;
		}

		// The first feedback is anchored on the time it was received.
		REQUIRE(result.packetFeedbacks[0].receiveTimeUs == InitialTimeUs + 50000);
		REQUIRE(result.packetFeedbacks[1].receiveTimeUs == InitialTimeUs + 60000);
		REQUIRE(result.packetFeedbacks[2].receiveTimeUs == InitialTimeUs + 70000);
	}

	SECTION("a packet reported as lost comes back with no arrival time")
	{
		RTC::BWE::SendPacketHistory sendPacketHistory;
		RTC::BWE::FeedbackAdapter feedbackAdapter(&sendPacketHistory);

		sendPacketHistory.AddPacket(Ssrc, 100, PacketSize, false, InitialTimeUs);
		sendPacketHistory.AddPacket(Ssrc, 101, PacketSize, false, InitialTimeUs + 10000);
		sendPacketHistory.AddPacket(Ssrc, 102, PacketSize, false, InitialTimeUs + 20000);

		const auto feedback = createFeedback(
		  0,
		  RemoteTimeUs,
		  {
		    { 0, RemoteTimeUs         },
        { 2, RemoteTimeUs + 20000 }
    });

		const auto processedFeedback =
		  feedbackAdapter.ProcessTransportFeedback(feedback.get(), InitialTimeUs + 50000);

		REQUIRE(processedFeedback.has_value());

		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		const auto& result = processedFeedback.value();

		REQUIRE(result.packetFeedbacks.size() == 3);
		REQUIRE(result.packetFeedbacks[0].IsReceived());
		REQUIRE(!result.packetFeedbacks[1].IsReceived());
		REQUIRE(result.packetFeedbacks[2].IsReceived());
	}

	SECTION("whether a packet is audio survives the feedback")
	{
		RTC::BWE::SendPacketHistory sendPacketHistory;
		RTC::BWE::FeedbackAdapter feedbackAdapter(&sendPacketHistory);

		sendPacketHistory.AddPacket(Ssrc, 100, PacketSize, true, InitialTimeUs);
		sendPacketHistory.AddPacket(Ssrc, 101, PacketSize, false, InitialTimeUs + 10000);

		const auto feedback = createFeedback(
		  0,
		  RemoteTimeUs,
		  {
		    { 0, RemoteTimeUs         },
        { 1, RemoteTimeUs + 10000 }
    });

		const auto processedFeedback =
		  feedbackAdapter.ProcessTransportFeedback(feedback.get(), InitialTimeUs + 50000);

		REQUIRE(processedFeedback.has_value());

		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		const auto& result = processedFeedback.value();

		REQUIRE(result.packetFeedbacks.size() == 2);
		REQUIRE(result.packetFeedbacks[0].sentPacket.audio == true);
		REQUIRE(result.packetFeedbacks[1].sentPacket.audio == false);
	}

	SECTION("the feedback tells the data still in flight once it's been resolved")
	{
		RTC::BWE::SendPacketHistory sendPacketHistory;
		RTC::BWE::FeedbackAdapter feedbackAdapter(&sendPacketHistory);

		sendPacketHistory.AddPacket(Ssrc, 100, 200, false, InitialTimeUs);
		sendPacketHistory.AddPacket(Ssrc, 101, 300, false, InitialTimeUs + 10000);

		const auto firstFeedback = createFeedback(
		  0,
		  RemoteTimeUs,
		  {
		    { 0, RemoteTimeUs }
    });
		const auto secondFeedback = createFeedback(
		  1,
		  RemoteTimeUs + 10000,
		  {
		    { 1, RemoteTimeUs + 10000 }
    });

		const auto firstResult =
		  feedbackAdapter.ProcessTransportFeedback(firstFeedback.get(), InitialTimeUs + 50000);

		REQUIRE(firstResult.has_value());
		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		REQUIRE(firstResult->dataInFlight == 300);

		const auto secondResult =
		  feedbackAdapter.ProcessTransportFeedback(secondFeedback.get(), InitialTimeUs + 60000);

		REQUIRE(secondResult.has_value());
		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		REQUIRE(secondResult->dataInFlight == 0);
	}

	SECTION("the packets the history no longer holds are left out")
	{
		constexpr int64_t WindowDurationUs{ 1000000 };

		RTC::BWE::SendPacketHistory sendPacketHistory({ .windowDurationUs = WindowDurationUs });
		RTC::BWE::FeedbackAdapter feedbackAdapter(&sendPacketHistory);

		sendPacketHistory.AddPacket(Ssrc, 100, PacketSize, false, InitialTimeUs);

		// Long enough afterwards for the window to drop the first packet.
		sendPacketHistory.AddPacket(
		  Ssrc, 101, PacketSize, false, InitialTimeUs + WindowDurationUs + 10000);

		const auto feedback = createFeedback(
		  0,
		  RemoteTimeUs,
		  {
		    { 0, RemoteTimeUs         },
        { 1, RemoteTimeUs + 10000 }
    });

		const auto processedFeedback = feedbackAdapter.ProcessTransportFeedback(
		  feedback.get(), InitialTimeUs + WindowDurationUs + 50000);

		REQUIRE(processedFeedback.has_value());

		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		const auto& result = processedFeedback.value();

		REQUIRE(result.packetFeedbacks.size() == 1);
		REQUIRE(result.packetFeedbacks[0].sentPacket.sequenceNumber == 1);
	}

	SECTION("the arrival times of consecutive feedbacks share a single timeline")
	{
		RTC::BWE::SendPacketHistory sendPacketHistory;
		RTC::BWE::FeedbackAdapter feedbackAdapter(&sendPacketHistory);

		sendPacketHistory.AddPacket(Ssrc, 100, PacketSize, false, InitialTimeUs);
		sendPacketHistory.AddPacket(Ssrc, 101, PacketSize, false, InitialTimeUs + 100000);

		const auto firstFeedback = createFeedback(
		  0,
		  RemoteTimeUs,
		  {
		    { 0, RemoteTimeUs }
    });
		const auto secondFeedback = createFeedback(
		  1,
		  RemoteTimeUs + 100000,
		  {
		    { 1, RemoteTimeUs + 100000 }
    });

		const auto firstProcessedFeedback =
		  feedbackAdapter.ProcessTransportFeedback(firstFeedback.get(), InitialTimeUs + 50000);
		const auto secondProcessedFeedback =
		  feedbackAdapter.ProcessTransportFeedback(secondFeedback.get(), InitialTimeUs + 150000);

		REQUIRE(firstProcessedFeedback.has_value());
		REQUIRE(secondProcessedFeedback.has_value());

		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		const auto& firstResult = firstProcessedFeedback.value();
		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		const auto& secondResult = secondProcessedFeedback.value();

		REQUIRE(firstResult.packetFeedbacks.size() == 1);
		REQUIRE(secondResult.packetFeedbacks.size() == 1);

		// Both packets took the very same time to arrive, so both feedbacks have to
		// tell the same however each of them was anchored.
		const auto& firstPacketResult  = firstResult.packetFeedbacks[0];
		const auto& secondPacketResult = secondResult.packetFeedbacks[0];

		const int64_t firstDelayUs =
		  // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		  firstPacketResult.receiveTimeUs.value() - firstPacketResult.sentPacket.sendTimeUs;
		const int64_t secondDelayUs =
		  // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		  secondPacketResult.receiveTimeUs.value() - secondPacketResult.sentPacket.sendTimeUs;

		REQUIRE(firstDelayUs == secondDelayUs);
	}

	SECTION("a feedback arriving out of order keeps the timeline")
	{
		RTC::BWE::SendPacketHistory sendPacketHistory;
		RTC::BWE::FeedbackAdapter feedbackAdapter(&sendPacketHistory);

		sendPacketHistory.AddPacket(Ssrc, 100, PacketSize, false, InitialTimeUs);
		sendPacketHistory.AddPacket(Ssrc, 101, PacketSize, false, InitialTimeUs + 100000);

		const auto firstFeedback = createFeedback(
		  0,
		  RemoteTimeUs,
		  {
		    { 0, RemoteTimeUs }
    });
		const auto secondFeedback = createFeedback(
		  1,
		  RemoteTimeUs + 100000,
		  {
		    { 1, RemoteTimeUs + 100000 }
    });

		// The second feedback is processed first, and the first one arrives late.
		const auto secondResult =
		  feedbackAdapter.ProcessTransportFeedback(secondFeedback.get(), InitialTimeUs + 150000);
		const auto firstResult =
		  feedbackAdapter.ProcessTransportFeedback(firstFeedback.get(), InitialTimeUs + 160000);

		REQUIRE(secondResult.has_value());
		REQUIRE(firstResult.has_value());
		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		REQUIRE(secondResult->packetFeedbacks.size() == 1);
		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		REQUIRE(firstResult->packetFeedbacks.size() == 1);

		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		const auto& firstPacketResult = firstResult->packetFeedbacks[0];
		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		const auto& secondPacketResult = secondResult->packetFeedbacks[0];

		REQUIRE(
		  // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		  firstPacketResult.receiveTimeUs.value() - firstPacketResult.sentPacket.sendTimeUs ==
		  // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		  secondPacketResult.receiveTimeUs.value() - secondPacketResult.sentPacket.sendTimeUs);
	}

	SECTION("the base time wrapping around does not break the timeline")
	{
		constexpr int64_t FirstBaseTimeUs{ RTC::RTCP::FeedbackRtpTransportPacket::TimeWrapPeriodUs -
		                                   RTC::RTCP::FeedbackRtpTransportPacket::BaseTimeTickUs };
		constexpr int64_t SecondBaseTimeUs{ RTC::RTCP::FeedbackRtpTransportPacket::TimeWrapPeriodUs };
		constexpr int64_t ElapsedUs{ SecondBaseTimeUs - FirstBaseTimeUs };

		RTC::BWE::SendPacketHistory sendPacketHistory;
		RTC::BWE::FeedbackAdapter feedbackAdapter(&sendPacketHistory);

		sendPacketHistory.AddPacket(Ssrc, 100, PacketSize, false, InitialTimeUs);
		sendPacketHistory.AddPacket(Ssrc, 101, PacketSize, false, InitialTimeUs + ElapsedUs);

		const auto firstFeedback = createFeedback(
		  0,
		  FirstBaseTimeUs,
		  {
		    { 0, FirstBaseTimeUs }
    });
		const auto secondFeedback = createFeedback(
		  1,
		  SecondBaseTimeUs,
		  {
		    { 1, SecondBaseTimeUs }
    });

		const auto firstResult =
		  feedbackAdapter.ProcessTransportFeedback(firstFeedback.get(), InitialTimeUs + 50000);
		const auto secondResult = feedbackAdapter.ProcessTransportFeedback(
		  secondFeedback.get(), InitialTimeUs + ElapsedUs + 50000);

		REQUIRE(firstResult.has_value());
		REQUIRE(secondResult.has_value());
		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		REQUIRE(firstResult->packetFeedbacks.size() == 1);
		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		REQUIRE(secondResult->packetFeedbacks.size() == 1);

		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		const auto& firstPacketResult = firstResult->packetFeedbacks[0];
		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		const auto& secondPacketResult = secondResult->packetFeedbacks[0];

		REQUIRE(
		  // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		  firstPacketResult.receiveTimeUs.value() - firstPacketResult.sentPacket.sendTimeUs ==
		  // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		  secondPacketResult.receiveTimeUs.value() - secondPacketResult.sentPacket.sendTimeUs);
	}

	SECTION("the packets are reported in the order the feedback reports them")
	{
		RTC::BWE::SendPacketHistory sendPacketHistory;
		RTC::BWE::FeedbackAdapter feedbackAdapter(&sendPacketHistory);

		sendPacketHistory.AddPacket(Ssrc, 100, PacketSize, false, InitialTimeUs);
		sendPacketHistory.AddPacket(Ssrc, 101, PacketSize, false, InitialTimeUs + 10000);
		sendPacketHistory.AddPacket(Ssrc, 102, PacketSize, false, InitialTimeUs + 20000);

		// The packets sent later arrived earlier, which is up to whoever consumes
		// this to reorder.
		const auto feedback = createFeedback(
		  0,
		  RemoteTimeUs + 20000,
		  {
		    { 0, RemoteTimeUs + 20000 },
        { 1, RemoteTimeUs + 10000 },
        { 2, RemoteTimeUs         }
    });

		const auto result =
		  feedbackAdapter.ProcessTransportFeedback(feedback.get(), InitialTimeUs + 50000);

		REQUIRE(result.has_value());
		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		REQUIRE(result->packetFeedbacks.size() == 3);
		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		REQUIRE(result->packetFeedbacks[0].sentPacket.sequenceNumber == 0);
		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		REQUIRE(result->packetFeedbacks[1].sentPacket.sequenceNumber == 1);
		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		REQUIRE(result->packetFeedbacks[2].sentPacket.sequenceNumber == 2);
		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		REQUIRE(result->packetFeedbacks[0].receiveTimeUs > result->packetFeedbacks[1].receiveTimeUs);
		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		REQUIRE(result->packetFeedbacks[1].receiveTimeUs > result->packetFeedbacks[2].receiveTimeUs);
	}

	SECTION("the packets a lost feedback reported on are not reported as lost")
	{
		RTC::BWE::SendPacketHistory sendPacketHistory;
		RTC::BWE::FeedbackAdapter feedbackAdapter(&sendPacketHistory);

		sendPacketHistory.AddPacket(Ssrc, 100, PacketSize, false, InitialTimeUs);
		sendPacketHistory.AddPacket(Ssrc, 101, PacketSize, false, InitialTimeUs + 10000);
		sendPacketHistory.AddPacket(Ssrc, 102, PacketSize, false, InitialTimeUs + 20000);

		// The feedback reporting on the first two packets never made it, so the next
		// one starts at the third.
		const auto feedback = createFeedback(
		  2,
		  RemoteTimeUs + 20000,
		  {
		    { 2, RemoteTimeUs + 20000 }
    });

		const auto result =
		  feedbackAdapter.ProcessTransportFeedback(feedback.get(), InitialTimeUs + 50000);

		REQUIRE(result.has_value());
		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		REQUIRE(result->packetFeedbacks.size() == 1);
		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		REQUIRE(result->packetFeedbacks[0].sentPacket.sequenceNumber == 2);
	}

	SECTION("the feedbacks are resolved although the packets being sent run far ahead of them")
	{
		// More than half the range of the sequence numbers the wire carries, which is
		// what makes the latest packet sent useless to resolve against.
		constexpr int64_t PacketCount{ 40000 };

		RTC::BWE::SendPacketHistory sendPacketHistory;
		RTC::BWE::FeedbackAdapter feedbackAdapter(&sendPacketHistory);

		sendPacketHistory.AddPacket(Ssrc, 100, PacketSize, false, InitialTimeUs);
		sendPacketHistory.AddPacket(Ssrc, 101, PacketSize, false, InitialTimeUs);

		// The first feedback reports on the packets sent so far.
		const auto firstFeedback = createFeedback(
		  0,
		  RemoteTimeUs,
		  {
		    { 0, RemoteTimeUs },
        { 1, RemoteTimeUs }
    });

		const auto firstResult =
		  feedbackAdapter.ProcessTransportFeedback(firstFeedback.get(), InitialTimeUs + 50000);

		REQUIRE(firstResult.has_value());
		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		REQUIRE(firstResult->packetFeedbacks.size() == 2);

		// And now a lot of packets are sent before the next feedback comes.
		for (int64_t idx{ 2 }; idx < PacketCount; ++idx)
		{
			sendPacketHistory.AddPacket(Ssrc, 102, PacketSize, false, InitialTimeUs);
		}

		REQUIRE(sendPacketHistory.GetLastSequenceNumber() == PacketCount - 1);

		const auto secondFeedback = createFeedback(
		  2,
		  RemoteTimeUs,
		  {
		    { 2, RemoteTimeUs }
    });

		const auto secondResult =
		  feedbackAdapter.ProcessTransportFeedback(secondFeedback.get(), InitialTimeUs + 60000);

		REQUIRE(secondResult.has_value());
		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		REQUIRE(secondResult->packetFeedbacks.size() == 1);
		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		REQUIRE(secondResult->packetFeedbacks[0].sentPacket.sequenceNumber == 2);
	}

	SECTION("a feedback reporting on no packet at all yields no value")
	{
		RTC::BWE::SendPacketHistory sendPacketHistory;
		RTC::BWE::FeedbackAdapter feedbackAdapter(&sendPacketHistory);

		sendPacketHistory.AddPacket(Ssrc, 100, PacketSize, false, InitialTimeUs);

		const auto feedback = createFeedback(0, RemoteTimeUs, {});

		REQUIRE(feedback->GetPacketStatusCount() == 0);
		REQUIRE(
		  feedbackAdapter.ProcessTransportFeedback(feedback.get(), InitialTimeUs + 50000) == std::nullopt);
	}

	SECTION("a feedback received before any packet was sent yields no value")
	{
		RTC::BWE::SendPacketHistory sendPacketHistory;
		RTC::BWE::FeedbackAdapter feedbackAdapter(&sendPacketHistory);

		const auto feedback = createFeedback(
		  0,
		  RemoteTimeUs,
		  {
		    { 0, RemoteTimeUs }
    });

		REQUIRE(
		  feedbackAdapter.ProcessTransportFeedback(feedback.get(), InitialTimeUs + 50000) == std::nullopt);
	}

	SECTION("a feedback reporting on no packet the history holds yields no value")
	{
		RTC::BWE::SendPacketHistory sendPacketHistory;
		RTC::BWE::FeedbackAdapter feedbackAdapter(&sendPacketHistory);

		sendPacketHistory.AddPacket(Ssrc, 100, PacketSize, false, InitialTimeUs);

		// A sequence number this history never gave out.
		const auto feedback = createFeedback(
		  1000,
		  RemoteTimeUs,
		  {
		    { 1000, RemoteTimeUs }
    });

		REQUIRE(
		  feedbackAdapter.ProcessTransportFeedback(feedback.get(), InitialTimeUs + 50000) == std::nullopt);
	}
}
