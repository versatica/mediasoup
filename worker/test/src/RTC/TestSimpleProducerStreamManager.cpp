#include "DepLibUV.hpp"
#include "RTC/RTP/Codecs/PayloadDescriptorHandler.hpp"
#include "RTC/RTP/RtpStreamRecv.hpp"
#include "RTC/SimpleProducerStreamManager.hpp"
#include "Utils.hpp"
#include "test/include/RTC/RTP/rtpCommon.hpp"
#include "mocks/include/MockShared.hpp"
#include <catch2/catch_test_macros.hpp>

namespace
{
	using RtpPacketProcessResult = RTC::ProducerStreamManager::RtpPacketProcessResult;

	constexpr uint32_t MappedSsrc = 1234567890;

	class MockListener : public RTC::ProducerStreamManager::Listener
	{
	public:
		bool isActive{ true };
		int keyFrameRequestCount{ 0 };
		uint32_t lastKeyFrameRequestedMappedSsrc{ 0 };

		bool IsActive() const override
		{
			return this->isActive;
		}

		void OnProducerStreamManagerKeyFrameRequested(uint32_t mappedSsrc) override
		{
			this->keyFrameRequestCount++;
			this->lastKeyFrameRequestedMappedSsrc = mappedSsrc;
		}

		void OnProducerStreamManagerNeedBitrateChange() override
		{
		}

		void OnProducerStreamManagerLayersChanged() override
		{
		}

		void OnProducerStreamManagerClearRetransmissionBuffer() override
		{
		}

		void OnProducerStreamManagerScore() override
		{
		}
	};

	class RtpStreamRecvListener : public RTC::RTP::RtpStreamRecv::Listener
	{
	public:
		void OnRtpStreamScore(
		  RTC::RTP::RtpStream* /*rtpStream*/, uint8_t /*score*/, uint8_t /*previousScore*/) override
		{
		}

		void OnRtpStreamSendRtcpPacket(
		  RTC::RTP::RtpStreamRecv* /*rtpStream*/, RTC::RTCP::Packet* /*packet*/) override
		{
		}

		void OnRtpStreamNeedWorstRemoteFractionLost(
		  RTC::RTP::RtpStreamRecv* /*rtpStream*/, uint8_t& /*worstRemoteFractionLost*/) override
		{
		}
	};

	class MockEncodingContext : public RTC::RTP::Codecs::EncodingContext
	{
	public:
		MockEncodingContext() : RTC::RTP::Codecs::EncodingContext(MockEncodingContext::params)
		{
		}
		void SyncRequired() override
		{
		}

	private:
		static RTC::RTP::Codecs::EncodingContext::Params params;
	};

	// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
	RTC::RTP::Codecs::EncodingContext::Params MockEncodingContext::params;

	class MockPayloadDescriptorHandler : public RTC::RTP::Codecs::PayloadDescriptorHandler
	{
	public:
		explicit MockPayloadDescriptorHandler()  = default;
		~MockPayloadDescriptorHandler() override = default;
		void Dump(int /*indentation*/) const override
		{
		}
		bool Process(
		  RTC::RTP::Codecs::EncodingContext* /*context*/,
		  RTC::RTP::Packet* /*packet*/,
		  bool& /*marker*/) override
		{
			return this->processResult;
		}
		void RtpPacketChanged(RTC::RTP::Packet* /*packet*/) override
		{
		}
		std::unique_ptr<RTC::RTP::Codecs::PayloadDescriptor::Encoder> GetEncoder() const override
		{
			return nullptr;
		}
		void Encode(
		  RTC::RTP::Packet* /*packet*/, RTC::RTP::Codecs::PayloadDescriptor::Encoder* /*encoder*/) override
		{
		}
		void Restore(RTC::RTP::Packet* /*packet*/) override
		{
		}
		uint8_t GetSpatialLayer() const override
		{
			return 0;
		}
		uint8_t GetTemporalLayer() const override
		{
			return 0;
		}
		bool IsKeyFrame() const override
		{
			return this->isKeyFrame;
		}

