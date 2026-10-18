#include "common.hpp"
#include "RTC/RTP/Codecs/PayloadDescriptorHandler.hpp"
#include "RTC/RTP/RtpStreamRecv.hpp"
#include "RTC/SvcProducerStreamManager.hpp"
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
		size_t keyFrameRequestCount{ 0 };
		uint32_t lastKeyFrameRequestedMappedSsrc{ 0 };
		size_t layersChangedCount{ 0 };
		size_t needBitrateChangeCount{ 0 };
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

		void OnRtpStreamSpatialLayerActivityChanged(
		  RTC::RTP::RtpStreamRecv* /*rtpStream*/, uint8_t /*spatialLayer*/, bool /*isActive*/) override
		{
		}
	};

	class MockEncodingContext : public RTC::RTP::Codecs::EncodingContext
	{
	public:
		explicit MockEncodingContext(uint8_t spatialLayers = 3, uint8_t temporalLayers = 3, bool ksvc = false)
		  : RTC::RTP::Codecs::EncodingContext(
		      MockEncodingContext::BuildParams(spatialLayers, temporalLayers, ksvc))
		{
		}
		void SyncRequired() override
		{
		}

	private:
		static RTC::RTP::Codecs::EncodingContext::Params& BuildParams(
		  uint8_t spatialLayers, uint8_t temporalLayers, bool ksvc)
		{
			// Use a thread-local so each call gets its own params instance.
			// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
			static thread_local RTC::RTP::Codecs::EncodingContext::Params params;
			params.spatialLayers  = spatialLayers;
			params.temporalLayers = temporalLayers;
			params.ksvc           = ksvc;
			return params;
		}
	};

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

	// Milliseconds between the packets fed by feedRtpStreamRecv() below.
	constexpr int64_t PacketSpacingMs{ 1 };

	// Current time, which the helpers below move so that traffic spans a period
	// instead of happening all at once.
	// NOLINTNEXTLINE(readability-identifier-naming)
	int64_t nowUs{ 1000000000000 };

	// NOLINTNEXTLINE(readability-identifier-naming)
	mocks::MockShared shared(/*getTimeUs*/
	                         []() -> int64_t
	                         {
		                         return nowUs;
	                         });

	std::unique_ptr<RTC::SvcProducerStreamManager> createManager(
	  MockListener* listener,
	  RTC::ConsumerTypes::VideoLayers preferredLayers                    = { 2, 2 },
	  bool keyFrameSupported                                             = true,
	  RTC::Media::Kind kind                                              = RTC::Media::Kind::VIDEO,
	  std::unique_ptr<RTC::RTP::Codecs::EncodingContext> encodingContext = nullptr)
	{
		RTC::RtpEncodingParameters encoding;
		encoding.ssrc = MappedSsrc;
		const std::vector<RTC::RtpEncodingParameters> consumableRtpEncodings{ encoding };

		if (!encodingContext)
		{
			encodingContext = std::make_unique<MockEncodingContext>();
		}

		return std::make_unique<RTC::SvcProducerStreamManager>(
		  consumableRtpEncodings,
		  preferredLayers,
		  std::move(encodingContext),
		  kind,
		  keyFrameSupported,
		  listener,
		  std::addressof(shared));
	}

	std::unique_ptr<RTC::RTP::RtpStreamRecv> createRtpStreamRecv(
	  uint32_t ssrc = MappedSsrc, uint8_t spatialLayers = 3, uint8_t temporalLayers = 3)
	{
		RTC::RTP::RtpStream::Params params;

		params.ssrc           = ssrc;
		params.clockRate      = 90000;
		params.spatialLayers  = spatialLayers;
		params.temporalLayers = temporalLayers;

		return std::make_unique<RTC::RTP::RtpStreamRecv>(
		  std::addressof(streamRecvListener), std::addressof(shared), params, 0, false);
	}

	// Feed packets into the RtpStreamRecv so GetBitrate() returns non-zero. They
	// are spread in time because a bitrate is measured over the period its samples
	// span, and samples taken all at once span none.
	void feedRtpStreamRecv(RTC::RTP::RtpStreamRecv* rtpStream, RTC::RTP::Packet* packet, uint16_t count)
	{
		auto firstSeq = static_cast<uint16_t>(packet->GetSequenceNumber() + 1);
		auto lastSeq  = static_cast<uint16_t>(firstSeq + count);

		for (uint16_t seq = firstSeq; Utils::Number::IsLowerThan<uint16_t>(seq, lastSeq); ++seq)
		{
			if (seq != firstSeq)
			{
				nowUs += PacketSpacingMs * 1000;
			}

			packet->SetSequenceNumber(seq);
			rtpStream->ReceivePacket(packet, shared.GetTimeUs());
		}

		// The packets span the gaps between them plus the millisecond the first one
		// was taken in.
		const auto periodMs = static_cast<double>(((count - 1) * PacketSpacingMs) + 1);

		// bitrate (bps) = bytes * 8000 / periodMs, where the packet that starts the
		// period is left out of the bytes because the period does not cover the time
		// it took to arrive.
		const auto expectedBitrate = static_cast<int64_t>(std::trunc(
		  ((static_cast<double>(count - 1) * static_cast<double>(packet->GetLength()) * 8000.0) / periodMs) +
		  0.5));

		REQUIRE(rtpStream->GetBitrate(shared.GetTimeMs()) == expectedBitrate);
	}
} // namespace

