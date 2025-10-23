#include "common.hpp"
#include "RTC/Codecs/AV1.hpp"
#include "RTC/Codecs/PayloadDescriptorHandler.hpp"
#include "RTC/Codecs/VP8.hpp"
#include "RTC/RTCP/FeedbackRtpNack.hpp"
#include "RTC/RtpPacket.hpp"
#include "RTC/RtpStream.hpp"
#include "RTC/RtpStreamSend.hpp"
#include "RTC/SharedRtpPacket.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memcpy()
#include <vector>

// #define PERFORMANCE_TEST 1

using namespace RTC;

static std::unique_ptr<RtpPacket> CreateRtpPacket(
  uint8_t* buffer, size_t len, uint16_t seq, uint32_t timestamp)
{
	auto* packet = RtpPacket::Parse(buffer, len);

	packet->SetSequenceNumber(seq);
	packet->SetTimestamp(timestamp);

	return std::unique_ptr<RtpPacket>(packet);
}

static void SendRtpPacket(std::vector<std::pair<RtpStreamSend*, uint32_t>> streams, RtpPacket* packet)
{
	RTC::SharedRtpPacket sharedPacket;

	for (auto& stream : streams)
	{
		packet->SetSsrc(stream.second);

		auto result = stream.first->ReceivePacket(packet, sharedPacket);

		// NOTE: Here we must replicate the behaviour of Consumer::SendRtpPacket()
		// in which, if the shared packet has been stored and it didn't contain the
		// packet yet, we fill it with a cloned packet.
		if (result == RTC::RtpStreamSend::ReceivePacketResult::ACCEPTED_AND_STORED && !sharedPacket.HasPacket())
		{
			sharedPacket.Assign(packet);
		}
	}
}

static void CheckRtxPacket(RtpPacket* rtxPacket, RtpPacket* origPacket)
{
	REQUIRE(rtxPacket);
	REQUIRE(rtxPacket->GetSequenceNumber() == origPacket->GetSequenceNumber());
	REQUIRE(rtxPacket->GetTimestamp() == origPacket->GetTimestamp());
	REQUIRE(rtxPacket->HasMarker() == origPacket->HasMarker());
}

static void ParseAV1RtpPacket(
  RtpPacket* packet,
  std::unique_ptr<Codecs::DependencyDescriptor::TemplateDependencyStructure>& templateDependencyStructure)
{
	std::unique_ptr<Codecs::DependencyDescriptor> dependencyDescriptor;
	packet->ReadDependencyDescriptor(dependencyDescriptor, templateDependencyStructure);
	REQUIRE(dependencyDescriptor);

	auto* payloadDescriptor        = Codecs::AV1::Parse(dependencyDescriptor);
	auto* payloadDescriptorHandler = new Codecs::AV1::PayloadDescriptorHandler(payloadDescriptor);
	packet->SetPayloadDescriptorHandler(payloadDescriptorHandler);
}