	public:
		bool isKeyFrame{ false };
		bool processResult{ true };
	};

	// RtpStreamRecvListener must outlive the RtpStreamRecv.
	RtpStreamRecvListener streamRecvListener; // NOLINT(readability-identifier-naming)

	// NOLINTNEXTLINE(readability-identifier-naming)
	mocks::MockShared shared(/*getTimeMs*/
	                         []()
	                         {
		                         return DepLibUV::GetTimeMs();
	                         }); // NOLINT(readability-identifier-naming)

	std::unique_ptr<RTC::SimpleProducerStreamManager> createManager(
	  MockListener* listener,
	  bool keyFrameSupported                                             = true,
	  RTC::Media::Kind kind                                              = RTC::Media::Kind::VIDEO,
	  std::unique_ptr<RTC::RTP::Codecs::EncodingContext> encodingContext = nullptr)
	{
		RTC::RtpEncodingParameters encoding;
		encoding.ssrc = MappedSsrc;

		const std::vector<RTC::RtpEncodingParameters> consumableRtpEncodings{ encoding };
		const RTC::ConsumerTypes::VideoLayers preferredLayers;

		return std::make_unique<RTC::SimpleProducerStreamManager>(
		  consumableRtpEncodings,
		  preferredLayers,
		  std::move(encodingContext),
		  kind,
		  keyFrameSupported,
		  listener,
		  &shared);
	}

	std::unique_ptr<RTC::RTP::RtpStreamRecv> createRtpStreamRecv()
	{
		RTC::RTP::RtpStream::Params params;

		params.ssrc      = MappedSsrc;
		params.clockRate = 90000;

		return std::make_unique<RTC::RTP::RtpStreamRecv>(&streamRecvListener, &shared, params, 0u, false);
	}

	// Feed packets into the RtpStreamRecv so GetBitrate() returns non-zero.
	void feedRtpStreamRecv(RTC::RTP::RtpStreamRecv* rtpStream, RTC::RTP::Packet* packet, uint16_t count)
	{
		auto firstSeq = static_cast<uint16_t>(packet->GetSequenceNumber() + 1);
		auto lastSeq  = static_cast<uint16_t>(firstSeq + count);

		for (uint16_t seq = firstSeq; Utils::Number::IsLowerThan<uint16_t>(seq, lastSeq); ++seq)
		{
			packet->SetSequenceNumber(seq);
			rtpStream->ReceivePacket(packet);
		}

		auto nowMs = DepLibUV::GetTimeMs();

		// bitrate (bps) = totalBytes * 8000 / windowSizeMs.
		// windowSizeMs for RtpStreamRecv is 2500.
		auto expectedBitrate =
		  static_cast<uint32_t>(std::trunc((count * packet->GetLength() * 8000.0f / 2500) + 0.5f));

		REQUIRE(rtpStream->GetBitrate(nowMs) == expectedBitrate);
	}
} // namespace