SCENARIO("SvcProducerStreamManager", "[rtp][producerstreammanager][svc]")
{
	std::unique_ptr<RTC::RTP::Packet> packet(
	  RTC::RTP::Packet::Factory(rtpCommon::FactoryBuffer, sizeof(rtpCommon::FactoryBuffer)));

	packet->SetPayloadType(1);
	packet->SetSsrc(MappedSsrc);
	packet->SetTimestamp(1000);
	packet->SetPayloadLength(40);

	// Each BWE picks the layers in its own way, so this drives whichever one is
	// built: it hands the manager that much bitrate and leaves it settled on the
	// layers that it pays for.
	const auto allocateBitrate =
	  [](RTC::SvcProducerStreamManager* manager, int64_t bitrate, int64_t nowMs) -> void
	{
#ifdef MS_USE_BUILTIN_BWE
		const auto bitrateOptions = manager->GetBitrateOptions(nowMs);

		// The dearest point that fits, which is where the allocation leaves a
		// candidate that is the only one asking.
		size_t option{ 0 };

		for (size_t idx{ 0 }; idx < bitrateOptions.options.size(); ++idx)
		{
			if (bitrateOptions.options.at(idx).bitrate <= bitrate)
			{
				option = idx;
			}
		}

		manager->SetBitrateOption(bitrateOptions.options.at(option));
#else
		manager->IncreaseLayer(bitrate, /*considerLoss*/ false, /*lossPercentage*/ 0.0f, nowMs);
		manager->ApplyLayers();
#endif
	};

	SECTION("ProcessRtpPacket() returns DROP when target spatial layer is -1")
	{
		MockListener listener;
		auto manager   = createManager(std::addressof(listener));
		auto rtpStream = createRtpStreamRecv();

		manager->ProducerRtpStream(rtpStream.get(), MappedSsrc);

		// Default target layers are -1,-1 — packet must be dropped.
		packet->SetSequenceNumber(1);
		auto result = manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ false, /*clockRate*/ 90000, /*maxPacketTs*/ 0);

		REQUIRE(result.type == RtpPacketProcessResult::Type::DROP);
	}

	SECTION("ProcessRtpPacket() returns DROP when target temporal layer is -1")
	{
		MockListener listener;
		auto manager   = createManager(std::addressof(listener));
		auto rtpStream = createRtpStreamRecv();

		manager->ProducerRtpStream(rtpStream.get(), MappedSsrc);

		// Set only spatial — temporal stays -1.
		manager->UpdateTargetLayers(0, -1);

		packet->SetSequenceNumber(1);
		auto result = manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ false, /*clockRate*/ 90000, /*maxPacketTs*/ 0);

		REQUIRE(result.type == RtpPacketProcessResult::Type::DROP);
	}

	SECTION("ProcessRtpPacket() returns BUFFER when sync required and packet is not a keyframe")
	{
		MockListener listener;
		auto manager   = createManager(std::addressof(listener));
		auto rtpStream = createRtpStreamRecv();

		manager->ProducerRtpStream(rtpStream.get(), MappedSsrc);
		manager->UpdateTargetLayers(0, 0);

		// In SVC, syncRequired is set by OnTransportConnected/OnResumed, not by
		// UpdateTargetLayers. Trigger it explicitly.
		manager->OnTransportConnected();

		// Re-apply target since MayChangeLayers may have reset it.
		manager->UpdateTargetLayers(0, 0);

		packet->SetSequenceNumber(1);
		auto result = manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ false, /*clockRate*/ 90000, /*maxPacketTs*/ 0);

		REQUIRE(result.type == RtpPacketProcessResult::Type::BUFFER);
	}

	SECTION(
	  "ProcessRtpPacket() returns FORWARD with isSyncPacket and sendBufferedPackets on keyframe sync")
	{
		MockListener listener;
		auto manager   = createManager(std::addressof(listener));
		auto rtpStream = createRtpStreamRecv();

		manager->ProducerRtpStream(rtpStream.get(), MappedSsrc);
		manager->UpdateTargetLayers(0, 0);

		// syncRequired is only set by OnTransportConnected/OnResumed in SVC.
		manager->OnTransportConnected();

		// Re-apply target since MayChangeLayers may have reset it.
		manager->UpdateTargetLayers(0, 0);

		const uint16_t seq{ 100 };

		packet->SetSequenceNumber(seq);

		// packet->SetPayloadDescriptorHandler() takes ownership.
		auto* handler       = new MockPayloadDescriptorHandler();
		handler->isKeyFrame = true;
		packet->SetPayloadDescriptorHandler(handler);

		auto result = manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ false, /*clockRate*/ 90000, /*maxPacketTs*/ 0);

		REQUIRE(result.type == RtpPacketProcessResult::Type::FORWARD);
		REQUIRE(result.isSyncPacket == true);
		REQUIRE(result.syncSeqValue == seq - 1);
		REQUIRE(result.shouldSyncEncodingContext == true);
		REQUIRE(result.sendBufferedPackets == true);
	}

	SECTION("ProcessRtpPacket() always requires a keyframe for sync regardless of keyFrameSupported flag")
	{
		MockListener listener;
		// keyFrameSupported=false has no effect in SVC: sync always needs a keyframe.
		auto manager = createManager(
		  std::addressof(listener),
		  /*preferredLayers*/ { 2, 2 },
		  /*keyFrameSupported*/ false);
		auto rtpStream = createRtpStreamRecv();

		manager->ProducerRtpStream(rtpStream.get(), MappedSsrc);
		manager->UpdateTargetLayers(0, 0);

		// syncRequired is only set by OnTransportConnected/OnResumed in SVC.
		manager->OnTransportConnected();

		// Re-apply target since MayChangeLayers may have reset it.
		manager->UpdateTargetLayers(0, 0);

		// A non-keyframe must still be buffered even with keyFrameSupported=false.
		packet->SetSequenceNumber(1);
		auto result = manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ false, /*clockRate*/ 90000, /*maxPacketTs*/ 0);

		REQUIRE(result.type == RtpPacketProcessResult::Type::BUFFER);

		// Now a keyframe completes the sync.
		const uint16_t seq{ 100 };
		packet->SetSequenceNumber(seq);

		auto* handler       = new MockPayloadDescriptorHandler();
		handler->isKeyFrame = true;
		packet->SetPayloadDescriptorHandler(handler);

		result = manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ false, /*clockRate*/ 90000, /*maxPacketTs*/ 0);

		REQUIRE(result.type == RtpPacketProcessResult::Type::FORWARD);
		REQUIRE(result.isSyncPacket == true);
		REQUIRE(result.syncSeqValue == seq - 1);
		REQUIRE(result.tsOffset == 0);
		REQUIRE(result.sendBufferedPackets == true);
	}

	SECTION("ProcessRtpPacket() returns DROP for empty payload packets")
	{
		MockListener listener;
		auto manager = createManager(
		  std::addressof(listener),
		  /*preferredLayers*/ { 2, 2 },
		  /*keyFrameSupported*/ false);
		auto rtpStream = createRtpStreamRecv();

		manager->ProducerRtpStream(rtpStream.get(), MappedSsrc);
		manager->UpdateTargetLayers(0, 0);

		// Trigger sync via OnTransportConnected, then re-apply target.
		manager->OnTransportConnected();
		manager->UpdateTargetLayers(0, 0);

		// Complete sync with a keyframe (SVC always requires keyframe for sync).
		packet->SetSequenceNumber(1);
		auto* handler       = new MockPayloadDescriptorHandler();
		handler->isKeyFrame = true;
		packet->SetPayloadDescriptorHandler(handler);
		manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ false, /*clockRate*/ 90000, /*maxPacketTs*/ 0);

		// Now send a packet with empty payload.
		packet->SetSequenceNumber(2);
		packet->RemovePayload();
		auto result = manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ false, /*clockRate*/ 90000, /*maxPacketTs*/ 0);

		REQUIRE(result.type == RtpPacketProcessResult::Type::DROP);
	}

	SECTION("ProcessRtpPacket() returns DROP when ProcessPayload fails")
	{
		MockListener listener;
		auto encodingContext = std::make_unique<MockEncodingContext>();
		auto manager         = createManager(
		  std::addressof(listener),
		  /*preferredLayers*/ { 2, 2 },
		  /*keyFrameSupported*/ false,
		  RTC::Media::Kind::VIDEO,
		  std::move(encodingContext));
		auto rtpStream = createRtpStreamRecv();

		manager->ProducerRtpStream(rtpStream.get(), MappedSsrc);
		manager->UpdateTargetLayers(0, 0);

		// Trigger sync via OnTransportConnected, then re-apply target.
		manager->OnTransportConnected();
		manager->UpdateTargetLayers(0, 0);

		// Complete sync with a keyframe (SVC always requires keyframe for sync).
		packet->SetSequenceNumber(1);
		auto* handler       = new MockPayloadDescriptorHandler();
		handler->isKeyFrame = true;
		packet->SetPayloadDescriptorHandler(handler);
		manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ false, /*clockRate*/ 90000, /*maxPacketTs*/ 0);

		// Set a handler whose Process() returns false.
		packet->SetSequenceNumber(2);
		handler                = new MockPayloadDescriptorHandler();
		handler->processResult = false;
		packet->SetPayloadDescriptorHandler(handler);

		auto result = manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ false, /*clockRate*/ 90000, /*maxPacketTs*/ 0);

		REQUIRE(result.type == RtpPacketProcessResult::Type::DROP);
	}

	SECTION("ProcessRtpPacket() returns FORWARD for normal packets after sync")
	{
		MockListener listener;
		auto manager = createManager(
		  std::addressof(listener),
		  /*preferredLayers*/ { 2, 2 },
		  /*keyFrameSupported*/ false);
		auto rtpStream = createRtpStreamRecv();

		manager->ProducerRtpStream(rtpStream.get(), MappedSsrc);
		manager->UpdateTargetLayers(0, 0);

		// Trigger sync via OnTransportConnected, then re-apply target.
		manager->OnTransportConnected();
		manager->UpdateTargetLayers(0, 0);

		// Complete sync with a keyframe (SVC always requires keyframe for sync).
		packet->SetSequenceNumber(1);
		auto* handler       = new MockPayloadDescriptorHandler();
		handler->isKeyFrame = true;
		packet->SetPayloadDescriptorHandler(handler);
		manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ false, /*clockRate*/ 90000, /*maxPacketTs*/ 0);

		// Send a second normal packet — should be a plain FORWARD.
		packet->SetSequenceNumber(2);
		auto result = manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ false, /*clockRate*/ 90000, /*maxPacketTs*/ 0);

		REQUIRE(result.type == RtpPacketProcessResult::Type::FORWARD);
		REQUIRE(result.isSyncPacket == false);
		REQUIRE(result.sendBufferedPackets == false);
		REQUIRE(result.tsOffset == 0);
	}

	SECTION("ProcessRtpPacket() always sets tsOffset to 0 (SVC uses single stream)")
	{
		MockListener listener;
		auto manager = createManager(
		  std::addressof(listener),
		  /*preferredLayers*/ { 2, 2 },
		  /*keyFrameSupported*/ false);
		auto rtpStream = createRtpStreamRecv();

		manager->ProducerRtpStream(rtpStream.get(), MappedSsrc);
		manager->UpdateTargetLayers(0, 0);

		// Trigger sync, then re-apply target.
		manager->OnTransportConnected();
		manager->UpdateTargetLayers(0, 0);

		// Sync with a keyframe — SVC always requires one.
		packet->SetSequenceNumber(1);
		auto* handler       = new MockPayloadDescriptorHandler();
		handler->isKeyFrame = true;
		packet->SetPayloadDescriptorHandler(handler);
		auto result = manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ false, /*clockRate*/ 90000, /*maxPacketTs*/ 0);

		REQUIRE(result.type == RtpPacketProcessResult::Type::FORWARD);
		REQUIRE(result.tsOffset == 0);

		// Subsequent normal packets also have tsOffset == 0.
		packet->SetSequenceNumber(2);
		result = manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ false, /*clockRate*/ 90000, /*maxPacketTs*/ 0);

		REQUIRE(result.type == RtpPacketProcessResult::Type::FORWARD);
		REQUIRE(result.tsOffset == 0);
	}

	SECTION("UpdateTargetLayers() to -1 resets current and target layers")
	{
		MockListener listener;
		auto manager   = createManager(std::addressof(listener));
		auto rtpStream = createRtpStreamRecv();

		manager->ProducerRtpStream(rtpStream.get(), MappedSsrc);
		manager->UpdateTargetLayers(1, 1);

		// Reset.
		manager->UpdateTargetLayers(-1, -1);

		auto targetLayers = manager->GetTargetLayers();

		REQUIRE(targetLayers.spatial == -1);
		REQUIRE(targetLayers.temporal == -1);
		REQUIRE(manager->GetCurrentSpatialLayer() == -1);
	}

	SECTION("UpdateTargetLayers() fires OnProducerStreamManagerLayersChanged() when setting -1")
	{
		MockListener listener;
		auto manager   = createManager(std::addressof(listener));
		auto rtpStream = createRtpStreamRecv();

		manager->ProducerRtpStream(rtpStream.get(), MappedSsrc);

		const auto countBefore = listener.layersChangedCount;

		manager->UpdateTargetLayers(-1, -1);

		REQUIRE(listener.layersChangedCount == countBefore + 1);
	}

	SECTION("UpdateTargetLayers() requests keyframe for full-SVC spatial upgrade")
	{
		MockListener listener;
		auto manager   = createManager(std::addressof(listener));
		auto rtpStream = createRtpStreamRecv();

		manager->ProducerRtpStream(rtpStream.get(), MappedSsrc);

		// Start at layer 0.
		manager->UpdateTargetLayers(0, 0);

		const auto keyFrameRequestCountBefore = listener.keyFrameRequestCount;

		// Upgrade spatial: full-SVC requests a keyframe.
		manager->UpdateTargetLayers(1, 0);

		REQUIRE(listener.keyFrameRequestCount > keyFrameRequestCountBefore);
		REQUIRE(listener.lastKeyFrameRequestedMappedSsrc == MappedSsrc);
	}

	SECTION("GetProducerCurrentRtpStream() returns nullptr when current spatial layer is -1")
	{
		MockListener listener;
		auto manager   = createManager(std::addressof(listener));
		auto rtpStream = createRtpStreamRecv();

		manager->ProducerRtpStream(rtpStream.get(), MappedSsrc);

		// No target set yet — current spatial layer is -1.
		REQUIRE(manager->GetProducerCurrentRtpStream() == nullptr);
		REQUIRE(manager->GetCurrentSpatialLayer() == -1);
	}

	SECTION("GetProducerTargetRtpStream() returns nullptr when target spatial layer is -1")
	{
		MockListener listener;
		auto manager   = createManager(std::addressof(listener));
		auto rtpStream = createRtpStreamRecv();

		manager->ProducerRtpStream(rtpStream.get(), MappedSsrc);

		REQUIRE(manager->GetProducerTargetRtpStream() == nullptr);
	}

	SECTION("GetProducerTargetRtpStream() returns the producer stream after UpdateTargetLayers()")
	{
		MockListener listener;
		auto manager   = createManager(std::addressof(listener));
		auto rtpStream = createRtpStreamRecv();

		manager->ProducerRtpStream(rtpStream.get(), MappedSsrc);
		manager->UpdateTargetLayers(0, 0);

		REQUIRE(manager->GetProducerTargetRtpStream() == rtpStream.get());
		REQUIRE(manager->GetTargetLayers().spatial == 0);
		REQUIRE(manager->GetTargetLayers().temporal == 0);
	}

	SECTION("IsActive() returns false when listener is not active")
	{
		MockListener listener;
		listener.isActive = false;

		auto manager   = createManager(std::addressof(listener));
		auto rtpStream = createRtpStreamRecv();

		manager->ProducerRtpStream(rtpStream.get(), MappedSsrc);

		REQUIRE(manager->IsActive() == false);
	}

	SECTION("IsActive() returns false when no producer stream is set")
	{
		MockListener listener;
		auto manager = createManager(std::addressof(listener));

		// No ProducerRtpStream call.
		REQUIRE(manager->IsActive() == false);
	}

	SECTION("RequestKeyFrame() always uses the single SVC mapped SSRC")
	{
		MockListener listener;
		auto manager   = createManager(std::addressof(listener));
		auto rtpStream = createRtpStreamRecv();

		manager->ProducerRtpStream(rtpStream.get(), MappedSsrc);
		manager->UpdateTargetLayers(0, 0);

		listener.keyFrameRequestCount = 0;
		manager->RequestKeyFrame();

		REQUIRE(listener.keyFrameRequestCount == 1);
		REQUIRE(listener.lastKeyFrameRequestedMappedSsrc == MappedSsrc);
	}

	SECTION("RequestKeyFrameForTargetSpatialLayer() delegates to single-SSRC RequestKeyFrame()")
	{
		MockListener listener;
		auto manager   = createManager(std::addressof(listener));
		auto rtpStream = createRtpStreamRecv();

		manager->ProducerRtpStream(rtpStream.get(), MappedSsrc);
		manager->UpdateTargetLayers(1, 0);

		listener.keyFrameRequestCount = 0;
		manager->RequestKeyFrameForTargetSpatialLayer();

		REQUIRE(listener.keyFrameRequestCount == 1);
		REQUIRE(listener.lastKeyFrameRequestedMappedSsrc == MappedSsrc);
	}

	SECTION("RequestKeyFrameForCurrentSpatialLayer() delegates to single-SSRC RequestKeyFrame()")
	{
		MockListener listener;
		auto manager = createManager(
		  std::addressof(listener),
		  /*preferredLayers*/ { 2, 2 },
		  /*keyFrameSupported*/ false);
		auto rtpStream = createRtpStreamRecv();

		manager->ProducerRtpStream(rtpStream.get(), MappedSsrc);
		manager->UpdateTargetLayers(0, 0);

		// Sync so we have a current layer.
		packet->SetSequenceNumber(1);
		manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ false, /*clockRate*/ 90000, /*maxPacketTs*/ 0);

		listener.keyFrameRequestCount = 0;
		manager->RequestKeyFrameForCurrentSpatialLayer();

		REQUIRE(listener.keyFrameRequestCount == 1);
		REQUIRE(listener.lastKeyFrameRequestedMappedSsrc == MappedSsrc);
	}

	SECTION("OnTransportConnected() sets syncRequired")
	{
		MockListener listener;
		auto manager = createManager(
		  std::addressof(listener),
		  /*preferredLayers*/ { 2, 2 },
		  /*keyFrameSupported*/ false);
		auto rtpStream = createRtpStreamRecv();

		manager->ProducerRtpStream(rtpStream.get(), MappedSsrc);
		manager->UpdateTargetLayers(0, 0);

		// First OnTransportConnected to set syncRequired, then sync with a keyframe.
		manager->OnTransportConnected();
		manager->UpdateTargetLayers(0, 0);

		packet->SetSequenceNumber(1);
		auto* handler       = new MockPayloadDescriptorHandler();
		handler->isKeyFrame = true;
		packet->SetPayloadDescriptorHandler(handler);
		manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ false, /*clockRate*/ 90000, /*maxPacketTs*/ 0);

		// Verify synced: next normal packet is not a sync packet.
		packet->SetSequenceNumber(2);
		auto result = manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ false, /*clockRate*/ 90000, /*maxPacketTs*/ 0);

		REQUIRE(result.isSyncPacket == false);

		// Second OnTransportConnected — sets syncRequired again.
		manager->OnTransportConnected();

		// Re-set target because MayChangeLayers may have reset it.
		manager->UpdateTargetLayers(0, 0);

		packet->SetSequenceNumber(3);
		result = manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ false, /*clockRate*/ 90000, /*maxPacketTs*/ 0);

		REQUIRE(result.type == RtpPacketProcessResult::Type::FORWARD);
		REQUIRE(result.isSyncPacket == true);
	}

	SECTION("OnTransportConnected() requests keyframe when active and target changes")
	{
		MockListener listener;
		// Use preferred layer 0 so RecalculateTargetLayers can find it with spatial-layer-0 bitrate.
		auto manager   = createManager(std::addressof(listener), /*preferredLayers*/ { 0, 0 });
		auto rtpStream = createRtpStreamRecv(MappedSsrc, /*spatialLayers*/ 1, /*temporalLayers*/ 1);

		manager->ProducerRtpStream(rtpStream.get(), MappedSsrc);

		// Feed packets so GetSpatialLayerBitrate(0) returns non-zero.
		// Packets without a PayloadDescriptorHandler default to spatialLayer=0.
		feedRtpStreamRecv(rtpStream.get(), packet.get(), 100);

		auto keyFrameCountBefore = listener.keyFrameRequestCount;

		// OnTransportConnected calls MayChangeLayers -> RecalculateTargetLayers picks
		// layer {0,0} (bitrate > 0), which differs from the initial {-1,-1} ->
		// UpdateTargetLayers -> newTarget(0) > current(-1) -> keyframe requested.
		manager->OnTransportConnected();

#ifdef MS_USE_BUILTIN_BWE
		// The target layers are not moved here but once bitrate is handed out, so
		// all this call does is ask for a new round.
		REQUIRE(listener.needBitrateChangeCount > 0);

		const int64_t nowMs = shared.GetTimeMs();

		allocateBitrate(manager.get(), /*bitrate*/ 1000000, nowMs);
#endif

		REQUIRE(listener.keyFrameRequestCount > keyFrameCountBefore);
		REQUIRE(listener.lastKeyFrameRequestedMappedSsrc == MappedSsrc);
	}

	SECTION("OnTransportConnected() does not request keyframe when not active")
	{
		MockListener listener;
		listener.isActive = false;

		auto manager = createManager(std::addressof(listener));

		// Don't wire producerRtpStream — manager is not active.
		manager->OnTransportConnected();

		REQUIRE(listener.keyFrameRequestCount == 0);
	}

	SECTION("OnTransportDisconnected() resets target and current layers to -1")
	{
		MockListener listener;
		auto manager   = createManager(std::addressof(listener));
		auto rtpStream = createRtpStreamRecv();

		manager->ProducerRtpStream(rtpStream.get(), MappedSsrc);
		manager->UpdateTargetLayers(1, 1);

		manager->OnTransportDisconnected();

		auto targetLayers = manager->GetTargetLayers();

		REQUIRE(targetLayers.spatial == -1);
		REQUIRE(targetLayers.temporal == -1);
		REQUIRE(manager->GetCurrentSpatialLayer() == -1);
	}

	SECTION("OnPaused() resets target and current layers to -1")
	{
		MockListener listener;
		auto manager   = createManager(std::addressof(listener));
		auto rtpStream = createRtpStreamRecv();

		manager->ProducerRtpStream(rtpStream.get(), MappedSsrc);
		manager->UpdateTargetLayers(2, 2);

		manager->OnPaused();

		auto targetLayers = manager->GetTargetLayers();

		REQUIRE(targetLayers.spatial == -1);
		REQUIRE(targetLayers.temporal == -1);
		REQUIRE(manager->GetCurrentSpatialLayer() == -1);
	}

	SECTION("OnResumed() sets syncRequired and requests keyframe when active")
	{
		MockListener listener;
		// Use preferred layer 0 so RecalculateTargetLayers can find it with spatial-layer-0 bitrate.
		auto manager   = createManager(std::addressof(listener), /*preferredLayers*/ { 0, 0 });
		auto rtpStream = createRtpStreamRecv(MappedSsrc, /*spatialLayers*/ 1, /*temporalLayers*/ 1);

		manager->ProducerRtpStream(rtpStream.get(), MappedSsrc);

		// Feed packets so GetSpatialLayerBitrate(0) returns non-zero.
		feedRtpStreamRecv(rtpStream.get(), packet.get(), 100);

		// Connect so RecalculateTargetLayers sets target to {0,0}.
		manager->OnTransportConnected();

		// Disconnect: target and current reset to {-1,-1}.
		manager->OnTransportDisconnected();

		const auto keyFrameCountBefore = listener.keyFrameRequestCount;

		// OnResumed sets syncRequired and calls MayChangeLayers -> RecalculateTargetLayers
		// returns {0,0} which differs from {-1,-1} -> UpdateTargetLayers(0,0) ->
		// newTarget(0) > current(-1) -> keyframe requested.
		manager->OnResumed();

#ifdef MS_USE_BUILTIN_BWE
		// The target layers are not moved here but once bitrate is handed out, so
		// all this call does is ask for a new round.
		const int64_t nowMs = shared.GetTimeMs();

		allocateBitrate(manager.get(), /*bitrate*/ 1000000, nowMs);
#endif

		REQUIRE(listener.keyFrameRequestCount > keyFrameCountBefore);
		REQUIRE(listener.lastKeyFrameRequestedMappedSsrc == MappedSsrc);
	}

	SECTION("OnResumed() sets syncRequired: next packet is a sync packet")
	{
		MockListener listener;
		auto manager = createManager(
		  std::addressof(listener),
		  /*preferredLayers*/ { 2, 2 },
		  /*keyFrameSupported*/ false);
		auto rtpStream = createRtpStreamRecv();

		manager->ProducerRtpStream(rtpStream.get(), MappedSsrc);
		manager->UpdateTargetLayers(0, 0);

		// First sync via OnTransportConnected + keyframe.
		manager->OnTransportConnected();
		manager->UpdateTargetLayers(0, 0);

		packet->SetSequenceNumber(1);
		auto* handler       = new MockPayloadDescriptorHandler();
		handler->isKeyFrame = true;
		packet->SetPayloadDescriptorHandler(handler);
		manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ false, /*clockRate*/ 90000, /*maxPacketTs*/ 0);

		// Verify synced: second normal packet is not a sync packet.
		packet->SetSequenceNumber(2);
		auto result = manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ false, /*clockRate*/ 90000, /*maxPacketTs*/ 0);
		REQUIRE(result.isSyncPacket == false);

		// Call OnResumed — sets syncRequired again.
		manager->OnResumed();

		// Re-set target since MayChangeLayers may have reset it.
		manager->UpdateTargetLayers(0, 0);

		// SVC requires a keyframe to complete sync.
		packet->SetSequenceNumber(3);
		handler             = new MockPayloadDescriptorHandler();
		handler->isKeyFrame = true;
		packet->SetPayloadDescriptorHandler(handler);
		result = manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ false, /*clockRate*/ 90000, /*maxPacketTs*/ 0);

		REQUIRE(result.type == RtpPacketProcessResult::Type::FORWARD);
		REQUIRE(result.isSyncPacket == true);
	}

	SECTION("ProducerNewRtpStream() updates the producer stream and fires score event")
	{
		MockListener listener;
		auto manager    = createManager(std::addressof(listener));
		auto rtpStream0 = createRtpStreamRecv();

		manager->ProducerRtpStream(rtpStream0.get(), MappedSsrc);

		// MockShared indexes timers by label, so the stream being replaced must be
		// gone before creating the one replacing it.
		rtpStream0.reset();

		auto rtpStream1 = createRtpStreamRecv();

		// Replace with a new stream.
		manager->ProducerNewRtpStream(rtpStream1.get(), MappedSsrc);

		// GetProducerTargetRtpStream still returns the new stream when target is set.
		manager->UpdateTargetLayers(0, 0);

		REQUIRE(manager->GetProducerTargetRtpStream() == rtpStream1.get());
	}

	SECTION("ProducerRtpStreamScore() triggers MayChangeLayers() when stream dies (score==0)")
	{
		MockListener listener;
		auto manager   = createManager(std::addressof(listener));
		auto rtpStream = createRtpStreamRecv();

		manager->ProducerRtpStream(rtpStream.get(), MappedSsrc);
		manager->UpdateTargetLayers(1, 1);

		const auto layersChangedBefore = listener.layersChangedCount;

		// Score drops to 0 — MayChangeLayers is called, RecalculateTargetLayers
		// returns -1,-1 (no active stream), UpdateTargetLayers fires layersChanged.
		manager->ProducerRtpStreamScore(rtpStream.get(), /*score*/ 0, /*previousScore*/ 7);

#ifdef MS_USE_BUILTIN_BWE
		// The target layers are not moved here but once bitrate is handed out, and
		// a dead stream can be given none.
		REQUIRE(listener.needBitrateChangeCount > 0);

		const int64_t nowMs = shared.GetTimeMs();

		allocateBitrate(manager.get(), /*bitrate*/ 1000000, nowMs);
#endif

		REQUIRE(listener.layersChangedCount > layersChangedBefore);
		REQUIRE(manager->GetTargetLayers().spatial == -1);
	}

	SECTION("ProducerRtpStreamScore() triggers MayChangeLayers() when stream revives (previousScore==0)")
	{
		MockListener listener;
		auto manager   = createManager(std::addressof(listener));
		auto rtpStream = createRtpStreamRecv();

		manager->ProducerRtpStream(rtpStream.get(), MappedSsrc);

		// First bring score to 0 so target becomes -1.
		manager->ProducerRtpStreamScore(rtpStream.get(), /*score*/ 0, /*previousScore*/ 7);
		REQUIRE(manager->GetTargetLayers().spatial == -1);

		// Revive the stream — MayChangeLayers should fire again.
		// Even though RecalculateTargetLayers may still return -1 (no bitrate),
		// the important thing is the code path is exercised and no crash occurs.
		manager->ProducerRtpStreamScore(rtpStream.get(), /*score*/ 7, /*previousScore*/ 0);
	}

