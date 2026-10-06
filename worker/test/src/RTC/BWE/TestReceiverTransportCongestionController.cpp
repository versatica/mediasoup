#include "common.hpp"
#include "RTC/BWE/ReceiverTransportCongestionController.hpp"
#include "RTC/RTCP/FeedbackPsRemb.hpp"
#include "RTC/RTP/HeaderExtensionIds.hpp"
#include "RTC/RTP/Packet.hpp"
#include "RTC/RtpDictionaries.hpp"
#include "Utils.hpp"
#include "test/include/RTC/RTP/rtpCommon.hpp"
#include "mocks/include/MockShared.hpp"
#include <catch2/catch_test_macros.hpp>
#include <string_view>
#include <vector>

SCENARIO("BWE ReceiverTransportCongestionController", "[bwe][receivertransportcongestioncontroller]")
{
	class TestReceiverTransportCongestionControllerListener
	  : public RTC::BWE::ReceiverTransportCongestionController::Listener
	{
	public:
		void OnReceiverTransportCongestionControllerSendRtcpPacket(
		  RTC::BWE::ReceiverTransportCongestionController* /*receiverTransportCongestionControl*/,
		  RTC::RTCP::Packet* packet) override
		{
			this->rtcpTypes.push_back(packet->GetType());

			if (packet->GetType() == RTC::RTCP::Type::PSFB)
			{
				const auto* rembPacket = static_cast<RTC::RTCP::FeedbackPsRembPacket*>(packet);

				this->rembBitrates.push_back(rembPacket->GetBitrate());
				this->rembSsrcs.push_back(rembPacket->GetSsrcs());
			}
		}

	public:
		std::vector<RTC::RTCP::Type> rtcpTypes;
		std::vector<int64_t> rembBitrates;
		std::vector<std::vector<uint32_t>> rembSsrcs;
	};

	constexpr uint32_t Ssrc{ 1111 };
	constexpr uint32_t Ssrc2{ 2222 };
	constexpr uint8_t TransportWideCc01Id{ 5 };
	constexpr uint8_t AbsSendTimeId{ 4 };
	// Instant the scenarios below start at, which is irrelevant other than for
	// being far from zero.
	constexpr int64_t BaseTimeUs{ 1000 * 1000 };
	constexpr size_t PayloadSize{ 1000 };
	// Packets a burst is made of, which is what the estimation of the incoming link
	// is drawn from.
	constexpr int Probes{ 5 };
	constexpr std::string_view FeedbackTimerLabel{ "transport-wide-cc-feedback-generator-send" };

	int64_t nowUs{ BaseTimeUs };

	mocks::MockShared shared(
	  [&nowUs]() -> int64_t
	  {
		  return nowUs;
	  });

	// Builds a packet carrying the transport wide sequence number, which is what a
	// sender that negotiated transport-cc emits.
	//
	// NOTE: Every scenario reuses `rtpCommon::FactoryBuffer`, so the returned packet
	// is only valid until the next call.
	const auto buildTransportCcPacket = [](uint16_t wideSeqNumber) -> std::unique_ptr<RTC::RTP::Packet>
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
		packet->SetPayloadLength(PayloadSize);

		REQUIRE(packet->UpdateTransportWideCc01(wideSeqNumber));

		return packet;
	};

	// Builds a packet carrying 'abs-send-time', which is what a sender that
	// negotiated REMB emits.
	const auto buildRembPacket =
	  [](int64_t sendTimeUs, uint32_t ssrc) -> std::unique_ptr<RTC::RTP::Packet>
	{
		std::unique_ptr<RTC::RTP::Packet> packet(
		  RTC::RTP::Packet::Factory(rtpCommon::FactoryBuffer, sizeof(rtpCommon::FactoryBuffer)));

		REQUIRE(packet);

		packet->SetSsrc(ssrc);

		const std::vector<RTC::RTP::Packet::Extension> extensions{
			{ RTC::RtpHeaderExtensionUri::Type::ABS_SEND_TIME, AbsSendTimeId, /*len*/ 3, rtpCommon::DataBuffer }
		};

		packet->SetExtensions(RTC::RTP::Packet::ExtensionsType::OneByte, extensions);

		RTC::RTP::HeaderExtensionIds headerExtensionIds;

		headerExtensionIds.absSendTime = AbsSendTimeId;

		packet->AssignExtensionIds(headerExtensionIds);
		packet->SetPayloadLength(PayloadSize);

		REQUIRE(packet->UpdateAbsSendTime(Utils::Time::TimeUsToAbsSendTime(sendTimeUs)));

		return packet;
	};

	SECTION("with transport-cc the arrival times reach the listener and nothing is estimated")
	{
		TestReceiverTransportCongestionControllerListener listener;

		RTC::BWE::ReceiverTransportCongestionController receiverTransportCongestionControl(
		  std::addressof(listener),
		  std::addressof(shared),
		  { .congestionControlType = RTC::BWE::Types::CongestionControlType::TRANSPORT_CC });

		for (uint16_t wideSeqNumber{ 0 }; wideSeqNumber < 10; ++wideSeqNumber)
		{
			nowUs += 10 * 1000;

			const auto packet = buildTransportCcPacket(wideSeqNumber);

			receiverTransportCongestionControl.ReceiveRtpPacket(
			  nowUs, packet.get(), RTC::Media::Kind::VIDEO);
		}

		// Nothing goes out until the periodic timer of the feedback generator fires.
		REQUIRE(listener.rtcpTypes.empty());

		auto* timer = shared.GetTimer(FeedbackTimerLabel);

		REQUIRE(timer);

		nowUs += timer->GetRepeatMs() * 1000;

		REQUIRE(timer->EvaluateHasExpired());

		REQUIRE(listener.rtcpTypes.size() == 1);
		REQUIRE(listener.rtcpTypes.at(0) == RTC::RTCP::Type::RTPFB);

		// The estimating is the remote sender's job in this mode.
		REQUIRE(receiverTransportCongestionControl.GetAvailableBitrate().has_value() == false);
	}

	SECTION("with REMB the estimation of the incoming link reaches the listener")
	{
		TestReceiverTransportCongestionControllerListener listener;

		RTC::BWE::ReceiverTransportCongestionController receiverTransportCongestionControl(
		  std::addressof(listener),
		  std::addressof(shared),
		  { .congestionControlType = RTC::BWE::Types::CongestionControlType::REMB });

		REQUIRE(receiverTransportCongestionControl.GetAvailableBitrate().has_value() == false);

		// A burst of packets spaced evenly, which is what the estimation is drawn
		// from at the beginning of a call.
		for (int idx{ 0 }; idx < Probes; ++idx)
		{
			nowUs += 10 * 1000;

			const auto packet = buildRembPacket(nowUs, Ssrc);

			receiverTransportCongestionControl.ReceiveRtpPacket(
			  nowUs, packet.get(), RTC::Media::Kind::VIDEO);
		}

		REQUIRE(listener.rtcpTypes.size() == 1);
		REQUIRE(listener.rtcpTypes.at(0) == RTC::RTCP::Type::PSFB);
		REQUIRE(listener.rembBitrates.at(0) > 0);

		// What went out is what is reported as available.
		REQUIRE(receiverTransportCongestionControl.GetAvailableBitrate() == listener.rembBitrates.at(0));
	}

	SECTION("a stream that is removed stops being announced")
	{
		TestReceiverTransportCongestionControllerListener listener;

		RTC::BWE::ReceiverTransportCongestionController receiverTransportCongestionControl(
		  std::addressof(listener),
		  std::addressof(shared),
		  { .congestionControlType = RTC::BWE::Types::CongestionControlType::REMB });

		// A burst where two streams alternate, so that the estimation applies to both.
		for (int idx{ 0 }; idx < Probes; ++idx)
		{
			nowUs += 10 * 1000;

			const auto packet = buildRembPacket(nowUs, idx % 2 == 0 ? Ssrc : Ssrc2);

			receiverTransportCongestionControl.ReceiveRtpPacket(
			  nowUs, packet.get(), RTC::Media::Kind::VIDEO);
		}

		REQUIRE(listener.rembSsrcs.size() == 1);
		REQUIRE(listener.rembSsrcs.at(0).size() == 2);

		receiverTransportCongestionControl.RemoveStream(Ssrc2);

		// Long enough for the next packet to bring another estimation out, and far
		// shorter than what it takes for a silent stream to be forgotten on its own.
		nowUs += 250 * 1000;

		const auto packet = buildRembPacket(nowUs, Ssrc);

		receiverTransportCongestionControl.ReceiveRtpPacket(nowUs, packet.get(), RTC::Media::Kind::VIDEO);

		REQUIRE(listener.rembSsrcs.size() == 2);
		REQUIRE(listener.rembSsrcs.at(1) == std::vector<uint32_t>{ Ssrc });
	}

	SECTION("with REMB audio takes no part in the estimation")
	{
		TestReceiverTransportCongestionControllerListener listener;

		RTC::BWE::ReceiverTransportCongestionController receiverTransportCongestionControl(
		  std::addressof(listener),
		  std::addressof(shared),
		  { .congestionControlType = RTC::BWE::Types::CongestionControlType::REMB });

		// The very same burst that gives an estimation when it carries video.
		for (int idx{ 0 }; idx < Probes; ++idx)
		{
			nowUs += 10 * 1000;

			const auto packet = buildRembPacket(nowUs, Ssrc);

			receiverTransportCongestionControl.ReceiveRtpPacket(
			  nowUs, packet.get(), RTC::Media::Kind::AUDIO);
		}

		REQUIRE(listener.rtcpTypes.empty());
		REQUIRE(receiverTransportCongestionControl.GetAvailableBitrate().has_value() == false);
	}

	SECTION("with transport-cc audio is reported like video")
	{
		TestReceiverTransportCongestionControllerListener listener;

		RTC::BWE::ReceiverTransportCongestionController receiverTransportCongestionControl(
		  std::addressof(listener),
		  std::addressof(shared),
		  { .congestionControlType = RTC::BWE::Types::CongestionControlType::TRANSPORT_CC });

		for (uint16_t wideSeqNumber{ 0 }; wideSeqNumber < 10; ++wideSeqNumber)
		{
			nowUs += 10 * 1000;

			const auto packet = buildTransportCcPacket(wideSeqNumber);

			receiverTransportCongestionControl.ReceiveRtpPacket(
			  nowUs, packet.get(), RTC::Media::Kind::AUDIO);
		}

		auto* timer = shared.GetTimer(FeedbackTimerLabel);

		REQUIRE(timer);

		nowUs += timer->GetRepeatMs() * 1000;

		REQUIRE(timer->EvaluateHasExpired());

		REQUIRE(listener.rtcpTypes.size() == 1);
		REQUIRE(listener.rtcpTypes.at(0) == RTC::RTCP::Type::RTPFB);
	}

	SECTION("the cap is announced even when transport-cc was negotiated")
	{
		constexpr int64_t MaxIncomingBitrate{ 500000 };

		TestReceiverTransportCongestionControllerListener listener;

		RTC::BWE::ReceiverTransportCongestionController receiverTransportCongestionControl(
		  std::addressof(listener),
		  std::addressof(shared),
		  { .congestionControlType = RTC::BWE::Types::CongestionControlType::TRANSPORT_CC });

		receiverTransportCongestionControl.SetMaxIncomingBitrate(MaxIncomingBitrate);

		nowUs += 10 * 1000;

		const auto packet = buildTransportCcPacket(/*wideSeqNumber*/ 0);

		receiverTransportCongestionControl.ReceiveRtpPacket(nowUs, packet.get(), RTC::Media::Kind::VIDEO);

		// Nothing else can tell the remote sender to hold back, since in this mode it
		// is the one estimating.
		REQUIRE(listener.rembBitrates.size() == 1);
		REQUIRE(listener.rembBitrates.at(0) == MaxIncomingBitrate);
	}

	SECTION("a cap of zero is no cap at all")
	{
		constexpr int64_t MaxIncomingBitrate{ 500000 };

		TestReceiverTransportCongestionControllerListener listener;

		RTC::BWE::ReceiverTransportCongestionController receiverTransportCongestionControl(
		  std::addressof(listener),
		  std::addressof(shared),
		  { .congestionControlType = RTC::BWE::Types::CongestionControlType::TRANSPORT_CC });

		receiverTransportCongestionControl.SetMaxIncomingBitrate(MaxIncomingBitrate);

		// The cap went out as soon as it was set.
		REQUIRE(listener.rembBitrates.size() == 1);
		REQUIRE(listener.rembBitrates.at(0) == MaxIncomingBitrate);

		receiverTransportCongestionControl.SetMaxIncomingBitrate(0);

		// A REMB of zero is how the remote sender is told that it may send whatever it
		// wants again, and the first of them goes out without waiting.
		REQUIRE(listener.rembBitrates.size() == 2);
		REQUIRE(listener.rembBitrates.at(1) == 0);

		// Losing that single REMB would leave the remote sender limited forever, so the
		// rest of them ride on the incoming packets once the announcing interval is past.
		nowUs += (1500 + 1) * 1000;

		const auto packet = buildTransportCcPacket(/*wideSeqNumber*/ 0);

		receiverTransportCongestionControl.ReceiveRtpPacket(nowUs, packet.get(), RTC::Media::Kind::VIDEO);

		REQUIRE(listener.rembBitrates.size() == 3);
		REQUIRE(listener.rembBitrates.back() == 0);
	}
}