SCENARIO("SimpleProducerStreamManager", "[rtp][producerstreammanager][simple]")
{
	std::unique_ptr<RTC::RTP::Packet> packet(
	  RTC::RTP::Packet::Factory(rtpCommon::FactoryBuffer, sizeof(rtpCommon::FactoryBuffer)));

	packet->SetPayloadType(1);
	packet->SetSsrc(MappedSsrc);
	packet->SetPayloadLength(40);

	SECTION("returns BUFFER when sync required and packet is not a keyframe")
	{
		MockListener listener;
		auto manager   = createManager(&listener);
		auto rtpStream = createRtpStreamRecv();

		manager->ProducerRtpStream(rtpStream.get(), MappedSsrc);
		manager->OnTransportConnected();

		auto result = manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ false, /*clockRate*/ 0, /*maxPacketTs*/ 0);

		REQUIRE(result.type == RtpPacketProcessResult::Type::BUFFER);
	}

	SECTION("returns FORWARD with sendBufferedPackets when syncing with a keyframe")
	{
		MockListener listener;
		auto manager   = createManager(&listener);
		auto rtpStream = createRtpStreamRecv();

		manager->ProducerRtpStream(rtpStream.get(), MappedSsrc);
		manager->OnTransportConnected();
		const uint16_t seq{ 100 };

		packet->SetSequenceNumber(seq);

		// packet->SetPayloadDescriptorHandler() will take the ownership.
		auto* payloadDescriptorHandler       = new MockPayloadDescriptorHandler();
		payloadDescriptorHandler->isKeyFrame = true;
		packet->SetPayloadDescriptorHandler(payloadDescriptorHandler);

		auto result = manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ false, /*clockRate*/ 0, /*maxPacketTs*/ 0);

		REQUIRE(result.type == RtpPacketProcessResult::Type::FORWARD);
		REQUIRE(result.isSyncPacket == true);
		REQUIRE(result.syncSeqValue == seq - 1);
		REQUIRE(result.sendBufferedPackets == true);
	}

	SECTION("returns DROP for empty payload packets")
	{
		MockListener listener;
		auto manager   = createManager(&listener, /*keyFrameSupported*/ false);
		auto rtpStream = createRtpStreamRecv();

		manager->ProducerRtpStream(rtpStream.get(), MappedSsrc);
		manager->OnTransportConnected();

		packet->SetSequenceNumber(1);
		packet->RemovePayload();
		auto result = manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ false, /*clockRate*/ 0, /*maxPacketTs*/ 0);

		REQUIRE(result.type == RtpPacketProcessResult::Type::DROP);
	}

	SECTION("returns DROP when ProcessPayload fails")
	{
		MockListener listener;
		auto encodingContext = std::make_unique<MockEncodingContext>();
		auto manager         = createManager(
		  &listener,
		  /*keyFrameSupported*/ false,
		  RTC::Media::Kind::VIDEO,
		  std::move(encodingContext));
		auto rtpStream = createRtpStreamRecv();

		manager->ProducerRtpStream(rtpStream.get(), MappedSsrc);
		manager->OnTransportConnected();

		packet->SetSequenceNumber(1);

		// Set a handler whose Process() returns false.
		auto* handler          = new MockPayloadDescriptorHandler();
		handler->processResult = false;
		packet->SetPayloadDescriptorHandler(handler);

		auto result = manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ false, /*clockRate*/ 0, /*maxPacketTs*/ 0);

		REQUIRE(result.type == RtpPacketProcessResult::Type::DROP);
	}

	SECTION("returns FORWARD and completes sync when keyFrameSupported is false for the first packet")
	{
		MockListener listener;
		auto manager   = createManager(&listener, /*keyFrameSupported*/ false);
		auto rtpStream = createRtpStreamRecv();

		manager->ProducerRtpStream(rtpStream.get(), MappedSsrc);
		manager->OnTransportConnected();

		const uint16_t seq{ 100 };

		packet->SetSequenceNumber(seq);

		auto result = manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ false, /*clockRate*/ 0, /*maxPacketTs*/ 0);

		REQUIRE(result.type == RtpPacketProcessResult::Type::FORWARD);
		REQUIRE(result.isSyncPacket == true);
		REQUIRE(result.syncSeqValue == seq - 1);
		REQUIRE(result.sendBufferedPackets == false);
		REQUIRE(result.tsOffset == 0u);
	}

	SECTION("returns FORWARD for normal packets after sync")
	{
		MockListener listener;
		auto manager   = createManager(&listener, /*keyFrameSupported*/ false);
		auto rtpStream = createRtpStreamRecv();

		manager->ProducerRtpStream(rtpStream.get(), MappedSsrc);
		manager->OnTransportConnected();

		// Complete sync with first packet.

		packet->SetSequenceNumber(1);
		manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ false, /*clockRate*/ 0, /*maxPacketTs*/ 0);

		// Send a second normal packet.
		packet->SetSequenceNumber(2);
		auto result = manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ false, /*clockRate*/ 0, /*maxPacketTs*/ 0);

		REQUIRE(result.type == RtpPacketProcessResult::Type::FORWARD);
		REQUIRE(result.isSyncPacket == false);
		REQUIRE(result.sendBufferedPackets == false);
		REQUIRE(result.tsOffset == 0u);
	}

	SECTION("OnTransportConnected() requests keyframe when active")
	{
		MockListener listener;
		auto manager   = createManager(&listener);
		auto rtpStream = createRtpStreamRecv();

		manager->ProducerRtpStream(rtpStream.get(), MappedSsrc);

		// OnTransportConnected should request a keyframe since the manager is active.
		manager->OnTransportConnected();

		REQUIRE(listener.keyFrameRequestCount == 1);
		REQUIRE(listener.lastKeyFrameRequestedMappedSsrc == MappedSsrc);
	}

	SECTION("OnTransportConnected() does not request keyframe when not active")
	{
		MockListener listener;
		listener.isActive = false;

		auto manager = createManager(&listener);

		// Don't wire producerRtpStream — manager is not active.
		manager->OnTransportConnected();

		REQUIRE(listener.keyFrameRequestCount == 0);
	}

	SECTION("OnResumed() sets syncRequired and requests keyframe")
	{
		MockListener listener;
		auto manager   = createManager(&listener);
		auto rtpStream = createRtpStreamRecv();

		manager->ProducerRtpStream(rtpStream.get(), MappedSsrc);
		manager->OnTransportConnected();

		const int keyFrameCount = listener.keyFrameRequestCount;

		// Call OnResumed — should set syncRequired and request keyframe again.
		manager->OnResumed();

		REQUIRE(listener.keyFrameRequestCount == keyFrameCount + 1);

		// Prove syncRequired was set: sending a non-keyframe returns BUFFER.

		packet->SetSequenceNumber(1);
		auto result = manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ false, /*clockRate*/ 0, /*maxPacketTs*/ 0);

		REQUIRE(result.type == RtpPacketProcessResult::Type::BUFFER);
	}

	SECTION("IncreaseLayer() returns producer bitrate when it is less than available bitrate")
	{
		MockListener listener;
		auto manager   = createManager(&listener);
		auto rtpStream = createRtpStreamRecv();

		manager->ProducerRtpStream(rtpStream.get(), MappedSsrc);
		manager->SetExternallyManagedBitrate();
		manager->OnTransportConnected();

		// Feed packets so the stream has non-zero bitrate.
		feedRtpStreamRecv(rtpStream.get(), packet.get(), 100);

		auto nowMs        = DepLibUV::GetTimeMs();
		auto steamBitrate = rtpStream->GetBitrate(nowMs);
		auto usedBitrate  = manager->IncreaseLayer(
		  /*bitrate*/ steamBitrate + 1u, /*considerLoss*/ false, /*lossPercentage*/ 0.0f, nowMs);

		REQUIRE(usedBitrate == steamBitrate);
	}

	SECTION("IncreaseLayer() returns available bitrate when it is less than producer bitrate")
	{
		MockListener listener;
		auto manager   = createManager(&listener);
		auto rtpStream = createRtpStreamRecv();

		manager->ProducerRtpStream(rtpStream.get(), MappedSsrc);
		manager->SetExternallyManagedBitrate();
		manager->OnTransportConnected();

		// Feed packets so the stream has non-zero bitrate.
		feedRtpStreamRecv(rtpStream.get(), packet.get(), 100);

		auto nowMs                      = DepLibUV::GetTimeMs();
		const auto streamBitrate        = rtpStream->GetBitrate(nowMs);
		const uint32_t availableBitrate = streamBitrate - 1;
		auto usedBitrate                = manager->IncreaseLayer(
		  /*bitrate*/ availableBitrate, /*considerLoss*/ false, /*lossPercentage*/ 0.0f, nowMs);

		REQUIRE(usedBitrate == availableBitrate);
	}

	SECTION("IncreaseLayer() returns 0 on second call in same iteration")
	{
		MockListener listener;
		auto manager   = createManager(&listener);
		auto rtpStream = createRtpStreamRecv();

		manager->ProducerRtpStream(rtpStream.get(), MappedSsrc);
		manager->SetExternallyManagedBitrate();
		manager->OnTransportConnected();

		// Feed packets so the stream has non-zero bitrate.
		feedRtpStreamRecv(rtpStream.get(), packet.get(), 100);

		auto nowMs = DepLibUV::GetTimeMs();

		// First call claims bitrate.
		auto usedBitrate = manager->IncreaseLayer(
		  /*bitrate*/ 1000000u, /*considerLoss*/ false, /*lossPercentage*/ 0.0f, nowMs);

		REQUIRE(usedBitrate > 0u);

		// Second call in same iteration should return 0.
		auto usedBitrate2 = manager->IncreaseLayer(
		  /*bitrate*/ 1000000u, /*considerLoss*/ false, /*lossPercentage*/ 0.0f, nowMs);

		REQUIRE(usedBitrate2 == 0u);
	}

	SECTION("IncreaseLayer() works again after ApplyLayers()")
	{
		MockListener listener;
		auto manager   = createManager(&listener);
		auto rtpStream = createRtpStreamRecv();

		manager->ProducerRtpStream(rtpStream.get(), MappedSsrc);
		manager->SetExternallyManagedBitrate();
		manager->OnTransportConnected();

		// Feed packets so the stream has non-zero bitrate.
		feedRtpStreamRecv(rtpStream.get(), packet.get(), 100);

		auto nowMs = DepLibUV::GetTimeMs();

		// First iteration: claim bitrate and apply.
		manager->IncreaseLayer(
		  /*bitrate*/ 1000000u, /*considerLoss*/ false, /*lossPercentage*/ 0.0f, nowMs);
		manager->ApplyLayers(/*rtpStreamActiveMs*/ 0u);

		// After ApplyLayers, IncreaseLayer should work again.
		auto usedBitrate = manager->IncreaseLayer(
		  /*bitrate*/ 1000000u, /*considerLoss*/ false, /*lossPercentage*/ 0.0f, nowMs);

		REQUIRE(usedBitrate > 0u);
	}

	SECTION("GetDesiredBitrate() returns producer bitrate for video")
	{
		MockListener listener;
		auto manager   = createManager(&listener);
		auto rtpStream = createRtpStreamRecv();

		manager->ProducerRtpStream(rtpStream.get(), MappedSsrc);
		manager->SetExternallyManagedBitrate();
		manager->OnTransportConnected();

		// Feed packets so the stream has non-zero bitrate.
		feedRtpStreamRecv(rtpStream.get(), packet.get(), 100);

		auto nowMs          = DepLibUV::GetTimeMs();
		auto steamBitrate   = rtpStream->GetBitrate(nowMs);
		auto desiredBitrate = manager->GetDesiredBitrate(nowMs);

		REQUIRE(desiredBitrate == steamBitrate);
	}

	SECTION("GetDesiredBitrate() returns 0 for audio kind")
	{
		MockListener listener;
		auto manager   = createManager(&listener, /*keyFrameSupported*/ false, RTC::Media::Kind::AUDIO);
		auto rtpStream = createRtpStreamRecv();

		manager->ProducerRtpStream(rtpStream.get(), MappedSsrc);
		manager->SetExternallyManagedBitrate();
		manager->OnTransportConnected();

		// Feed packets so the stream has non-zero bitrate.
		feedRtpStreamRecv(rtpStream.get(), packet.get(), 100);

		auto nowMs          = DepLibUV::GetTimeMs();
		auto desiredBitrate = manager->GetDesiredBitrate(nowMs);

		REQUIRE(desiredBitrate == 0u);
	}
}