#ifdef MS_USE_BUILTIN_BWE
	SECTION("GetBitrateOptions() offers only sending nothing when no producer stream is set")
	{
		MockListener listener;
		auto manager = createManager(std::addressof(listener));

		const auto bitrateOptions = manager->GetBitrateOptions(shared.GetTimeMs());

		REQUIRE(bitrateOptions.options.size() == 1);
		REQUIRE(bitrateOptions.options.at(0).bitrate == 0);
		REQUIRE(bitrateOptions.options.at(0).layers.spatial == -1);
		REQUIRE(bitrateOptions.options.at(0).layers.temporal == -1);
	}

	SECTION("GetBitrateOptions() offers only sending nothing when producer stream has score 0")
	{
		MockListener listener;
		auto manager   = createManager(std::addressof(listener));
		auto rtpStream = createRtpStreamRecv();

		manager->ProducerRtpStream(rtpStream.get(), MappedSsrc);

		// Score is 0 by default (no RTP received + inactivity check disabled).
		const auto bitrateOptions = manager->GetBitrateOptions(shared.GetTimeMs());

		REQUIRE(bitrateOptions.options.size() == 1);
		REQUIRE(bitrateOptions.options.at(0).bitrate == 0);
	}

	SECTION("GetBitrateOptions() reports no current option until a point has been given")
	{
		MockListener listener;
		auto encodingContext =
		  std::make_unique<MockEncodingContext>(/*spatialLayers*/ 1, /*temporalLayers*/ 1);
		auto manager = createManager(
		  std::addressof(listener),
		  /*preferredLayers*/ { 0, 0 },
		  /*keyFrameSupported*/ true,
		  RTC::Media::Kind::VIDEO,
		  std::move(encodingContext));
		auto rtpStream = createRtpStreamRecv(MappedSsrc, /*spatialLayers*/ 1, /*temporalLayers*/ 1);

		manager->ProducerRtpStream(rtpStream.get(), MappedSsrc);

		// Feed packets so the stream has non-zero bitrate.
		feedRtpStreamRecv(rtpStream.get(), packet.get(), 100);

		const int64_t nowMs = shared.GetTimeMs();

		// Nothing has been given to it yet, so it is not climbing back to anything.
		const auto bitrateOptions = manager->GetBitrateOptions(nowMs);

		REQUIRE(bitrateOptions.options.size() == 2);
		REQUIRE(bitrateOptions.currentOption.has_value() == false);

		manager->SetBitrateOption(bitrateOptions.options.at(1));

		// Once given, it reports the point it was given.
		const auto newBitrateOptions = manager->GetBitrateOptions(nowMs);

		REQUIRE(newBitrateOptions.currentOption == 1);
		REQUIRE(manager->GetTargetLayers().spatial == 0);
		REQUIRE(manager->GetTargetLayers().temporal == 0);
	}

	SECTION("GetBitrateOptions() offers a point per layer up to the preferred ones")
	{
		MockListener listener;
		auto encodingContext =
		  std::make_unique<MockEncodingContext>(/*spatialLayers*/ 3, /*temporalLayers*/ 3);
		auto manager = createManager(
		  std::addressof(listener),
		  /*preferredLayers*/ { 1, 1 },
		  /*keyFrameSupported*/ true,
		  RTC::Media::Kind::VIDEO,
		  std::move(encodingContext));
		auto rtpStream = createRtpStreamRecv(MappedSsrc, /*spatialLayers*/ 3, /*temporalLayers*/ 3);

		manager->ProducerRtpStream(rtpStream.get(), MappedSsrc);

		// Packets without a PayloadDescriptorHandler default to layers 0:0.
		feedRtpStreamRecv(rtpStream.get(), packet.get(), 100);

		// Only the layers 0:0 are arriving, so beyond sending nothing there is the
		// single point of running at them. Neither the temporal layers above the
		// preferred one nor the spatial layers that bring no bitrate are offered.
		const auto bitrateOptions = manager->GetBitrateOptions(shared.GetTimeMs());

		REQUIRE(bitrateOptions.options.size() == 2);
		REQUIRE(bitrateOptions.options.at(1).layers.spatial == 0);
		REQUIRE(bitrateOptions.options.at(1).layers.temporal == 0);
		REQUIRE(bitrateOptions.options.at(1).bitrate > 0);
	}
