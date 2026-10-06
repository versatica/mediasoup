#include "common.hpp"
#include "RTC/RTCP/FeedbackRtpNack.hpp"
#include "RTC/RTCP/ReceiverReport.hpp"
#include "RTC/RTCP/SenderReport.hpp"
#include "RTC/RTP/Codecs/AV1.hpp"
#include "RTC/RTP/Codecs/PayloadDescriptorHandler.hpp"
#include "RTC/RTP/Codecs/VP8.hpp"
#include "RTC/RTP/HeaderExtensionIds.hpp"
#include "RTC/RTP/Packet.hpp"
#include "RTC/RTP/RtpStream.hpp"
#include "RTC/RTP/RtpStreamSend.hpp"
#include "RTC/RTP/SharedPacket.hpp"
#include "mocks/include/MockShared.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memcpy()
#include <memory>
#include <string_view>
#include <vector>

// #define PERFORMANCE_TEST 1

#ifdef PERFORMANCE_TEST
#include <chrono>
#include <iostream>
#endif

SCENARIO("RtpStreamSend", "[rtp][rtcp][nack][rtpstream][rtpstreamsend]")
{
	constexpr std::string_view RetransmissionTimerLabel{ "rtp-stream-send-retransmissions" };

	class TestRtpStreamListener : public RTC::RTP::RtpStreamSend::Listener
	{
	public:
		void OnRtpStreamScore(
		  RTC::RTP::RtpStream* /*rtpStream*/, uint8_t /*score*/, uint8_t /*previousScore*/) override
		{
		}

		void OnRtpStreamRetransmitRtpPacket(
		  RTC::RTP::RtpStreamSend* /*rtpStream*/, RTC::RTP::Packet* packet) override
		{
			this->retransmittedPackets.push_back(packet);
		}

	public:
		std::vector<RTC::RTP::Packet*> retransmittedPackets;
	};

	const auto createRtpPacket = [](uint8_t* buffer, size_t len, uint16_t seq, uint32_t timestamp)
	{
		auto* packet = RTC::RTP::Packet::Parse(buffer, len);

		REQUIRE(packet);

		packet->SetPayloadType(123);
		packet->SetSequenceNumber(seq);
		packet->SetTimestamp(timestamp);

		return std::unique_ptr<RTC::RTP::Packet>(packet);
	};

	const auto sendRtpPacket =
	  [](
	    // NOTE: clang-tidy suggests passing `streams` by reference but that's
	    // wrong because we create `streams` in place when calling this function.
	    // NOLINTNEXTLINE(performance-unnecessary-value-param)
	    std::vector<std::pair<RTC::RTP::RtpStreamSend*, uint32_t>> streams,
	    RTC::RTP::Packet* packet)
	{
		RTC::RTP::SharedPacket sharedPacket;

		for (auto& kv : streams)
		{
			auto* stream  = kv.first;
			auto ssrc     = kv.second;
			auto origSsrc = packet->GetSsrc();

			packet->SetSsrc(ssrc);

			auto result = stream->ReceivePacket(packet, sharedPacket);

			packet->SetSsrc(origSsrc);

			// NOTE: Here we must replicate the behaviour of Consumer::sendRtpPacket()
			// in which, if the shared packet has been stored and it didn't contain the
			// packet yet, we fill it with a cloned packet.
			if (
			  result == RTC::RTP::RtpStreamSend::ReceivePacketResult::ACCEPTED_AND_STORED &&
			  !sharedPacket.HasPacket())
			{
				sharedPacket.Assign(packet);
			}
		}
	};

	const auto checkRtxPacket = [](RTC::RTP::Packet* rtxPacket, RTC::RTP::Packet* origPacket)
	{
		REQUIRE(rtxPacket);
		REQUIRE(rtxPacket->GetSequenceNumber() == origPacket->GetSequenceNumber());
		REQUIRE(rtxPacket->GetTimestamp() == origPacket->GetTimestamp());
		REQUIRE(rtxPacket->HasMarker() == origPacket->HasMarker());
	};

	const auto parseAV1RtpPacket =
	  [](
	    RTC::RTP::Packet* packet,
	    std::unique_ptr<RTC::RTP::Codecs::DependencyDescriptor::TemplateDependencyStructure>&
	      templateDependencyStructure)
	{
		std::unique_ptr<RTC::RTP::Codecs::DependencyDescriptor> dependencyDescriptor;
		packet->ReadDependencyDescriptor(dependencyDescriptor, templateDependencyStructure);
		REQUIRE(dependencyDescriptor);

		auto* payloadDescriptor = RTC::RTP::Codecs::AV1::Parse(dependencyDescriptor);
		auto* payloadDescriptorHandler =
		  new RTC::RTP::Codecs::AV1::PayloadDescriptorHandler(payloadDescriptor);
		packet->SetPayloadDescriptorHandler(payloadDescriptorHandler);
	};

	// Instant reported by the mocked clock, which sections move to let timers
	// expire.
	int64_t nowUs{ 1000 * 1000 };

	mocks::MockShared shared(/*getTimeUs*/
	                         [&nowUs]() -> int64_t
	                         {
		                         return nowUs;
	                         });

	// For sections with two streams using NACK, since the mocked Shared doesn't
	// allow two alive timers with the same label.
	mocks::MockShared shared2(/*getTimeUs*/
	                          [&nowUs]() -> int64_t
	                          {
		                          return nowUs;
	                          });

	// Move the clock to when the retransmission timer of the stream created with
	// the given Shared is due and let it expire. Answers whether it was running.
	const auto expireRetransmissionTimer =
	  [&nowUs, RetransmissionTimerLabel](mocks::MockShared& streamShared) -> bool
	{
		auto* const timer = streamShared.GetTimer(RetransmissionTimerLabel);

		REQUIRE(timer);

		if (!timer->IsActive())
		{
			return false;
		}

		nowUs = timer->GetExpiresAtMs() * 1000;

		REQUIRE(timer->EvaluateHasExpired());

		return true;
	};

	// clang-format off
	uint8_t rtpBuffer1[] =
	{
		0b10000000, 0b01111011, 0b01010010, 0b00001110,
		0b01011011, 0b01101011, 0b11001010, 0b10110101,
		0, 0, 0, 2
	};
	// clang-format on

	uint8_t rtpBuffer2[1500];
	uint8_t rtpBuffer3[1500];
	uint8_t rtpBuffer4[1500];
	uint8_t rtpBuffer5[1500];

	std::memcpy(rtpBuffer2, rtpBuffer1, sizeof(rtpBuffer1));
	std::memcpy(rtpBuffer3, rtpBuffer1, sizeof(rtpBuffer1));
	std::memcpy(rtpBuffer4, rtpBuffer1, sizeof(rtpBuffer1));
	std::memcpy(rtpBuffer5, rtpBuffer1, sizeof(rtpBuffer1));

	SECTION("receive NACK and get retransmitted packets")
	{
		// packet1 [seq:21006, timestamp:1533790901]
		auto packet1(createRtpPacket(rtpBuffer1, sizeof(rtpBuffer1), 21006, 1533790901));
		// packet2 [seq:21007, timestamp:1533790901]
		auto packet2(createRtpPacket(rtpBuffer2, sizeof(rtpBuffer2), 21007, 1533790901));
		packet2->SetMarker(true);
		// packet3 [seq:21008, timestamp:1533793871]
		auto packet3(createRtpPacket(rtpBuffer3, sizeof(rtpBuffer3), 21008, 1533793871));
		// packet4 [seq:21009, timestamp:1533793871]
		auto packet4(createRtpPacket(rtpBuffer4, sizeof(rtpBuffer4), 21009, 1533793871));
		// packet5 [seq:21010, timestamp:1533796931]
		auto packet5(createRtpPacket(rtpBuffer5, sizeof(rtpBuffer5), 21010, 1533796931));
		packet5->SetMarker(true);

		// Create a RtpStreamSend instance.
		TestRtpStreamListener testRtpStreamListener;

		RTC::RTP::RtpStream::Params params;

		params.ssrc          = 1111;
		params.clockRate     = 90000;
		params.useNack       = true;
		params.mimeType.type = RTC::RtpCodecMimeType::Type::VIDEO;

		std::string mid;
		auto stream = std::make_unique<RTC::RTP::RtpStreamSend>(
		  std::addressof(testRtpStreamListener), std::addressof(shared), params, mid);

		// Receive all the packets (some of them not in order and/or duplicated).
		sendRtpPacket(
		  {
		    { stream.get(), params.ssrc }
    },
		  packet1.get());
		sendRtpPacket(
		  {
		    { stream.get(), params.ssrc }
    },
		  packet3.get());
		sendRtpPacket(
		  {
		    { stream.get(), params.ssrc }
    },
		  packet2.get());
		sendRtpPacket(
		  {
		    { stream.get(), params.ssrc }
    },
		  packet3.get());
		sendRtpPacket(
		  {
		    { stream.get(), params.ssrc }
    },
		  packet4.get());
		sendRtpPacket(
		  {
		    { stream.get(), params.ssrc }
    },
		  packet5.get());
		sendRtpPacket(
		  {
		    { stream.get(), params.ssrc }
    },
		  packet5.get());

		// Create a NACK item that request for all the packets.
		RTC::RTCP::FeedbackRtpNackPacket nackPacket(0, params.ssrc);
		auto* nackItem = new RTC::RTCP::FeedbackRtpNackItem(21006, 0b0000000000001111);

		nackPacket.AddItem(nackItem);

		REQUIRE(nackItem->GetPacketId() == 21006);
		REQUIRE(nackItem->GetLostPacketBitmask() == 0b0000000000001111);

		stream->ReceiveNack(&nackPacket);

		// The first two requested packets are retransmitted right away.
		REQUIRE(testRtpStreamListener.retransmittedPackets.size() == 2);

		// The rest are retransmitted up to two per expiration of the timer.
		REQUIRE(expireRetransmissionTimer(shared));
		REQUIRE(testRtpStreamListener.retransmittedPackets.size() == 4);

		REQUIRE(expireRetransmissionTimer(shared));
		REQUIRE(testRtpStreamListener.retransmittedPackets.size() == 5);

		// Nothing is left so the timer is not running anymore.
		REQUIRE(expireRetransmissionTimer(shared) == false);

		auto* rtxPacket1 = testRtpStreamListener.retransmittedPackets[0];
		auto* rtxPacket2 = testRtpStreamListener.retransmittedPackets[1];
		auto* rtxPacket3 = testRtpStreamListener.retransmittedPackets[2];
		auto* rtxPacket4 = testRtpStreamListener.retransmittedPackets[3];
		auto* rtxPacket5 = testRtpStreamListener.retransmittedPackets[4];

		testRtpStreamListener.retransmittedPackets.clear();

		checkRtxPacket(rtxPacket1, packet1.get());
		checkRtxPacket(rtxPacket2, packet2.get());
		checkRtxPacket(rtxPacket3, packet3.get());
		checkRtxPacket(rtxPacket4, packet4.get());
		checkRtxPacket(rtxPacket5, packet5.get());
	}

	SECTION("receive NACK and get zero retransmitted packets if useNack is not set")
	{
		// packet1 [seq:21006, timestamp:1533790901]
		auto packet1(createRtpPacket(rtpBuffer1, sizeof(rtpBuffer1), 21006, 1533790901));
		// packet2 [seq:21007, timestamp:1533790901]
		auto packet2(createRtpPacket(rtpBuffer2, sizeof(rtpBuffer2), 21007, 1533790901));
		// packet3 [seq:21008, timestamp:1533793871]
		auto packet3(createRtpPacket(rtpBuffer3, sizeof(rtpBuffer3), 21008, 1533793871));
		// packet4 [seq:21009, timestamp:1533793871]
		auto packet4(createRtpPacket(rtpBuffer4, sizeof(rtpBuffer4), 21009, 1533793871));
		// packet5 [seq:21010, timestamp:1533796931]
		auto packet5(createRtpPacket(rtpBuffer5, sizeof(rtpBuffer5), 21010, 1533796931));

		// Create a RtpStreamSend instance.
		TestRtpStreamListener testRtpStreamListener;

		RTC::RTP::RtpStream::Params params;

		params.ssrc          = 1111;
		params.clockRate     = 90000;
		params.useNack       = false;
		params.mimeType.type = RTC::RtpCodecMimeType::Type::VIDEO;

		std::string mid;
		auto stream = std::make_unique<RTC::RTP::RtpStreamSend>(
		  std::addressof(testRtpStreamListener), std::addressof(shared), params, mid);

		// Receive all the packets (some of them not in order and/or duplicated).
		sendRtpPacket(
		  {
		    { stream.get(), params.ssrc }
    },
		  packet1.get());
		sendRtpPacket(
		  {
		    { stream.get(), params.ssrc }
    },
		  packet3.get());
		sendRtpPacket(
		  {
		    { stream.get(), params.ssrc }
    },
		  packet2.get());
		sendRtpPacket(
		  {
		    { stream.get(), params.ssrc }
    },
		  packet3.get());
		sendRtpPacket(
		  {
		    { stream.get(), params.ssrc }
    },
		  packet4.get());
		sendRtpPacket(
		  {
		    { stream.get(), params.ssrc }
    },
		  packet5.get());
		sendRtpPacket(
		  {
		    { stream.get(), params.ssrc }
    },
		  packet5.get());

		// Create a NACK item that request for all the packets.
		RTC::RTCP::FeedbackRtpNackPacket nackPacket(0, params.ssrc);
		auto* nackItem = new RTC::RTCP::FeedbackRtpNackItem(21006, 0b0000000000001111);

		nackPacket.AddItem(nackItem);

		REQUIRE(nackItem->GetPacketId() == 21006);
		REQUIRE(nackItem->GetLostPacketBitmask() == 0b0000000000001111);

		stream->ReceiveNack(&nackPacket);

		REQUIRE(testRtpStreamListener.retransmittedPackets.empty());

		// No retransmission timer is created without NACK.
		REQUIRE(shared.GetTimer(RetransmissionTimerLabel) == nullptr);

		testRtpStreamListener.retransmittedPackets.clear();
	}

	SECTION("receive NACK and get zero retransmitted packets for audio")
	{
		// packet1 [seq:21006, timestamp:1533790901]
		auto packet1(createRtpPacket(rtpBuffer1, sizeof(rtpBuffer1), 21006, 1533790901));
		// packet2 [seq:21007, timestamp:1533790901]
		auto packet2(createRtpPacket(rtpBuffer2, sizeof(rtpBuffer2), 21007, 1533790901));
		// packet3 [seq:21008, timestamp:1533793871]
		auto packet3(createRtpPacket(rtpBuffer3, sizeof(rtpBuffer3), 21008, 1533793871));
		// packet4 [seq:21009, timestamp:1533793871]
		auto packet4(createRtpPacket(rtpBuffer4, sizeof(rtpBuffer4), 21009, 1533793871));
		// packet5 [seq:21010, timestamp:1533796931]
		auto packet5(createRtpPacket(rtpBuffer5, sizeof(rtpBuffer5), 21010, 1533796931));

		// Create a RtpStreamSend instance.
		TestRtpStreamListener testRtpStreamListener;

		RTC::RTP::RtpStream::Params params;

		params.ssrc          = 1111;
		params.clockRate     = 90000;
		params.useNack       = false;
		params.mimeType.type = RTC::RtpCodecMimeType::Type::AUDIO;

		std::string mid;
		auto stream = std::make_unique<RTC::RTP::RtpStreamSend>(
		  std::addressof(testRtpStreamListener), std::addressof(shared), params, mid);

		// Receive all the packets (some of them not in order and/or duplicated).
		sendRtpPacket(
		  {
		    { stream.get(), params.ssrc }
    },
		  packet1.get());
		sendRtpPacket(
		  {
		    { stream.get(), params.ssrc }
    },
		  packet3.get());
		sendRtpPacket(
		  {
		    { stream.get(), params.ssrc }
    },
		  packet2.get());
		sendRtpPacket(
		  {
		    { stream.get(), params.ssrc }
    },
		  packet3.get());
		sendRtpPacket(
		  {
		    { stream.get(), params.ssrc }
    },
		  packet4.get());
		sendRtpPacket(
		  {
		    { stream.get(), params.ssrc }
    },
		  packet5.get());
		sendRtpPacket(
		  {
		    { stream.get(), params.ssrc }
    },
		  packet5.get());

		// Create a NACK item that request for all the packets.
		RTC::RTCP::FeedbackRtpNackPacket nackPacket(0, params.ssrc);
		auto* nackItem = new RTC::RTCP::FeedbackRtpNackItem(21006, 0b0000000000001111);

		nackPacket.AddItem(nackItem);

		REQUIRE(nackItem->GetPacketId() == 21006);
		REQUIRE(nackItem->GetLostPacketBitmask() == 0b0000000000001111);

		stream->ReceiveNack(&nackPacket);

		REQUIRE(testRtpStreamListener.retransmittedPackets.empty());

		testRtpStreamListener.retransmittedPackets.clear();
	}

	SECTION("receive NACK in different RtpStreamSend instances and get retransmitted packets")
	{
		// packet1 [seq:21006, timestamp:1533790901]
		auto packet1(createRtpPacket(rtpBuffer1, sizeof(rtpBuffer1), 21006, 1533790901));
		// packet2 [seq:21007, timestamp:1533790901]
		auto packet2(createRtpPacket(rtpBuffer2, sizeof(rtpBuffer2), 21007, 1533790901));

		// Create two RtpStreamSend instances.
		TestRtpStreamListener testRtpStreamListener1;
		TestRtpStreamListener testRtpStreamListener2;

		RTC::RTP::RtpStream::Params params1;

		params1.ssrc          = 1111;
		params1.clockRate     = 90000;
		params1.useNack       = true;
		params1.mimeType.type = RTC::RtpCodecMimeType::Type::VIDEO;

		std::string mid;
		std::unique_ptr<RTC::RTP::RtpStreamSend> stream1(new RTC::RTP::RtpStreamSend(
		  std::addressof(testRtpStreamListener1), std::addressof(shared), params1, mid));

		RTC::RTP::RtpStream::Params params2;

		params2.ssrc          = 2222;
		params2.clockRate     = 90000;
		params2.useNack       = true;
		params2.mimeType.type = RTC::RtpCodecMimeType::Type::VIDEO;

		std::unique_ptr<RTC::RTP::RtpStreamSend> stream2(new RTC::RTP::RtpStreamSend(
		  std::addressof(testRtpStreamListener2), std::addressof(shared2), params2, mid));

		// Receive all the packets in both streams.
		sendRtpPacket(
		  {
		    { stream1.get(), params1.ssrc },
        { stream2.get(), params2.ssrc }
    },
		  packet1.get());
		sendRtpPacket(
		  {
		    { stream1.get(), params1.ssrc },
        { stream2.get(), params2.ssrc }
    },
		  packet2.get());

		// Create a NACK item that request for all the packets.
		RTC::RTCP::FeedbackRtpNackPacket nackPacket(0, params1.ssrc);
		auto* nackItem = new RTC::RTCP::FeedbackRtpNackItem(21006, 0b0000000000000001);

		nackPacket.AddItem(nackItem);

		REQUIRE(nackItem->GetPacketId() == 21006);
		REQUIRE(nackItem->GetLostPacketBitmask() == 0b0000000000000001);

		// Process the NACK packet on stream1.
		stream1->ReceiveNack(&nackPacket);

		REQUIRE(testRtpStreamListener1.retransmittedPackets.size() == 2);
		REQUIRE(expireRetransmissionTimer(shared) == false);

		auto* rtxPacket1 = testRtpStreamListener1.retransmittedPackets[0];
		auto* rtxPacket2 = testRtpStreamListener1.retransmittedPackets[1];

		testRtpStreamListener1.retransmittedPackets.clear();

		checkRtxPacket(rtxPacket1, packet1.get());
		checkRtxPacket(rtxPacket2, packet2.get());

		// Process the NACK packet on stream2.
		stream2->ReceiveNack(&nackPacket);

		REQUIRE(testRtpStreamListener2.retransmittedPackets.size() == 2);
		REQUIRE(expireRetransmissionTimer(shared2) == false);

		rtxPacket1 = testRtpStreamListener2.retransmittedPackets[0];
		rtxPacket2 = testRtpStreamListener2.retransmittedPackets[1];

		testRtpStreamListener2.retransmittedPackets.clear();

		checkRtxPacket(rtxPacket1, packet1.get());
		checkRtxPacket(rtxPacket2, packet2.get());
	}

	SECTION("retransmitted packets are correctly encoded [VP8]")
	{
		// clang-format off
		uint8_t rtpBuffer1[] =
		{
			0x80, 0x7b, 0x52, 0x0e,
			0x5b, 0x6b, 0xca, 0xb5,
			0x00, 0x00, 0x00, 0x02,
			0x80, 0xe0, 0x80, 0x01,
			0xe8, 0x40, 0x7a, 0xd8
		};
		uint8_t rtpBuffer2[] =
		{
			0x80, 0x7b, 0x52, 0x0e,
			0x5b, 0x6b, 0xca, 0xb5,
			0x00, 0x00, 0x00, 0x02,
			0x80, 0xe0, 0x80, 0x02,
			0xe9, 0x40, 0x7a, 0xd8
		};
		uint8_t rtpBuffer3[] =
		{
			0x80, 0x7b, 0x52, 0x0e,
			0x5b, 0x6b, 0xca, 0xb5,
			0x00, 0x00, 0x00, 0x02,
			0x80, 0xe0, 0x80, 0x03,
			0xea, 0x40, 0x7a, 0xd8
		};
		// clang-format on

		// packet1 [seq:1, timestamp:1]
		auto packet1(createRtpPacket(rtpBuffer1, sizeof(rtpBuffer1), 1, 1));
		// packet2 [seq:2, timestamp:1]
		auto packet2(createRtpPacket(rtpBuffer2, sizeof(rtpBuffer2), 2, 1));
		// packet3 [seq:3, timestamp:1]
		auto packet3(createRtpPacket(rtpBuffer3, sizeof(rtpBuffer3), 3, 1));

		// Create two RtpStreamSend instances.
		TestRtpStreamListener testRtpStreamListener1;
		TestRtpStreamListener testRtpStreamListener2;

		RTC::RTP::RtpStream::Params params1;

		params1.ssrc          = 1111;
		params1.clockRate     = 90000;
		params1.useNack       = true;
		params1.mimeType.type = RTC::RtpCodecMimeType::Type::VIDEO;

		std::string mid;
		std::unique_ptr<RTC::RTP::RtpStreamSend> stream1(new RTC::RTP::RtpStreamSend(
		  std::addressof(testRtpStreamListener1), std::addressof(shared), params1, mid));

		RTC::RTP::RtpStream::Params params2;

		params2.ssrc          = 2222;
		params2.clockRate     = 90000;
		params2.useNack       = true;
		params2.mimeType.type = RTC::RtpCodecMimeType::Type::VIDEO;

		std::unique_ptr<RTC::RTP::RtpStreamSend> stream2(new RTC::RTP::RtpStreamSend(
		  std::addressof(testRtpStreamListener2), std::addressof(shared2), params2, mid));

		// Create two VP8 encoding contexts.
		RTC::RTP::Codecs::EncodingContext::Params params;
		params.spatialLayers  = 0;
		params.temporalLayers = 3;
		RTC::RTP::Codecs::VP8::EncodingContext context1(params);

		context1.SetCurrentTemporalLayer(3);
		context1.SetTargetTemporalLayer(3);

		RTC::RTP::Codecs::VP8::EncodingContext context2(params);

		context2.SetCurrentTemporalLayer(0);
		context2.SetTargetTemporalLayer(0);

		// Parse the first packet.
		auto* payloadDescriptor1 =
		  RTC::RTP::Codecs::VP8::Parse(packet1->GetPayload(), packet1->GetPayloadLength());
		REQUIRE(payloadDescriptor1->pictureId == 1);

		auto* payloadDescriptorHandler1 =
		  new RTC::RTP::Codecs::VP8::PayloadDescriptorHandler(payloadDescriptor1);
		packet1->SetPayloadDescriptorHandler(payloadDescriptorHandler1);

		bool marker = false;

		// Process the first packet with context1.
		auto forwarded = payloadDescriptorHandler1->Process(&context1, packet1.get(), marker);
		REQUIRE(forwarded);

		// Parse the second packet.
		auto* payloadDescriptor2 =
		  RTC::RTP::Codecs::VP8::Parse(packet2->GetPayload(), packet2->GetPayloadLength());
		REQUIRE(payloadDescriptor2->pictureId == 2);

		auto* payloadDescriptorHandler2 =
		  new RTC::RTP::Codecs::VP8::PayloadDescriptorHandler(payloadDescriptor2);
		packet2->SetPayloadDescriptorHandler(payloadDescriptorHandler2);

		// Process the second packet with context1.
		forwarded = payloadDescriptorHandler2->Process(&context1, packet2.get(), marker);
		REQUIRE(forwarded);

		// Process the second packet for context2.
		forwarded = payloadDescriptorHandler2->Process(&context2, packet2.get(), marker);
		// It must not forwared because the target temporal layer is 0.
		REQUIRE(!forwarded);

		// Parse the third packet
		auto* payloadDescriptor3 =
		  RTC::RTP::Codecs::VP8::Parse(packet3->GetPayload(), packet3->GetPayloadLength());
		REQUIRE(payloadDescriptor3->pictureId == 3);

		auto* payloadDescriptorHandler3 =
		  new RTC::RTP::Codecs::VP8::PayloadDescriptorHandler(payloadDescriptor3);
		packet2->SetPayloadDescriptorHandler(payloadDescriptorHandler3);

		// Process the third packet for context1.
		forwarded = payloadDescriptorHandler3->Process(&context1, packet3.get(), marker);
		REQUIRE(forwarded);

		// Receive the third packet in the first stream.
		sendRtpPacket(
		  {
		    { stream1.get(), params1.ssrc }
    },
		  packet3.get());

		// Update current/target temporal layers for context2.
		context2.SetCurrentTemporalLayer(3);
		context2.SetTargetTemporalLayer(3);

		forwarded = payloadDescriptorHandler3->Process(&context2, packet3.get(), marker);
		REQUIRE(forwarded);

		// Receive the third packet in the second stream.
		sendRtpPacket(
		  {
		    { stream2.get(), params2.ssrc }
    },
		  packet3.get());

		// Create a NACK item that requests the third packet.
		RTC::RTCP::FeedbackRtpNackPacket nackPacket(0, params1.ssrc);
		auto* nackItem = new RTC::RTCP::FeedbackRtpNackItem(3, 0b0000000000000000);

		nackPacket.AddItem(nackItem);

		REQUIRE(nackItem->GetPacketId() == 3);
		REQUIRE(nackItem->GetLostPacketBitmask() == 0b0000000000000000);

		// Process the NACK packet on stream1.
		stream1->ReceiveNack(&nackPacket);

		REQUIRE(testRtpStreamListener1.retransmittedPackets.size() == 1);

		auto* packet = testRtpStreamListener1.retransmittedPackets[0];

		// Parse payload and check pictureId.
		auto* payloadDescriptor4 =
		  RTC::RTP::Codecs::VP8::Parse(packet->GetPayload(), packet->GetPayloadLength());
		REQUIRE(payloadDescriptor4->pictureId == 3);

		// Process the NACK packet on stream2.
		stream2->ReceiveNack(&nackPacket);

		REQUIRE(testRtpStreamListener2.retransmittedPackets.size() == 1);

		packet = testRtpStreamListener2.retransmittedPackets[0];

		// Parse payload and check pictureId.
		auto* payloadDescriptor5 =
		  RTC::RTP::Codecs::VP8::Parse(packet->GetPayload(), packet->GetPayloadLength());
		REQUIRE(payloadDescriptor5);
		REQUIRE(payloadDescriptor5->pictureId == 2);

		delete payloadDescriptor4;
		delete payloadDescriptor5;
	}

	SECTION("retransmitted packets are correctly encoded [AV1]")
	{
		/*
		 * <DependencyDescriptor>
		 *	 startOfFrame: true
		 *	 endOfFrame: false
		 *	 frameDependencyTemplateId: 0
		 *	 frameNumber: 1
		 *	 templateId: 0
		 *	 spatialLayer: 0
		 *	 temporalLayer: 0
		 *	 <TemplateDependencyStructure>
		 *	 spatialLayers: 0
		 *	 temporalLayers: 1
		 *	 templateIdOffset: 0
		 *	 decodeTargetCount: 2
		 *	 <TemplateLayers>
		 *    <FrameDependencyTemplate>
		 *      spatialLayerId: 0
		 *      temporalLayerId: 0
		 *      <DecodeTargetIndications> SS </DecodeTargetIndications>
		 *      <FrameDiffs>  </FrameDiffs>
		 *      <FrameDiffChains> 0 </FrameDiffChains>
		 *    <FrameDependencyTemplate>
		 *    <FrameDependencyTemplate>
		 *      spatialLayerId: 0
		 *      temporalLayerId: 0
		 *      <DecodeTargetIndications> SS </DecodeTargetIndications>
		 *      <FrameDiffs> 2 </FrameDiffs>
		 *      <FrameDiffChains> 2 </FrameDiffChains>
		 *    </FrameDependencyTemplate>
		 *    <FrameDependencyTemplate>
		 *      spatialLayerId: 0
		 *      temporalLayerId: 1
		 *      <DecodeTargetIndications> -D </DecodeTargetIndications>
		 *      <FrameDiffs> 1 </FrameDiffs>
		 *      <FrameDiffChains> 1 </FrameDiffChains>
		 *    </FrameDependencyTemplate>
		 *    <FrameDependencyTemplate>
		 *      spatialLayerId: 0
		 *      temporalLayerId: 1
		 *      <DecodeTargetIndications> -D </DecodeTargetIndications>
		 *      <FrameDiffs> 1 </FrameDiffs>
		 *      <FrameDiffChains> 1 </FrameDiffChains>
		 *    </FrameDependencyTemplate>
		 *	 </TemplateLayers>
		 *	 </TemplateDependencyStructure>
		 *	</DependencyDescriptor>
		 */
		// clang-format off
		uint8_t rtpBuffer1[] =
		{
			0x90, 0x2D, 0x56, 0xA5,
			0x8D, 0x76, 0xF5, 0x02,
			0xDD, 0xD5, 0x4C, 0xB9,
			0xBE, 0xDE, 0x00, 0x07,
			0x22, 0x89, 0xDF, 0xFE,
			0x31, 0x00, 0x07, 0x40,
			0x31, 0xCE, 0x80, 0x00,
			0x01, 0x80, 0x01, 0x1E,
			0xA8, 0x51, 0x41, 0x01,
			0x0C, 0x13, 0xFC, 0x0B,
			0x3C, 0x00, 0x00, 0x00
		};

		/*
		 * <DependencyDescriptor>
		 * 	 startOfFrame: true
		 * 	 endOfFrame: true
		 * 	 frameDependencyTemplateId: 2
		 * 	 frameNumber: 2
		 * 	 templateId: 2
		 * 	 temporalLayer: 1
		 * 	 spatialLayer: 0
		 * 	</DependencyDescriptor>
		 */
		uint8_t rtpBuffer2[] =
		{
			0x90, 0xAD, 0x56, 0xA9,
			0x8D, 0x77, 0x02, 0xB8,
			0xDD, 0xD5, 0x4C, 0xB9,
			0xBE, 0xDE, 0x00, 0x04,
			0x22, 0x8A, 0x07, 0xAB,
			0x31, 0x00, 0x18, 0x40,
			0x31, 0xC2, 0xC2, 0x00,
			0x02, 0x00, 0x00, 0x00
		};

		/*
		 * <DependencyDescriptor>
		 * 	 startOfFrame: false
		 * 	 endOfFrame: true
		 * 	 frameDependencyTemplateId: 0
		 * 	 frameNumber: 1
		 * 	 templateId: 0
		 * 	 spatialLayer: 0
		 * 	 temporalLayer: 0
		 * 	</DependencyDescriptor>
		 */
		uint8_t rtpBuffer3[] =
		{
			0x90, 0xAD, 0x56, 0xA8,
			0x8D, 0x76, 0xF5, 0x02,
			0xDD, 0xD5, 0x4C, 0xB9,
			0xBE, 0xDE, 0x00, 0x04,
			0x22, 0x8A, 0x03, 0xE5,
			0xD0, 0x00, 0x31, 0x00,
			0x17, 0xC2, 0x40, 0x00,
			0x01, 0x40, 0x31, 0x00
		};
		// clang-format on

		RTC::RTP::HeaderExtensionIds headerExtensionIds{};

		headerExtensionIds.dependencyDescriptor = 12;

		// packet1 [seq:1, timestamp:1]
		auto packet1(createRtpPacket(rtpBuffer1, sizeof(rtpBuffer1), 1, 1));
		packet1->AssignExtensionIds(headerExtensionIds);

		// packet2 [seq:2, timestamp:1]
		auto packet2(createRtpPacket(rtpBuffer2, sizeof(rtpBuffer2), 2, 1));
		packet2->AssignExtensionIds(headerExtensionIds);

		// packet3 [seq:3, timestamp:1]
		auto packet3(createRtpPacket(rtpBuffer3, sizeof(rtpBuffer3), 3, 1));
		packet3->AssignExtensionIds(headerExtensionIds);

		// Create two RtpStreamSend instances.
		TestRtpStreamListener testRtpStreamListener1;
		TestRtpStreamListener testRtpStreamListener2;

		RTC::RTP::RtpStream::Params params1;

		params1.ssrc          = 1111;
		params1.clockRate     = 90000;
		params1.useNack       = true;
		params1.mimeType.type = RTC::RtpCodecMimeType::Type::VIDEO;

		std::string mid;
		std::unique_ptr<RTC::RTP::RtpStreamSend> stream1(new RTC::RTP::RtpStreamSend(
		  std::addressof(testRtpStreamListener1), std::addressof(shared), params1, mid));

		RTC::RTP::RtpStream::Params params2;

		params2.ssrc          = 2222;
		params2.clockRate     = 90000;
		params2.useNack       = true;
		params2.mimeType.type = RTC::RtpCodecMimeType::Type::VIDEO;

		std::unique_ptr<RTC::RTP::RtpStreamSend> stream2(new RTC::RTP::RtpStreamSend(
		  std::addressof(testRtpStreamListener2), std::addressof(shared2), params2, mid));

		// Create two AV1 encoding contexts.
		RTC::RTP::Codecs::EncodingContext::Params params;
		params.spatialLayers  = 1;
		params.temporalLayers = 2;

		RTC::RTP::Codecs::AV1::EncodingContext context1(params);
		context1.SetCurrentSpatialLayer(0);
		context1.SetCurrentTemporalLayer(0);
		context1.SetTargetSpatialLayer(0);
		context1.SetTargetTemporalLayer(0);

		RTC::RTP::Codecs::AV1::EncodingContext context2(params);
		context2.SetCurrentSpatialLayer(0);
		context2.SetCurrentTemporalLayer(0);
		context2.SetTargetSpatialLayer(0);
		context2.SetTargetTemporalLayer(1);

		std::unique_ptr<RTC::RTP::Codecs::DependencyDescriptor::TemplateDependencyStructure>
		  templateDependencyStructure;

		// Parse the first packet for the shake of having the template dependency structure.
		parseAV1RtpPacket(packet1.get(), templateDependencyStructure);
		// Parse the second packet.
		parseAV1RtpPacket(packet2.get(), templateDependencyStructure);

		bool marker    = false;
		bool forwarded = false;

		// Process the second packet for context1.
		forwarded = packet2->ProcessPayload(&context1, marker);
		REQUIRE(!forwarded);

		// Process the second packet with context2.
		forwarded = packet2->ProcessPayload(&context2, marker);
		REQUIRE(forwarded);
		REQUIRE(context2.GetCurrentSpatialLayer() == 0);
		REQUIRE(context2.GetCurrentTemporalLayer() == 1);

		// Parse the third packet
		parseAV1RtpPacket(packet3.get(), templateDependencyStructure);

		// Process the third packet with context1 and verify current spatial layers.
		forwarded = packet3->ProcessPayload(&context1, marker);
		REQUIRE(forwarded);
		REQUIRE(context1.GetCurrentSpatialLayer() == 0);
		REQUIRE(context1.GetCurrentTemporalLayer() == 0);

		RTC::RTP::SharedPacket sharedPacket;

		packet3->SetSsrc(params1.ssrc);
		// Whenever packet3 is Nacked on stream1, it must always be set a
		// 00000001 (S0_T1) active decode target bitmas.
		auto result = stream1->ReceivePacket(packet3.get(), sharedPacket);

		REQUIRE(result == RTC::RTP::RtpStreamSend::ReceivePacketResult::ACCEPTED_AND_STORED);
		sharedPacket.Assign(packet3.get());

		// Process the third packet with context2 and verify current spatial layers.
		forwarded = packet3->ProcessPayload(&context2, marker);
		REQUIRE(forwarded);
		REQUIRE(context2.GetCurrentSpatialLayer() == 0);
		REQUIRE(context2.GetCurrentTemporalLayer() == 1);

		packet3->SetSsrc(params2.ssrc);

		// Whenever packet3 is Nacked on stream2, it must always be set a
		// 00000011 (S0_T1) active decode target bitmas.
		stream2->ReceivePacket(packet3.get(), sharedPacket);

		// Create a NACK item that requests the third packet.
		RTC::RTCP::FeedbackRtpNackPacket nackPacket(0, params1.ssrc);
		auto* nackItem = new RTC::RTCP::FeedbackRtpNackItem(3, 0b0000000000000000);

		nackPacket.AddItem(nackItem);

		REQUIRE(nackItem->GetPacketId() == 3);
		REQUIRE(nackItem->GetLostPacketBitmask() == 0b0000000000000000);

		// Process the NACK packet on stream1.
		stream1->ReceiveNack(&nackPacket);

		REQUIRE(testRtpStreamListener1.retransmittedPackets.size() == 1);

		auto* packet = testRtpStreamListener1.retransmittedPackets[0];

		// Parse DD and check bitmask.
		std::unique_ptr<RTC::RTP::Codecs::DependencyDescriptor> dependencyDescriptor4;

		packet->ReadDependencyDescriptor(dependencyDescriptor4, templateDependencyStructure);
		REQUIRE(dependencyDescriptor4);
		// TODO: Enable once we write DD.
		// REQUIRE(dependencyDescriptor4->activeDecodeTargetsBitmask == 0b0000000000000001);

		// Process the NACK packet on stream2.
		stream2->ReceiveNack(&nackPacket);

		REQUIRE(testRtpStreamListener2.retransmittedPackets.size() == 1);

		packet = testRtpStreamListener2.retransmittedPackets[0];

		// Parse DD and check bitmask.
		std::unique_ptr<RTC::RTP::Codecs::DependencyDescriptor> dependencyDescriptor5;

		packet->ReadDependencyDescriptor(dependencyDescriptor5, templateDependencyStructure);
		REQUIRE(dependencyDescriptor5);
		// TODO: Enable once we write DD.
		// REQUIRE(dependencyDescriptor5->activeDecodeTargetsBitmask == 0b0000000000000011);
	}

	SECTION("packets get retransmitted as long as they don't exceed MaxRetransmissionDelayForVideoMs")
	{
		const uint32_t clockRate = 90000;
		const uint32_t firstTs   = 1533790901;
		const uint32_t diffTs =
		  RTC::RTP::RtpStreamSend::MaxRetransmissionDelayForVideoMs * clockRate / 1000;
		const uint32_t secondTs = firstTs + diffTs;

		auto packet1(createRtpPacket(rtpBuffer1, sizeof(rtpBuffer1), 21006, firstTs));
		auto packet2(createRtpPacket(rtpBuffer2, sizeof(rtpBuffer2), 21007, secondTs - 1));

		// Create a RtpStreamSend instance.
		TestRtpStreamListener testRtpStreamListener;

		RTC::RTP::RtpStream::Params params1;

		params1.ssrc          = 1111;
		params1.clockRate     = clockRate;
		params1.useNack       = true;
		params1.mimeType.type = RTC::RtpCodecMimeType::Type::VIDEO;

		std::string mid;
		auto stream = std::make_unique<RTC::RTP::RtpStreamSend>(
		  std::addressof(testRtpStreamListener), std::addressof(shared), params1, mid);

		// Receive all the packets.
		sendRtpPacket(
		  {
		    { stream.get(), params1.ssrc }
    },
		  packet1.get());
		sendRtpPacket(
		  {
		    { stream.get(), params1.ssrc }
    },
		  packet2.get());

		// Create a NACK item that request for all the packets.
		RTC::RTCP::FeedbackRtpNackPacket nackPacket(0, params1.ssrc);
		auto* nackItem = new RTC::RTCP::FeedbackRtpNackItem(21006, 0b0000000000000001);

		nackPacket.AddItem(nackItem);

		REQUIRE(nackItem->GetPacketId() == 21006);
		REQUIRE(nackItem->GetLostPacketBitmask() == 0b0000000000000001);

		// Process the NACK packet on stream1.
		stream->ReceiveNack(&nackPacket);

		REQUIRE(testRtpStreamListener.retransmittedPackets.size() == 2);
		REQUIRE(expireRetransmissionTimer(shared) == false);

		auto* rtxPacket1 = testRtpStreamListener.retransmittedPackets[0];
		auto* rtxPacket2 = testRtpStreamListener.retransmittedPackets[1];

		testRtpStreamListener.retransmittedPackets.clear();

		checkRtxPacket(rtxPacket1, packet1.get());
		checkRtxPacket(rtxPacket2, packet2.get());
	}

	SECTION("packets don't get retransmitted if MaxRetransmissionDelayForVideoMs is exceeded")
	{
		const uint32_t clockRate = 90000;
		const uint32_t firstTs   = 1533790901;
		const uint32_t diffTs =
		  RTC::RTP::RtpStreamSend::MaxRetransmissionDelayForVideoMs * clockRate / 1000;
		// Make second packet arrive more than MaxRetransmissionDelayForVideoMs later.
		const uint32_t secondTs = firstTs + diffTs + 100;
		// Send a third packet so it will clean old packets from the buffer.
		const uint32_t thirdTs = firstTs + (2 * diffTs);

		auto packet1(createRtpPacket(rtpBuffer1, sizeof(rtpBuffer1), 21006, firstTs));
		auto packet2(createRtpPacket(rtpBuffer2, sizeof(rtpBuffer2), 21007, secondTs));
		auto packet3(createRtpPacket(rtpBuffer3, sizeof(rtpBuffer3), 21008, thirdTs));

		// Create a RtpStreamSend instance.
		TestRtpStreamListener testRtpStreamListener;

		RTC::RTP::RtpStream::Params params1;

		params1.ssrc          = 1111;
		params1.clockRate     = clockRate;
		params1.useNack       = true;
		params1.mimeType.type = RTC::RtpCodecMimeType::Type::VIDEO;

		std::string mid;
		auto stream = std::make_unique<RTC::RTP::RtpStreamSend>(
		  std::addressof(testRtpStreamListener), std::addressof(shared), params1, mid);

		// Receive all the packets.
		sendRtpPacket(
		  {
		    { stream.get(), params1.ssrc }
    },
		  packet1.get());
		sendRtpPacket(
		  {
		    { stream.get(), params1.ssrc }
    },
		  packet2.get());
		sendRtpPacket(
		  {
		    { stream.get(), params1.ssrc }
    },
		  packet3.get());

		// Create a NACK item that requests for all packets.
		RTC::RTCP::FeedbackRtpNackPacket nackPacket(0, params1.ssrc);
		auto* nackItem = new RTC::RTCP::FeedbackRtpNackItem(21006, 0b0000000000000001);

		nackPacket.AddItem(nackItem);

		REQUIRE(nackItem->GetPacketId() == 21006);
		REQUIRE(nackItem->GetLostPacketBitmask() == 0b0000000000000001);

		// Process the NACK packet on stream1.
		stream->ReceiveNack(&nackPacket);

		// The first requested packet is not stored anymore, which doesn't prevent
		// the second one from being retransmitted right away, so nothing is left.
		REQUIRE(testRtpStreamListener.retransmittedPackets.size() == 1);
		REQUIRE(expireRetransmissionTimer(shared) == false);

		auto* rtxPacket2 = testRtpStreamListener.retransmittedPackets[0];

		testRtpStreamListener.retransmittedPackets.clear();

		checkRtxPacket(rtxPacket2, packet2.get());
	}

	SECTION("packets get removed from the retransmission buffer if seq number of the stream is reset")
	{
		// This scenario reproduce the "too bad sequence number" and "bad sequence
		// number" scenarios in RtpStream::UpdateSeq().
		auto packet1(createRtpPacket(rtpBuffer1, sizeof(rtpBuffer1), 50001, 1000001));
		auto packet2(createRtpPacket(rtpBuffer2, sizeof(rtpBuffer2), 50002, 1000002));
		// Third packet has bad sequence number (its seq is more than MaxDropout=3000
		// older than current max seq) and will be dropped.
		auto packet3(createRtpPacket(rtpBuffer3, sizeof(rtpBuffer3), 40003, 1000003));
		// Forth packet has seq=badSeq+1 so will be accepted and will trigger a
		// stream reset.
		auto packet4(createRtpPacket(rtpBuffer4, sizeof(rtpBuffer4), 40004, 1000004));

		// Create a RtpStreamSend instance.
		TestRtpStreamListener testRtpStreamListener;

		RTC::RTP::RtpStream::Params params1;

		params1.ssrc          = 1111;
		params1.clockRate     = 90000;
		params1.useNack       = true;
		params1.mimeType.type = RTC::RtpCodecMimeType::Type::VIDEO;

		std::string mid;
		auto stream = std::make_unique<RTC::RTP::RtpStreamSend>(
		  std::addressof(testRtpStreamListener), std::addressof(shared), params1, mid);

		sendRtpPacket(
		  {
		    { stream.get(), params1.ssrc }
    },
		  packet1.get());
		sendRtpPacket(
		  {
		    { stream.get(), params1.ssrc }
    },
		  packet2.get());
		sendRtpPacket(
		  {
		    { stream.get(), params1.ssrc }
    },
		  packet3.get());
		sendRtpPacket(
		  {
		    { stream.get(), params1.ssrc }
    },
		  packet4.get());

		// Create a NACK item that requests for packets 1 and 2.
		RTC::RTCP::FeedbackRtpNackPacket nackPacket2(0, params1.ssrc);
		auto* nackItem2 = new RTC::RTCP::FeedbackRtpNackItem(50001, 0b0000000000000001);

		nackPacket2.AddItem(nackItem2);

		// Process the NACK packet on stream1.
		stream->ReceiveNack(&nackPacket2);

		REQUIRE(testRtpStreamListener.retransmittedPackets.empty());
	}

	SECTION("packets stored too long ago don't get retransmitted even if no newer packet arrived")
	{
		const auto packet1(createRtpPacket(rtpBuffer1, sizeof(rtpBuffer1), 21006, 1533790901));
		const auto packet2(createRtpPacket(rtpBuffer2, sizeof(rtpBuffer2), 21007, 1533790901));

		// Create a RtpStreamSend instance.
		TestRtpStreamListener testRtpStreamListener;

		RTC::RTP::RtpStream::Params params;

		params.ssrc          = 1111;
		params.clockRate     = 90000;
		params.useNack       = true;
		params.mimeType.type = RTC::RtpCodecMimeType::Type::VIDEO;

		std::string mid;
		auto stream = std::make_unique<RTC::RTP::RtpStreamSend>(
		  std::addressof(testRtpStreamListener), std::addressof(shared), params, mid);

		sendRtpPacket(
		  {
		    { stream.get(), params.ssrc }
    },
		  packet1.get());

		// The second packet is stored 1 ms later.
		nowUs += 1000;

		sendRtpPacket(
		  {
		    { stream.get(), params.ssrc }
    },
		  packet2.get());

		// Move the clock so the first packet has been stored for longer than
		// MaxRetransmissionDelayForVideoMs and the second one exactly for that.
		nowUs += RTC::RTP::RtpStreamSend::MaxRetransmissionDelayForVideoMs * 1000;

		// Create a NACK item that requests for both packets.
		RTC::RTCP::FeedbackRtpNackPacket nackPacket(0, params.ssrc);

		auto* const nackItem = new RTC::RTCP::FeedbackRtpNackItem(21006, 0b0000000000000001);

		nackPacket.AddItem(nackItem);

		stream->ReceiveNack(&nackPacket);

		// Only the second packet is retransmitted.
		REQUIRE(testRtpStreamListener.retransmittedPackets.size() == 1);
		REQUIRE(expireRetransmissionTimer(shared) == false);

		checkRtxPacket(testRtpStreamListener.retransmittedPackets[0], packet2.get());
	}

	SECTION("pending retransmissions not retransmitted don't count towards the limit per iteration")
	{
		const auto packet1(createRtpPacket(rtpBuffer1, sizeof(rtpBuffer1), 21006, 1533790901));
		// No packet with seq 21007 is ever stored.
		const auto packet3(createRtpPacket(rtpBuffer3, sizeof(rtpBuffer3), 21008, 1533790901));
		const auto packet4(createRtpPacket(rtpBuffer4, sizeof(rtpBuffer4), 21009, 1533790901));

		// Create a RtpStreamSend instance.
		TestRtpStreamListener testRtpStreamListener;

		RTC::RTP::RtpStream::Params params;

		params.ssrc          = 1111;
		params.clockRate     = 90000;
		params.useNack       = true;
		params.mimeType.type = RTC::RtpCodecMimeType::Type::VIDEO;

		std::string mid;
		auto stream = std::make_unique<RTC::RTP::RtpStreamSend>(
		  std::addressof(testRtpStreamListener), std::addressof(shared), params, mid);

		sendRtpPacket(
		  {
		    { stream.get(), params.ssrc }
    },
		  packet1.get());
		sendRtpPacket(
		  {
		    { stream.get(), params.ssrc }
    },
		  packet3.get());
		sendRtpPacket(
		  {
		    { stream.get(), params.ssrc }
    },
		  packet4.get());

		// Create a NACK item that requests for seqs 21006 to 21009.
		RTC::RTCP::FeedbackRtpNackPacket nackPacket(0, params.ssrc);

		auto* const nackItem = new RTC::RTCP::FeedbackRtpNackItem(21006, 0b0000000000000111);

		nackPacket.AddItem(nackItem);

		stream->ReceiveNack(&nackPacket);

		// The missing 21007 doesn't count, so two packets are retransmitted right
		// away and the last one remains pending.
		REQUIRE(testRtpStreamListener.retransmittedPackets.size() == 2);

		REQUIRE(expireRetransmissionTimer(shared));
		REQUIRE(testRtpStreamListener.retransmittedPackets.size() == 3);

		REQUIRE(expireRetransmissionTimer(shared) == false);

		checkRtxPacket(testRtpStreamListener.retransmittedPackets[0], packet1.get());
		checkRtxPacket(testRtpStreamListener.retransmittedPackets[1], packet3.get());
		checkRtxPacket(testRtpStreamListener.retransmittedPackets[2], packet4.get());
	}

	SECTION("packet requested again while pending is not queued twice")
	{
		const auto packet1(createRtpPacket(rtpBuffer1, sizeof(rtpBuffer1), 21006, 1533790901));
		const auto packet2(createRtpPacket(rtpBuffer2, sizeof(rtpBuffer2), 21007, 1533790901));
		const auto packet3(createRtpPacket(rtpBuffer3, sizeof(rtpBuffer3), 21008, 1533790901));
		const auto packet4(createRtpPacket(rtpBuffer4, sizeof(rtpBuffer4), 21009, 1533790901));

		// Create a RtpStreamSend instance.
		TestRtpStreamListener testRtpStreamListener;

		RTC::RTP::RtpStream::Params params;

		params.ssrc          = 1111;
		params.clockRate     = 90000;
		params.useNack       = true;
		params.mimeType.type = RTC::RtpCodecMimeType::Type::VIDEO;

		std::string mid;
		auto stream = std::make_unique<RTC::RTP::RtpStreamSend>(
		  std::addressof(testRtpStreamListener), std::addressof(shared), params, mid);

		sendRtpPacket(
		  {
		    { stream.get(), params.ssrc }
    },
		  packet1.get());
		sendRtpPacket(
		  {
		    { stream.get(), params.ssrc }
    },
		  packet2.get());
		sendRtpPacket(
		  {
		    { stream.get(), params.ssrc }
    },
		  packet3.get());
		sendRtpPacket(
		  {
		    { stream.get(), params.ssrc }
    },
		  packet4.get());

		// Create a NACK item that requests for all the packets.
		RTC::RTCP::FeedbackRtpNackPacket nackPacket1(0, params.ssrc);

		auto* const nackItem1 = new RTC::RTCP::FeedbackRtpNackItem(21006, 0b0000000000000111);

		nackPacket1.AddItem(nackItem1);

		stream->ReceiveNack(&nackPacket1);

		// The first two packets are retransmitted right away and the other two
		// remain pending.
		REQUIRE(testRtpStreamListener.retransmittedPackets.size() == 2);

		// Create a NACK item that requests for the third packet, still pending.
		RTC::RTCP::FeedbackRtpNackPacket nackPacket2(0, params.ssrc);

		auto* const nackItem2 = new RTC::RTCP::FeedbackRtpNackItem(21008, 0b0000000000000000);

		nackPacket2.AddItem(nackItem2);

		stream->ReceiveNack(&nackPacket2);

		// Nothing is retransmitted right away since an iteration is scheduled.
		REQUIRE(testRtpStreamListener.retransmittedPackets.size() == 2);

		REQUIRE(expireRetransmissionTimer(shared));
		REQUIRE(testRtpStreamListener.retransmittedPackets.size() == 4);

		// The second request of the third packet was not queued, so nothing is
		// left and the timer is not running anymore.
		REQUIRE(expireRetransmissionTimer(shared) == false);
		REQUIRE(testRtpStreamListener.retransmittedPackets.size() == 4);

		checkRtxPacket(testRtpStreamListener.retransmittedPackets[0], packet1.get());
		checkRtxPacket(testRtpStreamListener.retransmittedPackets[1], packet2.get());
		checkRtxPacket(testRtpStreamListener.retransmittedPackets[2], packet3.get());
		checkRtxPacket(testRtpStreamListener.retransmittedPackets[3], packet4.get());
	}

	SECTION("pausing the stream discards pending retransmissions")
	{
		const auto packet1(createRtpPacket(rtpBuffer1, sizeof(rtpBuffer1), 21006, 1533790901));
		const auto packet2(createRtpPacket(rtpBuffer2, sizeof(rtpBuffer2), 21007, 1533790901));
		const auto packet3(createRtpPacket(rtpBuffer3, sizeof(rtpBuffer3), 21008, 1533790901));

		// Create a RtpStreamSend instance.
		TestRtpStreamListener testRtpStreamListener;

		RTC::RTP::RtpStream::Params params;

		params.ssrc          = 1111;
		params.clockRate     = 90000;
		params.useNack       = true;
		params.mimeType.type = RTC::RtpCodecMimeType::Type::VIDEO;

		std::string mid;
		auto stream = std::make_unique<RTC::RTP::RtpStreamSend>(
		  std::addressof(testRtpStreamListener), std::addressof(shared), params, mid);

		sendRtpPacket(
		  {
		    { stream.get(), params.ssrc }
    },
		  packet1.get());
		sendRtpPacket(
		  {
		    { stream.get(), params.ssrc }
    },
		  packet2.get());
		sendRtpPacket(
		  {
		    { stream.get(), params.ssrc }
    },
		  packet3.get());

		// Create a NACK item that requests for all the packets.
		RTC::RTCP::FeedbackRtpNackPacket nackPacket(0, params.ssrc);

		auto* const nackItem = new RTC::RTCP::FeedbackRtpNackItem(21006, 0b0000000000000011);

		nackPacket.AddItem(nackItem);

		stream->ReceiveNack(&nackPacket);

		REQUIRE(testRtpStreamListener.retransmittedPackets.size() == 2);

		stream->Pause();

		// The timer is not running anymore and the third packet is never
		// retransmitted.
		REQUIRE(expireRetransmissionTimer(shared) == false);
		REQUIRE(testRtpStreamListener.retransmittedPackets.size() == 2);
	}

	SECTION("resetting the seq number of the stream discards pending retransmissions")
	{
		const auto packet1(createRtpPacket(rtpBuffer1, sizeof(rtpBuffer1), 50001, 1000001));
		const auto packet2(createRtpPacket(rtpBuffer2, sizeof(rtpBuffer2), 50002, 1000002));
		const auto packet3(createRtpPacket(rtpBuffer3, sizeof(rtpBuffer3), 50003, 1000003));
		// Fourth packet has bad sequence number (its seq is more than MaxDropout=3000
		// older than current max seq) and will be dropped.
		const auto packet4(createRtpPacket(rtpBuffer4, sizeof(rtpBuffer4), 40004, 1000004));
		// Fifth packet has seq=badSeq+1 so will be accepted and will trigger a
		// stream reset.
		const auto packet5(createRtpPacket(rtpBuffer5, sizeof(rtpBuffer5), 40005, 1000005));

		// Create a RtpStreamSend instance.
		TestRtpStreamListener testRtpStreamListener;

		RTC::RTP::RtpStream::Params params;

		params.ssrc          = 1111;
		params.clockRate     = 90000;
		params.useNack       = true;
		params.mimeType.type = RTC::RtpCodecMimeType::Type::VIDEO;

		std::string mid;
		auto stream = std::make_unique<RTC::RTP::RtpStreamSend>(
		  std::addressof(testRtpStreamListener), std::addressof(shared), params, mid);

		sendRtpPacket(
		  {
		    { stream.get(), params.ssrc }
    },
		  packet1.get());
		sendRtpPacket(
		  {
		    { stream.get(), params.ssrc }
    },
		  packet2.get());
		sendRtpPacket(
		  {
		    { stream.get(), params.ssrc }
    },
		  packet3.get());

		// Create a NACK item that requests for the first three packets.
		RTC::RTCP::FeedbackRtpNackPacket nackPacket(0, params.ssrc);

		auto* const nackItem = new RTC::RTCP::FeedbackRtpNackItem(50001, 0b0000000000000011);

		nackPacket.AddItem(nackItem);

		stream->ReceiveNack(&nackPacket);

		REQUIRE(testRtpStreamListener.retransmittedPackets.size() == 2);

		// Reset the seq number of the stream while the third one is pending.
		sendRtpPacket(
		  {
		    { stream.get(), params.ssrc }
    },
		  packet4.get());
		sendRtpPacket(
		  {
		    { stream.get(), params.ssrc }
    },
		  packet5.get());

		// The timer is not running anymore and the pending packet is never
		// retransmitted.
		REQUIRE(expireRetransmissionTimer(shared) == false);
		REQUIRE(testRtpStreamListener.retransmittedPackets.size() == 2);
	}

	SECTION("packets requested by several NACK items are queued in order and only once")
	{
		const auto packet1(createRtpPacket(rtpBuffer1, sizeof(rtpBuffer1), 21006, 1533790901));
		const auto packet2(createRtpPacket(rtpBuffer2, sizeof(rtpBuffer2), 21007, 1533790901));
		const auto packet3(createRtpPacket(rtpBuffer3, sizeof(rtpBuffer3), 21008, 1533790901));
		const auto packet4(createRtpPacket(rtpBuffer4, sizeof(rtpBuffer4), 21009, 1533790901));
		const auto packet5(createRtpPacket(rtpBuffer5, sizeof(rtpBuffer5), 21010, 1533790901));

		// Create a RtpStreamSend instance.
		TestRtpStreamListener testRtpStreamListener;

		RTC::RTP::RtpStream::Params params;

		params.ssrc          = 1111;
		params.clockRate     = 90000;
		params.useNack       = true;
		params.mimeType.type = RTC::RtpCodecMimeType::Type::VIDEO;

		std::string mid;
		auto stream = std::make_unique<RTC::RTP::RtpStreamSend>(
		  std::addressof(testRtpStreamListener), std::addressof(shared), params, mid);

		sendRtpPacket(
		  {
		    { stream.get(), params.ssrc }
    },
		  packet1.get());
		sendRtpPacket(
		  {
		    { stream.get(), params.ssrc }
    },
		  packet2.get());
		sendRtpPacket(
		  {
		    { stream.get(), params.ssrc }
    },
		  packet3.get());
		sendRtpPacket(
		  {
		    { stream.get(), params.ssrc }
    },
		  packet4.get());
		sendRtpPacket(
		  {
		    { stream.get(), params.ssrc }
    },
		  packet5.get());

		// Create a NACK with a first item that requests for packets 3, 4 and 5 and
		// a second one that requests for packets 1 and 4.
		RTC::RTCP::FeedbackRtpNackPacket nackPacket(0, params.ssrc);

		auto* const nackItem1 = new RTC::RTCP::FeedbackRtpNackItem(21008, 0b0000000000000011);
		auto* const nackItem2 = new RTC::RTCP::FeedbackRtpNackItem(21006, 0b0000000000000100);

		nackPacket.AddItem(nackItem1);
		nackPacket.AddItem(nackItem2);

		stream->ReceiveNack(&nackPacket);

		REQUIRE(testRtpStreamListener.retransmittedPackets.size() == 2);

		REQUIRE(expireRetransmissionTimer(shared));
		REQUIRE(testRtpStreamListener.retransmittedPackets.size() == 4);

		// Packet 4 was requested twice but queued once, so nothing is left.
		REQUIRE(expireRetransmissionTimer(shared) == false);

		checkRtxPacket(testRtpStreamListener.retransmittedPackets[0], packet3.get());
		checkRtxPacket(testRtpStreamListener.retransmittedPackets[1], packet4.get());
		checkRtxPacket(testRtpStreamListener.retransmittedPackets[2], packet5.get());
		checkRtxPacket(testRtpStreamListener.retransmittedPackets[3], packet1.get());
	}

	SECTION("packet retransmitted in the last RTT is not retransmitted again")
	{
		// RTT assumed while no Receiver Report has told the real one.
		constexpr int64_t DefaultRttMs{ 100 };

		const auto packet1(createRtpPacket(rtpBuffer1, sizeof(rtpBuffer1), 21006, 1533790901));

		// Create a RtpStreamSend instance.
		TestRtpStreamListener testRtpStreamListener;

		RTC::RTP::RtpStream::Params params;

		params.ssrc          = 1111;
		params.clockRate     = 90000;
		params.useNack       = true;
		params.mimeType.type = RTC::RtpCodecMimeType::Type::VIDEO;

		std::string mid;
		auto stream = std::make_unique<RTC::RTP::RtpStreamSend>(
		  std::addressof(testRtpStreamListener), std::addressof(shared), params, mid);

		sendRtpPacket(
		  {
		    { stream.get(), params.ssrc }
    },
		  packet1.get());

		// Create a NACK item that requests for the packet.
		RTC::RTCP::FeedbackRtpNackPacket nackPacket(0, params.ssrc);

		auto* const nackItem = new RTC::RTCP::FeedbackRtpNackItem(21006, 0b0000000000000000);

		nackPacket.AddItem(nackItem);

		stream->ReceiveNack(&nackPacket);

		REQUIRE(testRtpStreamListener.retransmittedPackets.size() == 1);

		// Right at the end of the RTT the packet is not retransmitted again.
		nowUs += DefaultRttMs * 1000;

		stream->ReceiveNack(&nackPacket);

		REQUIRE(testRtpStreamListener.retransmittedPackets.size() == 1);
		REQUIRE(expireRetransmissionTimer(shared) == false);

		// Past the RTT it is.
		nowUs += 1000;

		stream->ReceiveNack(&nackPacket);

		REQUIRE(testRtpStreamListener.retransmittedPackets.size() == 2);
		REQUIRE(expireRetransmissionTimer(shared) == false);
	}

	SECTION("duplicated packets are discarded")
	{
		auto packet(createRtpPacket(rtpBuffer1, sizeof(rtpBuffer1), 50001, 1000001));

		// Create a RtpStreamSend instance.
		TestRtpStreamListener testRtpStreamListener;

		RTC::RTP::RtpStream::Params params;

		params.ssrc          = packet->GetSsrc();
		params.clockRate     = 90000;
		params.useNack       = true;
		params.mimeType.type = RTC::RtpCodecMimeType::Type::VIDEO;

		std::string mid;
		auto stream = std::make_unique<RTC::RTP::RtpStreamSend>(
		  std::addressof(testRtpStreamListener), std::addressof(shared), params, mid);

		const RTC::RTP::SharedPacket sharedPacket;

		auto result = stream->ReceivePacket(packet.get(), sharedPacket);

		REQUIRE(result == RTC::RTP::RtpStreamSend::ReceivePacketResult::ACCEPTED_AND_STORED);

		result = stream->ReceivePacket(packet.get(), sharedPacket);

		REQUIRE(result == RTC::RTP::RtpStreamSend::ReceivePacketResult::DISCARDED);
	}

	SECTION("Sender Report RTP timestamp is based on the capture instant when known")
	{
		// Instant reported by the mocked clock, and hence the one at which packets are seen.
		constexpr int64_t PacketAtUs{ 1000 * 1000 };
		constexpr uint32_t PacketTs{ 1533790901 };

		TestRtpStreamListener testRtpStreamListener;

		RTC::RTP::RtpStream::Params params;

		params.ssrc          = 1111;
		params.clockRate     = 90000;
		params.mimeType.type = RTC::RtpCodecMimeType::Type::VIDEO;

		std::string mid;

		// Without capture instant, the instant at which the packet was seen is used.
		{
			RTC::RTP::RtpStreamSend stream(
			  std::addressof(testRtpStreamListener), std::addressof(shared), params, mid);
			auto packet(createRtpPacket(rtpBuffer1, sizeof(rtpBuffer1), 21006, PacketTs));

			sendRtpPacket(
			  {
			    { std::addressof(stream), params.ssrc }
      },
			  packet.get());

			const std::unique_ptr<RTC::RTCP::SenderReport> report(
			  stream.GetRtcpSenderReport(PacketAtUs + 1000000));

			REQUIRE(report);
			REQUIRE(report->GetRtpTs() == PacketTs + params.clockRate);
		}

		// With capture instant, the extra half second since it is accounted for.
		{
			RTC::RTP::RtpStreamSend stream(
			  std::addressof(testRtpStreamListener), std::addressof(shared), params, mid);
			auto packet(createRtpPacket(rtpBuffer1, sizeof(rtpBuffer1), 21006, PacketTs));

			packet->SetCaptureAtUs(PacketAtUs - 500000);

			sendRtpPacket(
			  {
			    { std::addressof(stream), params.ssrc }
      },
			  packet.get());

			const std::unique_ptr<RTC::RTCP::SenderReport> report(
			  stream.GetRtcpSenderReport(PacketAtUs + 1000000));

			REQUIRE(report);
			REQUIRE(report->GetRtpTs() == PacketTs + ((1500 * params.clockRate) / 1000));
		}

		// A capture instant ahead of now, which the estimation may yield, does not make the
		// difference wrap around.
		{
			RTC::RTP::RtpStreamSend stream(
			  std::addressof(testRtpStreamListener), std::addressof(shared), params, mid);
			auto packet(createRtpPacket(rtpBuffer1, sizeof(rtpBuffer1), 21006, PacketTs));

			packet->SetCaptureAtUs(PacketAtUs + 3000000);

			sendRtpPacket(
			  {
			    { std::addressof(stream), params.ssrc }
      },
			  packet.get());

			const std::unique_ptr<RTC::RTCP::SenderReport> report(
			  stream.GetRtcpSenderReport(PacketAtUs + 1000000));

			REQUIRE(report);
			REQUIRE(report->GetRtpTs() == PacketTs);
		}
	}

	SECTION("no Sender Report is generated once the stream has stopped sending")
	{
		// Instant reported by the mocked clock, and hence the one at which packets are seen.
		constexpr int64_t PacketAtUs{ 1000 * 1000 };
		constexpr uint32_t PacketTs{ 1533790901 };

		TestRtpStreamListener testRtpStreamListener;

		RTC::RTP::RtpStream::Params params;

		params.ssrc          = 1111;
		params.clockRate     = 90000;
		params.mimeType.type = RTC::RtpCodecMimeType::Type::VIDEO;

		std::string mid;

		RTC::RTP::RtpStreamSend stream(
		  std::addressof(testRtpStreamListener), std::addressof(shared), params, mid);
		auto packet(createRtpPacket(rtpBuffer1, sizeof(rtpBuffer1), 21006, PacketTs));

		sendRtpPacket(
		  {
		    { std::addressof(stream), params.ssrc }
    },
		  packet.get());

		// Right at the limit the Sender Report is still generated.
		const std::unique_ptr<RTC::RTCP::SenderReport> report(stream.GetRtcpSenderReport(
		  PacketAtUs + (RTC::RTP::RtpStreamSend::MaxSenderReportReferenceAgeMs * 1000)));

		REQUIRE(report);

		// Past the limit it is not.
		const std::unique_ptr<RTC::RTCP::SenderReport> staleReport(stream.GetRtcpSenderReport(
		  PacketAtUs + ((RTC::RTP::RtpStreamSend::MaxSenderReportReferenceAgeMs + 1) * 1000)));

		REQUIRE(!staleReport);
	}

	SECTION("RTT is computed from Receiver Reports")
	{
		TestRtpStreamListener testRtpStreamListener;

		RTC::RTP::RtpStream::Params params;

		params.ssrc          = 1111;
		params.clockRate     = 90000;
		params.mimeType.type = RTC::RtpCodecMimeType::Type::VIDEO;

		std::string mid;

		RTC::RTP::RtpStreamSend stream(
		  std::addressof(testRtpStreamListener), std::addressof(shared), params, mid);

		const auto receiveReceiverReport = [&](int64_t receivedAtUs, uint32_t lastSr, uint32_t dlsr)
		{
			RTC::RTCP::ReceiverReport report;

			report.SetSsrc(params.ssrc);
			report.SetLastSenderReport(lastSr);
			report.SetDelaySinceLastSenderReport(dlsr);

			stream.ReceiveRtcpReceiverReport(std::addressof(report), receivedAtUs);
		};

		// The compact NTP representation has 16 bits of seconds, so it wraps around
		// every 65536 seconds.
		constexpr int64_t WrapUs{ 65536 * 1000000LL };

		// The Sender Report is sent 1 second before the wrap, the remote endpoint holds
		// it for half a second, and the Receiver Report arrives right at the wrap.
		receiveReceiverReport(WrapUs, 0xFFFF0000, 0x8000);

		REQUIRE(stream.GetRttMs() == 500.0f);

		// No Sender Report was received by the remote endpoint yet, so the last RTT is
		// kept.
		receiveReceiverReport(WrapUs, 0, 0);

		REQUIRE(stream.GetRttMs() == 500.0f);

		// The remote endpoint answers the Sender Report right away.
		receiveReceiverReport((10 * 1000000) + 500000, 0x000A0000, 0);

		REQUIRE(stream.GetRttMs() == 500.0f);

		// A negative RTT yields 1 millisecond.
		receiveReceiverReport(20 * 1000000, 0x00140000, 0x8000);

		REQUIRE(stream.GetRttMs() == 1.0f);

		// So does a RTT too small to be true.
		receiveReceiverReport(20 * 1000000, 0x0013FFFF, 0);

		REQUIRE(stream.GetRttMs() == 1.0f);
	}

#ifdef PERFORMANCE_TEST
	SECTION("performance")
	{
		// Create a RtpStreamSend instance.
		TestRtpStreamListener testRtpStreamListener;

		RTC::RTP::RtpStream::Params params;

		params.ssrc          = 1111;
		params.clockRate     = 90000;
		params.useNack       = true;
		params.mimeType.type = RTC::RtpCodecMimeType::Type::VIDEO;

		std::string mid;
		std::unique_ptr<RTC::RTP::RtpStreamSend> stream1(new RTC::RTP::RtpStreamSend(
		  std::addressof(testRtpStreamListener), std::addressof(shared), params, mid));

		size_t iterations = 10000000;

		auto start = std::chrono::system_clock::now();

		for (size_t i = 0; i < iterations; i++)
		{
			// Create packet.
			const std::unique_ptr<RTC::RTP::Packet> packet(
			  RTC::RTP::Packet::Parse(rtpBuffer1, sizeof(rtpBuffer1)));

			packet->SetSsrc(1111);

			const RTC::RTP::SharedPacket sharedPacket(packet.get());

			stream1->ReceivePacket(packet.get(), sharedPacket);
		}

		std::chrono::duration<double> dur = std::chrono::system_clock::now() - start;
		std::cout << "nullptr && initialized shared_ptr: \t" << dur.count() << " seconds" << std::endl;

		params.mimeType.type = RTC::RtpCodecMimeType::Type::AUDIO;
		std::unique_ptr<RTC::RTP::RtpStreamSend> stream2(new RTC::RTP::RtpStreamSend(
		  std::addressof(testRtpStreamListener), std::addressof(shared2), params, mid));

		start = std::chrono::system_clock::now();

		for (size_t i = 0; i < iterations; i++)
		{
			const RTC::RTP::SharedPacket sharedPacket;

			// Create packet.
			const std::unique_ptr<RTC::RTP::Packet> packet(
			  RTC::RTP::Packet::Parse(rtpBuffer1, sizeof(rtpBuffer1)));

			packet->SetSsrc(1111);

			stream2->ReceivePacket(packet.get(), sharedPacket);
		}

		dur = std::chrono::system_clock::now() - start;
		std::cout << "raw && empty shared_ptr duration: \t" << dur.count() << " seconds" << std::endl;
	}
#endif
}