SCENARIO("NACK and RTP packets retransmission", "[rtp][rtcp][nack][rtpstreamsend]")
{
	class TestRtpStreamListener : public RtpStreamSend::Listener
	{
	public:
		void OnRtpStreamScore(RtpStream* /*rtpStream*/, uint8_t /*score*/, uint8_t /*previousScore*/) override
		{
		}

		void OnRtpStreamRetransmitRtpPacket(RtpStreamSend* /*rtpStream*/, RtpPacket* packet) override
		{
			this->retransmittedPackets.push_back(packet);
		}

	public:
		std::vector<RtpPacket*> retransmittedPackets;
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
		// packet1 [pt:123, seq:21006, timestamp:1533790901]
		auto packet1(CreateRtpPacket(rtpBuffer1, sizeof(rtpBuffer1), 21006, 1533790901));
		// packet2 [pt:123, seq:21007, timestamp:1533790901]
		auto packet2(CreateRtpPacket(rtpBuffer2, sizeof(rtpBuffer2), 21007, 1533790901));
		packet2->SetMarker(true);
		// packet3 [pt:123, seq:21008, timestamp:1533793871]
		auto packet3(CreateRtpPacket(rtpBuffer3, sizeof(rtpBuffer3), 21008, 1533793871));
		// packet4 [pt:123, seq:21009, timestamp:1533793871]
		auto packet4(CreateRtpPacket(rtpBuffer4, sizeof(rtpBuffer4), 21009, 1533793871));
		// packet5 [pt:123, seq:21010, timestamp:1533796931]
		auto packet5(CreateRtpPacket(rtpBuffer5, sizeof(rtpBuffer5), 21010, 1533796931));
		packet5->SetMarker(true);

		// Create a RtpStreamSend instance.
		TestRtpStreamListener testRtpStreamListener;

		RtpStream::Params params;

		params.ssrc          = 1111;
		params.clockRate     = 90000;
		params.useNack       = true;
		params.mimeType.type = RTC::RtpCodecMimeType::Type::VIDEO;

		std::string mid;
		auto stream = std::make_unique<RtpStreamSend>(&testRtpStreamListener, params, mid);

		// Receive all the packets (some of them not in order and/or duplicated).
		SendRtpPacket({ { stream.get(), params.ssrc } }, packet1.get());
		SendRtpPacket({ { stream.get(), params.ssrc } }, packet3.get());
		SendRtpPacket({ { stream.get(), params.ssrc } }, packet2.get());
		SendRtpPacket({ { stream.get(), params.ssrc } }, packet3.get());
		SendRtpPacket({ { stream.get(), params.ssrc } }, packet4.get());
		SendRtpPacket({ { stream.get(), params.ssrc } }, packet5.get());
		SendRtpPacket({ { stream.get(), params.ssrc } }, packet5.get());

		// Create a NACK item that request for all the packets.
		RTCP::FeedbackRtpNackPacket nackPacket(0, params.ssrc);
		auto* nackItem = new RTCP::FeedbackRtpNackItem(21006, 0b0000000000001111);

		nackPacket.AddItem(nackItem);

		REQUIRE(nackItem->GetPacketId() == 21006);
		REQUIRE(nackItem->GetLostPacketBitmask() == 0b0000000000001111);

		stream->ReceiveNack(&nackPacket);

		REQUIRE(testRtpStreamListener.retransmittedPackets.size() == 5);

		auto* rtxPacket1 = testRtpStreamListener.retransmittedPackets[0];
		auto* rtxPacket2 = testRtpStreamListener.retransmittedPackets[1];
		auto* rtxPacket3 = testRtpStreamListener.retransmittedPackets[2];
		auto* rtxPacket4 = testRtpStreamListener.retransmittedPackets[3];
		auto* rtxPacket5 = testRtpStreamListener.retransmittedPackets[4];

		testRtpStreamListener.retransmittedPackets.clear();

		CheckRtxPacket(rtxPacket1, packet1.get());
		CheckRtxPacket(rtxPacket2, packet2.get());
		CheckRtxPacket(rtxPacket3, packet3.get());
		CheckRtxPacket(rtxPacket4, packet4.get());
		CheckRtxPacket(rtxPacket5, packet5.get());
	}

	SECTION("receive NACK and get zero retransmitted packets if useNack is not set")
	{
		// packet1 [pt:123, seq:21006, timestamp:1533790901]
		auto packet1(CreateRtpPacket(rtpBuffer1, sizeof(rtpBuffer1), 21006, 1533790901));
		// packet2 [pt:123, seq:21007, timestamp:1533790901]
		auto packet2(CreateRtpPacket(rtpBuffer2, sizeof(rtpBuffer2), 21007, 1533790901));
		// packet3 [pt:123, seq:21008, timestamp:1533793871]
		auto packet3(CreateRtpPacket(rtpBuffer3, sizeof(rtpBuffer3), 21008, 1533793871));
		// packet4 [pt:123, seq:21009, timestamp:1533793871]
		auto packet4(CreateRtpPacket(rtpBuffer4, sizeof(rtpBuffer4), 21009, 1533793871));
		// packet5 [pt:123, seq:21010, timestamp:1533796931]
		auto packet5(CreateRtpPacket(rtpBuffer5, sizeof(rtpBuffer5), 21010, 1533796931));

		// Create a RtpStreamSend instance.
		TestRtpStreamListener testRtpStreamListener;

		RtpStream::Params params;

		params.ssrc          = 1111;
		params.clockRate     = 90000;
		params.useNack       = false;
		params.mimeType.type = RTC::RtpCodecMimeType::Type::VIDEO;

		std::string mid;
		auto stream = std::make_unique<RtpStreamSend>(&testRtpStreamListener, params, mid);

		// Receive all the packets (some of them not in order and/or duplicated).
		SendRtpPacket({ { stream.get(), params.ssrc } }, packet1.get());
		SendRtpPacket({ { stream.get(), params.ssrc } }, packet3.get());
		SendRtpPacket({ { stream.get(), params.ssrc } }, packet2.get());
		SendRtpPacket({ { stream.get(), params.ssrc } }, packet3.get());
		SendRtpPacket({ { stream.get(), params.ssrc } }, packet4.get());
		SendRtpPacket({ { stream.get(), params.ssrc } }, packet5.get());
		SendRtpPacket({ { stream.get(), params.ssrc } }, packet5.get());

		// Create a NACK item that request for all the packets.
		RTCP::FeedbackRtpNackPacket nackPacket(0, params.ssrc);
		auto* nackItem = new RTCP::FeedbackRtpNackItem(21006, 0b0000000000001111);

		nackPacket.AddItem(nackItem);

		REQUIRE(nackItem->GetPacketId() == 21006);
		REQUIRE(nackItem->GetLostPacketBitmask() == 0b0000000000001111);

		stream->ReceiveNack(&nackPacket);

		REQUIRE(testRtpStreamListener.retransmittedPackets.empty());

		testRtpStreamListener.retransmittedPackets.clear();
	}

	SECTION("receive NACK and get zero retransmitted packets for audio")
	{
		// packet1 [pt:123, seq:21006, timestamp:1533790901]
		auto packet1(CreateRtpPacket(rtpBuffer1, sizeof(rtpBuffer1), 21006, 1533790901));
		// packet2 [pt:123, seq:21007, timestamp:1533790901]
		auto packet2(CreateRtpPacket(rtpBuffer2, sizeof(rtpBuffer2), 21007, 1533790901));
		// packet3 [pt:123, seq:21008, timestamp:1533793871]
		auto packet3(CreateRtpPacket(rtpBuffer3, sizeof(rtpBuffer3), 21008, 1533793871));
		// packet4 [pt:123, seq:21009, timestamp:1533793871]
		auto packet4(CreateRtpPacket(rtpBuffer4, sizeof(rtpBuffer4), 21009, 1533793871));
		// packet5 [pt:123, seq:21010, timestamp:1533796931]
		auto packet5(CreateRtpPacket(rtpBuffer5, sizeof(rtpBuffer5), 21010, 1533796931));

		// Create a RtpStreamSend instance.
		TestRtpStreamListener testRtpStreamListener;

		RtpStream::Params params;

		params.ssrc          = 1111;
		params.clockRate     = 90000;
		params.useNack       = false;
		params.mimeType.type = RTC::RtpCodecMimeType::Type::AUDIO;

		std::string mid;
		auto stream = std::make_unique<RtpStreamSend>(&testRtpStreamListener, params, mid);

		// Receive all the packets (some of them not in order and/or duplicated).
		SendRtpPacket({ { stream.get(), params.ssrc } }, packet1.get());
		SendRtpPacket({ { stream.get(), params.ssrc } }, packet3.get());
		SendRtpPacket({ { stream.get(), params.ssrc } }, packet2.get());
		SendRtpPacket({ { stream.get(), params.ssrc } }, packet3.get());
		SendRtpPacket({ { stream.get(), params.ssrc } }, packet4.get());
		SendRtpPacket({ { stream.get(), params.ssrc } }, packet5.get());
		SendRtpPacket({ { stream.get(), params.ssrc } }, packet5.get());

		// Create a NACK item that request for all the packets.
		RTCP::FeedbackRtpNackPacket nackPacket(0, params.ssrc);
		auto* nackItem = new RTCP::FeedbackRtpNackItem(21006, 0b0000000000001111);

		nackPacket.AddItem(nackItem);

		REQUIRE(nackItem->GetPacketId() == 21006);
		REQUIRE(nackItem->GetLostPacketBitmask() == 0b0000000000001111);

		stream->ReceiveNack(&nackPacket);

		REQUIRE(testRtpStreamListener.retransmittedPackets.empty());

		testRtpStreamListener.retransmittedPackets.clear();
	}

	SECTION("receive NACK in different RtpStreamSend instances and get retransmitted packets")
	{
		// packet1 [pt:123, seq:21006, timestamp:1533790901]
		auto packet1(CreateRtpPacket(rtpBuffer1, sizeof(rtpBuffer1), 21006, 1533790901));
		// packet2 [pt:123, seq:21007, timestamp:1533790901]
		auto packet2(CreateRtpPacket(rtpBuffer2, sizeof(rtpBuffer2), 21007, 1533790901));

		// Create two RtpStreamSend instances.
		TestRtpStreamListener testRtpStreamListener1;
		TestRtpStreamListener testRtpStreamListener2;

		RtpStream::Params params1;

		params1.ssrc          = 1111;
		params1.clockRate     = 90000;
		params1.useNack       = true;
		params1.mimeType.type = RTC::RtpCodecMimeType::Type::VIDEO;

		std::string mid;
		std::unique_ptr<RtpStreamSend> stream1(new RtpStreamSend(&testRtpStreamListener1, params1, mid));

		RtpStream::Params params2;

		params2.ssrc          = 2222;
		params2.clockRate     = 90000;
		params2.useNack       = true;
		params2.mimeType.type = RTC::RtpCodecMimeType::Type::VIDEO;

		std::unique_ptr<RtpStreamSend> stream2(new RtpStreamSend(&testRtpStreamListener2, params2, mid));

		// Receive all the packets in both streams.
		SendRtpPacket({ { stream1.get(), params1.ssrc }, { stream2.get(), params2.ssrc } }, packet1.get());
		SendRtpPacket({ { stream1.get(), params1.ssrc }, { stream2.get(), params2.ssrc } }, packet2.get());

		// Create a NACK item that request for all the packets.
		RTCP::FeedbackRtpNackPacket nackPacket(0, params1.ssrc);
		auto* nackItem = new RTCP::FeedbackRtpNackItem(21006, 0b0000000000000001);

		nackPacket.AddItem(nackItem);

		REQUIRE(nackItem->GetPacketId() == 21006);
		REQUIRE(nackItem->GetLostPacketBitmask() == 0b0000000000000001);

		// Process the NACK packet on stream1.
		stream1->ReceiveNack(&nackPacket);

		REQUIRE(testRtpStreamListener1.retransmittedPackets.size() == 2);

		auto* rtxPacket1 = testRtpStreamListener1.retransmittedPackets[0];
		auto* rtxPacket2 = testRtpStreamListener1.retransmittedPackets[1];

		testRtpStreamListener1.retransmittedPackets.clear();

		CheckRtxPacket(rtxPacket1, packet1.get());
		CheckRtxPacket(rtxPacket2, packet2.get());

		// Process the NACK packet on stream2.
		stream2->ReceiveNack(&nackPacket);

		REQUIRE(testRtpStreamListener2.retransmittedPackets.size() == 2);

		rtxPacket1 = testRtpStreamListener2.retransmittedPackets[0];
		rtxPacket2 = testRtpStreamListener2.retransmittedPackets[1];

		testRtpStreamListener2.retransmittedPackets.clear();

		CheckRtxPacket(rtxPacket1, packet1.get());
		CheckRtxPacket(rtxPacket2, packet2.get());
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

		// packet1 [pt:123, seq:1, timestamp:1]
		auto packet1(CreateRtpPacket(rtpBuffer1, sizeof(rtpBuffer1), 1, 1));
		// packet2 [pt:123, seq:2, timestamp:1]
		auto packet2(CreateRtpPacket(rtpBuffer2, sizeof(rtpBuffer2), 2, 1));
		// packet3 [pt:123, seq:3, timestamp:1]
		auto packet3(CreateRtpPacket(rtpBuffer3, sizeof(rtpBuffer3), 3, 1));

		// Create two RtpStreamSend instances.
		TestRtpStreamListener testRtpStreamListener1;
		TestRtpStreamListener testRtpStreamListener2;

		RtpStream::Params params1;

		params1.ssrc          = 1111;
		params1.clockRate     = 90000;
		params1.useNack       = true;
		params1.mimeType.type = RTC::RtpCodecMimeType::Type::VIDEO;

		std::string mid;
		std::unique_ptr<RtpStreamSend> stream1(new RtpStreamSend(&testRtpStreamListener1, params1, mid));

		RtpStream::Params params2;

		params2.ssrc          = 2222;
		params2.clockRate     = 90000;
		params2.useNack       = true;
		params2.mimeType.type = RTC::RtpCodecMimeType::Type::VIDEO;

		std::unique_ptr<RtpStreamSend> stream2(new RtpStreamSend(&testRtpStreamListener2, params2, mid));

		// Create two VP8 encoding contexts.
		RTC::Codecs::EncodingContext::Params params;
		params.spatialLayers  = 0;
		params.temporalLayers = 3;
		Codecs::VP8::EncodingContext context1(params);

		context1.SetCurrentTemporalLayer(3);
		context1.SetTargetTemporalLayer(3);

		Codecs::VP8::EncodingContext context2(params);

		context2.SetCurrentTemporalLayer(0);
		context2.SetTargetTemporalLayer(0);

		// Parse the first packet.
		auto* payloadDescriptor1 = Codecs::VP8::Parse(packet1->GetPayload(), packet1->GetPayloadLength());
		REQUIRE(payloadDescriptor1->pictureId == 1);

		auto* payloadDescriptorHandler1 = new Codecs::VP8::PayloadDescriptorHandler(payloadDescriptor1);
		packet1->SetPayloadDescriptorHandler(payloadDescriptorHandler1);

		bool marker = false;

		// Process the first packet with context1.
		auto forwarded = payloadDescriptorHandler1->Process(&context1, packet1.get(), marker);
		REQUIRE(forwarded);

		// Parse the second packet.
		auto* payloadDescriptor2 = Codecs::VP8::Parse(packet2->GetPayload(), packet2->GetPayloadLength());
		REQUIRE(payloadDescriptor2->pictureId == 2);

		auto* payloadDescriptorHandler2 = new Codecs::VP8::PayloadDescriptorHandler(payloadDescriptor2);
		packet2->SetPayloadDescriptorHandler(payloadDescriptorHandler2);

		// Process the second packet with context1.
		forwarded = payloadDescriptorHandler2->Process(&context1, packet2.get(), marker);
		REQUIRE(forwarded);

		// Process the second packet for context2.
		forwarded = payloadDescriptorHandler2->Process(&context2, packet2.get(), marker);
		// It must not forwared because the target temporal layer is 0.
		REQUIRE(!forwarded);

		// Parse the third packet
		auto* payloadDescriptor3 = Codecs::VP8::Parse(packet3->GetPayload(), packet3->GetPayloadLength());
		REQUIRE(payloadDescriptor3->pictureId == 3);

		auto* payloadDescriptorHandler3 = new Codecs::VP8::PayloadDescriptorHandler(payloadDescriptor3);
		packet2->SetPayloadDescriptorHandler(payloadDescriptorHandler3);

		// Process the third packet for context1.
		forwarded = payloadDescriptorHandler3->Process(&context1, packet3.get(), marker);
		REQUIRE(forwarded);

		// Receive the third packet in the first stream.
		SendRtpPacket({ { stream1.get(), params1.ssrc } }, packet3.get());

		// Update current/target temporal layers for context2.
		context2.SetCurrentTemporalLayer(3);
		context2.SetTargetTemporalLayer(3);

		forwarded = payloadDescriptorHandler3->Process(&context2, packet3.get(), marker);
		REQUIRE(forwarded);

		// Receive the third packet in the second stream.
		SendRtpPacket({ { stream2.get(), params2.ssrc } }, packet3.get());

		// Create a NACK item that requests the third packet.
		RTCP::FeedbackRtpNackPacket nackPacket(0, params1.ssrc);
		auto* nackItem = new RTCP::FeedbackRtpNackItem(3, 0b0000000000000000);

		nackPacket.AddItem(nackItem);

		REQUIRE(nackItem->GetPacketId() == 3);
		REQUIRE(nackItem->GetLostPacketBitmask() == 0b0000000000000000);

		// Process the NACK packet on stream1.
		stream1->ReceiveNack(&nackPacket);

		REQUIRE(testRtpStreamListener1.retransmittedPackets.size() == 1);

		auto* packet = testRtpStreamListener1.retransmittedPackets[0];

		// Parse payload and check pictureId.
		auto* payloadDescriptor4 = Codecs::VP8::Parse(packet->GetPayload(), packet->GetPayloadLength());
		REQUIRE(payloadDescriptor4->pictureId == 3);

		// Process the NACK packet on stream2.
		stream2->ReceiveNack(&nackPacket);

		REQUIRE(testRtpStreamListener2.retransmittedPackets.size() == 1);

		packet = testRtpStreamListener2.retransmittedPackets[0];

		// Parse payload and check pictureId.
		auto* payloadDescriptor5 = Codecs::VP8::Parse(packet->GetPayload(), packet->GetPayloadLength());
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

		// packet1 [pt:123, seq:1, timestamp:1]
		auto packet1(CreateRtpPacket(rtpBuffer1, sizeof(rtpBuffer1), 1, 1));
		packet1->SetDependencyDescriptorExtensionId(12);
		// packet2 [pt:123, seq:2, timestamp:1]
		auto packet2(CreateRtpPacket(rtpBuffer2, sizeof(rtpBuffer2), 2, 1));
		packet2->SetDependencyDescriptorExtensionId(12);
		// packet3 [pt:123, seq:3, timestamp:1]
		auto packet3(CreateRtpPacket(rtpBuffer3, sizeof(rtpBuffer3), 3, 1));
		packet3->SetDependencyDescriptorExtensionId(12);

		// Create two RtpStreamSend instances.
		TestRtpStreamListener testRtpStreamListener1;
		TestRtpStreamListener testRtpStreamListener2;

		RtpStream::Params params1;

		params1.ssrc          = 1111;
		params1.clockRate     = 90000;
		params1.useNack       = true;
		params1.mimeType.type = RTC::RtpCodecMimeType::Type::VIDEO;

		std::string mid;
		std::unique_ptr<RtpStreamSend> stream1(new RtpStreamSend(&testRtpStreamListener1, params1, mid));

		RtpStream::Params params2;

		params2.ssrc          = 2222;
		params2.clockRate     = 90000;
		params2.useNack       = true;
		params2.mimeType.type = RTC::RtpCodecMimeType::Type::VIDEO;

		std::unique_ptr<RtpStreamSend> stream2(new RtpStreamSend(&testRtpStreamListener2, params2, mid));

		// Create two AV1 encoding contexts.
		RTC::Codecs::EncodingContext::Params params;
		params.spatialLayers  = 1;
		params.temporalLayers = 2;

		Codecs::AV1::EncodingContext context1(params);
		context1.SetCurrentSpatialLayer(0);
		context1.SetCurrentTemporalLayer(0);
		context1.SetTargetSpatialLayer(0);
		context1.SetTargetTemporalLayer(0);

		Codecs::AV1::EncodingContext context2(params);
		context2.SetCurrentSpatialLayer(0);
		context2.SetCurrentTemporalLayer(0);
		context2.SetTargetSpatialLayer(0);
		context2.SetTargetTemporalLayer(1);

		std::unique_ptr<Codecs::DependencyDescriptor::TemplateDependencyStructure> templateDependencyStructure;

		// Parse the first packet for the shake of having the template dependency structure.
		ParseAV1RtpPacket(packet1.get(), templateDependencyStructure);
		// Parse the second packet.
		ParseAV1RtpPacket(packet2.get(), templateDependencyStructure);

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
		ParseAV1RtpPacket(packet3.get(), templateDependencyStructure);

		// Process the third packet with context1 and verify current spatial layers.
		forwarded = packet3->ProcessPayload(&context1, marker);
		REQUIRE(forwarded);
		REQUIRE(context1.GetCurrentSpatialLayer() == 0);
		REQUIRE(context1.GetCurrentTemporalLayer() == 0);

		RTC::SharedRtpPacket sharedPacket;

		packet3->SetSsrc(params1.ssrc);
		// Whenever packet3 is Nacked on stream1, it must always be set a
		// 00000001 (S0_T1) active decode target bitmas.
		auto result = stream1->ReceivePacket(packet3.get(), sharedPacket);

		REQUIRE(result == RTC::RtpStreamSend::ReceivePacketResult::ACCEPTED_AND_STORED);
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
		RTCP::FeedbackRtpNackPacket nackPacket(0, params1.ssrc);
		auto* nackItem = new RTCP::FeedbackRtpNackItem(3, 0b0000000000000000);

		nackPacket.AddItem(nackItem);

		REQUIRE(nackItem->GetPacketId() == 3);
		REQUIRE(nackItem->GetLostPacketBitmask() == 0b0000000000000000);

		// Process the NACK packet on stream1.
		stream1->ReceiveNack(&nackPacket);

		REQUIRE(testRtpStreamListener1.retransmittedPackets.size() == 1);

		auto* packet = testRtpStreamListener1.retransmittedPackets[0];

		// Parse DD and check bitmask.
		std::unique_ptr<Codecs::DependencyDescriptor> dependencyDescriptor4;

		packet->ReadDependencyDescriptor(dependencyDescriptor4, templateDependencyStructure);
		REQUIRE(dependencyDescriptor4);
		// TODO: Enable once we write DD.
		// REQUIRE(dependencyDescriptor4->activeDecodeTargetsBitmask == 0b0000000000000001);

		// Process the NACK packet on stream2.
		stream2->ReceiveNack(&nackPacket);

		REQUIRE(testRtpStreamListener2.retransmittedPackets.size() == 1);

		packet = testRtpStreamListener2.retransmittedPackets[0];

		// Parse DD and check bitmask.
		std::unique_ptr<Codecs::DependencyDescriptor> dependencyDescriptor5;

		packet->ReadDependencyDescriptor(dependencyDescriptor5, templateDependencyStructure);
		REQUIRE(dependencyDescriptor5);
		// TODO: Enable once we write DD.
		// REQUIRE(dependencyDescriptor5->activeDecodeTargetsBitmask == 0b0000000000000011);
	}

	SECTION("packets get retransmitted as long as they don't exceed MaxRetransmissionDelayForVideoMs")
	{
		uint32_t clockRate = 90000;
		uint32_t firstTs   = 1533790901;
		uint32_t diffTs    = RtpStreamSend::MaxRetransmissionDelayForVideoMs * clockRate / 1000;
		uint32_t secondTs  = firstTs + diffTs;

		auto packet1(CreateRtpPacket(rtpBuffer1, sizeof(rtpBuffer1), 21006, firstTs));
		auto packet2(CreateRtpPacket(rtpBuffer2, sizeof(rtpBuffer2), 21007, secondTs - 1));

		// Create a RtpStreamSend instance.
		TestRtpStreamListener testRtpStreamListener;

		RtpStream::Params params1;

		params1.ssrc          = 1111;
		params1.clockRate     = clockRate;
		params1.useNack       = true;
		params1.mimeType.type = RTC::RtpCodecMimeType::Type::VIDEO;

		std::string mid;
		auto stream = std::make_unique<RtpStreamSend>(&testRtpStreamListener, params1, mid);

		// Receive all the packets.
		SendRtpPacket({ { stream.get(), params1.ssrc } }, packet1.get());
		SendRtpPacket({ { stream.get(), params1.ssrc } }, packet2.get());

		// Create a NACK item that request for all the packets.
		RTCP::FeedbackRtpNackPacket nackPacket(0, params1.ssrc);
		auto* nackItem = new RTCP::FeedbackRtpNackItem(21006, 0b0000000000000001);

		nackPacket.AddItem(nackItem);

		REQUIRE(nackItem->GetPacketId() == 21006);
		REQUIRE(nackItem->GetLostPacketBitmask() == 0b0000000000000001);

		// Process the NACK packet on stream1.
		stream->ReceiveNack(&nackPacket);

		REQUIRE(testRtpStreamListener.retransmittedPackets.size() == 2);

		auto* rtxPacket1 = testRtpStreamListener.retransmittedPackets[0];
		auto* rtxPacket2 = testRtpStreamListener.retransmittedPackets[1];

		testRtpStreamListener.retransmittedPackets.clear();

		CheckRtxPacket(rtxPacket1, packet1.get());
		CheckRtxPacket(rtxPacket2, packet2.get());
	}

	SECTION("packets don't get retransmitted if MaxRetransmissionDelayForVideoMs is exceeded")
	{
		uint32_t clockRate = 90000;
		uint32_t firstTs   = 1533790901;
		uint32_t diffTs    = RtpStreamSend::MaxRetransmissionDelayForVideoMs * clockRate / 1000;
		// Make second packet arrive more than MaxRetransmissionDelayForVideoMs later.
		uint32_t secondTs = firstTs + diffTs + 100;
		// Send a third packet so it will clean old packets from the buffer.
		uint32_t thirdTs = firstTs + (2 * diffTs);

		auto packet1(CreateRtpPacket(rtpBuffer1, sizeof(rtpBuffer1), 21006, firstTs));
		auto packet2(CreateRtpPacket(rtpBuffer2, sizeof(rtpBuffer2), 21007, secondTs));
		auto packet3(CreateRtpPacket(rtpBuffer3, sizeof(rtpBuffer3), 21008, thirdTs));

		// Create a RtpStreamSend instance.
		TestRtpStreamListener testRtpStreamListener;

		RtpStream::Params params1;

		params1.ssrc          = 1111;
		params1.clockRate     = clockRate;
		params1.useNack       = true;
		params1.mimeType.type = RTC::RtpCodecMimeType::Type::VIDEO;

		std::string mid;
		auto stream = std::make_unique<RtpStreamSend>(&testRtpStreamListener, params1, mid);

		// Receive all the packets.
		SendRtpPacket({ { stream.get(), params1.ssrc } }, packet1.get());
		SendRtpPacket({ { stream.get(), params1.ssrc } }, packet2.get());
		SendRtpPacket({ { stream.get(), params1.ssrc } }, packet3.get());

		// Create a NACK item that requests for all packets.
		RTCP::FeedbackRtpNackPacket nackPacket(0, params1.ssrc);
		auto* nackItem = new RTCP::FeedbackRtpNackItem(21006, 0b0000000000000001);

		nackPacket.AddItem(nackItem);

		REQUIRE(nackItem->GetPacketId() == 21006);
		REQUIRE(nackItem->GetLostPacketBitmask() == 0b0000000000000001);

		// Process the NACK packet on stream1.
		stream->ReceiveNack(&nackPacket);

		REQUIRE(testRtpStreamListener.retransmittedPackets.size() == 1);

		auto* rtxPacket2 = testRtpStreamListener.retransmittedPackets[0];

		testRtpStreamListener.retransmittedPackets.clear();

		CheckRtxPacket(rtxPacket2, packet2.get());
	}

	SECTION("packets get removed from the retransmission buffer if seq number of the stream is reset")
	{
		// This scenario reproduce the "too bad sequence number" and "bad sequence
		// number" scenarios in RtpStream::UpdateSeq().
		auto packet1(CreateRtpPacket(rtpBuffer1, sizeof(rtpBuffer1), 50001, 1000001));
		auto packet2(CreateRtpPacket(rtpBuffer2, sizeof(rtpBuffer2), 50002, 1000002));
		// Third packet has bad sequence number (its seq is more than MaxDropout=3000
		// older than current max seq) and will be dropped.
		auto packet3(CreateRtpPacket(rtpBuffer3, sizeof(rtpBuffer3), 40003, 1000003));
		// Forth packet has seq=badSeq+1 so will be accepted and will trigger a
		// stream reset.
		auto packet4(CreateRtpPacket(rtpBuffer4, sizeof(rtpBuffer4), 40004, 1000004));

		// Create a RtpStreamSend instance.
		TestRtpStreamListener testRtpStreamListener;

		RtpStream::Params params1;

		params1.ssrc          = 1111;
		params1.clockRate     = 90000;
		params1.useNack       = true;
		params1.mimeType.type = RTC::RtpCodecMimeType::Type::VIDEO;

		std::string mid;
		auto stream = std::make_unique<RtpStreamSend>(&testRtpStreamListener, params1, mid);

		SendRtpPacket({ { stream.get(), params1.ssrc } }, packet1.get());
		SendRtpPacket({ { stream.get(), params1.ssrc } }, packet2.get());
		SendRtpPacket({ { stream.get(), params1.ssrc } }, packet3.get());
		SendRtpPacket({ { stream.get(), params1.ssrc } }, packet4.get());

		// Create a NACK item that requests for packets 1 and 2.
		RTCP::FeedbackRtpNackPacket nackPacket2(0, params1.ssrc);
		auto* nackItem2 = new RTCP::FeedbackRtpNackItem(50001, 0b0000000000000001);

		nackPacket2.AddItem(nackItem2);

		// Process the NACK packet on stream1.
		stream->ReceiveNack(&nackPacket2);

		REQUIRE(testRtpStreamListener.retransmittedPackets.empty());
	}

#ifdef PERFORMANCE_TEST
	SECTION("Performance")
	{
		// Create a RtpStreamSend instance.
		TestRtpStreamListener testRtpStreamListener;

		RtpStream::Params params;

		params.ssrc          = 1111;
		params.clockRate     = 90000;
		params.useNack       = true;
		params.mimeType.type = RTC::RtpCodecMimeType::Type::VIDEO;

		std::string mid;
		std::unique_ptr<RtpStreamSend> stream1(new RtpStreamSend(&testRtpStreamListener, params, mid));

		size_t iterations = 10000000;

		auto start = std::chrono::system_clock::now();

		for (size_t i = 0; i < iterations; i++)
		{
			// Create packet.
			auto* packet = RtpPacket::Parse(rtpBuffer1, 1500);
			packet->SetSsrc(1111);

			std::shared_ptr<RtpPacket> sharedPacket(packet);

			stream1->ReceivePacket(packet, sharedPacket);
		}

		std::chrono::duration<double> dur = std::chrono::system_clock::now() - start;
		std::cout << "nullptr && initialized shared_ptr: \t" << dur.count() << " seconds" << std::endl;

		params.mimeType.type = RTC::RtpCodecMimeType::Type::AUDIO;
		std::unique_ptr<RtpStreamSend> stream2(new RtpStreamSend(&testRtpStreamListener, params, mid));

		start = std::chrono::system_clock::now();

		for (size_t i = 0; i < iterations; i++)
		{
			std::shared_ptr<RtpPacket> sharedPacket;

			// Create packet.
			auto* packet = RtpPacket::Parse(rtpBuffer1, 1500);
			packet->SetSsrc(1111);

			stream2->ReceivePacket(packet, sharedPacket);
		}

		dur = std::chrono::system_clock::now() - start;
		std::cout << "raw && empty shared_ptr duration: \t" << dur.count() << " seconds" << std::endl;
	}
#endif
}