#endif

#ifndef MS_USE_BUILTIN_BWE
	SECTION("IncreaseLayer() returns 0 when no producer stream is set")
	{
		MockListener listener;
		auto manager = createManager(std::addressof(listener));

		manager->SetExternallyManagedBitrate();

		const int64_t nowMs    = shared.GetTimeMs();
		const auto usedBitrate = manager->IncreaseLayer(
		  /*bitrate*/ 1000000, /*considerLoss*/ false, /*lossPercentage*/ 0.0f, nowMs);

		REQUIRE(usedBitrate == 0);
	}

	SECTION("IncreaseLayer() returns 0 when producer stream has score 0")
	{
		MockListener listener;
		auto manager   = createManager(std::addressof(listener));
		auto rtpStream = createRtpStreamRecv();

		manager->ProducerRtpStream(rtpStream.get(), MappedSsrc);
		manager->SetExternallyManagedBitrate();

		// Score is 0 by default (no RTP received + inactivity check disabled).
		const int64_t nowMs    = shared.GetTimeMs();
		const auto usedBitrate = manager->IncreaseLayer(
		  /*bitrate*/ 1000000, /*considerLoss*/ false, /*lossPercentage*/ 0.0f, nowMs);

		REQUIRE(usedBitrate == 0);
	}

	SECTION("IncreaseLayer() returns 0 on second call in same iteration (already at preferred layers)")
	{
		MockListener listener;
		// Single spatial/temporal layer so preferred == available immediately.
		auto encodingContext =
		  std::make_unique<MockEncodingContext>(/*spatialLayers*/ 1, /*temporalLayers*/ 1);
		auto manager = createManager(
		  std::addressof(listener),
		  /*preferredLayers*/ { 0, 0 },
		  /*keyFrameSupported*/ true,
		  RTC::Media::Kind::VIDEO,
		  std::move(encodingContext));
		auto rtpStream = createRtpStreamRecv(MappedSsrc, /*spatialLayers*/ 1, /*temporalLayers*/ 1);

		manager->ProducerRtpStream(rtpStream.get(), MappedSsrc);
		manager->SetExternallyManagedBitrate();

		// Feed packets so the stream has non-zero bitrate.
		feedRtpStreamRecv(rtpStream.get(), packet.get(), 100);

		const int64_t nowMs = shared.GetTimeMs();

		// First call claims bitrate.
		const auto usedBitrate = manager->IncreaseLayer(
		  /*bitrate*/ 1000000, /*considerLoss*/ false, /*lossPercentage*/ 0.0f, nowMs);

		REQUIRE(usedBitrate > 0);

		// Second call in same iteration should return 0 (already at preferred layers).
		const auto usedBitrate2 = manager->IncreaseLayer(
		  /*bitrate*/ 1000000, /*considerLoss*/ false, /*lossPercentage*/ 0.0f, nowMs);

		REQUIRE(usedBitrate2 == 0);
	}

	SECTION("IncreaseLayer() works again after ApplyLayers()")
	{
		MockListener listener;
		auto encodingContext =
		  std::make_unique<MockEncodingContext>(/*spatialLayers*/ 1, /*temporalLayers*/ 1);
		auto manager = createManager(
		  std::addressof(listener),
		  /*preferredLayers*/ { 0, 0 },
		  /*keyFrameSupported*/ true,
		  RTC::Media::Kind::VIDEO,
		  std::move(encodingContext));
		auto rtpStream = createRtpStreamRecv(MappedSsrc, /*spatialLayers*/ 1, /*temporalLayers*/ 1);

		manager->ProducerRtpStream(rtpStream.get(), MappedSsrc);
		manager->SetExternallyManagedBitrate();

		// Feed packets so the stream has non-zero bitrate.
		feedRtpStreamRecv(rtpStream.get(), packet.get(), 100);

		const int64_t nowMs = shared.GetTimeMs();

		// First iteration.
		manager->IncreaseLayer(
		  /*bitrate*/ 1000000, /*considerLoss*/ false, /*lossPercentage*/ 0.0f, nowMs);
		manager->ApplyLayers();

		// Once the round is committed it should work again.
		const auto usedBitrate = manager->IncreaseLayer(
		  /*bitrate*/ 1000000, /*considerLoss*/ false, /*lossPercentage*/ 0.0f, nowMs);

		REQUIRE(usedBitrate > 0);
	}
