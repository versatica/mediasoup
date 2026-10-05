#include "common.hpp"
#include "RTC/BWE/SenderTransportCongestionController.hpp"
#include "RTC/Consts.hpp"
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

		bool OnSenderTransportCongestionControllerSendRtpPacket(
		  RTC::BWE::SenderTransportCongestionController* /*senderTransportCongestionController*/,
		  RTC::RTP::Packet* packet) override
		{
			this->sentLengths.push_back(packet->GetLength());

			return true;
		}

	public:
		std::vector<int64_t> targetBitrates;
		std::vector<size_t> sentLengths;
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

		const int64_t sequenceNumber = senderTransportCongestionController.OnRtpPacketToBeSent(
		  packet.get(), RTC::BWE::SenderTransportCongestionController::RtpPacketToBeSentOptions{});

		uint16_t wideSeqNumber{ 0 };

		REQUIRE(packet->ReadTransportWideCc01(wideSeqNumber));
		REQUIRE(wideSeqNumber == static_cast<uint16_t>(sequenceNumber));

		uint32_t absSendTime{ 0 };

		REQUIRE(packet->ReadAbsSendTime(absSendTime));
		REQUIRE(absSendTime == Utils::Time::TimeUsToAbsSendTime(nowUs));
	}

	SECTION("the sequence numbers handed out follow one another")
	{
		const auto firstPacket = buildPacket();

		const int64_t firstSequenceNumber = senderTransportCongestionController.OnRtpPacketToBeSent(
		  firstPacket.get(), RTC::BWE::SenderTransportCongestionController::RtpPacketToBeSentOptions{});

		const auto secondPacket = buildPacket();

		const int64_t secondSequenceNumber = senderTransportCongestionController.OnRtpPacketToBeSent(
		  secondPacket.get(), RTC::BWE::SenderTransportCongestionController::RtpPacketToBeSentOptions{});

		REQUIRE(secondSequenceNumber == firstSequenceNumber + 1);
	}

	SECTION("a confirmation for a packet that was never taken note of does nothing")
	{
		senderTransportCongestionController.OnRtpPacketSent(/*sequenceNumber*/ 12345, nowUs);

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
}
