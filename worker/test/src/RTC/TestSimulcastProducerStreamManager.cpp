#include "DepLibUV.hpp"
#include "RTC/RTCP/SenderReport.hpp"
#include "RTC/RTP/Codecs/PayloadDescriptorHandler.hpp"
#include "RTC/RTP/RtpStreamRecv.hpp"
#include "RTC/SimulcastProducerStreamManager.hpp"
#include "Utils.hpp"
#include "test/include/RTC/RTP/rtpCommon.hpp"
#include "mocks/include/MockShared.hpp"

namespace
{
	constexpr uint32_t MappedSsrc0 = 1000;
	constexpr uint32_t MappedSsrc1 = 2000;
	constexpr uint32_t MappedSsrc2 = 3000;

	const std::vector<uint32_t> ThreeSsrcs = { MappedSsrc0, MappedSsrc1, MappedSsrc2 };
	const std::vector<uint32_t> TwoSsrcs   = { MappedSsrc0, MappedSsrc1 };
	const std::vector<uint32_t> OneSsrc    = { MappedSsrc0 };

	class MockListener : public RTC::ProducerStreamManager::Listener
	{
	public:
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
			this->needBitrateChangeCount++;
		}

		void OnProducerStreamManagerLayersChanged() override
		{
			this->layersChangedCount++;
		}

		void OnProducerStreamManagerClearRetransmissionBuffer() override
		{
		}

		void OnProducerStreamManagerScore() override
		{
		}

	public:
		bool isActive{ true };
		int keyFrameRequestCount{ 0 };
		uint32_t lastKeyFrameRequestedMappedSsrc{ 0 };
		int layersChangedCount{ 0 };
		int needBitrateChangeCount{ 0 };
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

		uint8_t OnRtpStreamNeedWorstRemoteFractionLost(RTC::RTP::RtpStreamRecv* /*rtpStream*/) override
		{
			return 0;
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

	std::unique_ptr<RTC::SimulcastProducerStreamManager> createManager(
	  MockListener* listener,
	  const std::vector<uint32_t>& ssrcs                                 = ThreeSsrcs,
	  RTC::ConsumerTypes::VideoLayers preferredLayers                    = { 2, 2 },
	  bool keyFrameSupported                                             = true,
	  RTC::Media::Kind kind                                              = RTC::Media::Kind::VIDEO,
	  std::unique_ptr<RTC::RTP::Codecs::EncodingContext> encodingContext = nullptr)
	{
		std::vector<RTC::RtpEncodingParameters> consumableRtpEncodings;

		for (auto ssrc : ssrcs)
		{
			RTC::RtpEncodingParameters encoding;
			encoding.ssrc = ssrc;
			consumableRtpEncodings.push_back(encoding);
		}

		if (!encodingContext)
		{
			encodingContext = std::make_unique<MockEncodingContext>();
		}

		return std::make_unique<RTC::SimulcastProducerStreamManager>(
		  consumableRtpEncodings,
		  preferredLayers,
		  std::move(encodingContext),
		  kind,
		  keyFrameSupported,
		  listener,
		  &shared);
	}

	std::unique_ptr<RTC::RTP::RtpStreamRecv> createRtpStreamRecv(uint32_t ssrc)
	{
		RTC::RTP::RtpStream::Params params;

		params.ssrc      = ssrc;
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

SCENARIO("SimulcastProducerStreamManager", "[rtp][producerstreammanager][simulcast]")
{
	std::unique_ptr<RTC::RTP::Packet> packet(
	  RTC::RTP::Packet::Factory(rtpCommon::FactoryBuffer, sizeof(rtpCommon::FactoryBuffer)));

	packet->SetPayloadType(1);
	packet->SetSsrc(MappedSsrc0);
	packet->SetTimestamp(1000);
	packet->SetPayloadLength(40);

	SECTION("ProcessRtpPacket() returns DROP when target temporal layer is -1")
	{
		MockListener listener;
		auto manager    = createManager(&listener);
		auto rtpStream0 = createRtpStreamRecv(MappedSsrc0);

		manager->ProducerRtpStream(rtpStream0.get(), MappedSsrc0);

		// Set target layer to 0, complete sync.
		manager->UpdateTargetLayers(0, 0);

		packet->SetSsrc(MappedSsrc0);
		packet->SetSequenceNumber(1);

		auto* handler       = new MockPayloadDescriptorHandler();
		handler->isKeyFrame = true;
		packet->SetPayloadDescriptorHandler(handler);

		manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ false, /*clockRate*/ 90000, /*maxPacketTs*/ 0);

		// Reset target layers to -1,-1 (simulates pause/disconnect).
		manager->UpdateTargetLayers(-1, -1);

		// Packet from previous current layer should be dropped.
		packet->SetSequenceNumber(2);
		auto result = manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ false, /*clockRate*/ 90000, /*maxPacketTs*/ 0);

		REQUIRE(result.type == RTC::ProducerStreamManager::RtpPacketProcessResult::Type::SILENT_DROP);
	}

	SECTION(
	  "ProcessRtpPacket() returns BUFFER when sync required and packet is from target spatial layer but not keyframe")
	{
		MockListener listener;
		auto manager    = createManager(&listener);
		auto rtpStream0 = createRtpStreamRecv(MappedSsrc0);

		manager->ProducerRtpStream(rtpStream0.get(), MappedSsrc0);

		// Set target layer to 0. This sets syncRequired since target != current.
		manager->UpdateTargetLayers(0, 0);

		packet->SetSsrc(MappedSsrc0);
		packet->SetSequenceNumber(1);

		auto result = manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ false, /*clockRate*/ 90000, /*maxPacketTs*/ 0);

		REQUIRE(result.type == RTC::ProducerStreamManager::RtpPacketProcessResult::Type::BUFFER);
	}

	SECTION("ProcessRtpPacket() requires keyframe for spatial layer switch")
	{
		MockListener listener;
		auto manager    = createManager(&listener);
		auto rtpStream0 = createRtpStreamRecv(MappedSsrc0);
		auto rtpStream1 = createRtpStreamRecv(MappedSsrc1);

		manager->ProducerRtpStream(rtpStream0.get(), MappedSsrc0);
		manager->ProducerRtpStream(rtpStream1.get(), MappedSsrc1);

		// Set target layer to 0 and complete sync.
		manager->UpdateTargetLayers(0, 0);

		packet->SetSsrc(MappedSsrc0);
		packet->SetSequenceNumber(1);

		auto* handler0       = new MockPayloadDescriptorHandler();
		handler0->isKeyFrame = true;
		packet->SetPayloadDescriptorHandler(handler0);

		manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ false, /*clockRate*/ 90000, /*maxPacketTs*/ 0);

