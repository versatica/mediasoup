#include "common.hpp"
#include "RTC/BWE/ReceiverTransportCongestionControl.hpp"
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

SCENARIO("BWE ReceiverTransportCongestionControl", "[bwe][receivertransportcongestioncontrol]")
{
	class TestReceiverTransportCongestionControlListener
	  : public RTC::BWE::ReceiverTransportCongestionControl::Listener
	{
	public:
		void OnReceiverTransportCongestionControlSendRtcpPacket(
		  RTC::BWE::ReceiverTransportCongestionControl* /*receiverTransportCongestionControl*/,
		  RTC::RTCP::Packet* packet) override
		{
			this->rtcpTypes.push_back(packet->GetType());

			if (packet->GetType() == RTC::RTCP::Type::PSFB)
			{
				this->rembBitrates.push_back(
				  static_cast<RTC::RTCP::FeedbackPsRembPacket*>(packet)->GetBitrate());
			}
		}

	public:
		std::vector<RTC::RTCP::Type> rtcpTypes;
		std::vector<int64_t> rembBitrates;
	};

	constexpr uint32_t Ssrc{ 1111 };
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
	const auto buildRembPacket = [](int64_t sendTimeUs) -> std::unique_ptr<RTC::RTP::Packet>
	{
		std::unique_ptr<RTC::RTP::Packet> packet(
		  RTC::RTP::Packet::Factory(rtpCommon::FactoryBuffer, sizeof(rtpCommon::FactoryBuffer)));

		REQUIRE(packet);

		packet->SetSsrc(Ssrc);

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
		TestReceiverTransportCongestionControlListener listener;

		RTC::BWE::ReceiverTransportCongestionControl receiverTransportCongestionControl(
		  std::addressof(listener),
		  std::addressof(shared),
		  { .congestionControlType = RTC::BWE::Types::CongestionControlType::TRANSPORT_CC });

		for (uint16_t wideSeqNumber{ 0 }; wideSeqNumber < 10; ++wideSeqNumber)
		{
			nowUs += 10 * 1000;

			const auto packet = buildTransportCcPacket(wideSeqNumber);

			receiverTransportCongestionControl.ReceiveRtpPacket(nowUs, packet.get());
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
		REQUIRE_FALSE(receiverTransportCongestionControl.GetAvailableBitrate().has_value());
	}

	SECTION("with REMB the estimation of the incoming link reaches the listener")
	{
		TestReceiverTransportCongestionControlListener listener;

		RTC::BWE::ReceiverTransportCongestionControl receiverTransportCongestionControl(
		  std::addressof(listener),
		  std::addressof(shared),
		  { .congestionControlType = RTC::BWE::Types::CongestionControlType::REMB });

		REQUIRE_FALSE(receiverTransportCongestionControl.GetAvailableBitrate().has_value());

		// A burst of packets spaced evenly, which is what the estimation is drawn
		// from at the beginning of a call.
		for (int idx{ 0 }; idx < Probes; ++idx)
		{
			nowUs += 10 * 1000;

			const auto packet = buildRembPacket(nowUs);

			receiverTransportCongestionControl.ReceiveRtpPacket(nowUs, packet.get());
		}

		REQUIRE(listener.rtcpTypes.size() == 1);
		REQUIRE(listener.rtcpTypes.at(0) == RTC::RTCP::Type::PSFB);
		REQUIRE(listener.rembBitrates.at(0) > 0);

		// What went out is what is reported as available.
		REQUIRE(receiverTransportCongestionControl.GetAvailableBitrate() == listener.rembBitrates.at(0));
	}

	SECTION("the cap is announced even when transport-cc was negotiated")
	{
		constexpr int64_t MaxIncomingBitrate{ 500000 };

		TestReceiverTransportCongestionControlListener listener;

		RTC::BWE::ReceiverTransportCongestionControl receiverTransportCongestionControl(
		  std::addressof(listener),
		  std::addressof(shared),
		  { .congestionControlType = RTC::BWE::Types::CongestionControlType::TRANSPORT_CC });

		receiverTransportCongestionControl.SetMaxIncomingBitrate(MaxIncomingBitrate);

		nowUs += 10 * 1000;

		const auto packet = buildTransportCcPacket(/*wideSeqNumber*/ 0);

		receiverTransportCongestionControl.ReceiveRtpPacket(nowUs, packet.get());

		// Nothing else can tell the remote sender to hold back, since in this mode it
		// is the one estimating.
		REQUIRE(listener.rembBitrates.size() == 1);
		REQUIRE(listener.rembBitrates.at(0) == MaxIncomingBitrate);
	}

	SECTION("a cap of zero is no cap at all")
	{
		constexpr int64_t MaxIncomingBitrate{ 500000 };

		TestReceiverTransportCongestionControlListener listener;

		RTC::BWE::ReceiverTransportCongestionControl receiverTransportCongestionControl(
		  std::addressof(listener),
		  std::addressof(shared),
		  { .congestionControlType = RTC::BWE::Types::CongestionControlType::TRANSPORT_CC });

		receiverTransportCongestionControl.SetMaxIncomingBitrate(MaxIncomingBitrate);
		receiverTransportCongestionControl.SetMaxIncomingBitrate(0);

		nowUs += 10 * 1000;

		const auto packet = buildTransportCcPacket(/*wideSeqNumber*/ 0);

		receiverTransportCongestionControl.ReceiveRtpPacket(nowUs, packet.get());

		// A REMB of zero is how the remote sender is told that it may send whatever it
		// wants again.
		REQUIRE_FALSE(listener.rembBitrates.empty());
		REQUIRE(listener.rembBitrates.back() == 0);
	}
}
