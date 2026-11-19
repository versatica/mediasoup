#include "common.hpp"
#include "RTC/BWE/SenderTransportCongestionController.hpp"
#include "RTC/Consts.hpp"
#include "RTC/RTCP/packet/TransportWideCcPacket.hpp"
#include "RTC/RTP/HeaderExtensionIds.hpp"
#include "RTC/RTP/Packet.hpp"
#include "RTC/RtpDictionaries.hpp"
#include "Utils.hpp"
#include "test/include/RTC/RTCP/rtcpCommon.hpp"
#include "test/include/RTC/RTP/rtpCommon.hpp"
#include "mocks/include/MockShared.hpp"
#include <catch2/catch_test_macros.hpp>
#include <deque>
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
		  std::optional<int64_t> sequenceNumber) override
		{
			REQUIRE(sequenceNumber.has_value());

			// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
			const int64_t sequenceNumberValue = sequenceNumber.value();

			this->sentLengths.push_back(packet->GetLength());
			this->sentSequenceNumbers.push_back(sequenceNumberValue);

			// What goes on the wire has to be what we were handed, since that is what
			// the remote endpoint will report back about.
			uint16_t wideSeqNumber{ 0 };
			uint32_t absSendTime{ 0 };

			this->sentWithExtensions.push_back(
			  packet->ReadTransportWideCc01(wideSeqNumber) && packet->ReadAbsSendTime(absSendTime) &&
			  wideSeqNumber == static_cast<uint16_t>(sequenceNumberValue));
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

	// Every scenario below is written for a peer that negotiated transport-cc,
	// which is what makes its packets worth taking note of.
	senderTransportCongestionController.SetTransportCcFeedbackSupported(true);

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

		REQUIRE(packet->SetExtensions(RTC::RTP::Packet::ExtensionsType::OneByte, extensions) == true);

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
	//
	// NOTE: Every scenario reuses `rtcpCommon::FactoryBuffer`, so the returned
	// feedback is only valid until the next call.
	const auto createFeedback = [](
	                              uint16_t baseSequenceNumber,
	                              int64_t baseTimeUs,
	                              const std::vector<std::pair<uint16_t, int64_t>>& receivedPackets)
	  -> std::unique_ptr<RTC::RTCP::TransportWideCcPacket>
	{
		constexpr uint32_t SenderSsrc{ 2222 };
		constexpr uint32_t MediaSsrc{ 3333 };
		constexpr size_t RtcpMtu{ 1200 };

		std::unique_ptr<RTC::RTCP::TransportWideCcPacket> feedback(
		  RTC::RTCP::TransportWideCcPacket::Factory(
		    rtcpCommon::FactoryBuffer, RtcpMtu, /*includeDeltas*/ true));

		feedback->SetSenderSsrc(SenderSsrc);
		feedback->SetMediaSsrc(MediaSsrc);
		feedback->SetBase(baseSequenceNumber, baseTimeUs);

		for (const auto& [sequenceNumber, receivedAtUs] : receivedPackets)
		{
			REQUIRE(feedback->AddReceivedRtpPacket(sequenceNumber, receivedAtUs));
		}

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

	// Lets a whole burst out of a controller of its own, so that two runs can be
	// compared, and answers the length of every packet that went out.
	//
	// NOTE: It brings its own clock and its own timers, since the ones of the
	// scenario are already taken by the controller above.
	const auto runBurst = [](size_t packetOverhead, int64_t desiredBitrate) -> std::vector<size_t>
	{
		int64_t burstNowUs{ InitialTimeUs };

		mocks::MockShared burstShared(/*getTimeUs*/
		                              [&burstNowUs]() -> int64_t
		                              {
			                              return burstNowUs;
		                              });

		TestSenderTransportCongestionControllerListener burstListener;

		RTC::BWE::SenderTransportCongestionController burstController(
		  std::addressof(burstListener),
		  std::addressof(burstShared),
		  { .startBitrate = StartBitrate, .minBitrate = MinBitrate, .maxBitrate = MaxBitrate });

		burstController.SetTransportCcFeedbackSupported(true);
		burstController.SetPacketOverhead(packetOverhead);

		if (desiredBitrate != 0)
		{
			burstController.SetDesiredBitrate(desiredBitrate);
		}

		burstController.SetEnableProbing(true);
		burstController.SetNetworkAvailable(true);

		auto* timer = burstShared.GetTimer("probing-scheduler-next-probe");

		REQUIRE(timer);

		while (timer->IsActive())
		{
			burstNowUs = std::max(burstNowUs, timer->GetExpiresAtMs() * 1000);

			REQUIRE(timer->EvaluateHasExpired());
		}

		return burstListener.sentLengths;
	};

	// What the burst put on the wire altogether.
	const auto totalBytes = [](const std::vector<size_t>& lengths) -> size_t
	{
		size_t total{ 0 };

		for (const auto length : lengths)
		{
			total += length;
		}

		return total;
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

	SECTION("the loss the Receiver Reports measure brings the target down")
	{
		runProcessTimer();

		const int64_t targetBeforeLoss = listener.targetBitrates.back();

		// Well above the tenth of the packets where the loss rules start cutting.
		for (size_t idx{ 0 }; idx < 10; ++idx)
		{
			senderTransportCongestionController.ReceiveRtcpReceiverReport(30, 100);

			runProcessTimer();
		}

		REQUIRE(listener.targetBitrates.back() < targetBeforeLoss);
	}

	SECTION("a stretch where nothing got through at all is not a measurement")
	{
		runProcessTimer();

		REQUIRE(listener.targetBitrates.size() == 1);

		// Not a single packet arriving is a stretch the remote endpoint could not
		// report on rather than a loss of everything, so it is left out instead of
		// being fed as the worst measurement there is.
		for (size_t idx{ 0 }; idx < 10; ++idx)
		{
			senderTransportCongestionController.ReceiveRtcpReceiverReport(100, 100);

			runProcessTimer();
		}

		REQUIRE(listener.targetBitrates.size() == 1);
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

		// A link that never delays anything gives the estimator no reason to back
		// off, so whatever it was given to start from still stands.
		REQUIRE(targetBeforeDelay >= StartBitrate);

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

	SECTION("nothing is probed while the probing is not allowed")
	{
		auto* timer = shared.GetTimer("probing-scheduler-next-probe");

		REQUIRE(timer);

		senderTransportCongestionController.SetNetworkAvailable(true);

		// The estimator asked for the bursts the connection opens with, and none of
		// them reached the scheduler.
		REQUIRE(timer->IsActive() == false);

		senderTransportCongestionController.SetEnableProbing(true);

		// What was asked for while it was not allowed is not kept waiting, so the
		// next burst the estimator asks for is what gets this going.
		REQUIRE(timer->IsActive() == false);
	}

	SECTION("taking the probing back stops the bursts that were waiting")
	{
		senderTransportCongestionController.SetEnableProbing(true);
		senderTransportCongestionController.SetNetworkAvailable(true);

		auto* timer = shared.GetTimer("probing-scheduler-next-probe");

		REQUIRE(timer);
		REQUIRE(timer->IsActive());

		senderTransportCongestionController.SetEnableProbing(false);

		// Let out whatever was already going out, shot by shot.
		while (timer->IsActive())
		{
			nowUs = std::max(nowUs, timer->GetExpiresAtMs() * 1000);

			REQUIRE(timer->EvaluateHasExpired());
		}

		const auto sentShots = listener.sentLengths.size();

		// And the estimator keeps running without anything else going out.
		runProcessTimer();

		REQUIRE(listener.sentLengths.size() == sentShots);
	}

	SECTION("nothing is probed until there is a network to probe")
	{
		senderTransportCongestionController.SetEnableProbing(true);

		auto* timer = shared.GetTimer("probing-scheduler-next-probe");

		REQUIRE(timer);
		REQUIRE(timer->IsActive() == false);

		senderTransportCongestionController.SetNetworkAvailable(true);

		REQUIRE(timer->IsActive());
	}

	SECTION("every packet of a burst goes out numbered and with both extensions written")
	{
		senderTransportCongestionController.SetEnableProbing(true);
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

	SECTION("a round trip time of less than a millisecond says nothing")
	{
		runProcessTimer();

		REQUIRE(listener.targetBitrates.size() == 1);

		// It is taken in whole milliseconds, which is the resolution of the
		// thresholds it is compared against, so this one rounds to nothing.
		senderTransportCongestionController.OnRttUpdate(400);

		runProcessTimer();

		REQUIRE(listener.targetBitrates.size() == 1);
	}

	SECTION("a round trip time that moved is told to the listener")
	{
		runProcessTimer();

		REQUIRE(listener.targetBitrates.size() == 1);

		senderTransportCongestionController.OnRttUpdate(50000);

		runProcessTimer();

		// The round trip time is part of what the listener reasons about, so it
		// hears about it even though the target did not move.
		REQUIRE(listener.targetBitrates.size() == 2);
		REQUIRE(listener.targetBitrates.back() == listener.targetBitrates.front());
	}

	SECTION("the overhead of each packet counts towards the bytes of a burst")
	{
		const auto withoutOverhead = runBurst(/*packetOverhead*/ 0, /*desiredBitrate*/ 0);
		const auto withOverhead    = runBurst(/*packetOverhead*/ 500, /*desiredBitrate*/ 0);

		REQUIRE(!withoutOverhead.empty());
		REQUIRE(!withOverhead.empty());

		// What the link carries is the packet plus everything below RTP, so a burst
		// that accounts for it needs fewer packets of its own to carry the bytes it
		// was meant to carry.
		REQUIRE(withOverhead.size() < withoutOverhead.size());
	}

	SECTION("what the Consumers want caps the bitrate of a burst")
	{
		const auto unbounded = runBurst(/*packetOverhead*/ 0, /*desiredBitrate*/ 0);
		const auto bounded   = runBurst(/*packetOverhead*/ 0, /*desiredBitrate*/ MinBitrate);

		REQUIRE(!unbounded.empty());
		REQUIRE(!bounded.empty());

		// Aiming far above what the application could ever send would measure room
		// that nothing is going to use, so the burst is held to what is wanted.
		REQUIRE(totalBytes(bounded) < totalBytes(unbounded));
	}

	SECTION("a sender that can only put out a few bitrates climbs back once a capped link opens")
	{
		// An SFU forwarding simulcast layers sends exactly what it is allowed and
		// nothing in between, so the link never carries more than the layer it is
		// on. Once a cap brought the estimate below the next layer up, only a burst
		// can tell that the link has room again.
		constexpr size_t PacketOverhead{ 40 };
		constexpr int64_t PropagationUs{ 20 * 1000 };
		constexpr int64_t MaxQueueingDelayUs{ 150 * 1000 };
		constexpr int64_t FeedbackIntervalUs{ 100 * 1000 };
		constexpr int64_t TickUs{ 1000 };
		// What the allocator charges for climbing back to a point already held.
		constexpr double ClimbBackHysteresis{ 1.2 };
		// Open link, 30 s at 900 kbit, 30 s at 400 kbit with a spell of loss as it
		// begins, which is what a queue overflowing on the way down looks like to
		// the receiver, then open again.
		constexpr int64_t OpenCapacity{ 20000000 };
		constexpr int64_t Cap1Capacity{ 900000 };
		constexpr int64_t Cap2Capacity{ 400000 };
		constexpr int64_t CapStartUs{ 20 * 1000 * 1000 };
		constexpr int64_t Cap2StartUs{ 50 * 1000 * 1000 };
		constexpr int64_t LossStartUs{ 53 * 1000 * 1000 };
		constexpr int64_t LossEndUs{ 57 * 1000 * 1000 };
		constexpr uint32_t LossPercent{ 20 };
		constexpr int64_t CapEndUs{ 80 * 1000 * 1000 };
		constexpr int64_t EndUs{ 120 * 1000 * 1000 };
		constexpr int64_t MaxRecoveryUs{ 10 * 1000 * 1000 };
		// What the allocation offers: the lowest spatial layer at two temporal
		// layers, then the two spatial layers above it.
		const std::vector<int64_t> layerBitrates{ 80000, 150000, 340000, 700000 };

		struct InFlight
		{
			int64_t sequenceNumber;
			size_t size;
			std::optional<int64_t> arrivalUs;
		};

		class LinkListener : public RTC::BWE::SenderTransportCongestionController::Listener
		{
		public:
			explicit LinkListener(std::function<void(int64_t, size_t)> onProbePacket)
			  : onProbePacket(std::move(onProbePacket))
			{
			}

			void OnSenderTransportCongestionControllerTargetBitrate(
			  RTC::BWE::SenderTransportCongestionController* /*senderTransportCongestionController*/,
			  int64_t targetBitrate) override
			{
				this->targetBitrate = targetBitrate;
			}

			void OnSenderTransportCongestionControllerSendRtpProbePacket(
			  RTC::BWE::SenderTransportCongestionController* /*senderTransportCongestionController*/,
			  RTC::RTP::Packet* packet,
			  std::optional<int64_t> sequenceNumber) override
			{
				// A packet without a sequence number was not taken note of, so there is
				// nothing to report back about it.
				if (sequenceNumber.has_value())
				{
					// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
					this->onProbePacket(sequenceNumber.value(), packet->GetLength());
				}
			}

		public:
			int64_t targetBitrate{ 0 };

		private:
			// Hands a packet of a burst to the link, in the order it was numbered.
			const std::function<void(int64_t, size_t)> onProbePacket;
		};

		int64_t linkNowUs{ InitialTimeUs };
		mocks::MockShared linkShared(/*getTimeUs*/
		                             [&linkNowUs]() -> int64_t
		                             {
			                             return linkNowUs;
		                             });

		// One queue draining at the capacity of the moment, dropping what would
		// wait too long in it.
		int64_t linkCapacity{ OpenCapacity };
		int64_t linkNextFreeUs{ 0 };
		bool linkLossy{ false };
		uint32_t lcg{ 12345 };
		std::deque<InFlight> inFlight;
		const auto sendOverLink = [&](int64_t sequenceNumber, size_t size) -> void
		{
			// A linear congruential generator, so that the loss is the same every run.
			lcg = lcg * 1664525u + 1013904223u;

			if (linkLossy && (lcg >> 16) % 100 < LossPercent)
			{
				inFlight.push_back(
				  { .sequenceNumber = sequenceNumber, .size = size, .arrivalUs = std::nullopt });

				return;
			}

			const int64_t startUs   = std::max(linkNextFreeUs, linkNowUs);
			const int64_t serviceUs = static_cast<int64_t>(size) * 8 * 1000000 / linkCapacity;

			if (startUs - linkNowUs > MaxQueueingDelayUs)
			{
				inFlight.push_back(
				  { .sequenceNumber = sequenceNumber, .size = size, .arrivalUs = std::nullopt });

				return;
			}

			linkNextFreeUs = startUs + serviceUs;

			inFlight.push_back(
			  { .sequenceNumber = sequenceNumber, .size = size, .arrivalUs = linkNextFreeUs + PropagationUs });
		};

		// NOTE: The listener needs the controller and the controller needs the
		// listener, hence the pointer filled in right after construction.
		RTC::BWE::SenderTransportCongestionController* linkControllerPtr{ nullptr };
		LinkListener linkListener(
		  [&](int64_t sequenceNumber, size_t length) -> void
		  {
			  const size_t size = length + PacketOverhead;

			  linkControllerPtr->OnRtpPacketSent(sequenceNumber, size, linkNowUs);
			  sendOverLink(sequenceNumber, size);
		  });
		RTC::BWE::SenderTransportCongestionController linkController(
		  std::addressof(linkListener),
		  std::addressof(linkShared),
		  { .startBitrate = StartBitrate, .minBitrate = MinBitrate, .maxBitrate = MaxBitrate });

		linkControllerPtr = std::addressof(linkController);
		linkController.SetTransportCcFeedbackSupported(true);
		linkController.SetPacketOverhead(PacketOverhead);
		linkController.SetDesiredBitrate(layerBitrates.back());
		linkController.SetEnableProbing(true);
		linkController.SetNetworkAvailable(true);
		linkListener.targetBitrate = linkController.GetAvailableBitrate();

		// The layer picked for a target, with the allocator's hysteresis for one
		// that was held before.
		size_t option{ 0 };
		size_t highestOptionHeld{ 0 };
		const auto chooseOption = [&](int64_t target) -> void
		{
			size_t chosen{ 0 };

			for (size_t i = 0; i < layerBitrates.size(); ++i)
			{
				const double factor = i > option && i <= highestOptionHeld ? ClimbBackHysteresis : 1.0;

				if (static_cast<double>(layerBitrates[i]) * factor <= static_cast<double>(target))
				{
					chosen = i;
				}
			}

			option            = chosen;
			highestOptionHeld = std::max(highestOptionHeld, option);
		};

		const auto fireLinkTimers = [&]() -> void
		{
			for (const auto label :
			     { ProcessTimerLabel, std::string_view{ "probing-scheduler-next-probe" } })
			{
				auto* timer = linkShared.GetTimer(label);

				if (timer && timer->IsActive() && timer->GetExpiresAtMs() <= linkNowUs / 1000)
				{
					timer->EvaluateHasExpired();
				}
			}
		};

		uint16_t linkRtpSequenceNumber{ 0 };
		double bytesDue{ 0 };
		int64_t nextFeedbackAtUs{ linkNowUs + FeedbackIntervalUs };
		int64_t maxTargetWhileCapped{ 0 };
		std::optional<int64_t> recoveredAfterUs;

		while (linkNowUs - InitialTimeUs < EndUs)
		{
			const int64_t tUs = linkNowUs - InitialTimeUs;

			linkCapacity = tUs < CapStartUs || tUs >= CapEndUs ? OpenCapacity
			               : tUs < Cap2StartUs                 ? Cap1Capacity
			                                                   : Cap2Capacity;
			linkLossy    = tUs >= LossStartUs && tUs < LossEndUs;

			chooseOption(linkListener.targetBitrate);

			// The layer's bitrate goes out packet by packet.
			bytesDue +=
			  static_cast<double>(layerBitrates[option]) / 8.0 * (static_cast<double>(TickUs) / 1e6);

			while (bytesDue >= static_cast<double>(PayloadSize))
			{
				bytesDue -= static_cast<double>(PayloadSize);

				const auto packet = buildPacket();

				packet->SetSequenceNumber(linkRtpSequenceNumber++);

				const auto sequenceNumber = linkController.OnRtpPacketToBeSent(
				  packet.get(), RTC::BWE::SenderTransportCongestionController::RtpPacketToBeSentOptions{});

				REQUIRE(sequenceNumber.has_value());

				const size_t size = packet->GetLength() + PacketOverhead;

				linkController.OnRtpPacketSent(sequenceNumber, size, linkNowUs);
				// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
				sendOverLink(sequenceNumber.value(), size);
			}

			if (linkNowUs >= nextFeedbackAtUs)
			{
				nextFeedbackAtUs += FeedbackIntervalUs;

				std::vector<std::pair<uint16_t, int64_t>> received;
				std::optional<uint16_t> baseSequenceNumber;

				while (!inFlight.empty() && (!inFlight.front().arrivalUs.has_value() ||
				                             inFlight.front().arrivalUs.value() <= linkNowUs))
				{
					const auto& packet = inFlight.front();

					if (!baseSequenceNumber.has_value())
					{
						baseSequenceNumber = static_cast<uint16_t>(packet.sequenceNumber);
					}

					if (packet.arrivalUs.has_value())
					{
						received.emplace_back(
						  static_cast<uint16_t>(packet.sequenceNumber),
						  // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
						  RemoteTimeUs + (packet.arrivalUs.value() - InitialTimeUs));
					}

					inFlight.pop_front();
				}

				if (baseSequenceNumber.has_value())
				{
					const auto feedback = createFeedback(
					  // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
					  baseSequenceNumber.value(),
					  RemoteTimeUs + (linkNowUs - InitialTimeUs),
					  received);

					linkController.ReceiveTransportWideCcFeedback(feedback.get(), linkNowUs + PropagationUs);
				}
			}

			fireLinkTimers();

			// The estimate takes a few seconds to come down once a cap is there, so
			// what the bursts do to it is read in the second half of the lower cap.
			if (tUs >= (Cap2StartUs + CapEndUs) / 2 && tUs < CapEndUs)
			{
				maxTargetWhileCapped = std::max(maxTargetWhileCapped, linkListener.targetBitrate);
			}

			if (tUs >= CapEndUs && !recoveredAfterUs.has_value() && option == layerBitrates.size() - 1)
			{
				recoveredAfterUs = tUs - CapEndUs;
			}

			linkNowUs += TickUs;
		}

		// The bursts sent while the link stayed capped did not talk it into more
		// than it carries.
		REQUIRE(maxTargetWhileCapped < 2 * Cap2Capacity);

		// And once the cap is gone the top layer is back within seconds, which is
		// what the bursts for an allocation above the estimate are for.
		REQUIRE(recoveredAfterUs.has_value());
		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		REQUIRE(recoveredAfterUs.value() <= MaxRecoveryUs);
	}
}