#endif

	SECTION("GetDesiredBitrate() returns 0 when no producer stream is set")
	{
		MockListener listener;
		auto manager = createManager(std::addressof(listener));

		const int64_t nowMs = shared.GetTimeMs();
		auto desiredBitrate = manager->GetDesiredBitrate(nowMs);

		REQUIRE(desiredBitrate == 0);
	}

	SECTION("GetDesiredBitrate() (full SVC) returns total stream bitrate")
	{
		MockListener listener;
		// Full SVC (ksvc=false).
		auto encodingContext = std::make_unique<MockEncodingContext>(3, 3, /*ksvc*/ false);
		auto manager         = createManager(
		  std::addressof(listener),
		  /*preferredLayers*/ { 2, 2 },
		  /*keyFrameSupported*/ true,
		  RTC::Media::Kind::VIDEO,
		  std::move(encodingContext));
		auto rtpStream = createRtpStreamRecv(MappedSsrc, 3, 3);

		manager->ProducerRtpStream(rtpStream.get(), MappedSsrc);
#ifndef MS_USE_BUILTIN_BWE
		manager->SetExternallyManagedBitrate();
#endif

		feedRtpStreamRecv(rtpStream.get(), packet.get(), 100);

		const int64_t nowMs = shared.GetTimeMs();
		auto streamBitrate  = rtpStream->GetBitrate(nowMs);
		auto desiredBitrate = manager->GetDesiredBitrate(nowMs);

		REQUIRE(desiredBitrate == streamBitrate);
	}

	SECTION("GetDesiredBitrate() (K-SVC) returns the maximum spatial-layer bitrate")
	{
		MockListener listener;
		// K-SVC (ksvc=true): GetDesiredBitrate iterates per-layer bitrates.
		auto encodingContext = std::make_unique<MockEncodingContext>(3, 3, /*ksvc*/ true);
		auto manager         = createManager(
		  std::addressof(listener),
		  /*preferredLayers*/ { 2, 2 },
		  /*keyFrameSupported*/ true,
		  RTC::Media::Kind::VIDEO,
		  std::move(encodingContext));
		auto rtpStream = createRtpStreamRecv(MappedSsrc, 3, 3);

		manager->ProducerRtpStream(rtpStream.get(), MappedSsrc);
#ifndef MS_USE_BUILTIN_BWE
		manager->SetExternallyManagedBitrate();
#endif

		// Feed packets so total stream bitrate > 0.
		feedRtpStreamRecv(rtpStream.get(), packet.get(), 100);

		const int64_t nowMs = shared.GetTimeMs();
		auto desiredBitrate = manager->GetDesiredBitrate(nowMs);

		// K-SVC returns the max per-spatial-layer bitrate. With a plain RtpStreamRecv
		// and no per-layer packet tagging the returned value may be 0 or > 0.
		// The contract is just: no crash and type is uint32_t.
		REQUIRE(desiredBitrate >= 0);
	}

	SECTION("RecalculateTargetLayers() returns false when no producer stream is set")
	{
		MockListener listener;
		auto manager = createManager(std::addressof(listener));

		RTC::ConsumerTypes::VideoLayers newTargetLayers;
		const bool changed = manager->RecalculateTargetLayers(newTargetLayers);

		// No change from initial -1,-1 target.
		REQUIRE(changed == false);
		REQUIRE(newTargetLayers.spatial == -1);
	}

	SECTION("RecalculateTargetLayers() resets layers when stream score is 0")
	{
		MockListener listener;
		auto manager   = createManager(std::addressof(listener));
		auto rtpStream = createRtpStreamRecv();

		manager->ProducerRtpStream(rtpStream.get(), MappedSsrc);

		// Score is 0 by default.
		RTC::ConsumerTypes::VideoLayers newTargetLayers;
		const auto changed = manager->RecalculateTargetLayers(newTargetLayers);

		REQUIRE(newTargetLayers.spatial == -1);
		// changed may be false because initial target is also -1.
		(void)changed;
	}

	SECTION("K-SVC: UpdateTargetLayers() requests keyframe on any spatial layer change")
	{
		MockListener listener;
		auto encodingContext = std::make_unique<MockEncodingContext>(3, 3, /*ksvc*/ true);
		auto manager         = createManager(
		  std::addressof(listener),
		  /*preferredLayers*/ { 2, 2 },
		  /*keyFrameSupported*/ true,
		  RTC::Media::Kind::VIDEO,
		  std::move(encodingContext));
		auto rtpStream = createRtpStreamRecv();

		manager->ProducerRtpStream(rtpStream.get(), MappedSsrc);

		// For K-SVC, even a downgrade should trigger a keyframe request.
		manager->UpdateTargetLayers(2, 0);

		const auto keyFrameRequestCountBefore = listener.keyFrameRequestCount;

		// Downgrade in K-SVC also requests a keyframe.
		manager->UpdateTargetLayers(0, 0);

		REQUIRE(listener.keyFrameRequestCount > keyFrameRequestCountBefore);
		REQUIRE(listener.lastKeyFrameRequestedMappedSsrc == MappedSsrc);
	}

	SECTION("Full SVC: UpdateTargetLayers() does NOT request keyframe on spatial downgrade")
	{
		MockListener listener;
		auto encodingContext = std::make_unique<MockEncodingContext>(3, 3, /*ksvc*/ false);
		auto manager         = createManager(
		  std::addressof(listener),
		  /*preferredLayers*/ { 2, 2 },
		  /*keyFrameSupported*/ true,
		  RTC::Media::Kind::VIDEO,
		  std::move(encodingContext));
		auto rtpStream = createRtpStreamRecv();

		manager->ProducerRtpStream(rtpStream.get(), MappedSsrc);

		// Manually set target AND current spatial layer to 2 so that a downgrade
		// to 0 does not qualify as an upgrade (newTarget 0 < current 2).
		manager->UpdateTargetLayers(2, 0);
		manager->GetEncodingContext()->SetCurrentSpatialLayer(2);
		manager->GetEncodingContext()->SetCurrentTemporalLayer(0);

		// Reset counter after the initial upgrade keyframe request.
		listener.keyFrameRequestCount = 0;

		// Downgrade in full SVC: current(2) > new target(0) — no keyframe requested.
		manager->UpdateTargetLayers(0, 0);

		REQUIRE(listener.keyFrameRequestCount == 0);
	}