		// Now switch target layer to 1.
		manager->UpdateTargetLayers(1, 0);

		// Send non-keyframe from layer 1 — should be buffered.
		packet->SetSsrc(MappedSsrc1);
		packet->SetSequenceNumber(1);

		// Explicitly set a non-keyframe handler.
		auto* handler1       = new MockPayloadDescriptorHandler();
		handler1->isKeyFrame = false;
		packet->SetPayloadDescriptorHandler(handler1);

		auto result = manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ false, /*clockRate*/ 90000, /*maxPacketTs*/ 0);

		REQUIRE(result.type == RTC::ProducerStreamManager::RtpPacketProcessResult::Type::BUFFER);

		// Send keyframe from layer 1 — should switch and forward.
		packet->SetSequenceNumber(2);

		auto* handler2       = new MockPayloadDescriptorHandler();
		handler2->isKeyFrame = true;
		packet->SetPayloadDescriptorHandler(handler2);

		result = manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ false, /*clockRate*/ 90000, /*maxPacketTs*/ 0);

		REQUIRE(result.type == RTC::ProducerStreamManager::RtpPacketProcessResult::Type::FORWARD);
		REQUIRE(result.isSyncPacket == true);
		REQUIRE(result.spatialLayerSwitched == true);
		REQUIRE(result.sendBufferedPackets == true);
		REQUIRE(manager->GetCurrentSpatialLayer() == 1);
	}

	SECTION("ProcessRtpPacket() returns SILENT_DROP for non-target and non-current spatial layer packet")
	{
		MockListener listener;
		auto manager    = createManager(&listener);
		auto rtpStream0 = createRtpStreamRecv(MappedSsrc0);

		manager->ProducerRtpStream(rtpStream0.get(), MappedSsrc0);

		// Set target to spatial layer 0.
		manager->UpdateTargetLayers(0, 0);

		// Sync on layer 0 with a keyframe.
		packet->SetSsrc(MappedSsrc0);
		packet->SetSequenceNumber(1);

		auto* handler0       = new MockPayloadDescriptorHandler();
		handler0->isKeyFrame = true;
		packet->SetPayloadDescriptorHandler(handler0);

		auto result = manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ false, /*clockRate*/ 90000, /*maxPacketTs*/ 0);

		REQUIRE(result.type == RTC::ProducerStreamManager::RtpPacketProcessResult::Type::FORWARD);

		// Now send a packet from spatial layer 1 — should be silently dropped.
		packet->SetSsrc(MappedSsrc1);
		packet->SetSequenceNumber(1);

		result = manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ false, /*clockRate*/ 90000, /*maxPacketTs*/ 0);

		REQUIRE(result.type == RTC::ProducerStreamManager::RtpPacketProcessResult::Type::SILENT_DROP);
	}

	SECTION(
	  "ProcessRtpPacket() returns SILENT_DROP when sync required and packet is from non-target spatial layer")
	{
		MockListener listener;
		auto manager    = createManager(&listener, /*ssrcs*/ TwoSsrcs);
		auto rtpStream0 = createRtpStreamRecv(MappedSsrc0);
		auto rtpStream1 = createRtpStreamRecv(MappedSsrc1);

		manager->ProducerRtpStream(rtpStream0.get(), MappedSsrc0);
		manager->ProducerRtpStream(rtpStream1.get(), MappedSsrc1);

		// Set target layer to 0. This sets syncRequired since target != current.
		manager->UpdateTargetLayers(0, 0);

		// Send a packet from layer 1 (non-target) while sync is pending.
		packet->SetSsrc(MappedSsrc1);
		packet->SetSequenceNumber(1);

		auto result = manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ false, /*clockRate*/ 90000, /*maxPacketTs*/ 0);

		REQUIRE(result.type == RTC::ProducerStreamManager::RtpPacketProcessResult::Type::SILENT_DROP);
	}

	SECTION("ProcessRtpPacket() returns FORWARD with sendBufferedPackets when syncing with a keyframe")
	{
		MockListener listener;
		auto manager    = createManager(&listener);
		auto rtpStream0 = createRtpStreamRecv(MappedSsrc0);

		manager->ProducerRtpStream(rtpStream0.get(), MappedSsrc0);
		manager->UpdateTargetLayers(0, 0);

		const uint16_t seq{ 100 };

		packet->SetSsrc(MappedSsrc0);
		packet->SetSequenceNumber(seq);

		auto* handler       = new MockPayloadDescriptorHandler();
		handler->isKeyFrame = true;
		packet->SetPayloadDescriptorHandler(handler);

		auto result = manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ false, /*clockRate*/ 90000, /*maxPacketTs*/ 0);

		REQUIRE(result.type == RTC::ProducerStreamManager::RtpPacketProcessResult::Type::FORWARD);
		REQUIRE(result.isSyncPacket == true);
		REQUIRE(result.sendBufferedPackets == true);
	}

	SECTION("ProcessRtpPacket() returns FORWARD and completes sync when keyFrameSupported is false")
	{
		MockListener listener;
		auto manager = createManager(
		  &listener,
		  /*ssrcs*/ ThreeSsrcs,
		  /*preferredLayers*/ { 2, 2 },
		  /*keyFrameSupported*/ false);
		auto rtpStream0 = createRtpStreamRecv(MappedSsrc0);

		manager->ProducerRtpStream(rtpStream0.get(), MappedSsrc0);
		manager->UpdateTargetLayers(0, 0);

		const uint16_t seq{ 100 };

		packet->SetSsrc(MappedSsrc0);
		packet->SetSequenceNumber(seq);

		auto result = manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ false, /*clockRate*/ 90000, /*maxPacketTs*/ 0);

		REQUIRE(result.type == RTC::ProducerStreamManager::RtpPacketProcessResult::Type::FORWARD);
		REQUIRE(result.isSyncPacket == true);
		REQUIRE(result.tsOffset == 0u);
	}

	SECTION("ProcessRtpPacket() returns DROP for empty payload packets on current spatial layer")
	{
		MockListener listener;
		auto manager = createManager(
		  &listener,
		  /*ssrcs*/ ThreeSsrcs,
		  /*preferredLayers*/ { 2, 2 },
		  /*keyFrameSupported*/ false);
		auto rtpStream0 = createRtpStreamRecv(MappedSsrc0);

		manager->ProducerRtpStream(rtpStream0.get(), MappedSsrc0);
		manager->UpdateTargetLayers(0, 0);

		// Complete sync.
		packet->SetSsrc(MappedSsrc0);
		packet->SetSequenceNumber(1);
		manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ false, /*clockRate*/ 90000, /*maxPacketTs*/ 0);

		// Send empty payload packet on current layer.
		packet->SetSequenceNumber(2);
		packet->RemovePayload();
		auto result = manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ false, /*clockRate*/ 90000, /*maxPacketTs*/ 0);

		REQUIRE(result.type == RTC::ProducerStreamManager::RtpPacketProcessResult::Type::DROP);
	}

	SECTION(
	  "ProcessRtpPacket() returns SILENT_DROP for empty payload packets on non-current spatial layer")
	{
		MockListener listener;
		auto manager = createManager(
		  &listener,
		  /*ssrcs*/ ThreeSsrcs,
		  /*preferredLayers*/ { 2, 2 },
		  /*keyFrameSupported*/ false);
		auto rtpStream0 = createRtpStreamRecv(MappedSsrc0);

		manager->ProducerRtpStream(rtpStream0.get(), MappedSsrc0);
		manager->UpdateTargetLayers(0, 0);

		// Complete sync on layer 0.
		packet->SetSsrc(MappedSsrc0);
		packet->SetSequenceNumber(1);
		manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ false, /*clockRate*/ 90000, /*maxPacketTs*/ 0);

		// Send empty payload packet on non-current layer.
		packet->SetSsrc(MappedSsrc1);
		packet->SetSequenceNumber(1);
		packet->RemovePayload();
		auto result = manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ false, /*clockRate*/ 90000, /*maxPacketTs*/ 0);

		REQUIRE(result.type == RTC::ProducerStreamManager::RtpPacketProcessResult::Type::SILENT_DROP);
	}

	SECTION("ProcessRtpPacket() returns FORWARD for current layer packets after sync")
	{
		MockListener listener;
		auto manager = createManager(
		  &listener,
		  /*ssrcs*/ ThreeSsrcs,
		  /*preferredLayers*/ { 2, 2 },
		  /*keyFrameSupported*/ false);
		auto rtpStream0 = createRtpStreamRecv(MappedSsrc0);

		manager->ProducerRtpStream(rtpStream0.get(), MappedSsrc0);
		manager->UpdateTargetLayers(0, 0);

		// Complete sync with first packet.
		packet->SetSsrc(MappedSsrc0);
		packet->SetSequenceNumber(1);
		manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ false, /*clockRate*/ 90000, /*maxPacketTs*/ 0);

		// Send a second normal packet.
		packet->SetSequenceNumber(2);
		auto result = manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ false, /*clockRate*/ 90000, /*maxPacketTs*/ 0);

		REQUIRE(result.type == RTC::ProducerStreamManager::RtpPacketProcessResult::Type::FORWARD);
		REQUIRE(result.isSyncPacket == false);
		REQUIRE(result.sendBufferedPackets == false);
		REQUIRE(result.tsOffset == 0u);
	}

	SECTION("ProcessRtpPacket() forwards packets from current layer after spatial switch")
	{
		MockListener listener;
		auto manager = createManager(
		  &listener,
		  /*ssrcs*/ TwoSsrcs,
		  /*preferredLayers*/ { 1, 0 },
		  /*keyFrameSupported*/ false);
		auto rtpStream0 = createRtpStreamRecv(MappedSsrc0);
		auto rtpStream1 = createRtpStreamRecv(MappedSsrc1);

		manager->ProducerRtpStream(rtpStream0.get(), MappedSsrc0);
		manager->ProducerRtpStream(rtpStream1.get(), MappedSsrc1);

		// Set target layer to 0 and sync.
		manager->UpdateTargetLayers(0, 0);

		packet->SetSsrc(MappedSsrc0);
		packet->SetSequenceNumber(1);
		manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ false, /*clockRate*/ 90000, /*maxPacketTs*/ 0);

		REQUIRE(manager->GetCurrentSpatialLayer() == 0);

		// Switch to layer 1 — keyFrameSupported=false so first packet syncs.
		manager->UpdateTargetLayers(1, 0);

		packet->SetSsrc(MappedSsrc1);
		packet->SetSequenceNumber(1);
		auto result = manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ false, /*clockRate*/ 90000, /*maxPacketTs*/ 0);

		REQUIRE(result.type == RTC::ProducerStreamManager::RtpPacketProcessResult::Type::FORWARD);
		REQUIRE(result.spatialLayerSwitched == true);
		REQUIRE(manager->GetCurrentSpatialLayer() == 1);

		// Subsequent packets on layer 1 should be forwarded normally.
		packet->SetSequenceNumber(2);
		result = manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ false, /*clockRate*/ 90000, /*maxPacketTs*/ 0);

		REQUIRE(result.type == RTC::ProducerStreamManager::RtpPacketProcessResult::Type::FORWARD);
		REQUIRE(result.spatialLayerSwitched == false);

		// Packets from old layer 0 should be silently dropped.
		packet->SetSsrc(MappedSsrc0);
		packet->SetSequenceNumber(2);
		result = manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ false, /*clockRate*/ 90000, /*maxPacketTs*/ 0);

		REQUIRE(result.type == RTC::ProducerStreamManager::RtpPacketProcessResult::Type::SILENT_DROP);
	}

	SECTION("OnTransportConnected() sets syncRequired")
	{
		MockListener listener;
		auto manager = createManager(
		  &listener,
		  /*ssrcs*/ OneSsrc,
		  /*preferredLayers*/ { 0, 0 },
		  /*keyFrameSupported*/ false);
		auto rtpStream0 = createRtpStreamRecv(MappedSsrc0);

		manager->ProducerRtpStream(rtpStream0.get(), MappedSsrc0);

		// Give score > 0 so RecalculateTargetLayers() keeps target at layer 0.
		manager->ProducerRtpStreamScore(rtpStream0.get(), /*score*/ 7, /*previousScore*/ 0);

		// Set target and sync on layer 0.
		manager->UpdateTargetLayers(0, 0);

		packet->SetSsrc(MappedSsrc0);
		packet->SetSequenceNumber(1);
		manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ false, /*clockRate*/ 90000, /*maxPacketTs*/ 0);

		// Verify we are synced: normal packet is forwarded without sync flag.
		packet->SetSequenceNumber(2);
		auto result = manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ false, /*clockRate*/ 90000, /*maxPacketTs*/ 0);

		REQUIRE(result.type == RTC::ProducerStreamManager::RtpPacketProcessResult::Type::FORWARD);
		REQUIRE(result.isSyncPacket == false);

		// Now OnTransportConnected resets syncRequired.
		manager->OnTransportConnected();

		// Next packet should be a sync packet.
		packet->SetSequenceNumber(3);
		result = manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ false, /*clockRate*/ 90000, /*maxPacketTs*/ 0);

		REQUIRE(result.type == RTC::ProducerStreamManager::RtpPacketProcessResult::Type::FORWARD);
		REQUIRE(result.isSyncPacket == true);
	}

	SECTION("OnTransportConnected() requests keyframe when active and target changes")
	{
		MockListener listener;
		auto manager    = createManager(&listener, /*ssrcs*/ TwoSsrcs, /*preferredLayers*/ { 1, 0 });
		auto rtpStream0 = createRtpStreamRecv(MappedSsrc0);
		auto rtpStream1 = createRtpStreamRecv(MappedSsrc1);

		manager->ProducerRtpStream(rtpStream0.get(), MappedSsrc0);
		manager->ProducerRtpStream(rtpStream1.get(), MappedSsrc1);

		// Both streams have score > 0 so RecalculateTargetLayers() can pick layers.
		manager->ProducerRtpStreamScore(rtpStream0.get(), /*score*/ 7, /*previousScore*/ 0);
		manager->ProducerRtpStreamScore(rtpStream1.get(), /*score*/ 7, /*previousScore*/ 0);

		// Sync on layer 1 with a keyframe.
		manager->UpdateTargetLayers(1, 0);

		packet->SetSsrc(MappedSsrc1);
		packet->SetSequenceNumber(1);

		auto* handler       = new MockPayloadDescriptorHandler();
		handler->isKeyFrame = true;
		packet->SetPayloadDescriptorHandler(handler);

		manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ false, /*clockRate*/ 90000, /*maxPacketTs*/ 0);

		// current=1, target=1. Disconnect resets everything to -1.
		manager->OnTransportDisconnected();

		auto keyFrameCountBefore = listener.keyFrameRequestCount;

		// Reconnect: RecalculateTargetLayers() picks preferred layer 1,
		// which differs from current (-1), so a keyframe is requested.
		manager->OnTransportConnected();

		REQUIRE(listener.keyFrameRequestCount > keyFrameCountBefore);
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

	SECTION("RecalculateTargetLayers() skips spatial layer without capture instant")
	{
		MockListener listener;
		auto manager    = createManager(&listener, /*ssrcs*/ TwoSsrcs, /*preferredLayers*/ { 1, 0 });
		auto rtpStream0 = createRtpStreamRecv(MappedSsrc0);
		auto rtpStream1 = createRtpStreamRecv(MappedSsrc1);

		manager->ProducerRtpStream(rtpStream0.get(), MappedSsrc0);
		manager->ProducerRtpStream(rtpStream1.get(), MappedSsrc1);

		// Set target layer to 0. This sets tsReferenceSpatialLayer = 0.
		manager->UpdateTargetLayers(0, 0);

		// Sync on layer 0 with a keyframe so current = 0.
		packet->SetSsrc(MappedSsrc0);
		packet->SetSequenceNumber(1);

		auto* handler       = new MockPayloadDescriptorHandler();
		handler->isKeyFrame = true;
		packet->SetPayloadDescriptorHandler(handler);

		manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ false, /*clockRate*/ 90000, /*maxPacketTs*/ 0);

		REQUIRE(manager->GetCurrentSpatialLayer() == 0);
		REQUIRE(manager->GetTargetLayers().temporal == 0);

		// Give both streams score > 0. RecalculateTargetLayers() will be called
		// but CanSwitchToSpatialLayer(1) returns false because the capture instant of
		// layer 1 cannot be told and tsReferenceSpatialLayer is 0 (not -1).
		manager->ProducerRtpStreamScore(rtpStream0.get(), /*score*/ 7, /*previousScore*/ 0);

		REQUIRE(manager->GetTargetLayers().spatial == 0);
		REQUIRE(manager->GetTargetLayers().temporal == 0);

		manager->ProducerRtpStreamScore(rtpStream1.get(), /*score*/ 7, /*previousScore*/ 0);

		// Despite preferred being layer 1, target stays at layer 0.
		REQUIRE(manager->GetTargetLayers().spatial == 0);
		REQUIRE(manager->GetTargetLayers().temporal == 0);

		// Verify packets on layer 0 are still forwarded with tsOffset 0.
		packet->SetSsrc(MappedSsrc0);
		packet->SetSequenceNumber(2);

		auto result = manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ false, /*clockRate*/ 90000, /*maxPacketTs*/ 0);

		REQUIRE(result.type == RTC::ProducerStreamManager::RtpPacketProcessResult::Type::FORWARD);
		REQUIRE(result.tsOffset == 0u);
	}

	SECTION("RecalculateTargetLayers() switches to preferred layer once its capture instant is known")
	{
		MockListener listener;
		auto manager    = createManager(&listener, /*ssrcs*/ TwoSsrcs, /*preferredLayers*/ { 1, 0 });
		auto rtpStream0 = createRtpStreamRecv(MappedSsrc0);
		auto rtpStream1 = createRtpStreamRecv(MappedSsrc1);

		manager->ProducerRtpStream(rtpStream0.get(), MappedSsrc0);
		manager->ProducerRtpStream(rtpStream1.get(), MappedSsrc1);

		// Set target layer to 0. This sets tsReferenceSpatialLayer = 0.
		manager->UpdateTargetLayers(0, 0);

		// Sync on layer 0 with a keyframe so current = 0.
		packet->SetSsrc(MappedSsrc0);
		packet->SetSequenceNumber(1);

		auto* handler       = new MockPayloadDescriptorHandler();
		handler->isKeyFrame = true;
		packet->SetPayloadDescriptorHandler(handler);

		manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ false, /*clockRate*/ 90000, /*maxPacketTs*/ 0);

		// Target stays at 0 because the capture instant of layer 1 cannot be told.
		REQUIRE(manager->GetTargetLayers().spatial == 0);

		// Both layers hold RTP timestamp 1000 and both were captured at the same instant,
		// which is what makes them alignable to each other.
		rtpStream0->SetCaptureMapping(/*captureMs*/ 1000, /*ts*/ 1000);
		rtpStream1->SetCaptureMapping(/*captureMs*/ 1000, /*ts*/ 1000);

		// Feed packets to both streams so ReceiveRtcpSenderReport's UpdateScore
		// doesn't drop the score to 0.
		packet->SetSsrc(MappedSsrc0);
		feedRtpStreamRecv(rtpStream0.get(), packet.get(), 10);

		packet->SetSsrc(MappedSsrc1);
		feedRtpStreamRecv(rtpStream1.get(), packet.get(), 10);

		// Provide Sender Reports for both streams so that they get a score.
		RTC::RTCP::SenderReport sr0;
		sr0.SetSsrc(MappedSsrc0);
		sr0.SetNtpSec(1000);
		sr0.SetNtpFrac(0);
		sr0.SetRtpTs(90000);
		rtpStream0->ReceiveRtcpSenderReport(&sr0);

		RTC::RTCP::SenderReport sr1;
		sr1.SetSsrc(MappedSsrc1);
		sr1.SetNtpSec(1000);
		sr1.SetNtpFrac(0);
		sr1.SetRtpTs(90000);
		rtpStream1->ReceiveRtcpSenderReport(&sr1);

		// Notify the manager about the first Sender Report for layer 1.
		// This triggers MayChangeLayers() -> RecalculateTargetLayers().
		// Now CanSwitchToSpatialLayer(1) returns true, so preferred layer 1 is picked.
		manager->ProducerRtcpSenderReport(rtpStream1.get(), /*first*/ true);

		REQUIRE(manager->GetTargetLayers().spatial == 1);

		// Send a keyframe from layer 1 to complete the spatial switch.
		packet->SetSsrc(MappedSsrc1);
		packet->SetSequenceNumber(100);

		auto* handler1       = new MockPayloadDescriptorHandler();
		handler1->isKeyFrame = true;
		packet->SetPayloadDescriptorHandler(handler1);

		auto result = manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ false, /*clockRate*/ 90000, /*maxPacketTs*/ 0);

		REQUIRE(result.type == RTC::ProducerStreamManager::RtpPacketProcessResult::Type::FORWARD);
		REQUIRE(result.isSyncPacket == true);
		REQUIRE(result.spatialLayerSwitched == true);
		// Both streams have the same capture instant and RTP timestamp, so tsOffset is 0.
		REQUIRE(result.tsOffset == 0u);
		REQUIRE(manager->GetCurrentSpatialLayer() == 1);
	}

	SECTION("ProcessRtpPacket() calculates tsOffset from the capture instant of both layers")
	{
		MockListener listener;
		auto manager    = createManager(&listener, /*ssrcs*/ TwoSsrcs, /*preferredLayers*/ { 1, 0 });
		auto rtpStream0 = createRtpStreamRecv(MappedSsrc0);
		auto rtpStream1 = createRtpStreamRecv(MappedSsrc1);

		manager->ProducerRtpStream(rtpStream0.get(), MappedSsrc0);
		manager->ProducerRtpStream(rtpStream1.get(), MappedSsrc1);

		// Set target layer to 0. This sets tsReferenceSpatialLayer = 0.
		manager->UpdateTargetLayers(0, 0);

		// Sync on layer 0 with a keyframe so current = 0.
		packet->SetSsrc(MappedSsrc0);
		packet->SetSequenceNumber(1);

		auto* handler       = new MockPayloadDescriptorHandler();
		handler->isKeyFrame = true;
		packet->SetPayloadDescriptorHandler(handler);

		manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ false, /*clockRate*/ 90000, /*maxPacketTs*/ 0);

		// Both layers hold RTP timestamp 1000, but the one of layer 1 was captured 500 ms
		// before the one of layer 0, so its RTP timeline runs 45000 ticks ahead.
		rtpStream0->SetCaptureMapping(/*captureMs*/ 1000, /*ts*/ 1000);
		rtpStream1->SetCaptureMapping(/*captureMs*/ 500, /*ts*/ 1000);

		packet->SetSsrc(MappedSsrc0);
		feedRtpStreamRecv(rtpStream0.get(), packet.get(), 10);

		packet->SetSsrc(MappedSsrc1);
		feedRtpStreamRecv(rtpStream1.get(), packet.get(), 10);

		// Provide Sender Reports for both streams so that they get a score.
		RTC::RTCP::SenderReport sr0;
		sr0.SetSsrc(MappedSsrc0);
		sr0.SetNtpSec(1000);
		sr0.SetNtpFrac(0);
		sr0.SetRtpTs(90000);
		rtpStream0->ReceiveRtcpSenderReport(&sr0);

		RTC::RTCP::SenderReport sr1;
		sr1.SetSsrc(MappedSsrc1);
		sr1.SetNtpSec(1000);
		sr1.SetNtpFrac(0);
		sr1.SetRtpTs(90000);
		rtpStream1->ReceiveRtcpSenderReport(&sr1);

		manager->ProducerRtcpSenderReport(rtpStream1.get(), /*first*/ true);

		REQUIRE(manager->GetTargetLayers().spatial == 1);

		// Send a keyframe from layer 1 to complete the spatial switch.
		packet->SetSsrc(MappedSsrc1);
		packet->SetSequenceNumber(100);
		packet->SetTimestamp(91000);

		auto* handler1       = new MockPayloadDescriptorHandler();
		handler1->isKeyFrame = true;
		packet->SetPayloadDescriptorHandler(handler1);

		auto result = manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ false, /*clockRate*/ 90000, /*maxPacketTs*/ 0);

		REQUIRE(result.type == RTC::ProducerStreamManager::RtpPacketProcessResult::Type::FORWARD);
		REQUIRE(result.spatialLayerSwitched == true);
		REQUIRE(result.tsOffset == 45000u);
		REQUIRE(manager->GetCurrentSpatialLayer() == 1);
	}

	SECTION(
	  "ProcessRtpPacket() discards the packet while its spatial layer cannot tell its capture instant")
	{
		MockListener listener;
		auto manager = createManager(
		  &listener,
		  /*ssrcs*/ TwoSsrcs,
		  /*preferredLayers*/ { 1, 0 },
		  /*keyFrameSupported*/ false);
		auto rtpStream0 = createRtpStreamRecv(MappedSsrc0);
		auto rtpStream1 = createRtpStreamRecv(MappedSsrc1);

		manager->ProducerRtpStream(rtpStream0.get(), MappedSsrc0);
		manager->ProducerRtpStream(rtpStream1.get(), MappedSsrc1);

		// Only layer 0 can tell its capture instant, so it remains the TS reference one and
		// layer 1 cannot be aligned to it yet.
		rtpStream0->SetCaptureMapping(/*captureMs*/ 1000, /*ts*/ 1000);

		// Set target layer to 0 and sync. This sets tsReferenceSpatialLayer = 0.
		manager->UpdateTargetLayers(0, 0);

		packet->SetSsrc(MappedSsrc0);
		packet->SetSequenceNumber(1);
		manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ false, /*clockRate*/ 90000, /*maxPacketTs*/ 0);

		REQUIRE(manager->GetCurrentSpatialLayer() == 0);

		// Aim at layer 1, whose RTP timeline cannot be placed on the one of layer 0 yet.
		manager->UpdateTargetLayers(1, 0);

		const auto keyFrameRequestCountBefore = listener.keyFrameRequestCount;

		packet->SetSsrc(MappedSsrc1);
		packet->SetSequenceNumber(1);

		auto result = manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ false, /*clockRate*/ 90000, /*maxPacketTs*/ 0);

		REQUIRE(result.type == RTC::ProducerStreamManager::RtpPacketProcessResult::Type::SILENT_DROP);
		REQUIRE(manager->GetCurrentSpatialLayer() == 0);
		// The discarded packet is not a key frame, so no key frame is requested for it.
		REQUIRE(listener.keyFrameRequestCount == keyFrameRequestCountBefore);

		// Once layer 1 can tell its capture instant the switch completes with the proper
		// offset.
		rtpStream1->SetCaptureMapping(/*captureMs*/ 500, /*ts*/ 1000);

		packet->SetSequenceNumber(2);

		result = manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ false, /*clockRate*/ 90000, /*maxPacketTs*/ 0);

		REQUIRE(result.type == RTC::ProducerStreamManager::RtpPacketProcessResult::Type::FORWARD);
		REQUIRE(result.spatialLayerSwitched == true);
		REQUIRE(result.tsOffset == 45000u);
		REQUIRE(manager->GetCurrentSpatialLayer() == 1);
	}

	SECTION(
	  "ProcessRtpPacket() takes the spatial layer as TS reference when the TS reference one "
	  "cannot tell its capture instant")
	{
		MockListener listener;
		auto manager = createManager(
		  &listener,
		  /*ssrcs*/ TwoSsrcs,
		  /*preferredLayers*/ { 1, 0 },
		  /*keyFrameSupported*/ false);
		auto rtpStream0 = createRtpStreamRecv(MappedSsrc0);
		auto rtpStream1 = createRtpStreamRecv(MappedSsrc1);

		manager->ProducerRtpStream(rtpStream0.get(), MappedSsrc0);
		manager->ProducerRtpStream(rtpStream1.get(), MappedSsrc1);

		// No stream can tell its capture instant in this SECTION.

		// Feed packets and a Sender Report to both streams so that they get a score and
		// RecalculateTargetLayers() takes them into account.
		for (auto* rtpStream : { rtpStream0.get(), rtpStream1.get() })
		{
			packet->SetSsrc(rtpStream->GetSsrc());
			feedRtpStreamRecv(rtpStream, packet.get(), 10);

			RTC::RTCP::SenderReport sr;
			sr.SetSsrc(rtpStream->GetSsrc());
			sr.SetNtpSec(1000);
			sr.SetNtpFrac(0);
			sr.SetRtpTs(90000);
			rtpStream->ReceiveRtcpSenderReport(&sr);
		}

		// Set target layer to 0 and sync. This sets tsReferenceSpatialLayer = 0.
		manager->UpdateTargetLayers(0, 0);

		packet->SetSsrc(MappedSsrc0);
		packet->SetSequenceNumber(1);
		manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ false, /*clockRate*/ 90000, /*maxPacketTs*/ 0);

		REQUIRE(manager->GetCurrentSpatialLayer() == 0);

		// Aim at layer 1. Since layer 0 cannot tell its capture instant, layer 1 takes over
		// as TS reference. A transport reconnection then asks for a resync of whatever
		// layer arrives first.
		manager->UpdateTargetLayers(1, 0);
		manager->OnTransportConnected();

		REQUIRE(manager->GetTargetLayers().spatial == 1);

		// A packet of the current layer 0 arrives. It is not the TS reference one and there
		// is no way to align it to layer 1, so layer 0 takes over as TS reference and its
		// RTP timestamps go untouched.
		packet->SetSsrc(MappedSsrc0);
		packet->SetSequenceNumber(2);

		auto result = manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ false, /*clockRate*/ 90000, /*maxPacketTs*/ 0);

		REQUIRE(result.type == RTC::ProducerStreamManager::RtpPacketProcessResult::Type::FORWARD);
		REQUIRE(result.isSyncPacket == true);
		REQUIRE(result.tsOffset == 0u);
	}

	SECTION(
	  "ProducerRtcpSenderReport() checks layers upon a Sender Report that is not the first "
	  "one of its stream")
	{
		MockListener listener;
		auto manager    = createManager(&listener, /*ssrcs*/ TwoSsrcs, /*preferredLayers*/ { 1, 0 });
		auto rtpStream0 = createRtpStreamRecv(MappedSsrc0);
		auto rtpStream1 = createRtpStreamRecv(MappedSsrc1);

		manager->ProducerRtpStream(rtpStream0.get(), MappedSsrc0);
		manager->ProducerRtpStream(rtpStream1.get(), MappedSsrc1);

		rtpStream0->SetCaptureMapping(/*captureMs*/ 1000, /*ts*/ 1000);
		rtpStream1->SetCaptureMapping(/*captureMs*/ 1000, /*ts*/ 1000);

		// Feed packets and a Sender Report to both streams so that they get a score and
		// RecalculateTargetLayers() takes them into account.
		for (auto* rtpStream : { rtpStream0.get(), rtpStream1.get() })
		{
			packet->SetSsrc(rtpStream->GetSsrc());
			feedRtpStreamRecv(rtpStream, packet.get(), 10);

			RTC::RTCP::SenderReport sr;
			sr.SetSsrc(rtpStream->GetSsrc());
			sr.SetNtpSec(1000);
			sr.SetNtpFrac(0);
			sr.SetRtpTs(90000);
			rtpStream->ReceiveRtcpSenderReport(&sr);
		}

		// Set target layer to 0. This sets tsReferenceSpatialLayer = 0.
		manager->UpdateTargetLayers(0, 0);

		REQUIRE(manager->GetTargetLayers().spatial == 0);

		// A Sender Report that is not the first one of its stream may be the one that made
		// the capture instant of the TS reference stream known, so layers are checked and
		// the preferred layer 1 is picked.
		manager->ProducerRtcpSenderReport(rtpStream1.get(), /*first*/ false);

		REQUIRE(manager->GetTargetLayers().spatial == 1);
	}

	SECTION("ProcessRtpPacket() calculates tsOffset of the spatial layer of the packet")
	{
		MockListener listener;
		auto manager = createManager(
		  &listener,
		  /*ssrcs*/ ThreeSsrcs,
		  /*preferredLayers*/ { 2, 0 },
		  /*keyFrameSupported*/ false);
		auto rtpStream0 = createRtpStreamRecv(MappedSsrc0);
		auto rtpStream1 = createRtpStreamRecv(MappedSsrc1);
		auto rtpStream2 = createRtpStreamRecv(MappedSsrc2);

		manager->ProducerRtpStream(rtpStream0.get(), MappedSsrc0);
		manager->ProducerRtpStream(rtpStream1.get(), MappedSsrc1);
		manager->ProducerRtpStream(rtpStream2.get(), MappedSsrc2);

		// All layers hold RTP timestamp 1000. Layer 1 was captured 500 ms before layer 0
		// and layer 2 a whole second before it, so their offsets differ.
		rtpStream0->SetCaptureMapping(/*captureMs*/ 1000, /*ts*/ 1000);
		rtpStream1->SetCaptureMapping(/*captureMs*/ 500, /*ts*/ 1000);
		rtpStream2->SetCaptureMapping(/*captureMs*/ 0, /*ts*/ 1000);

		// Feed packets and a Sender Report to every stream so that they all get a score
		// and RecalculateTargetLayers() takes them into account.
		for (auto* rtpStream : { rtpStream0.get(), rtpStream1.get(), rtpStream2.get() })
		{
			packet->SetSsrc(rtpStream->GetSsrc());
			feedRtpStreamRecv(rtpStream, packet.get(), 10);

			RTC::RTCP::SenderReport sr;
			sr.SetSsrc(rtpStream->GetSsrc());
			sr.SetNtpSec(1000);
			sr.SetNtpFrac(0);
			sr.SetRtpTs(90000);
			rtpStream->ReceiveRtcpSenderReport(&sr);
		}

		// Set target layer to 0 and sync. This sets tsReferenceSpatialLayer = 0, which
		// stays there for the rest of the SECTION since it has a capture mapping.
		manager->UpdateTargetLayers(0, 0);

		packet->SetSsrc(MappedSsrc0);
		packet->SetSequenceNumber(1);
		manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ false, /*clockRate*/ 90000, /*maxPacketTs*/ 0);

		REQUIRE(manager->GetCurrentSpatialLayer() == 0);

		// Switch to layer 1.
		manager->UpdateTargetLayers(1, 0);

		packet->SetSsrc(MappedSsrc1);
		packet->SetSequenceNumber(1);

		auto result = manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ false, /*clockRate*/ 90000, /*maxPacketTs*/ 0);

		REQUIRE(result.spatialLayerSwitched == true);
		REQUIRE(result.tsOffset == 45000u);
		REQUIRE(manager->GetCurrentSpatialLayer() == 1);

		// A transport reconnection asks for a resync of whatever layer arrives first, and
		// moves the target to the preferred layer 2.
		manager->OnTransportConnected();

		REQUIRE(manager->GetTargetLayers().spatial == 2);

		// A packet of the current layer 1 arrives while layer 2 is the target one, so the
		// offset must be the one of layer 1 and not the one of layer 2.
		packet->SetSsrc(MappedSsrc1);
		packet->SetSequenceNumber(2);

		result = manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ false, /*clockRate*/ 90000, /*maxPacketTs*/ 0);

		REQUIRE(result.type == RTC::ProducerStreamManager::RtpPacketProcessResult::Type::FORWARD);
		REQUIRE(result.isSyncPacket == true);
		REQUIRE(result.tsOffset == 45000u);
	}

	SECTION("OnResumed() sets syncRequired")
	{
		MockListener listener;
		auto manager = createManager(
		  &listener,
		  /*ssrcs*/ ThreeSsrcs,
		  /*preferredLayers*/ { 2, 2 },
		  /*keyFrameSupported*/ false);
		auto rtpStream0 = createRtpStreamRecv(MappedSsrc0);

		manager->ProducerRtpStream(rtpStream0.get(), MappedSsrc0);

		// Set target and sync on layer 0.
		manager->UpdateTargetLayers(0, 0);

		packet->SetSsrc(MappedSsrc0);
		packet->SetSequenceNumber(1);
		manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ false, /*clockRate*/ 90000, /*maxPacketTs*/ 0);

		// Verify we are synced: normal packet is forwarded without sync flag.
		packet->SetSequenceNumber(2);
		auto result = manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ false, /*clockRate*/ 90000, /*maxPacketTs*/ 0);

		REQUIRE(result.type == RTC::ProducerStreamManager::RtpPacketProcessResult::Type::FORWARD);
		REQUIRE(result.isSyncPacket == false);

		// Call OnResumed — should set syncRequired.
		manager->OnResumed();

		// Reset target since OnResumed may have reset it via MayChangeLayers().
		manager->UpdateTargetLayers(0, 0);

		// Prove syncRequired was set: next packet is a sync packet.
		packet->SetSequenceNumber(3);
		result = manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ false, /*clockRate*/ 90000, /*maxPacketTs*/ 0);

		REQUIRE(result.type == RTC::ProducerStreamManager::RtpPacketProcessResult::Type::FORWARD);
		REQUIRE(result.isSyncPacket == true);
	}

	SECTION("OnTransportDisconnected() resets target layers")
	{
		MockListener listener;
		auto manager    = createManager(&listener);
		auto rtpStream0 = createRtpStreamRecv(MappedSsrc0);

		manager->ProducerRtpStream(rtpStream0.get(), MappedSsrc0);
		manager->UpdateTargetLayers(0, 0);

		manager->OnTransportDisconnected();

		auto targetLayers = manager->GetTargetLayers();

		REQUIRE(targetLayers.spatial == -1);
		REQUIRE(targetLayers.temporal == -1);
	}

	SECTION("OnPaused() resets target layers")
	{
		MockListener listener;
		auto manager    = createManager(&listener);
		auto rtpStream0 = createRtpStreamRecv(MappedSsrc0);

		manager->ProducerRtpStream(rtpStream0.get(), MappedSsrc0);
		manager->UpdateTargetLayers(0, 0);

		manager->OnPaused();

		auto targetLayers = manager->GetTargetLayers();

		REQUIRE(targetLayers.spatial == -1);
		REQUIRE(targetLayers.temporal == -1);
	}

	SECTION("IncreaseLayer() returns 0 on second call in same iteration")
	{
		MockListener listener;
		auto manager    = createManager(&listener, /*ssrcs*/ OneSsrc, /*preferredLayers*/ { 0, 0 });
		auto rtpStream0 = createRtpStreamRecv(MappedSsrc0);

		manager->ProducerRtpStream(rtpStream0.get(), MappedSsrc0);
		manager->SetExternallyManagedBitrate();

		// Feed packets so the stream has non-zero bitrate.
		packet->SetSsrc(MappedSsrc0);
		feedRtpStreamRecv(rtpStream0.get(), packet.get(), 100);

		auto nowMs = DepLibUV::GetTimeMs();

		// First call claims bitrate.
		auto usedBitrate = manager->IncreaseLayer(
		  /*bitrate*/ 1000000u, /*considerLoss*/ false, /*lossPercentage*/ 0.0f, nowMs);

		REQUIRE(usedBitrate > 0u);

		// Second call in same iteration should return 0 (already at preferred layers).
		auto usedBitrate2 = manager->IncreaseLayer(
		  /*bitrate*/ 1000000u, /*considerLoss*/ false, /*lossPercentage*/ 0.0f, nowMs);

		REQUIRE(usedBitrate2 == 0u);
	}

	SECTION("IncreaseLayer() works again after ApplyLayers()")
	{
		MockListener listener;
		auto manager    = createManager(&listener, /*ssrcs*/ TwoSsrcs, /*preferredLayers*/ { 1, 0 });
		auto rtpStream0 = createRtpStreamRecv(MappedSsrc0);
		auto rtpStream1 = createRtpStreamRecv(MappedSsrc1);

		manager->ProducerRtpStream(rtpStream0.get(), MappedSsrc0);
		manager->ProducerRtpStream(rtpStream1.get(), MappedSsrc1);
		manager->SetExternallyManagedBitrate();

		// Feed packets to layer 0.
		packet->SetSsrc(MappedSsrc0);
		feedRtpStreamRecv(rtpStream0.get(), packet.get(), 100);

		auto nowMs = DepLibUV::GetTimeMs();

		// First iteration: claim layer 0.
		manager->IncreaseLayer(
		  /*bitrate*/ 1000000u, /*considerLoss*/ false, /*lossPercentage*/ 0.0f, nowMs);
		manager->ApplyLayers(/*rtpStreamActiveMs*/ 0u);

		// Feed packets to layer 1.
		packet->SetSsrc(MappedSsrc1);
		feedRtpStreamRecv(rtpStream1.get(), packet.get(), 100);

		// After ApplyLayers, IncreaseLayer should work again for layer 1.
		auto usedBitrate = manager->IncreaseLayer(
		  /*bitrate*/ 1000000u, /*considerLoss*/ false, /*lossPercentage*/ 0.0f, nowMs);

		REQUIRE(usedBitrate > 0u);
	}

	SECTION("GetDesiredBitrate() returns max bitrate across all producer streams")
	{
		MockListener listener;
		auto manager    = createManager(&listener, /*ssrcs*/ TwoSsrcs, /*preferredLayers*/ { 1, 0 });
		auto rtpStream0 = createRtpStreamRecv(MappedSsrc0);
		auto rtpStream1 = createRtpStreamRecv(MappedSsrc1);

		manager->ProducerRtpStream(rtpStream0.get(), MappedSsrc0);
		manager->ProducerRtpStream(rtpStream1.get(), MappedSsrc1);
		manager->SetExternallyManagedBitrate();

		// Feed packets to both streams with different counts to get different bitrates.
		packet->SetSsrc(MappedSsrc0);
		feedRtpStreamRecv(rtpStream0.get(), packet.get(), 50);

		packet->SetSsrc(MappedSsrc1);
		feedRtpStreamRecv(rtpStream1.get(), packet.get(), 100);

		auto nowMs    = DepLibUV::GetTimeMs();
		auto bitrate0 = rtpStream0->GetBitrate(nowMs);
		auto bitrate1 = rtpStream1->GetBitrate(nowMs);

		REQUIRE(bitrate1 > bitrate0);

		auto desiredBitrate = manager->GetDesiredBitrate(nowMs);

		REQUIRE(desiredBitrate == bitrate1);
	}

	SECTION("GetProducerCurrentRtpStream() returns nullptr before sync")
	{
		MockListener listener;
		auto manager    = createManager(&listener);
		auto rtpStream0 = createRtpStreamRecv(MappedSsrc0);

		manager->ProducerRtpStream(rtpStream0.get(), MappedSsrc0);

		REQUIRE(manager->GetProducerCurrentRtpStream() == nullptr);
		REQUIRE(manager->GetCurrentSpatialLayer() == -1);
	}

	SECTION("GetProducerTargetRtpStream() returns correct stream after UpdateTargetLayers()")
	{
		MockListener listener;
		auto manager    = createManager(&listener);
		auto rtpStream0 = createRtpStreamRecv(MappedSsrc0);
		auto rtpStream1 = createRtpStreamRecv(MappedSsrc1);

		manager->ProducerRtpStream(rtpStream0.get(), MappedSsrc0);
		manager->ProducerRtpStream(rtpStream1.get(), MappedSsrc1);

		manager->UpdateTargetLayers(1, 0);

		REQUIRE(manager->GetProducerTargetRtpStream() == rtpStream1.get());
		REQUIRE(manager->GetTargetLayers().spatial == 1);
		REQUIRE(manager->GetTargetLayers().temporal == 0);
	}

	SECTION("RequestKeyFrame() requests for both target and current spatial layers")
	{
		MockListener listener;
		auto manager = createManager(
		  &listener,
		  /*ssrcs*/ TwoSsrcs,
		  /*preferredLayers*/ { 1, 0 },
		  /*keyFrameSupported*/ false);
		auto rtpStream0 = createRtpStreamRecv(MappedSsrc0);
		auto rtpStream1 = createRtpStreamRecv(MappedSsrc1);

		manager->ProducerRtpStream(rtpStream0.get(), MappedSsrc0);
		manager->ProducerRtpStream(rtpStream1.get(), MappedSsrc1);

		// Set target and sync on layer 0.
		manager->UpdateTargetLayers(0, 0);

		packet->SetSsrc(MappedSsrc0);
		packet->SetSequenceNumber(1);
		manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ false, /*clockRate*/ 90000, /*maxPacketTs*/ 0);

		// Change target layer to 1 (current stays at 0 until keyframe).
		manager->UpdateTargetLayers(1, 0);

		listener.keyFrameRequestCount = 0;

		// RequestKeyFrame should request for both target (1) and current (0).
		manager->RequestKeyFrame();

		REQUIRE(listener.keyFrameRequestCount == 2);
	}
}
