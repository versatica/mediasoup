#include "flatbuffers/buffer.h"
#include "Channel/ChannelNotifier.hpp"
#include "Channel/ChannelSocket.hpp"
#include "FBS/rtpParameters.h"
#include "FBS/transport.h"
#include "RTC/RtpDictionaries.hpp"
#include "RTC/RtpPacket.hpp"
#include "RTC/RtpStream.hpp"
#include "RTC/RtpStreamRecv.hpp"
#include "RTC/Shared.hpp"
#include "RTC/SharedRtpPacket.hpp"
#include "RTC/SimpleConsumer.hpp"
#include <catch2/catch_test_macros.hpp>

using namespace RTC;

const uint8_t PayloadType       = 111;
auto* channelMessageRegistrator = new ChannelMessageRegistrator();
auto* channelSocket             = new Channel::ChannelSocket();
auto* channelNotifier           = new Channel::ChannelNotifier(channelSocket);
auto shared                     = Shared(channelMessageRegistrator, channelNotifier);

class RtpStreamRecvListener : public RtpStreamRecv::Listener
{
public:
	void OnRtpStreamScore(RtpStream* /*rtpStream*/, uint8_t /*score*/, uint8_t /*previousScore*/) override
	{
	}

	void OnRtpStreamSendRtcpPacket(RtpStreamRecv* rtpStream, RTCP::Packet* packet) override
	{
	}

	void OnRtpStreamNeedWorstRemoteFractionLost(
	  RTC::RtpStreamRecv* /*rtpStream*/, uint8_t& /*worstRemoteFractionLost*/) override
	{
	}
};

class ConsumerListener : public Consumer::Listener
{
	void OnConsumerSendRtpPacket(RTC::Consumer* /*consumer*/, RTC::RtpPacket* packet) final
	{
		this->sent.push_back(packet->GetSequenceNumber());
	};
	void OnConsumerRetransmitRtpPacket(RTC::Consumer* consumer, RTC::RtpPacket* packet) final
	{
	}
	void OnConsumerKeyFrameRequested(RTC::Consumer* consumer, uint32_t mappedSsrc) final{};
	void OnConsumerNeedBitrateChange(RTC::Consumer* consumer) final{};
	void OnConsumerNeedZeroBitrate(RTC::Consumer* consumer) final{};
	void OnConsumerProducerClosed(RTC::Consumer* consumer) final{};

public:
	// Verifies that the given number of packets have been sent,
	// and that the sequence numbers are consecutive.
	void Verify(size_t size)
	{
		REQUIRE(this->sent.size() == size);

		if (this->sent.size() <= 1)
		{
			return;
		}

		auto currentSeq = this->sent[0];

		for (auto it = std::next(this->sent.begin()); it != this->sent.end(); ++it)
		{
			REQUIRE(*it == currentSeq + 1);
			currentSeq = *it;
		}
	}

private:
	std::vector<uint16_t> sent;
};

flatbuffers::Offset<::flatbuffers::Vector<::flatbuffers::Offset<FBS::RtpParameters::RtpEncodingParameters>>> CreateRtpEncodingParameters(
  flatbuffers::FlatBufferBuilder& builder)
{
	std::vector<flatbuffers::Offset<FBS::RtpParameters::RtpEncodingParameters>> encodings;

	auto encoding = RTC::RtpEncodingParameters();

	encoding.ssrc = 1234567890;

	encodings.emplace_back(encoding.FillBuffer(builder));

	return builder.CreateVector(encodings);
};

flatbuffers::Offset<FBS::RtpParameters::RtpParameters> CreateRtpParameters(
  flatbuffers::FlatBufferBuilder& builder)
{
	auto rtpParameters = RTC::RtpParameters();
	auto codec         = RTC::RtpCodecParameters();
	auto encoding      = RTC::RtpEncodingParameters();

	codec.mimeType.SetMimeType("audio/opus");
	codec.payloadType = PayloadType;

	encoding.ssrc = 1234567890;

	rtpParameters.mid = "mid";
	rtpParameters.codecs.emplace_back(codec);
	rtpParameters.encodings.emplace_back(encoding);
	rtpParameters.headerExtensions = std::vector<RtpHeaderExtensionParameters>();

	return rtpParameters.FillBuffer(builder);
};

std::unique_ptr<RTC::SimpleConsumer> CreateConsumer(ConsumerListener* listener)
{
	flatbuffers::FlatBufferBuilder bufferBuilder;

	auto consumerId          = bufferBuilder.CreateString("consumerId");
	auto producerId          = bufferBuilder.CreateString("producerId");
	auto rtpParameters       = CreateRtpParameters(bufferBuilder);
	auto consumableEncodings = CreateRtpEncodingParameters(bufferBuilder);

	auto consumeRequestBuilder = FBS::Transport::ConsumeRequestBuilder(bufferBuilder);

	consumeRequestBuilder.add_consumerId(consumerId);
	consumeRequestBuilder.add_producerId(producerId);
	consumeRequestBuilder.add_kind(FBS::RtpParameters::MediaKind::AUDIO);
	consumeRequestBuilder.add_rtpParameters(rtpParameters);
	consumeRequestBuilder.add_type(FBS::RtpParameters::Type::SIMPLE);
	consumeRequestBuilder.add_consumableRtpEncodings(consumableEncodings);
	consumeRequestBuilder.add_paused(false);
	consumeRequestBuilder.add_preferredLayers(0);
	consumeRequestBuilder.add_ignoreDtx(false);

	auto offset = consumeRequestBuilder.Finish();
	bufferBuilder.Finish(offset);

	auto* buf = bufferBuilder.GetBufferPointer();

	const auto* consumeRequest = flatbuffers::GetRoot<FBS::Transport::ConsumeRequest>(buf);

	return std::make_unique<SimpleConsumer>(
	  &shared,
	  consumeRequest->consumerId()->str(),
	  consumeRequest->producerId()->str(),
	  listener,
	  consumeRequest);
}

