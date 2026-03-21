#include "flatbuffers/buffer.h"
#include "Channel/ChannelNotifier.hpp"
#include "Channel/ChannelSocket.hpp"
#include "FBS/rtpParameters.h"
#include "FBS/transport.h"
#include "RTC/RTP/Packet.hpp"
#include "RTC/RTP/RtpStream.hpp"
#include "RTC/RTP/RtpStreamRecv.hpp"
#include "RTC/RTP/SharedPacket.hpp"
#include "RTC/RtpDictionaries.hpp"
#include "RTC/Shared.hpp"
#include "RTC/SimpleConsumer.hpp"
#include <catch2/catch_test_macros.hpp>

namespace
{
	// NOLINTBEGIN(readability-identifier-naming)
	const uint8_t payloadType       = 111;
	auto* channelMessageRegistrator = new ChannelMessageRegistrator();
	auto* channelSocket             = new Channel::ChannelSocket();
	auto* channelNotifier           = new Channel::ChannelNotifier(channelSocket);
	auto shared                     = RTC::Shared(channelMessageRegistrator, channelNotifier);
	// NOLINTEND(readability-identifier-naming)

	class RtpStreamRecvListener : public RTC::RTP::RtpStreamRecv::Listener
	{
	public:
		void OnRtpStreamScore(
		  RTC::RTP::RtpStream* /*rtpStream*/, uint8_t /*score*/, uint8_t /*previousScore*/) override
		{
		}

		void OnRtpStreamSendRtcpPacket(RTC::RTP::RtpStreamRecv* rtpStream, RTC::RTCP::Packet* packet) override
		{
		}

		void OnRtpStreamNeedWorstRemoteFractionLost(
		  RTC::RTP::RtpStreamRecv* /*rtpStream*/, uint8_t& /*worstRemoteFractionLost*/) override
		{
		}
	};

	class ConsumerListener : public RTC::Consumer::Listener
	{
		void OnConsumerSendRtpPacket(RTC::Consumer* /*consumer*/, RTC::RTP::Packet* packet) final
		{
			this->sent.push_back(packet->GetSequenceNumber());
		};
		void OnConsumerRetransmitRtpPacket(RTC::Consumer* consumer, RTC::RTP::Packet* packet) final
		{
		}
		void OnConsumerKeyFrameRequested(RTC::Consumer* consumer, uint32_t mappedSsrc) final {};
		void OnConsumerNeedBitrateChange(RTC::Consumer* consumer) final {};
		void OnConsumerNeedZeroBitrate(RTC::Consumer* consumer) final {};
		void OnConsumerProducerClosed(RTC::Consumer* consumer) final {};

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

	flatbuffers::Offset<::flatbuffers::Vector<::flatbuffers::Offset<FBS::RtpParameters::RtpEncodingParameters>>> createRtpEncodingParameters(
	  flatbuffers::FlatBufferBuilder& builder)
	{
		std::vector<flatbuffers::Offset<FBS::RtpParameters::RtpEncodingParameters>> encodings;

		auto encoding = RTC::RtpEncodingParameters();

		encoding.ssrc = 1234567890;

		encodings.emplace_back(encoding.FillBuffer(builder));

		return builder.CreateVector(encodings);
	};

	flatbuffers::Offset<FBS::RtpParameters::RtpParameters> createRtpParameters(
	  flatbuffers::FlatBufferBuilder& builder)
	{
		auto rtpParameters = RTC::RtpParameters();
		auto codec         = RTC::RtpCodecParameters();
		auto encoding      = RTC::RtpEncodingParameters();

		codec.mimeType.SetMimeType("audio/opus");
		codec.payloadType = payloadType;

		encoding.ssrc = 1234567890;

		rtpParameters.mid = "mid";
		rtpParameters.codecs.emplace_back(codec);
		rtpParameters.encodings.emplace_back(encoding);
		rtpParameters.headerExtensions = std::vector<RTC::RtpHeaderExtensionParameters>();

		return rtpParameters.FillBuffer(builder);
	};

	std::unique_ptr<RTC::SimpleConsumer> createConsumer(ConsumerListener* listener)
	{
		flatbuffers::FlatBufferBuilder bufferBuilder;

		auto consumerId          = bufferBuilder.CreateString("consumerId");
		auto producerId          = bufferBuilder.CreateString("producerId");
		auto rtpParameters       = createRtpParameters(bufferBuilder);
		auto consumableEncodings = createRtpEncodingParameters(bufferBuilder);

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

		return std::make_unique<RTC::SimpleConsumer>(
		  &shared,
		  consumeRequest->consumerId()->str(),
		  consumeRequest->producerId()->str(),
		  listener,
		  consumeRequest);
	}

	std::unique_ptr<RTC::RTP::RtpStreamRecv> createRtpStreamRecv()
	{
		RtpStreamRecvListener streamRecvListener;
		RTC::RTP::RtpStream::Params params;

		return std::make_unique<RTC::RTP::RtpStreamRecv>(&streamRecvListener, params, 0u, false);
	}

