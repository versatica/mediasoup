#include "common.hpp"
#include "RTC/RTCP/SenderReport.hpp"
#include "RTC/RTP/Codecs/PayloadDescriptorHandler.hpp"
#include "RTC/RTP/HeaderExtensionIds.hpp"
#include "RTC/RTP/RtpStreamRecv.hpp"
#include "RTC/RtpDictionaries.hpp"
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

		void OnRtpStreamSpatialLayerActivityChanged(
		  RTC::RTP::RtpStreamRecv* /*rtpStream*/, uint8_t /*spatialLayer*/, bool /*isActive*/) override
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
		  std::addressof(shared));
	}

	std::unique_ptr<RTC::RTP::RtpStreamRecv> createRtpStreamRecv(uint32_t ssrc)
	{
		RTC::RTP::RtpStream::Params params;

		params.ssrc      = ssrc;
		params.clockRate = 90000;

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

SCENARIO("SimulcastProducerStreamManager", "[rtp][producerstreammanager][simulcast]")
{
	std::unique_ptr<RTC::RTP::Packet> packet(
	  RTC::RTP::Packet::Factory(rtpCommon::FactoryBuffer, sizeof(rtpCommon::FactoryBuffer)));

	packet->SetPayloadType(1);
	packet->SetSsrc(MappedSsrc0);
	packet->SetTimestamp(1000);
	packet->SetPayloadLength(40);

	// The bidding protocol is named differently in each BWE, so these drive
	// whichever one is built and every SECTION below asserts the same thing.
	const auto takeBitrate =
	  [](RTC::SimulcastProducerStreamManager* manager, int64_t bitrate, int64_t nowMs) -> int64_t
	{
#ifdef MS_USE_BUILTIN_BWE
		return manager->TakeBitrate(bitrate, nowMs);
#else
		return manager->IncreaseLayer(bitrate, /*considerLoss*/ false, /*lossPercentage*/ 0.0f, nowMs);
#endif
	};

	const auto applyBitrateAllocation =
	  [](RTC::SimulcastProducerStreamManager* manager, int64_t rtpStreamActiveMs) -> void
	{
#ifdef MS_USE_BUILTIN_BWE
		manager->ApplyBitrateAllocation(rtpStreamActiveMs);
#else
		manager->ApplyLayers(rtpStreamActiveMs);
#endif
	};

	// Make the stream receive a Sender Report telling that the given RTP timestamp
	// was captured at the given instant of the clock of the remote sender.
	const auto receiveSenderReport =
	  [](RTC::RTP::RtpStreamRecv* rtpStream, int64_t captureAtUs, uint32_t rtpTs) -> void
	{
		const auto ntp = Utils::Time::TimeUsToNtp(captureAtUs);

		RTC::RTCP::SenderReport sr;

		sr.SetSsrc(rtpStream->GetSsrc());
		sr.SetNtpSec(ntp.seconds);
		sr.SetNtpFrac(ntp.fractions);
		sr.SetRtpTs(rtpTs);

		rtpStream->ReceiveRtcpSenderReport(std::addressof(sr), shared.GetTimeUs());
	};

	// Make the stream receive a packet carrying the 'abs-capture-time' extension,
	// which tells that the given RTP timestamp was captured at the given instant of
	// the clock of the remote sender.
	const auto receiveAbsCaptureTimePacket =
	  [](RTC::RTP::RtpStreamRecv* rtpStream, uint16_t seq, int64_t captureAtUs, uint32_t rtpTs) -> void
	{
		// Id the extension is given, which only has to match between what is written
		// and what is read back.
		constexpr uint8_t AbsCaptureTimeId{ 1 };

		// NOTE: Not built on `FactoryBuffer` since the packet of the SCENARIO lives
		// there.
		std::unique_ptr<RTC::RTP::Packet> absCaptureTimePacket(
		  RTC::RTP::Packet::Factory(rtpCommon::CloneBuffer, sizeof(rtpCommon::CloneBuffer)));

		REQUIRE(absCaptureTimePacket);

		absCaptureTimePacket->SetPayloadType(1);
		absCaptureTimePacket->SetSsrc(rtpStream->GetSsrc());
		absCaptureTimePacket->SetSequenceNumber(seq);
		absCaptureTimePacket->SetTimestamp(rtpTs);

		const std::vector<RTC::RTP::Packet::Extension> extensions{
			{ RTC::RtpHeaderExtensionUri::Type::ABS_CAPTURE_TIME,
			 AbsCaptureTimeId, /*len*/ 8,
			 rtpCommon::DataBuffer }
		};

		absCaptureTimePacket->SetExtensions(RTC::RTP::Packet::ExtensionsType::OneByte, extensions);

		RTC::RTP::HeaderExtensionIds headerExtensionIds;

		headerExtensionIds.absCaptureTime = AbsCaptureTimeId;

		absCaptureTimePacket->AssignExtensionIds(headerExtensionIds);

		const auto ntp = Utils::Time::TimeUsToNtp(captureAtUs);

		REQUIRE(absCaptureTimePacket->UpdateAbsCaptureTime(
		  (static_cast<uint64_t>(ntp.seconds) << 32) | static_cast<uint64_t>(ntp.fractions)));

		rtpStream->ReceivePacket(absCaptureTimePacket.get(), shared.GetTimeUs());

		REQUIRE(rtpStream->HasAbsCaptureTime());
	};

	SECTION("ProcessRtpPacket() returns DROP when target temporal layer is -1")
	{
		MockListener listener;
		auto manager    = createManager(std::addressof(listener));
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
		auto manager    = createManager(std::addressof(listener));
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
		auto manager    = createManager(std::addressof(listener));
		auto rtpStream0 = createRtpStreamRecv(MappedSsrc0);
		auto rtpStream1 = createRtpStreamRecv(MappedSsrc1);

		manager->ProducerRtpStream(rtpStream0.get(), MappedSsrc0);
		manager->ProducerRtpStream(rtpStream1.get(), MappedSsrc1);

		// Both layers get a Sender Report, so layer 1 can be aligned to the TS reference
		// layer 0.
		receiveSenderReport(rtpStream0.get(), /*captureAtUs*/ 1000 * 1000, /*rtpTs*/ 1000);
		receiveSenderReport(rtpStream1.get(), /*captureAtUs*/ 1000 * 1000, /*rtpTs*/ 1000);

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
		auto manager    = createManager(std::addressof(listener));
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
		auto manager    = createManager(std::addressof(listener), /*ssrcs*/ TwoSsrcs);
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
		auto manager    = createManager(std::addressof(listener));
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
		  std::addressof(listener),
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
		REQUIRE(result.tsOffset == 0);
	}

	SECTION("ProcessRtpPacket() returns DROP for empty payload packets on current spatial layer")
	{
		MockListener listener;
		auto manager = createManager(
		  std::addressof(listener),
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
		  std::addressof(listener),
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
		  std::addressof(listener),
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
		REQUIRE(result.tsOffset == 0);
	}

	SECTION("ProcessRtpPacket() forwards packets from current layer after spatial switch")
	{
		MockListener listener;
		auto manager = createManager(
		  std::addressof(listener),
		  /*ssrcs*/ TwoSsrcs,
		  /*preferredLayers*/ { 1, 0 },
		  /*keyFrameSupported*/ false);
		auto rtpStream0 = createRtpStreamRecv(MappedSsrc0);
		auto rtpStream1 = createRtpStreamRecv(MappedSsrc1);

		manager->ProducerRtpStream(rtpStream0.get(), MappedSsrc0);
		manager->ProducerRtpStream(rtpStream1.get(), MappedSsrc1);

		// Both layers get a Sender Report, so layer 1 can be aligned to the TS reference
		// layer 0.
		receiveSenderReport(rtpStream0.get(), /*captureAtUs*/ 1000 * 1000, /*rtpTs*/ 1000);
		receiveSenderReport(rtpStream1.get(), /*captureAtUs*/ 1000 * 1000, /*rtpTs*/ 1000);

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
		  std::addressof(listener),
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
		auto manager =
		  createManager(std::addressof(listener), /*ssrcs*/ TwoSsrcs, /*preferredLayers*/ { 1, 0 });
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

		auto manager = createManager(std::addressof(listener));

		// Don't wire producerRtpStream — manager is not active.
		manager->OnTransportConnected();

		REQUIRE(listener.keyFrameRequestCount == 0);
	}

	SECTION("RecalculateTargetLayers() skips spatial layer without capture instant")
	{
		MockListener listener;
		auto manager =
		  createManager(std::addressof(listener), /*ssrcs*/ TwoSsrcs, /*preferredLayers*/ { 1, 0 });
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
		// but CanSwitchToSpatialLayer(1) returns false because packets of layer 0 have
		// been forwarded, layer 1 cannot be aligned to it and layer 0 is alive.
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
		REQUIRE(result.tsOffset == 0);
	}

	SECTION("RecalculateTargetLayers() switches to preferred layer once its capture instant is known")
	{
		MockListener listener;
		auto manager =
		  createManager(std::addressof(listener), /*ssrcs*/ TwoSsrcs, /*preferredLayers*/ { 1, 0 });
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

		// Feed packets to both streams so ReceiveRtcpSenderReport's UpdateScore
		// doesn't drop the score to 0.
		packet->SetSsrc(MappedSsrc0);
		feedRtpStreamRecv(rtpStream0.get(), packet.get(), 10);

		packet->SetSsrc(MappedSsrc1);
		feedRtpStreamRecv(rtpStream1.get(), packet.get(), 10);

		// Provide Sender Reports for both streams so that they get a score. Both tell
		// that RTP timestamp 90000 was captured at the same instant, which is what makes
		// them alignable to each other.
		RTC::RTCP::SenderReport sr0;
		sr0.SetSsrc(MappedSsrc0);
		sr0.SetNtpSec(1000);
		sr0.SetNtpFrac(0);
		sr0.SetRtpTs(90000);
		rtpStream0->ReceiveRtcpSenderReport(std::addressof(sr0), shared.GetTimeUs());

		RTC::RTCP::SenderReport sr1;
		sr1.SetSsrc(MappedSsrc1);
		sr1.SetNtpSec(1000);
		sr1.SetNtpFrac(0);
		sr1.SetRtpTs(90000);
		rtpStream1->ReceiveRtcpSenderReport(std::addressof(sr1), shared.GetTimeUs());

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
		REQUIRE(result.tsOffset == 0);
		REQUIRE(manager->GetCurrentSpatialLayer() == 1);
	}

	SECTION("ProcessRtpPacket() calculates tsOffset from the capture instant of both layers")
	{
		MockListener listener;
		auto manager =
		  createManager(std::addressof(listener), /*ssrcs*/ TwoSsrcs, /*preferredLayers*/ { 1, 0 });
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

		packet->SetSsrc(MappedSsrc0);
		feedRtpStreamRecv(rtpStream0.get(), packet.get(), 10);

		packet->SetSsrc(MappedSsrc1);
		feedRtpStreamRecv(rtpStream1.get(), packet.get(), 10);

		// Provide Sender Reports for both streams so that they get a score. Both layers
		// hold RTP timestamp 1000, but the one of layer 1 was captured 500 ms before the
		// one of layer 0, so its RTP timeline runs 45000 ticks ahead.
		receiveSenderReport(rtpStream0.get(), /*captureAtUs*/ 1000 * 1000, /*rtpTs*/ 1000);
		receiveSenderReport(rtpStream1.get(), /*captureAtUs*/ 500 * 1000, /*rtpTs*/ 1000);

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
		REQUIRE(result.tsOffset == 45000);
		REQUIRE(manager->GetCurrentSpatialLayer() == 1);
	}

	SECTION(
	  "ProcessRtpPacket() discards the packet while its spatial layer cannot tell its capture instant")
	{
		MockListener listener;
		auto manager = createManager(
		  std::addressof(listener),
		  /*ssrcs*/ TwoSsrcs,
		  /*preferredLayers*/ { 1, 0 },
		  /*keyFrameSupported*/ false);
		auto rtpStream0 = createRtpStreamRecv(MappedSsrc0);
		auto rtpStream1 = createRtpStreamRecv(MappedSsrc1);

		manager->ProducerRtpStream(rtpStream0.get(), MappedSsrc0);
		manager->ProducerRtpStream(rtpStream1.get(), MappedSsrc1);

		// Only layer 0 gets a Sender Report, so it remains the TS reference one and layer 1
		// cannot be aligned to it yet.
		receiveSenderReport(rtpStream0.get(), /*captureAtUs*/ 1000 * 1000, /*rtpTs*/ 1000);

		// Layer 0 got its Sender Report before any packet, which left it with score 0,
		// but it is alive.
		rtpStream0->ResetScore(/*score*/ 10, /*notify*/ false);

		// Set target layer to 0 and sync. This sets tsReferenceSpatialLayer = 0.
		manager->UpdateTargetLayers(0, 0);

		packet->SetSsrc(MappedSsrc0);
		packet->SetSequenceNumber(1);
		rtpStream0->ReceivePacket(packet.get(), shared.GetTimeUs());
		manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ false, /*clockRate*/ 90000, /*maxPacketTs*/ 0);

		REQUIRE(manager->GetCurrentSpatialLayer() == 0);

		// Aim at layer 1, whose RTP timeline cannot be placed on the one of layer 0 yet.
		manager->UpdateTargetLayers(1, 0);

		const auto keyFrameRequestCountBefore = listener.keyFrameRequestCount;

		// A packet of layer 1 captured 100 ms after the forwarded one of layer 0.
		packet->SetSsrc(MappedSsrc1);
		packet->SetSequenceNumber(1);
		packet->SetTimestamp(1000 + 45000 + 9000);

		auto result = manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ false, /*clockRate*/ 90000, /*maxPacketTs*/ 1000);

		REQUIRE(result.type == RTC::ProducerStreamManager::RtpPacketProcessResult::Type::SILENT_DROP);
		REQUIRE(manager->GetCurrentSpatialLayer() == 0);
		// The discarded packet is not a key frame, so no key frame is requested for it.
		REQUIRE(listener.keyFrameRequestCount == keyFrameRequestCountBefore);

		// Once layer 1 gets a Sender Report the switch completes with the proper offset.
		receiveSenderReport(rtpStream1.get(), /*captureAtUs*/ 500 * 1000, /*rtpTs*/ 1000);

		packet->SetSequenceNumber(2);

		result = manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ false, /*clockRate*/ 90000, /*maxPacketTs*/ 1000);

		REQUIRE(result.type == RTC::ProducerStreamManager::RtpPacketProcessResult::Type::FORWARD);
		REQUIRE(result.spatialLayerSwitched == true);
		REQUIRE(result.tsOffset == 45000);
		REQUIRE(manager->GetCurrentSpatialLayer() == 1);
	}

	SECTION(
	  "ProcessRtpPacket() discards the packet while the TS reference spatial layer cannot tell its capture instant")
	{
		MockListener listener;
		auto manager = createManager(
		  std::addressof(listener),
		  /*ssrcs*/ TwoSsrcs,
		  /*preferredLayers*/ { 1, 0 },
		  /*keyFrameSupported*/ false);
		auto rtpStream0 = createRtpStreamRecv(MappedSsrc0);
		auto rtpStream1 = createRtpStreamRecv(MappedSsrc1);

		manager->ProducerRtpStream(rtpStream0.get(), MappedSsrc0);
		manager->ProducerRtpStream(rtpStream1.get(), MappedSsrc1);

		// Only layer 1 gets a Sender Report.
		receiveSenderReport(rtpStream1.get(), /*captureAtUs*/ 500 * 1000, /*rtpTs*/ 1000);

		// Set target layer to 0 and sync. This sets tsReferenceSpatialLayer = 0.
		manager->UpdateTargetLayers(0, 0);

		packet->SetSsrc(MappedSsrc0);
		packet->SetSequenceNumber(1);
		manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ false, /*clockRate*/ 90000, /*maxPacketTs*/ 0);

		REQUIRE(manager->GetCurrentSpatialLayer() == 0);

		// Aim at layer 1. Packets of layer 0 have been forwarded, so it remains the TS
		// reference one although it cannot tell its capture instant.
		manager->UpdateTargetLayers(1, 0);

		packet->SetSsrc(MappedSsrc1);
		packet->SetSequenceNumber(1);

		auto result = manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ false, /*clockRate*/ 90000, /*maxPacketTs*/ 0);

		REQUIRE(result.type == RTC::ProducerStreamManager::RtpPacketProcessResult::Type::SILENT_DROP);
		REQUIRE(manager->GetCurrentSpatialLayer() == 0);

		// Packets of the current layer 0 are still forwarded with their RTP timestamps
		// untouched.
		packet->SetSsrc(MappedSsrc0);
		packet->SetSequenceNumber(2);

		result = manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ false, /*clockRate*/ 90000, /*maxPacketTs*/ 0);

		REQUIRE(result.type == RTC::ProducerStreamManager::RtpPacketProcessResult::Type::FORWARD);
		REQUIRE(result.tsOffset == 0);
	}

	SECTION(
	  "ProcessRtpPacket() keeps forwarding the current spatial layer while waiting to be able to align the key frame of the target one")
	{
		MockListener listener;
		auto manager =
		  createManager(std::addressof(listener), /*ssrcs*/ TwoSsrcs, /*preferredLayers*/ { 1, 0 });
		auto rtpStream0 = createRtpStreamRecv(MappedSsrc0);
		auto rtpStream1 = createRtpStreamRecv(MappedSsrc1);

		manager->ProducerRtpStream(rtpStream0.get(), MappedSsrc0);
		manager->ProducerRtpStream(rtpStream1.get(), MappedSsrc1);

		// Set target layer to 0 and sync with a key frame. This sets
		// tsReferenceSpatialLayer = 0.
		manager->UpdateTargetLayers(0, 0);

		auto* handler0       = new MockPayloadDescriptorHandler();
		handler0->isKeyFrame = true;
		packet->SetPayloadDescriptorHandler(handler0);

		packet->SetSsrc(MappedSsrc0);
		packet->SetSequenceNumber(1);

		auto result = manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ true, /*clockRate*/ 90000, /*maxPacketTs*/ 0);

		REQUIRE(result.type == RTC::ProducerStreamManager::RtpPacketProcessResult::Type::FORWARD);
		REQUIRE(manager->GetCurrentSpatialLayer() == 0);

		// Aim at layer 1, which cannot be aligned to the alive layer 0 since none of
		// them can tell its capture instant, so its key frame is discarded.
		manager->UpdateTargetLayers(1, 0);

		auto* handler1       = new MockPayloadDescriptorHandler();
		handler1->isKeyFrame = true;
		packet->SetPayloadDescriptorHandler(handler1);

		packet->SetSsrc(MappedSsrc1);
		packet->SetSequenceNumber(100);

		result = manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ true, /*clockRate*/ 90000, /*maxPacketTs*/ 1000);

		REQUIRE(result.type == RTC::ProducerStreamManager::RtpPacketProcessResult::Type::SILENT_DROP);
		REQUIRE(manager->GetCurrentSpatialLayer() == 0);

		// Packets of the current layer 0 that are not key frames are still forwarded.
		auto* handler2       = new MockPayloadDescriptorHandler();
		handler2->isKeyFrame = false;
		packet->SetPayloadDescriptorHandler(handler2);

		packet->SetSsrc(MappedSsrc0);
		packet->SetSequenceNumber(2);

		result = manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ true, /*clockRate*/ 90000, /*maxPacketTs*/ 1000);

		REQUIRE(result.type == RTC::ProducerStreamManager::RtpPacketProcessResult::Type::FORWARD);
		REQUIRE(result.isSyncPacket == false);
	}

	SECTION(
	  "RecalculateTargetLayers() does not switch away from the TS reference spatial layer while it cannot tell its capture instant and its packets have been forwarded")
	{
		// Scenario based on https://github.com/versatica/mediasoup/issues/2000.
		MockListener listener;
		auto manager = createManager(
		  std::addressof(listener),
		  /*ssrcs*/ TwoSsrcs,
		  /*preferredLayers*/ { 1, 0 },
		  /*keyFrameSupported*/ false);
		auto rtpStream0 = createRtpStreamRecv(MappedSsrc0);
		auto rtpStream1 = createRtpStreamRecv(MappedSsrc1);

		manager->ProducerRtpStream(rtpStream0.get(), MappedSsrc0);
		manager->ProducerRtpStream(rtpStream1.get(), MappedSsrc1);

		// Only layer 1 gets a Sender Report.
		receiveSenderReport(rtpStream1.get(), /*captureAtUs*/ 500 * 1000, /*rtpTs*/ 1000);

		// Set target layer to 0 and sync. This sets tsReferenceSpatialLayer = 0.
		manager->UpdateTargetLayers(0, 0);

		packet->SetSsrc(MappedSsrc0);
		packet->SetSequenceNumber(1);
		packet->SetTimestamp(1000);

		// Layer 0 receives packets, so its highest RTP timestamp is 1000.
		feedRtpStreamRecv(rtpStream0.get(), packet.get(), 10);

		auto result = manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ false, /*clockRate*/ 90000, /*maxPacketTs*/ 0);

		REQUIRE(result.type == RTC::ProducerStreamManager::RtpPacketProcessResult::Type::FORWARD);
		REQUIRE(result.tsOffset == 0);
		REQUIRE(manager->GetCurrentSpatialLayer() == 0);

		// Layer 1 got its Sender Report before any packet, which left it with score 0,
		// so give it a score. Then notify the score of both layers, which makes the
		// manager check layers.
		rtpStream1->ResetScore(/*score*/ 10, /*notify*/ false);
		manager->ProducerRtpStreamScore(rtpStream0.get(), /*score*/ 10, /*previousScore*/ 0);
		manager->ProducerRtpStreamScore(rtpStream1.get(), /*score*/ 10, /*previousScore*/ 0);

		// Packets of layer 0 have been forwarded and its capture instant cannot be told,
		// so layer 1 cannot be aligned to it and the target stays at layer 0.
		REQUIRE(manager->GetTargetLayers().spatial == 0);

		packet->SetSsrc(MappedSsrc1);
		packet->SetSequenceNumber(1);
		packet->SetTimestamp(91000);

		result = manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ false, /*clockRate*/ 90000, /*maxPacketTs*/ 1000);

		REQUIRE(result.type == RTC::ProducerStreamManager::RtpPacketProcessResult::Type::SILENT_DROP);
		REQUIRE(manager->GetCurrentSpatialLayer() == 0);

		// Layer 0 gets its first Sender Report. Both layers hold RTP timestamp 1000, but
		// the one of layer 1 was captured 500 ms before the one of layer 0, so its RTP
		// timeline runs 45000 ticks ahead.
		receiveSenderReport(rtpStream0.get(), /*captureAtUs*/ 1000 * 1000, /*rtpTs*/ 1000);

		manager->ProducerRtcpSenderReport(rtpStream0.get(), /*first*/ true);

		REQUIRE(manager->GetTargetLayers().spatial == 1);

		packet->SetSequenceNumber(2);

		result = manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ false, /*clockRate*/ 90000, /*maxPacketTs*/ 1000);

		REQUIRE(result.type == RTC::ProducerStreamManager::RtpPacketProcessResult::Type::FORWARD);
		REQUIRE(result.spatialLayerSwitched == true);
		REQUIRE(result.tsOffset == 45000);
		REQUIRE(manager->GetCurrentSpatialLayer() == 1);
	}

	SECTION(
	  "ProducerRtcpSenderReport() switches to a spatial layer as soon as it and the TS reference one got a Sender Report")
	{
		MockListener listener;
		auto manager = createManager(
		  std::addressof(listener),
		  /*ssrcs*/ TwoSsrcs,
		  /*preferredLayers*/ { 1, 0 },
		  /*keyFrameSupported*/ false);
		auto rtpStream0 = createRtpStreamRecv(MappedSsrc0);
		auto rtpStream1 = createRtpStreamRecv(MappedSsrc1);

		manager->ProducerRtpStream(rtpStream0.get(), MappedSsrc0);
		manager->ProducerRtpStream(rtpStream1.get(), MappedSsrc1);

		// Set target layer to 0 and sync. This sets tsReferenceSpatialLayer = 0.
		manager->UpdateTargetLayers(0, 0);

		packet->SetSsrc(MappedSsrc0);
		packet->SetSequenceNumber(1);
		packet->SetTimestamp(90000);

		auto result = manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ false, /*clockRate*/ 90000, /*maxPacketTs*/ 0);

		REQUIRE(result.type == RTC::ProducerStreamManager::RtpPacketProcessResult::Type::FORWARD);
		REQUIRE(manager->GetCurrentSpatialLayer() == 0);

		// Layer 0 gets packets and its first Sender Report. Layer 1 has no Sender Report
		// yet, so it cannot be aligned to layer 0 and the target stays at layer 0.
		feedRtpStreamRecv(rtpStream0.get(), packet.get(), 10);

		RTC::RTCP::SenderReport sr0;
		sr0.SetSsrc(MappedSsrc0);
		sr0.SetNtpSec(1000);
		sr0.SetNtpFrac(0);
		sr0.SetRtpTs(90000);
		rtpStream0->ReceiveRtcpSenderReport(std::addressof(sr0), shared.GetTimeUs());

		manager->ProducerRtcpSenderReport(rtpStream0.get(), /*first*/ true);

		REQUIRE(manager->GetTargetLayers().spatial == 0);

		// Layer 1 gets packets and its first Sender Report, which tells that its RTP
		// timeline runs 45000 ticks ahead of the one of layer 0. No other Sender Report
		// is needed to align both layers.
		packet->SetSsrc(MappedSsrc1);
		packet->SetTimestamp(135000);

		feedRtpStreamRecv(rtpStream1.get(), packet.get(), 10);

		RTC::RTCP::SenderReport sr1;
		sr1.SetSsrc(MappedSsrc1);
		sr1.SetNtpSec(1000);
		sr1.SetNtpFrac(0);
		sr1.SetRtpTs(135000);
		rtpStream1->ReceiveRtcpSenderReport(std::addressof(sr1), shared.GetTimeUs());

		manager->ProducerRtcpSenderReport(rtpStream1.get(), /*first*/ true);

		REQUIRE(manager->GetTargetLayers().spatial == 1);

		// A packet of layer 1 captured 100 ms after the last forwarded one of layer 0.
		packet->SetSequenceNumber(100);
		packet->SetTimestamp(144000);

		result = manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ false, /*clockRate*/ 90000, /*maxPacketTs*/ 90000);

		REQUIRE(result.type == RTC::ProducerStreamManager::RtpPacketProcessResult::Type::FORWARD);
		REQUIRE(result.spatialLayerSwitched == true);
		REQUIRE(result.tsOffset == 45000);
		REQUIRE(manager->GetCurrentSpatialLayer() == 1);
	}

	SECTION(
	  "RecalculateTargetLayers() switches away from a dead TS reference spatial layer that never got a Sender Report keeping the RTP timeline")
	{
		MockListener listener;
		auto manager = createManager(
		  std::addressof(listener),
		  /*ssrcs*/ TwoSsrcs,
		  /*preferredLayers*/ { 1, 0 },
		  /*keyFrameSupported*/ false);
		auto rtpStream0 = createRtpStreamRecv(MappedSsrc0);
		auto rtpStream1 = createRtpStreamRecv(MappedSsrc1);

		manager->ProducerRtpStream(rtpStream0.get(), MappedSsrc0);
		manager->ProducerRtpStream(rtpStream1.get(), MappedSsrc1);

		// Layer 1 gets packets and a Sender Report. Layer 0 gets packets but no Sender
		// Report.
		packet->SetSsrc(MappedSsrc1);
		packet->SetTimestamp(135000);

		feedRtpStreamRecv(rtpStream1.get(), packet.get(), 10);
		receiveSenderReport(rtpStream1.get(), /*captureAtUs*/ 1000 * 1000, /*rtpTs*/ 135000);

		packet->SetSsrc(MappedSsrc0);
		packet->SetTimestamp(90000);

		feedRtpStreamRecv(rtpStream0.get(), packet.get(), 10);

		// Set target layer to 0 and sync. This sets tsReferenceSpatialLayer = 0.
		manager->UpdateTargetLayers(0, 0);

		packet->SetSequenceNumber(1);

		auto result = manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ false, /*clockRate*/ 90000, /*maxPacketTs*/ 0);

		REQUIRE(result.type == RTC::ProducerStreamManager::RtpPacketProcessResult::Type::FORWARD);
		REQUIRE(manager->GetCurrentSpatialLayer() == 0);

		manager->ProducerRtpStreamScore(rtpStream0.get(), /*score*/ 10, /*previousScore*/ 0);

		// Layer 1 cannot be aligned to layer 0, so the target stays at layer 0.
		REQUIRE(manager->GetTargetLayers().spatial == 0);

		// Layer 0 dies, so it cannot be relied on anymore and layer 1 is picked although
		// it cannot be aligned to it.
		rtpStream0->ResetScore(/*score*/ 0, /*notify*/ false);
		manager->ProducerRtpStreamScore(rtpStream0.get(), /*score*/ 0, /*previousScore*/ 10);

		REQUIRE(manager->GetTargetLayers().spatial == 1);

		// The first packet of layer 1 arrives 2 seconds after the last forwarded one, so
		// the RTP timeline of our stream goes on 2 seconds after the highest RTP
		// timestamp sent so far.
		nowUs += 2000 * 1000;

		packet->SetSsrc(MappedSsrc1);
		packet->SetSequenceNumber(100);
		packet->SetTimestamp(144000);

		result = manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ false, /*clockRate*/ 90000, /*maxPacketTs*/ 90000);

		REQUIRE(result.type == RTC::ProducerStreamManager::RtpPacketProcessResult::Type::FORWARD);
		REQUIRE(result.spatialLayerSwitched == true);
		REQUIRE(manager->GetCurrentSpatialLayer() == 1);
		REQUIRE(packet->GetTimestamp() - result.tsOffset == 90000 + (2 * 90000));

		// Following packets of layer 1 keep the same offset.
		packet->SetSequenceNumber(101);
		packet->SetTimestamp(147000);

		const auto tsOffset = result.tsOffset;

		result = manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ false, /*clockRate*/ 90000, /*maxPacketTs*/ 270000);

		REQUIRE(result.type == RTC::ProducerStreamManager::RtpPacketProcessResult::Type::FORWARD);
		REQUIRE(result.tsOffset == tsOffset);
	}

	SECTION(
	  "ProcessRtpPacket() places the first packet of the new spatial layer right after the highest RTP timestamp sent if it was captured at the same instant whatever the frame rate")
	{
		MockListener listener;
		auto manager = createManager(
		  std::addressof(listener),
		  /*ssrcs*/ TwoSsrcs,
		  /*preferredLayers*/ { 1, 0 },
		  /*keyFrameSupported*/ false);
		auto rtpStream0 = createRtpStreamRecv(MappedSsrc0);
		auto rtpStream1 = createRtpStreamRecv(MappedSsrc1);

		manager->ProducerRtpStream(rtpStream0.get(), MappedSsrc0);
		manager->ProducerRtpStream(rtpStream1.get(), MappedSsrc1);

		// The RTP timeline of layer 1 runs 45000 ticks ahead of the one of layer 0.
		receiveSenderReport(rtpStream0.get(), /*captureAtUs*/ 1000 * 1000, /*rtpTs*/ 90000);
		receiveSenderReport(rtpStream1.get(), /*captureAtUs*/ 1000 * 1000, /*rtpTs*/ 135000);

		// Set target layer to 0 and sync. This sets tsReferenceSpatialLayer = 0.
		manager->UpdateTargetLayers(0, 0);

		// Layer 0 is forwarded at 1 fps, as a static screen sharing would be, so a
		// frame lasts 90000 ticks.
		uint32_t maxPacketTs{ 0 };

		packet->SetSsrc(MappedSsrc0);

		for (uint16_t seq{ 1 }; seq <= 3; ++seq)
		{
			packet->SetSequenceNumber(seq);
			packet->SetTimestamp(90000 + ((seq - 1) * 90000));
			rtpStream0->ReceivePacket(packet.get(), shared.GetTimeUs());

			const auto result = manager->ProcessRtpPacket(
			  packet.get(), /*lastSentPacketHasMarker*/ true, /*clockRate*/ 90000, maxPacketTs);

			REQUIRE(result.type == RTC::ProducerStreamManager::RtpPacketProcessResult::Type::FORWARD);

			maxPacketTs = packet->GetTimestamp() - result.tsOffset;
		}

		REQUIRE(maxPacketTs == 270000);

		manager->UpdateTargetLayers(1, 0);

		// A packet of layer 1 captured at the same instant as the last forwarded one of
		// layer 0.
		packet->SetSsrc(MappedSsrc1);
		packet->SetSequenceNumber(100);
		packet->SetTimestamp(270000 + 45000);

		const auto result = manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ true, /*clockRate*/ 90000, maxPacketTs);

		REQUIRE(result.type == RTC::ProducerStreamManager::RtpPacketProcessResult::Type::FORWARD);
		REQUIRE(result.spatialLayerSwitched == true);
		REQUIRE(packet->GetTimestamp() - result.tsOffset == 270000 + 1);
	}

	SECTION(
	  "ProcessRtpPacket() places the first packet of the new spatial layer right after the highest RTP timestamp sent if it was captured at the same instant")
	{
		MockListener listener;
		auto manager = createManager(
		  std::addressof(listener),
		  /*ssrcs*/ TwoSsrcs,
		  /*preferredLayers*/ { 1, 0 },
		  /*keyFrameSupported*/ false);
		auto rtpStream0 = createRtpStreamRecv(MappedSsrc0);
		auto rtpStream1 = createRtpStreamRecv(MappedSsrc1);

		manager->ProducerRtpStream(rtpStream0.get(), MappedSsrc0);
		manager->ProducerRtpStream(rtpStream1.get(), MappedSsrc1);

		// The RTP timeline of layer 1 runs 45000 ticks ahead of the one of layer 0.
		receiveSenderReport(rtpStream0.get(), /*captureAtUs*/ 1000 * 1000, /*rtpTs*/ 90000);
		receiveSenderReport(rtpStream1.get(), /*captureAtUs*/ 1000 * 1000, /*rtpTs*/ 135000);

		// Set target layer to 0 and sync. This sets tsReferenceSpatialLayer = 0.
		manager->UpdateTargetLayers(0, 0);

		packet->SetSsrc(MappedSsrc0);
		packet->SetSequenceNumber(1);
		packet->SetTimestamp(90000);
		rtpStream0->ReceivePacket(packet.get(), shared.GetTimeUs());

		auto result = manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ true, /*clockRate*/ 90000, /*maxPacketTs*/ 0);

		REQUIRE(result.type == RTC::ProducerStreamManager::RtpPacketProcessResult::Type::FORWARD);

		manager->UpdateTargetLayers(1, 0);

		// A packet of layer 1 captured at the same instant as the forwarded one of
		// layer 0.
		packet->SetSsrc(MappedSsrc1);
		packet->SetSequenceNumber(100);
		packet->SetTimestamp(135000);

		result = manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ true, /*clockRate*/ 90000, /*maxPacketTs*/ 90000);

		REQUIRE(result.type == RTC::ProducerStreamManager::RtpPacketProcessResult::Type::FORWARD);
		REQUIRE(result.spatialLayerSwitched == true);
		REQUIRE(packet->GetTimestamp() - result.tsOffset == 90001);
	}

	SECTION(
	  "ProcessRtpPacket() places the first packet of the new spatial layer after the highest RTP timestamp sent when they are across the wrap around")
	{
		MockListener listener;
		auto manager = createManager(
		  std::addressof(listener),
		  /*ssrcs*/ TwoSsrcs,
		  /*preferredLayers*/ { 1, 0 },
		  /*keyFrameSupported*/ false);
		auto rtpStream0 = createRtpStreamRecv(MappedSsrc0);
		auto rtpStream1 = createRtpStreamRecv(MappedSsrc1);

		manager->ProducerRtpStream(rtpStream0.get(), MappedSsrc0);
		manager->ProducerRtpStream(rtpStream1.get(), MappedSsrc1);

		// The RTP timeline of layer 1 runs 45000 ticks ahead of the one of layer 0.
		receiveSenderReport(rtpStream0.get(), /*captureAtUs*/ 1000 * 1000, /*rtpTs*/ 100);
		receiveSenderReport(rtpStream1.get(), /*captureAtUs*/ 1000 * 1000, /*rtpTs*/ 45100);

		// Set target layer to 0 and sync. This sets tsReferenceSpatialLayer = 0.
		manager->UpdateTargetLayers(0, 0);

		packet->SetSsrc(MappedSsrc0);
		packet->SetSequenceNumber(1);
		packet->SetTimestamp(100);
		rtpStream0->ReceivePacket(packet.get(), shared.GetTimeUs());

		auto result = manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ true, /*clockRate*/ 90000, /*maxPacketTs*/ 0);

		REQUIRE(result.type == RTC::ProducerStreamManager::RtpPacketProcessResult::Type::FORWARD);

		manager->UpdateTargetLayers(1, 0);

		// A packet of layer 1 captured 10 ms before the forwarded one of layer 0, so once
		// aligned its RTP timestamp falls 900 ticks before 100, across the wrap around.
		packet->SetSsrc(MappedSsrc1);
		packet->SetSequenceNumber(100);
		packet->SetTimestamp(45100 - 900);

		result = manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ true, /*clockRate*/ 90000, /*maxPacketTs*/ 100);

		REQUIRE(result.type == RTC::ProducerStreamManager::RtpPacketProcessResult::Type::FORWARD);
		REQUIRE(result.spatialLayerSwitched == true);
		REQUIRE(packet->GetTimestamp() - result.tsOffset == 101);
	}

	SECTION(
	  "ProcessRtpPacket() places the first packet of the new spatial layer after the highest RTP timestamp sent when giving up on proper stream switching")
	{
		MockListener listener;
		auto manager = createManager(
		  std::addressof(listener),
		  /*ssrcs*/ TwoSsrcs,
		  /*preferredLayers*/ { 1, 0 },
		  /*keyFrameSupported*/ false);
		auto rtpStream0 = createRtpStreamRecv(MappedSsrc0);
		auto rtpStream1 = createRtpStreamRecv(MappedSsrc1);

		manager->ProducerRtpStream(rtpStream0.get(), MappedSsrc0);
		manager->ProducerRtpStream(rtpStream1.get(), MappedSsrc1);

		// The RTP timeline of layer 1 runs 45000 ticks ahead of the one of layer 0.
		receiveSenderReport(rtpStream0.get(), /*captureAtUs*/ 1000 * 1000, /*rtpTs*/ 90000);
		receiveSenderReport(rtpStream1.get(), /*captureAtUs*/ 1000 * 1000, /*rtpTs*/ 135000);

		// Set target layer to 0 and sync. This sets tsReferenceSpatialLayer = 0.
		manager->UpdateTargetLayers(0, 0);

		packet->SetSsrc(MappedSsrc0);
		packet->SetSequenceNumber(1);
		packet->SetTimestamp(90000);
		rtpStream0->ReceivePacket(packet.get(), shared.GetTimeUs());

		auto result = manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ true, /*clockRate*/ 90000, /*maxPacketTs*/ 0);

		REQUIRE(result.type == RTC::ProducerStreamManager::RtpPacketProcessResult::Type::FORWARD);

		manager->UpdateTargetLayers(1, 0);

		const auto keyFrameRequestCountBefore = listener.keyFrameRequestCount;

		// A packet of layer 1 captured 100 ms before the forwarded one of layer 0, which
		// needs more extra offset than allowed, so it is discarded and a key frame is
		// requested.
		packet->SetSsrc(MappedSsrc1);
		packet->SetSequenceNumber(100);
		packet->SetTimestamp(135000 - 9000);

		result = manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ true, /*clockRate*/ 90000, /*maxPacketTs*/ 90000);

		REQUIRE(result.type == RTC::ProducerStreamManager::RtpPacketProcessResult::Type::SILENT_DROP);
		REQUIRE(listener.keyFrameRequestCount == keyFrameRequestCountBefore + 1);

		// The requested key frame needs the same extra offset, so switching gives up on
		// keeping the capture instant but not on the RTP timeline.
		packet->SetSequenceNumber(101);

		result = manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ true, /*clockRate*/ 90000, /*maxPacketTs*/ 90000);

		REQUIRE(result.type == RTC::ProducerStreamManager::RtpPacketProcessResult::Type::FORWARD);
		REQUIRE(result.spatialLayerSwitched == true);
		REQUIRE(packet->GetTimestamp() - result.tsOffset == 90001);
	}

	SECTION(
	  "ProcessRtpPacket() places the first packet of the new spatial layer right after the highest RTP timestamp sent although temporal layers are dropped")
	{
		MockListener listener;
		auto manager = createManager(
		  std::addressof(listener),
		  /*ssrcs*/ TwoSsrcs,
		  /*preferredLayers*/ { 1, 0 },
		  /*keyFrameSupported*/ false);
		auto rtpStream0 = createRtpStreamRecv(MappedSsrc0);
		auto rtpStream1 = createRtpStreamRecv(MappedSsrc1);

		manager->ProducerRtpStream(rtpStream0.get(), MappedSsrc0);
		manager->ProducerRtpStream(rtpStream1.get(), MappedSsrc1);

		// The RTP timeline of layer 1 runs 45000 ticks ahead of the one of layer 0.
		receiveSenderReport(rtpStream0.get(), /*captureAtUs*/ 1000 * 1000, /*rtpTs*/ 90000);
		receiveSenderReport(rtpStream1.get(), /*captureAtUs*/ 1000 * 1000, /*rtpTs*/ 135000);

		// Set target layer to 0 and sync. This sets tsReferenceSpatialLayer = 0.
		manager->UpdateTargetLayers(0, 0);

		// Layer 0 is produced at 25 fps, so a frame lasts 3600 ticks, but only one out of
		// four frames belongs to the target temporal layer and is forwarded.
		uint32_t maxPacketTs{ 0 };

		packet->SetSsrc(MappedSsrc0);

		for (uint16_t seq{ 1 }; seq <= 9; ++seq)
		{
			const bool forwarded = (seq - 1) % 4 == 0;

			auto* handler          = new MockPayloadDescriptorHandler();
			handler->processResult = forwarded;
			packet->SetPayloadDescriptorHandler(handler);

			packet->SetSequenceNumber(seq);
			packet->SetTimestamp(90000 + ((seq - 1) * 3600));
			rtpStream0->ReceivePacket(packet.get(), shared.GetTimeUs());

			const auto result = manager->ProcessRtpPacket(
			  packet.get(), /*lastSentPacketHasMarker*/ true, /*clockRate*/ 90000, maxPacketTs);

			if (forwarded)
			{
				REQUIRE(result.type == RTC::ProducerStreamManager::RtpPacketProcessResult::Type::FORWARD);

				maxPacketTs = packet->GetTimestamp() - result.tsOffset;
			}
			else
			{
				REQUIRE(result.type == RTC::ProducerStreamManager::RtpPacketProcessResult::Type::DROP);
			}
		}

		REQUIRE(maxPacketTs == 90000 + (8 * 3600));

		manager->UpdateTargetLayers(1, 0);

		// A packet of layer 1 captured at the same instant as the last forwarded one of
		// layer 0.
		auto* handler1 = new MockPayloadDescriptorHandler();
		packet->SetPayloadDescriptorHandler(handler1);

		packet->SetSsrc(MappedSsrc1);
		packet->SetSequenceNumber(100);
		packet->SetTimestamp(maxPacketTs + 45000);

		const auto result = manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ true, /*clockRate*/ 90000, maxPacketTs);

		REQUIRE(result.type == RTC::ProducerStreamManager::RtpPacketProcessResult::Type::FORWARD);
		REQUIRE(result.spatialLayerSwitched == true);
		REQUIRE(packet->GetTimestamp() - result.tsOffset == maxPacketTs + 1);
	}

	SECTION(
	  "ProducerRtpStreamScore() keeps the current spatial layer when the TS reference one dies and cannot be aligned to anymore")
	{
		MockListener listener;
		auto manager = createManager(
		  std::addressof(listener),
		  /*ssrcs*/ TwoSsrcs,
		  /*preferredLayers*/ { 1, 0 },
		  /*keyFrameSupported*/ false);
		auto rtpStream0 = createRtpStreamRecv(MappedSsrc0);
		auto rtpStream1 = createRtpStreamRecv(MappedSsrc1);

		manager->ProducerRtpStream(rtpStream0.get(), MappedSsrc0);
		manager->ProducerRtpStream(rtpStream1.get(), MappedSsrc1);

		// Both layers get packets. The RTP timeline of layer 1 runs 45000 ticks ahead
		// of the one of layer 0.
		packet->SetSsrc(MappedSsrc0);
		packet->SetTimestamp(90000);
		feedRtpStreamRecv(rtpStream0.get(), packet.get(), 10);
		receiveSenderReport(rtpStream0.get(), /*captureAtUs*/ 1000 * 1000, /*rtpTs*/ 90000);

		packet->SetSsrc(MappedSsrc1);
		packet->SetTimestamp(144000);
		feedRtpStreamRecv(rtpStream1.get(), packet.get(), 10);
		receiveSenderReport(rtpStream1.get(), /*captureAtUs*/ 1000 * 1000, /*rtpTs*/ 135000);

		// Set target layer to 0 and sync. This sets tsReferenceSpatialLayer = 0.
		manager->UpdateTargetLayers(0, 0);

		packet->SetSsrc(MappedSsrc0);
		packet->SetSequenceNumber(1);
		packet->SetTimestamp(90000);

		auto result = manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ true, /*clockRate*/ 90000, /*maxPacketTs*/ 0);

		REQUIRE(result.type == RTC::ProducerStreamManager::RtpPacketProcessResult::Type::FORWARD);

		// Switch to layer 1, aligned to layer 0.
		manager->UpdateTargetLayers(1, 0);

		packet->SetSsrc(MappedSsrc1);
		packet->SetSequenceNumber(100);
		packet->SetTimestamp(144000);

		result = manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ true, /*clockRate*/ 90000, /*maxPacketTs*/ 90000);

		REQUIRE(result.type == RTC::ProducerStreamManager::RtpPacketProcessResult::Type::FORWARD);
		REQUIRE(result.spatialLayerSwitched == true);
		REQUIRE(result.tsOffset == 45000);

		// Layer 0 stops sending RTP but its Sender Reports go on 20 seconds later, so
		// the highest RTP timestamp of it is too far from them to be aligned to.
		receiveSenderReport(
		  rtpStream0.get(), /*captureAtUs*/ (1000 + 20000) * 1000, /*rtpTs*/ 90000 + (20 * 90000));

		// Then layer 0 dies.
		rtpStream0->ResetScore(/*score*/ 0, /*notify*/ false);
		manager->ProducerRtpStreamScore(rtpStream0.get(), /*score*/ 0, /*previousScore*/ 10);

		REQUIRE(manager->GetTargetLayers().spatial == 1);
		REQUIRE(manager->GetCurrentSpatialLayer() == 1);

		// Packets of layer 1 go on with the same offset.
		packet->SetSequenceNumber(101);
		packet->SetTimestamp(147600);

		result = manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ true, /*clockRate*/ 90000, /*maxPacketTs*/ 99000);

		REQUIRE(result.type == RTC::ProducerStreamManager::RtpPacketProcessResult::Type::FORWARD);
		REQUIRE(result.tsOffset == 45000);
	}

	SECTION(
	  "ProducerRtpStreamScore() switches away from the dying TS reference spatial layer when it cannot be aligned to anymore keeping the RTP timeline")
	{
		MockListener listener;
		auto manager = createManager(
		  std::addressof(listener),
		  /*ssrcs*/ TwoSsrcs,
		  /*preferredLayers*/ { 0, 0 },
		  /*keyFrameSupported*/ false);
		auto rtpStream0 = createRtpStreamRecv(MappedSsrc0);
		auto rtpStream1 = createRtpStreamRecv(MappedSsrc1);

		manager->ProducerRtpStream(rtpStream0.get(), MappedSsrc0);
		manager->ProducerRtpStream(rtpStream1.get(), MappedSsrc1);

		// Both layers get packets. The RTP timeline of layer 1 runs 45000 ticks ahead
		// of the one of layer 0.
		packet->SetSsrc(MappedSsrc0);
		packet->SetTimestamp(90000);
		feedRtpStreamRecv(rtpStream0.get(), packet.get(), 10);
		receiveSenderReport(rtpStream0.get(), /*captureAtUs*/ 1000 * 1000, /*rtpTs*/ 90000);

		packet->SetSsrc(MappedSsrc1);
		packet->SetTimestamp(144000);
		feedRtpStreamRecv(rtpStream1.get(), packet.get(), 10);
		receiveSenderReport(rtpStream1.get(), /*captureAtUs*/ 1000 * 1000, /*rtpTs*/ 135000);

		// Set target layer to 0 and sync. This sets tsReferenceSpatialLayer = 0.
		manager->UpdateTargetLayers(0, 0);

		packet->SetSsrc(MappedSsrc0);
		packet->SetSequenceNumber(1);
		packet->SetTimestamp(90000);

		auto result = manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ true, /*clockRate*/ 90000, /*maxPacketTs*/ 0);

		REQUIRE(result.type == RTC::ProducerStreamManager::RtpPacketProcessResult::Type::FORWARD);

		// Layer 0 stops sending RTP but its Sender Reports go on 20 seconds later, so
		// the highest RTP timestamp of it is too far from them to be aligned to.
		receiveSenderReport(
		  rtpStream0.get(), /*captureAtUs*/ (1000 + 20000) * 1000, /*rtpTs*/ 90000 + (20 * 90000));

		// Then layer 0 dies, so layer 1 is picked although it cannot be aligned to it.
		rtpStream0->ResetScore(/*score*/ 0, /*notify*/ false);
		manager->ProducerRtpStreamScore(rtpStream0.get(), /*score*/ 0, /*previousScore*/ 10);

		REQUIRE(manager->GetTargetLayers().spatial == 1);

		// The RTP timeline of our stream goes on right after the highest RTP timestamp
		// sent so far.
		packet->SetSsrc(MappedSsrc1);
		packet->SetSequenceNumber(100);
		packet->SetTimestamp(144000);

		result = manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ true, /*clockRate*/ 90000, /*maxPacketTs*/ 90000);

		REQUIRE(result.type == RTC::ProducerStreamManager::RtpPacketProcessResult::Type::FORWARD);
		REQUIRE(result.spatialLayerSwitched == true);
		REQUIRE(packet->GetTimestamp() - result.tsOffset == 90001);
	}

	SECTION(
	  "ProcessRtpPacket() takes the new spatial layer as TS reference once switched to it if the TS reference one is dead")
	{
		MockListener listener;
		auto manager = createManager(
		  std::addressof(listener),
		  /*ssrcs*/ ThreeSsrcs,
		  /*preferredLayers*/ { 1, 0 },
		  /*keyFrameSupported*/ false);
		auto rtpStream0 = createRtpStreamRecv(MappedSsrc0);
		auto rtpStream1 = createRtpStreamRecv(MappedSsrc1);
		auto rtpStream2 = createRtpStreamRecv(MappedSsrc2);

		manager->ProducerRtpStream(rtpStream0.get(), MappedSsrc0);
		manager->ProducerRtpStream(rtpStream1.get(), MappedSsrc1);
		manager->ProducerRtpStream(rtpStream2.get(), MappedSsrc2);

		// All layers get packets. The RTP timelines of layers 1 and 2 run 45000 and
		// 90000 ticks ahead of the one of layer 0.
		packet->SetSsrc(MappedSsrc0);
		packet->SetTimestamp(90000);
		feedRtpStreamRecv(rtpStream0.get(), packet.get(), 10);
		receiveSenderReport(rtpStream0.get(), /*captureAtUs*/ 1000 * 1000, /*rtpTs*/ 90000);

		packet->SetSsrc(MappedSsrc1);
		packet->SetTimestamp(144000);
		feedRtpStreamRecv(rtpStream1.get(), packet.get(), 10);
		receiveSenderReport(rtpStream1.get(), /*captureAtUs*/ 1000 * 1000, /*rtpTs*/ 135000);

		packet->SetSsrc(MappedSsrc2);
		packet->SetTimestamp(189000);
		feedRtpStreamRecv(rtpStream2.get(), packet.get(), 10);
		receiveSenderReport(rtpStream2.get(), /*captureAtUs*/ 1000 * 1000, /*rtpTs*/ 180000);

		// Set target layer to 0 and sync. This sets tsReferenceSpatialLayer = 0.
		manager->UpdateTargetLayers(0, 0);

		packet->SetSsrc(MappedSsrc0);
		packet->SetSequenceNumber(1);
		packet->SetTimestamp(90000);

		auto result = manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ true, /*clockRate*/ 90000, /*maxPacketTs*/ 0);

		REQUIRE(result.type == RTC::ProducerStreamManager::RtpPacketProcessResult::Type::FORWARD);

		// Layer 0 dies while being sent, so the preferred layer 1 is picked, which can
		// still be aligned to it.
		rtpStream0->ResetScore(/*score*/ 0, /*notify*/ false);
		manager->ProducerRtpStreamScore(rtpStream0.get(), /*score*/ 0, /*previousScore*/ 10);

		REQUIRE(manager->GetTargetLayers().spatial == 1);

		// A packet of layer 1 captured 100 ms after the forwarded one of layer 0.
		packet->SetSsrc(MappedSsrc1);
		packet->SetSequenceNumber(100);
		packet->SetTimestamp(144000);

		result = manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ true, /*clockRate*/ 90000, /*maxPacketTs*/ 90000);

		REQUIRE(result.type == RTC::ProducerStreamManager::RtpPacketProcessResult::Type::FORWARD);
		REQUIRE(result.spatialLayerSwitched == true);
		REQUIRE(result.tsOffset == 45000);

		// Sender Reports of layer 0 go on 20 seconds later, so it cannot be aligned to
		// anymore.
		receiveSenderReport(
		  rtpStream0.get(), /*captureAtUs*/ (1000 + 20000) * 1000, /*rtpTs*/ 90000 + (20 * 90000));

		// Layer 0 is still dead.
		// NOTE: The Sender Report above gave its score back since the RTP inactivity
		// check, which keeps a dead stream at score 0, does not run here.
		rtpStream0->ResetScore(/*score*/ 0, /*notify*/ false);

		// Switching to layer 2 aligns it to layer 1, which took over as TS reference.
		manager->UpdateTargetLayers(2, 0);

		// A packet of layer 2 captured 100 ms after the forwarded one of layer 1.
		packet->SetSsrc(MappedSsrc2);
		packet->SetSequenceNumber(200);
		packet->SetTimestamp(198000);

		result = manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ true, /*clockRate*/ 90000, /*maxPacketTs*/ 99000);

		REQUIRE(result.type == RTC::ProducerStreamManager::RtpPacketProcessResult::Type::FORWARD);
		REQUIRE(result.spatialLayerSwitched == true);
		REQUIRE(result.tsOffset == 90000);
	}

	SECTION("ProcessRtpPacket() does not apply any extra offset to the first packet ever forwarded")
	{
		MockListener listener;
		auto manager = createManager(
		  std::addressof(listener),
		  /*ssrcs*/ TwoSsrcs,
		  /*preferredLayers*/ { 1, 0 },
		  /*keyFrameSupported*/ false);
		auto rtpStream0 = createRtpStreamRecv(MappedSsrc0);
		auto rtpStream1 = createRtpStreamRecv(MappedSsrc1);

		manager->ProducerRtpStream(rtpStream0.get(), MappedSsrc0);
		manager->ProducerRtpStream(rtpStream1.get(), MappedSsrc1);

		// Set target layer to 0 and sync. This sets tsReferenceSpatialLayer = 0.
		manager->UpdateTargetLayers(0, 0);

		// Nothing has been sent yet, so the RTP timestamp of the packet goes untouched
		// even if it is beyond 2^31 ticks ahead of zero.
		packet->SetSsrc(MappedSsrc0);
		packet->SetSequenceNumber(1);
		packet->SetTimestamp(3000000000);

		const auto result = manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ true, /*clockRate*/ 90000, /*maxPacketTs*/ 0);

		REQUIRE(result.type == RTC::ProducerStreamManager::RtpPacketProcessResult::Type::FORWARD);
		REQUIRE(result.spatialLayerSwitched == true);
		REQUIRE(result.tsOffset == 0);
	}

	SECTION("ProcessRtpPacket() aligns spatial layers upon 'abs-capture-time' when both of them carry it")
	{
		MockListener listener;
		auto manager = createManager(
		  std::addressof(listener),
		  /*ssrcs*/ TwoSsrcs,
		  /*preferredLayers*/ { 1, 0 },
		  /*keyFrameSupported*/ false);
		auto rtpStream0 = createRtpStreamRecv(MappedSsrc0);
		auto rtpStream1 = createRtpStreamRecv(MappedSsrc1);

		manager->ProducerRtpStream(rtpStream0.get(), MappedSsrc0);
		manager->ProducerRtpStream(rtpStream1.get(), MappedSsrc1);

		// Sender Reports tell that the RTP timeline of layer 1 runs 45000 ticks ahead of
		// the one of layer 0.
		receiveSenderReport(rtpStream0.get(), /*captureAtUs*/ 1000 * 1000, /*rtpTs*/ 90000);
		receiveSenderReport(rtpStream1.get(), /*captureAtUs*/ 1000 * 1000, /*rtpTs*/ 135000);

		// 'abs-capture-time' tells that it runs 18000 ticks ahead instead, and it is the
		// one to take since both layers carry it.
		receiveAbsCaptureTimePacket(
		  rtpStream0.get(), /*seq*/ 1, /*captureAtUs*/ 1000 * 1000, /*rtpTs*/ 90000);
		receiveAbsCaptureTimePacket(
		  rtpStream1.get(), /*seq*/ 1, /*captureAtUs*/ 1000 * 1000, /*rtpTs*/ 108000);

		// Set target layer to 0 and sync. This sets tsReferenceSpatialLayer = 0.
		manager->UpdateTargetLayers(0, 0);

		packet->SetSsrc(MappedSsrc0);
		packet->SetSequenceNumber(1);
		packet->SetTimestamp(90000);

		auto result = manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ true, /*clockRate*/ 90000, /*maxPacketTs*/ 0);

		REQUIRE(result.type == RTC::ProducerStreamManager::RtpPacketProcessResult::Type::FORWARD);

		manager->UpdateTargetLayers(1, 0);

		// A packet of layer 1 captured 100 ms after the forwarded one of layer 0.
		packet->SetSsrc(MappedSsrc1);
		packet->SetSequenceNumber(100);
		packet->SetTimestamp(108000 + 9000);

		result = manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ true, /*clockRate*/ 90000, /*maxPacketTs*/ 90000);

		REQUIRE(result.type == RTC::ProducerStreamManager::RtpPacketProcessResult::Type::FORWARD);
		REQUIRE(result.spatialLayerSwitched == true);
		REQUIRE(result.tsOffset == 18000);
	}

	SECTION(
	  "ProcessRtpPacket() aligns spatial layers upon Sender Reports when only one of them carries 'abs-capture-time'")
	{
		MockListener listener;
		auto manager = createManager(
		  std::addressof(listener),
		  /*ssrcs*/ TwoSsrcs,
		  /*preferredLayers*/ { 1, 0 },
		  /*keyFrameSupported*/ false);
		auto rtpStream0 = createRtpStreamRecv(MappedSsrc0);
		auto rtpStream1 = createRtpStreamRecv(MappedSsrc1);

		manager->ProducerRtpStream(rtpStream0.get(), MappedSsrc0);
		manager->ProducerRtpStream(rtpStream1.get(), MappedSsrc1);

		// Sender Reports tell that the RTP timeline of layer 1 runs 45000 ticks ahead of
		// the one of layer 0.
		receiveSenderReport(rtpStream0.get(), /*captureAtUs*/ 1000 * 1000, /*rtpTs*/ 90000);
		receiveSenderReport(rtpStream1.get(), /*captureAtUs*/ 1000 * 1000, /*rtpTs*/ 135000);

		// Only layer 1 carries 'abs-capture-time', which tells otherwise, so it cannot be
		// used and Sender Reports are taken instead.
		receiveAbsCaptureTimePacket(
		  rtpStream1.get(), /*seq*/ 1, /*captureAtUs*/ 1000 * 1000, /*rtpTs*/ 108000);

		// Set target layer to 0 and sync. This sets tsReferenceSpatialLayer = 0.
		manager->UpdateTargetLayers(0, 0);

		packet->SetSsrc(MappedSsrc0);
		packet->SetSequenceNumber(1);
		packet->SetTimestamp(90000);
		rtpStream0->ReceivePacket(packet.get(), shared.GetTimeUs());

		auto result = manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ true, /*clockRate*/ 90000, /*maxPacketTs*/ 0);

		REQUIRE(result.type == RTC::ProducerStreamManager::RtpPacketProcessResult::Type::FORWARD);

		manager->UpdateTargetLayers(1, 0);

		// A packet of layer 1 captured 100 ms after the forwarded one of layer 0.
		packet->SetSsrc(MappedSsrc1);
		packet->SetSequenceNumber(100);
		packet->SetTimestamp(135000 + 9000);

		result = manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ true, /*clockRate*/ 90000, /*maxPacketTs*/ 90000);

		REQUIRE(result.type == RTC::ProducerStreamManager::RtpPacketProcessResult::Type::FORWARD);
		REQUIRE(result.spatialLayerSwitched == true);
		REQUIRE(result.tsOffset == 45000);
	}

	SECTION("ProducerRtcpSenderReport() ignores a Sender Report that is not the first one of its stream")
	{
		MockListener listener;
		auto manager =
		  createManager(std::addressof(listener), /*ssrcs*/ TwoSsrcs, /*preferredLayers*/ { 1, 0 });
		auto rtpStream0 = createRtpStreamRecv(MappedSsrc0);
		auto rtpStream1 = createRtpStreamRecv(MappedSsrc1);

		manager->ProducerRtpStream(rtpStream0.get(), MappedSsrc0);
		manager->ProducerRtpStream(rtpStream1.get(), MappedSsrc1);

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
			rtpStream->ReceiveRtcpSenderReport(std::addressof(sr), shared.GetTimeUs());
		}

		// Set target layer to 0. This sets tsReferenceSpatialLayer = 0.
		manager->UpdateTargetLayers(0, 0);

		REQUIRE(manager->GetTargetLayers().spatial == 0);

		// A Sender Report that is not the first one of its stream cannot make any layer
		// alignable, so layers are not checked and the target stays at layer 0 although
		// the preferred layer 1 could be picked.
		manager->ProducerRtcpSenderReport(rtpStream1.get(), /*first*/ false);

		REQUIRE(manager->GetTargetLayers().spatial == 0);
	}

	SECTION(
	  "ProducerRtcpSenderReport() checks layers upon a Sender Report that is not the first one of its stream if it makes a spatial layer switchable again")
	{
		MockListener listener;
		auto manager = createManager(
		  std::addressof(listener),
		  /*ssrcs*/ TwoSsrcs,
		  /*preferredLayers*/ { 1, 0 },
		  /*keyFrameSupported*/ false);
		auto rtpStream0 = createRtpStreamRecv(MappedSsrc0);
		auto rtpStream1 = createRtpStreamRecv(MappedSsrc1);

		manager->ProducerRtpStream(rtpStream0.get(), MappedSsrc0);
		manager->ProducerRtpStream(rtpStream1.get(), MappedSsrc1);

		packet->SetSsrc(MappedSsrc0);
		packet->SetTimestamp(90000);
		feedRtpStreamRecv(rtpStream0.get(), packet.get(), 10);
		receiveSenderReport(rtpStream0.get(), /*captureAtUs*/ 20000 * 1000, /*rtpTs*/ 90000);

		// Set target layer to 0 and sync. This sets tsReferenceSpatialLayer = 0.
		manager->UpdateTargetLayers(0, 0);

		packet->SetSequenceNumber(1);

		const auto result = manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ true, /*clockRate*/ 90000, /*maxPacketTs*/ 0);

		REQUIRE(result.type == RTC::ProducerStreamManager::RtpPacketProcessResult::Type::FORWARD);

		// The first Sender Report of layer 1 refers to an RTP timestamp 11 seconds
		// before its highest one, as if the ones in between had been lost, so it cannot
		// be aligned to layer 0 and the target stays at layer 0.
		packet->SetSsrc(MappedSsrc1);
		packet->SetTimestamp(144000);
		feedRtpStreamRecv(rtpStream1.get(), packet.get(), 10);
		receiveSenderReport(
		  rtpStream1.get(), /*captureAtUs*/ (20000 - 11000) * 1000, /*rtpTs*/ 144000 - (11 * 90000));

		manager->ProducerRtcpSenderReport(rtpStream1.get(), /*first*/ true);

		REQUIRE(manager->GetTargetLayers().spatial == 0);

		// A later Sender Report of layer 1 makes it switchable again, so layers are
		// checked and the preferred layer 1 is picked.
		receiveSenderReport(rtpStream1.get(), /*captureAtUs*/ 20000 * 1000, /*rtpTs*/ 135000);

		manager->ProducerRtcpSenderReport(rtpStream1.get(), /*first*/ false);

		REQUIRE(manager->GetTargetLayers().spatial == 1);
	}

	SECTION("ProcessRtpPacket() calculates tsOffset of the spatial layer of the packet")
	{
		MockListener listener;
		auto manager = createManager(
		  std::addressof(listener),
		  /*ssrcs*/ ThreeSsrcs,
		  /*preferredLayers*/ { 2, 0 },
		  /*keyFrameSupported*/ false);
		auto rtpStream0 = createRtpStreamRecv(MappedSsrc0);
		auto rtpStream1 = createRtpStreamRecv(MappedSsrc1);
		auto rtpStream2 = createRtpStreamRecv(MappedSsrc2);

		manager->ProducerRtpStream(rtpStream0.get(), MappedSsrc0);
		manager->ProducerRtpStream(rtpStream1.get(), MappedSsrc1);
		manager->ProducerRtpStream(rtpStream2.get(), MappedSsrc2);

		// Feed packets to every stream so that the Sender Reports below give them a score
		// and RecalculateTargetLayers() takes them into account.
		for (auto* rtpStream : { rtpStream0.get(), rtpStream1.get(), rtpStream2.get() })
		{
			packet->SetSsrc(rtpStream->GetSsrc());
			feedRtpStreamRecv(rtpStream, packet.get(), 10);
		}

		// All layers hold RTP timestamp 1000. Layer 1 was captured 500 ms before layer 0
		// and layer 2 a whole second before it, so their offsets differ.
		receiveSenderReport(rtpStream0.get(), /*captureAtUs*/ 2000 * 1000, /*rtpTs*/ 1000);
		receiveSenderReport(rtpStream1.get(), /*captureAtUs*/ 1500 * 1000, /*rtpTs*/ 1000);
		receiveSenderReport(rtpStream2.get(), /*captureAtUs*/ 1000 * 1000, /*rtpTs*/ 1000);

		// Set target layer to 0 and sync. This sets tsReferenceSpatialLayer = 0, which
		// stays there for the rest of the SECTION since packets of it are forwarded.
		manager->UpdateTargetLayers(0, 0);

		packet->SetSsrc(MappedSsrc0);
		packet->SetSequenceNumber(1);
		manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ false, /*clockRate*/ 90000, /*maxPacketTs*/ 0);

		REQUIRE(manager->GetCurrentSpatialLayer() == 0);

		// Switch to layer 1 with a packet captured 100 ms after the forwarded one of
		// layer 0.
		manager->UpdateTargetLayers(1, 0);

		packet->SetSsrc(MappedSsrc1);
		packet->SetSequenceNumber(1);
		packet->SetTimestamp(1000 + 45000 + 9000);

		auto result = manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ false, /*clockRate*/ 90000, /*maxPacketTs*/ 1000);

		REQUIRE(result.spatialLayerSwitched == true);
		REQUIRE(result.tsOffset == 45000);
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
		REQUIRE(result.tsOffset == 45000);
	}

	SECTION("OnResumed() sets syncRequired")
	{
		MockListener listener;
		auto manager = createManager(
		  std::addressof(listener),
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
		auto manager    = createManager(std::addressof(listener));
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
		auto manager    = createManager(std::addressof(listener));
		auto rtpStream0 = createRtpStreamRecv(MappedSsrc0);

		manager->ProducerRtpStream(rtpStream0.get(), MappedSsrc0);
		manager->UpdateTargetLayers(0, 0);

		manager->OnPaused();

		auto targetLayers = manager->GetTargetLayers();

		REQUIRE(targetLayers.spatial == -1);
		REQUIRE(targetLayers.temporal == -1);
	}

#ifdef MS_USE_BUILTIN_BWE
	SECTION("IsBitrateAdaptable() returns true")
	{
		MockListener listener;
		auto manager = createManager(std::addressof(listener));

		manager->SetExternallyManagedBitrate();

		REQUIRE(manager->IsBitrateAdaptable() == true);
	}
#endif

	SECTION("TakeBitrate() returns 0 on second call in same iteration")
	{
		MockListener listener;
		auto manager =
		  createManager(std::addressof(listener), /*ssrcs*/ OneSsrc, /*preferredLayers*/ { 0, 0 });
		auto rtpStream0 = createRtpStreamRecv(MappedSsrc0);

		manager->ProducerRtpStream(rtpStream0.get(), MappedSsrc0);
		manager->SetExternallyManagedBitrate();

		// Feed packets so the stream has non-zero bitrate.
		packet->SetSsrc(MappedSsrc0);
		feedRtpStreamRecv(rtpStream0.get(), packet.get(), 100);

		const int64_t nowMs = shared.GetTimeMs();

		// First call claims bitrate.
		const auto usedBitrate = takeBitrate(manager.get(), /*bitrate*/ 1000000, nowMs);

		REQUIRE(usedBitrate > 0);

		// Second call in same iteration should return 0 (already at preferred layers).
		const auto usedBitrate2 = takeBitrate(manager.get(), /*bitrate*/ 1000000, nowMs);

		REQUIRE(usedBitrate2 == 0);
	}

	SECTION("TakeBitrate() works again after ApplyBitrateAllocation()")
	{
		MockListener listener;
		auto manager =
		  createManager(std::addressof(listener), /*ssrcs*/ TwoSsrcs, /*preferredLayers*/ { 1, 0 });
		auto rtpStream0 = createRtpStreamRecv(MappedSsrc0);
		auto rtpStream1 = createRtpStreamRecv(MappedSsrc1);

		manager->ProducerRtpStream(rtpStream0.get(), MappedSsrc0);
		manager->ProducerRtpStream(rtpStream1.get(), MappedSsrc1);
		manager->SetExternallyManagedBitrate();

		// Feed packets to layer 0.
		packet->SetSsrc(MappedSsrc0);
		feedRtpStreamRecv(rtpStream0.get(), packet.get(), 100);

		const int64_t nowMs = shared.GetTimeMs();

		// First iteration: claim layer 0.
		takeBitrate(manager.get(), /*bitrate*/ 1000000, nowMs);
		applyBitrateAllocation(manager.get(), /*rtpStreamActiveMs*/ 0);

		// Feed packets to layer 1.
		packet->SetSsrc(MappedSsrc1);
		feedRtpStreamRecv(rtpStream1.get(), packet.get(), 100);

		// Once the round is committed it should work again for layer 1.
		const auto usedBitrate = takeBitrate(manager.get(), /*bitrate*/ 1000000, nowMs);

		REQUIRE(usedBitrate > 0);
	}

	SECTION("GetDesiredBitrate() returns max bitrate across all producer streams")
	{
		MockListener listener;
		auto manager =
		  createManager(std::addressof(listener), /*ssrcs*/ TwoSsrcs, /*preferredLayers*/ { 1, 0 });
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

		const int64_t nowMs = shared.GetTimeMs();
		auto bitrate0       = rtpStream0->GetBitrate(nowMs);
		auto bitrate1       = rtpStream1->GetBitrate(nowMs);

		REQUIRE(bitrate1 > bitrate0);

		auto desiredBitrate = manager->GetDesiredBitrate(nowMs);

		REQUIRE(desiredBitrate == bitrate1);
	}

	SECTION("GetProducerCurrentRtpStream() returns nullptr before sync")
	{
		MockListener listener;
		auto manager    = createManager(std::addressof(listener));
		auto rtpStream0 = createRtpStreamRecv(MappedSsrc0);

		manager->ProducerRtpStream(rtpStream0.get(), MappedSsrc0);

		REQUIRE(manager->GetProducerCurrentRtpStream() == nullptr);
		REQUIRE(manager->GetCurrentSpatialLayer() == -1);
	}

	SECTION("GetProducerTargetRtpStream() returns correct stream after UpdateTargetLayers()")
	{
		MockListener listener;
		auto manager    = createManager(std::addressof(listener));
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
		  std::addressof(listener),
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