std::unique_ptr<RtpStreamRecv> CreateRtpStreamRecv()
{
	RtpStreamRecvListener streamRecvListener;
	RtpStream::Params params;

	return std::make_unique<RtpStreamRecv>(&streamRecvListener, params, 0u, false);
}

/**
 * Centralize common setup and helper methods.
 */
class Fixture
{
public:
	Fixture()
	  : listener(std::make_unique<ConsumerListener>()), consumer(CreateConsumer(listener.get())),
	    rtpStream(CreateRtpStreamRecv())
	{
		// Set producer scores and producer stream.
		std::vector<uint8_t> scores{ 10 };
		consumer->ProducerRtpStreamScores(&scores);
		consumer->ProducerNewRtpStream(rtpStream.get(), 1234);
	}

	std::unique_ptr<ConsumerListener> listener;
	std::unique_ptr<SimpleConsumer> consumer;
	std::unique_ptr<RtpStreamRecv> rtpStream;
};

SCENARIO("SimpleConsumer", "[rtp][consumer]")
{
	// clang-format off
	uint8_t buffer[] =
	{
		0x80, 0x01, 0x00, 0x08,
		0x00, 0x00, 0x00, 0x04,
		0x00, 0x00, 0x00, 0x05,
		// Payload (4 bytes).
		0xFF, 0xFF, 0xFF, 0xFF,
		// From here this is just buffer enough for the fake
		// packet->SetPayloadLength() calls below so when cloning the packet it
		// doesn't read non allocated memory.
		0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF,
	};
	// clang-format on

	// This is the size of the original packet.
	size_t originalPacketLength{ 16 };

	SECTION("RTP packets are not forwarded when the consumer is not active")
	{
		Fixture fixture;
		auto* packet = RtpPacket::Parse(buffer, originalPacketLength);
		RTC::SharedRtpPacket sharedPacket(packet);

		packet->SetPayloadType(PayloadType);
		packet->SetPayloadLength(64);

		fixture.consumer->SendRtpPacket(packet, sharedPacket);

		fixture.listener->Verify(0);

		delete packet;
	}

	SECTION("RTP packets are not forwarded for unsupported payload types")
	{
		Fixture fixture;

		// Indicate that the transport is connected in order to activate the consumer.
		dynamic_cast<Consumer*>(fixture.consumer.get())->TransportConnected();

		auto* packet = RtpPacket::Parse(buffer, originalPacketLength);
		RTC::SharedRtpPacket sharedPacket(packet);

		packet->SetPayloadType(PayloadType + 1);
		packet->SetPayloadLength(64);

		fixture.consumer->SendRtpPacket(packet, sharedPacket);
		fixture.listener->Verify(0);

		delete packet;
	}

	SECTION("RTP packets with empty payload are not forwarded")
	{
		Fixture fixture;

		// Indicate that the transport is connected in order to activate the consumer.
		dynamic_cast<Consumer*>(fixture.consumer.get())->TransportConnected();

		auto* packet = RtpPacket::Parse(buffer, originalPacketLength);
		RTC::SharedRtpPacket sharedPacket(packet);

		packet->SetPayloadType(PayloadType + 1);
		packet->SetPayloadLength(0);

		fixture.consumer->SendRtpPacket(packet, sharedPacket);
		fixture.listener->Verify(0);

		delete packet;
	}

	SECTION("outgoing RTP packets are forwarded with increased sequence number")
	{
		Fixture fixture;

		// Indicate that the transport is connected in order to activate the consumer.
		dynamic_cast<Consumer*>(fixture.consumer.get())->TransportConnected();

		auto* packet = RtpPacket::Parse(buffer, originalPacketLength);
		RTC::SharedRtpPacket sharedPacket(packet);

		uint16_t seq{ 1 };

		packet->SetSequenceNumber(seq++);
		packet->SetPayloadType(PayloadType);
		packet->SetPayloadLength(64);
		sharedPacket.Assign(packet);

		fixture.consumer->SendRtpPacket(packet, sharedPacket);

		packet->SetSequenceNumber(seq++);
		sharedPacket.Assign(packet);

		fixture.consumer->SendRtpPacket(packet, sharedPacket);

		packet->SetSequenceNumber(seq++);
		packet->SetPayloadLength(0);
		sharedPacket.Assign(packet);

		fixture.consumer->SendRtpPacket(packet, sharedPacket);

		packet->SetSequenceNumber(seq++);
		packet->SetPayloadLength(20);
		sharedPacket.Assign(packet);

		fixture.consumer->SendRtpPacket(packet, sharedPacket);

		fixture.listener->Verify(3);

		delete packet;
	}
}