	/**
	 * Centralize common setup and helper methods.
	 */
	class Fixture
	{
	public:
		Fixture()
		  : listener(std::make_unique<ConsumerListener>()),
		    consumer(createConsumer(listener.get())),
		    rtpStream(createRtpStreamRecv())
		{
			// NOTE: This must be static because the Consumer stores the given vector
			// pointer which is supposed to exist in the associated Producer (but here
			// there is no associated Producer).
			const std::vector<uint8_t> scores{ 10 };

			consumer->ProducerRtpStreamScores(&scores);

			// NOTE: mappedSsrc here MUST be 1234567890 (otherwise Consumer will crash).
			// This is guaranteed by Producer class, however here we must do it manually.
			consumer->ProducerNewRtpStream(rtpStream.get(), 1234567890);
		}

		std::unique_ptr<ConsumerListener> listener;
		std::unique_ptr<RTC::SimpleConsumer> consumer;
		std::unique_ptr<RTC::RTP::RtpStreamRecv> rtpStream;
	};
} // namespace

SCENARIO("SimpleConsumer", "[rtp][consumer]")
{
	// TODO: We should NOT parse RTP packets for tests anymore. We should use
	// RTC::RTP::Packet::Factory() instead.
	// clang-format off
	alignas(4) uint8_t buffer[] =
	{
		0x80, 0x01, 0x00, 0x08,
		0x00, 0x00, 0x00, 0x04,
		0x49, 0x96, 0x02, 0xD2, // SSRC: 1234567890 (must be this exact value).
		// Payload (4 bytes).
		0xFF, 0xFF, 0xFF, 0xFF,
		// From here this is just buffer enough for the variable length payload so
		// when cloning the packet it doesn't read non allocated memory.
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
	const size_t originalPacketLength{ 16 };

	SECTION("RTP packets are not forwarded when the consumer is not active")
	{
		Fixture fixture;
		auto* packet = RTC::RTP::Packet::Parse(buffer, originalPacketLength + 64);
		RTC::RTP::SharedPacket sharedPacket(packet);

		packet->SetPayloadType(payloadType);

		fixture.consumer->SendRtpPacket(packet, sharedPacket);

		fixture.listener->Verify(0);

		delete packet;
	}

	SECTION("RTP packets are not forwarded for unsupported payload types")
	{
		Fixture fixture;

		// Indicate that the transport is connected in order to activate the consumer.
		dynamic_cast<RTC::Consumer*>(fixture.consumer.get())->TransportConnected();

		auto* packet = RTC::RTP::Packet::Parse(buffer, originalPacketLength + 64);
		RTC::RTP::SharedPacket sharedPacket(packet);

		packet->SetPayloadType(payloadType + 1);

		fixture.consumer->SendRtpPacket(packet, sharedPacket);
		fixture.listener->Verify(0);

		delete packet;
	}

	SECTION("RTP packets with empty payload are not forwarded")
	{
		Fixture fixture;

		// Indicate that the transport is connected in order to activate the consumer.
		dynamic_cast<RTC::Consumer*>(fixture.consumer.get())->TransportConnected();

		auto* packet = RTC::RTP::Packet::Parse(buffer, originalPacketLength + 0);
		RTC::RTP::SharedPacket sharedPacket(packet);

		packet->SetPayloadType(payloadType + 1);

		fixture.consumer->SendRtpPacket(packet, sharedPacket);
		fixture.listener->Verify(0);

		delete packet;
	}

	SECTION("outgoing RTP packets are forwarded with increased sequence number")
	{
		Fixture fixture;

		// Indicate that the transport is connected in order to activate the consumer.
		dynamic_cast<RTC::Consumer*>(fixture.consumer.get())->TransportConnected();

		auto* packet = RTC::RTP::Packet::Parse(buffer, originalPacketLength + 64);
		RTC::RTP::SharedPacket sharedPacket(packet);

		uint16_t seq{ 1 };

		packet->SetSequenceNumber(seq++);
		packet->SetPayloadType(payloadType);
		sharedPacket.Assign(packet);

		fixture.consumer->SendRtpPacket(packet, sharedPacket);

		packet->SetSequenceNumber(seq++);
		sharedPacket.Assign(packet);

		fixture.consumer->SendRtpPacket(packet, sharedPacket);

		packet->SetSequenceNumber(seq++);
		sharedPacket.Assign(packet);

		fixture.consumer->SendRtpPacket(packet, sharedPacket);

		packet->SetSequenceNumber(seq++);
		// Remove the payload so it won't be sent.
		packet->RemovePayload();
		sharedPacket.Assign(packet);

		fixture.consumer->SendRtpPacket(packet, sharedPacket);

		fixture.listener->Verify(3);

		delete packet;
	}
}