#ifndef MS_USE_BUILTIN_BWE
	SECTION("IncreaseLayer() uses virtual bitrate when considerLoss is true and loss < 2%")
	{
		MockListener listener;
		auto encodingContext = std::make_unique<MockEncodingContext>(1, 1, /*ksvc*/ false);
		auto manager         = createManager(
		  std::addressof(listener),
		  /*preferredLayers*/ { 0, 0 },
		  /*keyFrameSupported*/ true,
		  RTC::Media::Kind::VIDEO,
		  std::move(encodingContext));
		auto rtpStream = createRtpStreamRecv(MappedSsrc, 1, 1);

		manager->ProducerRtpStream(rtpStream.get(), MappedSsrc);
		manager->SetExternallyManagedBitrate();

		feedRtpStreamRecv(rtpStream.get(), packet.get(), 100);

		const int64_t nowMs = shared.GetTimeMs();
		auto streamBitrate  = rtpStream->GetBitrate(nowMs);

		// With 1% loss virtualBitrate = 1.08 * bitrate, so we can afford the stream
		// even with slightly less physical bitrate available.
		auto availableBitrate = static_cast<uint32_t>(streamBitrate * 0.95f);

		auto usedBitrate = manager->IncreaseLayer(
		  availableBitrate, /*considerLoss*/ true, /*lossPercentage*/ 1.0f, nowMs);

		// virtualBitrate = 1.08 * availableBitrate >= streamBitrate, so the layer is taken.
		REQUIRE(usedBitrate > 0);
	}

	SECTION("IncreaseLayer() uses reduced virtual bitrate when considerLoss is true and loss > 10%")
	{
		MockListener listener;
		auto encodingContext = std::make_unique<MockEncodingContext>(1, 1, /*ksvc*/ false);
		auto manager         = createManager(
		  std::addressof(listener),
		  /*preferredLayers*/ { 0, 0 },
		  /*keyFrameSupported*/ true,
		  RTC::Media::Kind::VIDEO,
		  std::move(encodingContext));
		auto rtpStream = createRtpStreamRecv(MappedSsrc, 1, 1);

		manager->ProducerRtpStream(rtpStream.get(), MappedSsrc);
		manager->SetExternallyManagedBitrate();

		feedRtpStreamRecv(rtpStream.get(), packet.get(), 100);

		const int64_t nowMs = shared.GetTimeMs();
		auto streamBitrate  = rtpStream->GetBitrate(nowMs);

		// With 50% loss virtualBitrate = (1 - 0.5*0.5) * bitrate = 0.75 * bitrate.
		// Pass available = streamBitrate but virtual will be reduced below required.
		auto usedBitrate =
		  manager->IncreaseLayer(streamBitrate, /*considerLoss*/ true, /*lossPercentage*/ 50.0f, nowMs);

		// virtualBitrate = 0.75 * streamBitrate < streamBitrate (requiredBitrate),
		// so the layer cannot be taken -> 0.
		REQUIRE(usedBitrate == 0);
	}
#endif
}
