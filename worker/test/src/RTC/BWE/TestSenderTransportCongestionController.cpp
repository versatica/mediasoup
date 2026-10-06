#include "common.hpp"
#include "RTC/BWE/SenderTransportCongestionController.hpp"
#include "RTC/Consts.hpp"
#include "RTC/RTCP/FeedbackRtpTransport.hpp"
#include "RTC/RTP/HeaderExtensionIds.hpp"
#include "RTC/RTP/Packet.hpp"
#include "RTC/RtpDictionaries.hpp"
#include "Utils.hpp"
#include "test/include/RTC/RTP/rtpCommon.hpp"
#include "mocks/include/MockShared.hpp"
#include <catch2/catch_test_macros.hpp>
#include <string_view>
#include <vector>

SCENARIO("BWE SenderTransportCongestionController", "[bwe][sendertransportcongestioncontroller]")
{
	// The clock starts well away from zero so that a mistake taking a time for a
	// duration doesn't go unnoticed.
	constexpr int64_t InitialTimeUs{ 100000000 };
	// The remote clock has nothing to do with ours, and is a whole number of base
	// time ticks so that the arrival times survive the feedback untouched.
	constexpr int64_t RemoteTimeUs{ 1000000000000 };
	constexpr int64_t StartBitrate{ 300000 };
	constexpr int64_t MinBitrate{ 30000 };
	constexpr int64_t MaxBitrate{ 5000000 };
	constexpr uint32_t Ssrc{ 1111 };
	constexpr uint8_t TransportWideCc01Id{ 5 };
	constexpr uint8_t AbsSendTimeId{ 4 };
	constexpr size_t PayloadSize{ 1000 };
	constexpr std::string_view ProcessTimerLabel{ "sender-transport-congestion-controller-process" };

	class TestSenderTransportCongestionControllerListener
	  : public RTC::BWE::SenderTransportCongestionController::Listener
	{
	public:
		void OnSenderTransportCongestionControllerTargetBitrate(
		  RTC::BWE::SenderTransportCongestionController* /*senderTransportCongestionController*/,
		  int64_t targetBitrate) override
		{
			this->targetBitrates.push_back(targetBitrate);
		}

		void OnSenderTransportCongestionControllerSendRtpProbePacket(
		  RTC::BWE::SenderTransportCongestionController* /*senderTransportCongestionController*/,
		  RTC::RTP::Packet* packet,
		  int64_t sequenceNumber) override
		{
			this->sentLengths.push_back(packet->GetLength());
			this->sentSequenceNumbers.push_back(sequenceNumber);

			// What goes on the wire has to be what we were handed, since that is what
			// the remote endpoint will report back about.
			uint16_t wideSeqNumber{ 0 };
			uint32_t absSendTime{ 0 };

			this->sentWithExtensions.push_back(
			  packet->ReadTransportWideCc01(wideSeqNumber) && packet->ReadAbsSendTime(absSendTime) &&
			  wideSeqNumber == static_cast<uint16_t>(sequenceNumber));
		}

	public:
		std::vector<int64_t> targetBitrates;
		std::vector<size_t> sentLengths;
		std::vector<int64_t> sentSequenceNumbers;
		std::vector<bool> sentWithExtensions;
	};

	int64_t nowUs{ InitialTimeUs };

	mocks::MockShared shared(/*getTimeUs*/
	                         [&nowUs]() -> int64_t
	                         {
		                         return nowUs;
	                         });

	TestSenderTransportCongestionControllerListener listener;

	RTC::BWE::SenderTransportCongestionController senderTransportCongestionController(
	  std::addressof(listener),
	  std::addressof(shared),
	  { .startBitrate = StartBitrate, .minBitrate = MinBitrate, .maxBitrate = MaxBitrate });

	// Builds a packet carrying both extensions with no value in either, which is
	// what the sending side hands over.
	//
	// NOTE: Every scenario reuses `rtpCommon::FactoryBuffer`, so the returned
	// packet is only valid until the next call.
	const auto buildPacket = []() -> std::unique_ptr<RTC::RTP::Packet>
	{
		std::unique_ptr<RTC::RTP::Packet> packet(
		  RTC::RTP::Packet::Factory(rtpCommon::FactoryBuffer, sizeof(rtpCommon::FactoryBuffer)));

		REQUIRE(packet);

		packet->SetSsrc(Ssrc);

		const std::vector<RTC::RTP::Packet::Extension> extensions{
			{ RTC::RtpHeaderExtensionUri::Type::TRANSPORT_WIDE_CC_01,
			 TransportWideCc01Id,			                                              /*len*/ 2,
			 rtpCommon::DataBuffer                                                                                    },
			{ RTC::RtpHeaderExtensionUri::Type::ABS_SEND_TIME,        AbsSendTimeId, /*len*/ 3, rtpCommon::DataBuffer }
		};

		packet->SetExtensions(RTC::RTP::Packet::ExtensionsType::OneByte, extensions);

		RTC::RTP::HeaderExtensionIds headerExtensionIds;

		headerExtensionIds.transportWideCc01 = TransportWideCc01Id;
		headerExtensionIds.absSendTime       = AbsSendTimeId;

		packet->AssignExtensionIds(headerExtensionIds);
		packet->SetPayloadLength(PayloadSize);

		return packet;
	};

	// Let the periodic work run once.
	const auto runProcessTimer = [&nowUs, &shared, &ProcessTimerLabel]()
	{
		auto* timer = shared.GetTimer(ProcessTimerLabel);

		REQUIRE(timer);

		nowUs += timer->GetRepeatMs() * 1000;

		REQUIRE(timer->EvaluateHasExpired());
	};

	// Builds the feedback a receiver would send, reporting the given arrival
	// times. The sequence numbers left out of them are reported as lost.
	const auto createFeedback = [](
	                              uint16_t baseSequenceNumber,
	                              int64_t baseTimeUs,
	                              const std::vector<std::pair<uint16_t, int64_t>>& receivedPackets)
	  -> std::unique_ptr<RTC::RTCP::FeedbackRtpTransportPacket>
	{
		constexpr uint32_t SenderSsrc{ 2222 };
		constexpr uint32_t MediaSsrc{ 3333 };
		constexpr size_t RtcpMtu{ 1200 };

		auto feedback = std::make_unique<RTC::RTCP::FeedbackRtpTransportPacket>(SenderSsrc, MediaSsrc);

		feedback->SetBase(baseSequenceNumber, baseTimeUs);

		for (const auto& [sequenceNumber, receivedAtUs] : receivedPackets)
		{
			feedback->AddPacket(sequenceNumber, receivedAtUs, RtcpMtu);
		}

		feedback->Finish();

		return feedback;
	};

	// Sends a packet every 50 ms for the given span and feeds back the arrival of
	// each one, with an arrival that falls further and further behind by
	// `delayPerPacketUs` on every packet, which is what a queue filling up looks
	// like. Answers the target the loop is left at.
	uint16_t nextRtpSequenceNumber{ 0 };

	const auto transmitAndFeedBack = [&nowUs,
	                                  &shared,
	                                  &ProcessTimerLabel,
	                                  &senderTransportCongestionController,
	                                  &buildPacket,
	                                  &createFeedback,
	                                  &nextRtpSequenceNumber](
	                                   int64_t runtimeMs, int64_t delayPerPacketUs) -> int64_t
	{
		const int64_t startUs = nowUs;
		int64_t delayBuildupUs{ 0 };

		while (nowUs - startUs < runtimeMs * 1000)
		{
			const auto packet = buildPacket();

			packet->SetSequenceNumber(nextRtpSequenceNumber++);

			const auto sequenceNumber = senderTransportCongestionController.OnRtpPacketToBeSent(
			  packet.get(), RTC::BWE::SenderTransportCongestionController::RtpPacketToBeSentOptions{});

			REQUIRE(sequenceNumber.has_value());

			senderTransportCongestionController.OnRtpPacketSent(sequenceNumber, PayloadSize, nowUs);

			// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
			const auto wideSeqNumber = static_cast<uint16_t>(sequenceNumber.value());
			const int64_t arrivalUs  = RemoteTimeUs + (nowUs - InitialTimeUs) + delayBuildupUs;

			const auto feedback = createFeedback(
			  wideSeqNumber,
			  arrivalUs,
			  {
			    { wideSeqNumber, arrivalUs }
      });

			delayBuildupUs += delayPerPacketUs;

			senderTransportCongestionController.ReceiveTransportWideCcFeedback(feedback.get(), nowUs);

			nowUs += 50000;

			shared.GetTimer(ProcessTimerLabel)->EvaluateHasExpired();
		}

		return senderTransportCongestionController.GetAvailableBitrate();
	};

	SECTION("the target starts at the bitrate it was given")
	{
		runProcessTimer();

		REQUIRE(listener.targetBitrates.size() == 1);
		REQUIRE(listener.targetBitrates.at(0) == StartBitrate);

		REQUIRE(senderTransportCongestionController.GetAvailableBitrate() == StartBitrate);
	}

	SECTION("nothing is told twice while nothing changes")
	{
		runProcessTimer();

		REQUIRE(listener.targetBitrates.size() == 1);

		runProcessTimer();
		runProcessTimer();

		REQUIRE(listener.targetBitrates.size() == 1);
	}

	SECTION("what the remote endpoint says it can take caps the target")
	{
		runProcessTimer();

		senderTransportCongestionController.ReceiveEstimatedBitrate(StartBitrate / 2);

		runProcessTimer();

		REQUIRE(listener.targetBitrates.back() == StartBitrate / 2);

		// And it follows it back up, since the cap is not a ratchet.
		senderTransportCongestionController.ReceiveEstimatedBitrate(StartBitrate);

		runProcessTimer();

		REQUIRE(listener.targetBitrates.back() == StartBitrate);
	}

	SECTION("a REMB of zero is no cap at all")
	{
		runProcessTimer();

		senderTransportCongestionController.ReceiveEstimatedBitrate(StartBitrate / 2);

		runProcessTimer();

		REQUIRE(listener.targetBitrates.back() == StartBitrate / 2);

		senderTransportCongestionController.ReceiveEstimatedBitrate(0);

		runProcessTimer();

		REQUIRE(listener.targetBitrates.back() == StartBitrate);
	}

	SECTION("changing the bounds does not send the target back to where it started")
	{
		runProcessTimer();

		// Something the remote endpoint says brings the target down, which is what a
		// reconfiguration must not undo.
		senderTransportCongestionController.ReceiveEstimatedBitrate(StartBitrate / 2);

		runProcessTimer();

		REQUIRE(senderTransportCongestionController.GetAvailableBitrate() == StartBitrate / 2);

		senderTransportCongestionController.SetBitrateLimits(MinBitrate, MaxBitrate / 2);

		REQUIRE(senderTransportCongestionController.GetAvailableBitrate() == StartBitrate / 2);
	}

	SECTION("the bounds are brought into a range the loop can work with")
	{
		// A maximum below the minimum is not a range, and a minimum below what the
		// module works with cannot be honoured either.
		senderTransportCongestionController.SetBitrateLimits(/*minBitrate*/ 1, /*maxBitrate*/ 1);

		REQUIRE(senderTransportCongestionController.GetAvailableBitrate() >= RTC::Consts::BweMinBitrate);
	}

	SECTION("a packet about to be sent gets both extensions written")
	{
		const auto packet = buildPacket();

		const auto sequenceNumber = senderTransportCongestionController.OnRtpPacketToBeSent(
		  packet.get(), RTC::BWE::SenderTransportCongestionController::RtpPacketToBeSentOptions{});

		REQUIRE(sequenceNumber.has_value());

		uint16_t wideSeqNumber{ 0 };

		REQUIRE(packet->ReadTransportWideCc01(wideSeqNumber));
		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		REQUIRE(wideSeqNumber == static_cast<uint16_t>(sequenceNumber.value()));

		uint32_t absSendTime{ 0 };

		REQUIRE(packet->ReadAbsSendTime(absSendTime));
		REQUIRE(absSendTime == Utils::Time::TimeUsToAbsSendTime(nowUs));
	}

	SECTION("the sequence numbers handed out follow one another")
	{
		const auto firstPacket = buildPacket();

		const auto firstSequenceNumber = senderTransportCongestionController.OnRtpPacketToBeSent(
		  firstPacket.get(), RTC::BWE::SenderTransportCongestionController::RtpPacketToBeSentOptions{});

		const auto secondPacket = buildPacket();

		const auto secondSequenceNumber = senderTransportCongestionController.OnRtpPacketToBeSent(
		  secondPacket.get(), RTC::BWE::SenderTransportCongestionController::RtpPacketToBeSentOptions{});

		REQUIRE(firstSequenceNumber.has_value());
		REQUIRE(secondSequenceNumber.has_value());
		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		REQUIRE(secondSequenceNumber.value() == firstSequenceNumber.value() + 1);
	}

	SECTION("a packet with no room for the sequence number is not taken note of")
	{
		const auto firstVideoPacket = buildPacket();

		const auto firstVideoSequenceNumber = senderTransportCongestionController.OnRtpPacketToBeSent(
		  firstVideoPacket.get(),
		  RTC::BWE::SenderTransportCongestionController::RtpPacketToBeSentOptions{});

		// Which is what every audio packet looks like, since the extension is only
		// written into video ones.
		std::unique_ptr<RTC::RTP::Packet> audioPacket(
		  RTC::RTP::Packet::Factory(rtpCommon::FactoryBuffer, sizeof(rtpCommon::FactoryBuffer)));

		REQUIRE(audioPacket);

		audioPacket->SetSsrc(Ssrc);

		RTC::RTP::HeaderExtensionIds headerExtensionIds;

		headerExtensionIds.transportWideCc01 = TransportWideCc01Id;
		headerExtensionIds.absSendTime       = AbsSendTimeId;

		audioPacket->AssignExtensionIds(headerExtensionIds);
		audioPacket->SetPayloadLength(PayloadSize);

		const auto audioSequenceNumber = senderTransportCongestionController.OnRtpPacketToBeSent(
		  audioPacket.get(), RTC::BWE::SenderTransportCongestionController::RtpPacketToBeSentOptions{});

		REQUIRE(audioSequenceNumber.has_value() == false);

		// And no sequence number was spent on it either, which would leave a hole in
		// the sequence space that the remote endpoint would report as a lost packet
		// that never existed.
		const auto secondVideoPacket = buildPacket();

		const auto secondVideoSequenceNumber = senderTransportCongestionController.OnRtpPacketToBeSent(
		  secondVideoPacket.get(),
		  RTC::BWE::SenderTransportCongestionController::RtpPacketToBeSentOptions{});

		REQUIRE(firstVideoSequenceNumber.has_value());
		REQUIRE(secondVideoSequenceNumber.has_value());
		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		REQUIRE(secondVideoSequenceNumber.value() == firstVideoSequenceNumber.value() + 1);
	}

	SECTION("a growing delay brings the target below what a clean link leaves it at")
	{
		constexpr int64_t RunTimeMs{ 6000 };

		senderTransportCongestionController.SetNetworkAvailable(true);

		// Long enough for the delay based path to have something to say, over a link
		// that delays nothing and therefore builds no queue.
		const int64_t targetBeforeDelay = transmitAndFeedBack(RunTimeMs, /*delayPerPacketUs*/ 0);

		// And again, with every packet arriving later than the one before it.
		const int64_t targetAfterDelay = transmitAndFeedBack(RunTimeMs, /*delayPerPacketUs*/ 50000);

		REQUIRE(targetAfterDelay < targetBeforeDelay);
	}

	SECTION("a feedback about packets that were never sent does nothing")
	{
		senderTransportCongestionController.SetNetworkAvailable(true);

		runProcessTimer();

		const int64_t targetBefore = senderTransportCongestionController.GetAvailableBitrate();

		const auto feedback = createFeedback(
		  1000,
		  RemoteTimeUs,
		  {
		    { 1000, RemoteTimeUs         },
        { 1001, RemoteTimeUs + 10000 }
    });

		senderTransportCongestionController.ReceiveTransportWideCcFeedback(feedback.get(), nowUs);

		REQUIRE(senderTransportCongestionController.GetAvailableBitrate() == targetBefore);
	}

	SECTION("a confirmation for a packet that was never taken note of does nothing")
	{
		senderTransportCongestionController.OnRtpPacketSent(
		  /*sequenceNumber*/ 12345, PayloadSize, nowUs);

		runProcessTimer();

		REQUIRE(listener.targetBitrates.size() == 1);
		REQUIRE(listener.targetBitrates.at(0) == StartBitrate);
	}

	SECTION("nothing is probed until there is a network to probe")
	{
		auto* timer = shared.GetTimer("probing-scheduler-next-probe");

		REQUIRE(timer);
		REQUIRE(timer->IsActive() == false);

		senderTransportCongestionController.SetNetworkAvailable(true);

		REQUIRE(timer->IsActive());
	}

	SECTION("every packet of a burst goes out numbered and with both extensions written")
	{
		senderTransportCongestionController.SetNetworkAvailable(true);

		auto* timer = shared.GetTimer("probing-scheduler-next-probe");

		REQUIRE(timer);

		// Let the whole burst out, shot by shot.
		while (timer->IsActive())
		{
			nowUs = std::max(nowUs, timer->GetExpiresAtMs() * 1000);

			REQUIRE(timer->EvaluateHasExpired());
		}

		REQUIRE(!listener.sentLengths.empty());
		REQUIRE(listener.sentSequenceNumbers.size() == listener.sentLengths.size());
		REQUIRE(listener.sentWithExtensions.size() == listener.sentLengths.size());

		for (size_t idx{ 0 }; idx < listener.sentSequenceNumbers.size(); ++idx)
		{
			// Each packet carries the very number it was handed over with, and the
			// room for the arrival time the remote endpoint fills in.
			REQUIRE(listener.sentWithExtensions.at(idx));

			// And they are handed out one after another, so that the feedback can
			// name every one of them.
			if (idx > 0)
			{
				REQUIRE(listener.sentSequenceNumbers.at(idx) == listener.sentSequenceNumbers.at(idx - 1) + 1);
			}
		}
	}
}
