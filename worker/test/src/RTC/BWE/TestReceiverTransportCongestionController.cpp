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

		REQUIRE(packet->SetExtensions(RTC::RTP::Packet::ExtensionsType::OneByte, extensions) == true);

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

		REQUIRE(packet->SetExtensions(RTC::RTP::Packet::ExtensionsType::OneByte, extensions) == true);

		RTC::RTP::HeaderExtensionIds headerExtensionIds;

		headerExtensionIds.absSendTime = AbsSendTimeId;

		packet->AssignExtensionIds(headerExtensionIds);
		packet->SetPayloadLength(PayloadSize);

		REQUIRE(packet->UpdateAbsSendTime(Utils::Time::TimeUsToAbsSendTime(sendTimeUs)));

		return packet;
	};

	SECTION("a packet carrying the transport wide sequence number is reported back and not estimated")
	{
		TestReceiverTransportCongestionControllerListener listener;

		RTC::BWE::ReceiverTransportCongestionController receiverTransportCongestionControl(
		  std::addressof(listener), std::addressof(shared), {});

		for (uint16_t wideSeqNumber{ 0 }; wideSeqNumber < 10; ++wideSeqNumber)
		{
			nowUs += 10 * 1000;

			const auto packet = buildTransportCcPacket(wideSeqNumber);

			receiverTransportCongestionControl.ReceiveRtpPacket(
			  packet.get(), RTC::Media::Kind::VIDEO, nowUs);
		}

		// Nothing goes out until the periodic timer of the feedback generator fires.
		REQUIRE(listener.rtcpTypes.empty());

		auto* timer = shared.GetTimer(FeedbackTimerLabel);

		REQUIRE(timer);

		nowUs += timer->GetRepeatMs() * 1000;

		REQUIRE(timer->EvaluateHasExpired());

		REQUIRE(listener.rtcpTypes.size() == 1);
		REQUIRE(listener.rtcpTypes.at(0) == RTC::RTCP::Type::RTPFB);

		// The estimating is the remote sender's job for such a packet.
		REQUIRE(receiverTransportCongestionControl.GetAvailableBitrate().has_value() == false);
	}

	SECTION("a packet whose Producer is already gone is still reported back")
	{
		TestReceiverTransportCongestionControllerListener listener;

		RTC::BWE::ReceiverTransportCongestionController receiverTransportCongestionControl(
		  std::addressof(listener), std::addressof(shared), {});

		for (uint16_t wideSeqNumber{ 0 }; wideSeqNumber < 10; ++wideSeqNumber)
		{
			nowUs += 10 * 1000;

			const auto packet = buildTransportCcPacket(wideSeqNumber);

			// A packet still in flight when its Producer was closed brings no media
			// kind, and leaving it out of the feedback would tell the remote sender
			// that the link lost it.
			receiverTransportCongestionControl.ReceiveRtpPacket(packet.get(), std::nullopt, nowUs);
		}

		auto* timer = shared.GetTimer(FeedbackTimerLabel);

		REQUIRE(timer);

		nowUs += timer->GetRepeatMs() * 1000;

		REQUIRE(timer->EvaluateHasExpired());

		REQUIRE(listener.rtcpTypes.size() == 1);
		REQUIRE(listener.rtcpTypes.at(0) == RTC::RTCP::Type::RTPFB);
	}

	SECTION("a packet carrying only abs-send-time is estimated here and announced as a REMB")
	{
		TestReceiverTransportCongestionControllerListener listener;

		RTC::BWE::ReceiverTransportCongestionController receiverTransportCongestionControl(
		  std::addressof(listener), std::addressof(shared), {});

		REQUIRE(receiverTransportCongestionControl.GetAvailableBitrate().has_value() == false);

		// A burst of packets spaced evenly, which is what the estimation is drawn
		// from at the beginning of a call.
		for (int idx{ 0 }; idx < Probes; ++idx)
		{
			nowUs += 10 * 1000;

			const auto packet = buildRembPacket(nowUs, Ssrc);

			receiverTransportCongestionControl.ReceiveRtpPacket(
			  packet.get(), RTC::Media::Kind::VIDEO, nowUs);
		}

		REQUIRE(listener.rtcpTypes.size() == 1);
		REQUIRE(listener.rtcpTypes.at(0) == RTC::RTCP::Type::PSFB);
		REQUIRE(listener.rembBitrates.at(0) > 0);

		// What went out is what is reported as available.
		REQUIRE(receiverTransportCongestionControl.GetAvailableBitrate() == listener.rembBitrates.at(0));
	}

	SECTION("each packet of a transport carrying both kinds of stream takes its own path")
	{
		TestReceiverTransportCongestionControllerListener listener;

		RTC::BWE::ReceiverTransportCongestionController receiverTransportCongestionControl(
		  std::addressof(listener), std::addressof(shared), {});

		// One stream negotiated transport-cc and another only abs-send-time, which
		// is what the dispatch per packet is for. They are interleaved so that
		// neither path gets a run of its own.
		for (uint16_t idx{ 0 }; idx < Probes; ++idx)
		{
			nowUs += 10 * 1000;

			// NOTE: The same packet holds both in turn, since every one of them is
			// built over the same buffer.
			auto packet = buildTransportCcPacket(idx);

			receiverTransportCongestionControl.ReceiveRtpPacket(
			  packet.get(), RTC::Media::Kind::VIDEO, nowUs);

			packet = buildRembPacket(nowUs, Ssrc2);

			receiverTransportCongestionControl.ReceiveRtpPacket(
			  packet.get(), RTC::Media::Kind::VIDEO, nowUs);
		}

		// The abs-send-time stream was estimated here and announced right away,
		// while the transport-cc one waits for the periodic feedback.
		REQUIRE(listener.rtcpTypes.size() == 1);
		REQUIRE(listener.rtcpTypes.at(0) == RTC::RTCP::Type::PSFB);

		// And only the abs-send-time stream is in the REMB, since the other one is
		// not estimated here at all.
		REQUIRE(listener.rembSsrcs.at(0) == std::vector<uint32_t>{ Ssrc2 });

		auto* timer = shared.GetTimer(FeedbackTimerLabel);

		REQUIRE(timer);

		nowUs += timer->GetRepeatMs() * 1000;

		REQUIRE(timer->EvaluateHasExpired());

		REQUIRE(listener.rtcpTypes.size() == 2);
		REQUIRE(listener.rtcpTypes.at(1) == RTC::RTCP::Type::RTPFB);
	}

	SECTION("a stream that is removed stops being announced")
	{
		TestReceiverTransportCongestionControllerListener listener;

		RTC::BWE::ReceiverTransportCongestionController receiverTransportCongestionControl(
		  std::addressof(listener), std::addressof(shared), {});

		// A burst where two streams alternate, so that the estimation applies to both.
		for (int idx{ 0 }; idx < Probes; ++idx)
		{
			nowUs += 10 * 1000;

			const auto packet = buildRembPacket(nowUs, idx % 2 == 0 ? Ssrc : Ssrc2);

			receiverTransportCongestionControl.ReceiveRtpPacket(
			  packet.get(), RTC::Media::Kind::VIDEO, nowUs);
		}

		REQUIRE(listener.rembSsrcs.size() == 1);
		REQUIRE(listener.rembSsrcs.at(0).size() == 2);

		receiverTransportCongestionControl.RemoveStream(Ssrc2);

		// Long enough for the next packet to bring another estimation out, and far
		// shorter than what it takes for a silent stream to be forgotten on its own.
		nowUs += 250 * 1000;

		const auto packet = buildRembPacket(nowUs, Ssrc);

		receiverTransportCongestionControl.ReceiveRtpPacket(packet.get(), RTC::Media::Kind::VIDEO, nowUs);

		REQUIRE(listener.rembSsrcs.size() == 2);
		REQUIRE(listener.rembSsrcs.at(1) == std::vector<uint32_t>{ Ssrc });
	}

	SECTION("an audio packet carrying only abs-send-time takes no part in the estimation")
	{
		TestReceiverTransportCongestionControllerListener listener;

		RTC::BWE::ReceiverTransportCongestionController receiverTransportCongestionControl(
		  std::addressof(listener), std::addressof(shared), {});

		// The very same burst that gives an estimation when it carries video.
		for (int idx{ 0 }; idx < Probes; ++idx)
		{
			nowUs += 10 * 1000;

			const auto packet = buildRembPacket(nowUs, Ssrc);

			receiverTransportCongestionControl.ReceiveRtpPacket(
			  packet.get(), RTC::Media::Kind::AUDIO, nowUs);
		}

		REQUIRE(listener.rtcpTypes.empty());
		REQUIRE(receiverTransportCongestionControl.GetAvailableBitrate().has_value() == false);
	}

	SECTION("an audio packet carrying the transport wide sequence number is reported like video")
	{
		TestReceiverTransportCongestionControllerListener listener;

		RTC::BWE::ReceiverTransportCongestionController receiverTransportCongestionControl(
		  std::addressof(listener), std::addressof(shared), {});

		for (uint16_t wideSeqNumber{ 0 }; wideSeqNumber < 10; ++wideSeqNumber)
		{
			nowUs += 10 * 1000;

			const auto packet = buildTransportCcPacket(wideSeqNumber);

			receiverTransportCongestionControl.ReceiveRtpPacket(
			  packet.get(), RTC::Media::Kind::AUDIO, nowUs);
		}

		auto* timer = shared.GetTimer(FeedbackTimerLabel);

		REQUIRE(timer);

		nowUs += timer->GetRepeatMs() * 1000;

		REQUIRE(timer->EvaluateHasExpired());

		REQUIRE(listener.rtcpTypes.size() == 1);
		REQUIRE(listener.rtcpTypes.at(0) == RTC::RTCP::Type::RTPFB);
	}

	SECTION("the cap is announced even for a packet that is only reported back")
	{
		constexpr int64_t MaxIncomingBitrate{ 500000 };

		TestReceiverTransportCongestionControllerListener listener;

		RTC::BWE::ReceiverTransportCongestionController receiverTransportCongestionControl(
		  std::addressof(listener), std::addressof(shared), {});

		receiverTransportCongestionControl.SetMaxIncomingBitrate(MaxIncomingBitrate);

		nowUs += 10 * 1000;

		const auto packet = buildTransportCcPacket(/*wideSeqNumber*/ 0);

		receiverTransportCongestionControl.ReceiveRtpPacket(packet.get(), RTC::Media::Kind::VIDEO, nowUs);

		// Nothing else can tell the remote sender to hold back, since in this mode it
		// is the one estimating.
		REQUIRE(listener.rembBitrates.size() == 1);
		REQUIRE(listener.rembBitrates.at(0) == MaxIncomingBitrate);
	}

	SECTION("no cap given is no cap at all")
	{
		constexpr int64_t MaxIncomingBitrate{ 500000 };

		TestReceiverTransportCongestionControllerListener listener;

		RTC::BWE::ReceiverTransportCongestionController receiverTransportCongestionControl(
		  std::addressof(listener), std::addressof(shared), {});

		receiverTransportCongestionControl.SetMaxIncomingBitrate(MaxIncomingBitrate);

		// The cap went out as soon as it was set.
		REQUIRE(listener.rembBitrates.size() == 1);
		REQUIRE(listener.rembBitrates.at(0) == MaxIncomingBitrate);

		receiverTransportCongestionControl.SetMaxIncomingBitrate(std::nullopt);

		// A REMB of zero is how the remote sender is told that it may send whatever it
		// wants again, and the first of them goes out without waiting.
		REQUIRE(listener.rembBitrates.size() == 2);
		REQUIRE(listener.rembBitrates.at(1) == 0);

		// Losing that single REMB would leave the remote sender limited forever, so the
		// rest of them ride on the incoming packets once the announcing interval is past.
		nowUs += (1500 + 1) * 1000;

		const auto packet = buildTransportCcPacket(/*wideSeqNumber*/ 0);

		receiverTransportCongestionControl.ReceiveRtpPacket(packet.get(), RTC::Media::Kind::VIDEO, nowUs);

		REQUIRE(listener.rembBitrates.size() == 3);
		REQUIRE(listener.rembBitrates.back() == 0);
	}
}
