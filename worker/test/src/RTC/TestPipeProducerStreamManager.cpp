#include "DepLibUV.hpp"
#include "RTC/PipeProducerStreamManager.hpp"
#include "RTC/RTP/Codecs/PayloadDescriptorHandler.hpp"
#include "test/include/RTC/RTP/rtpCommon.hpp"
#include "mocks/include/MockShared.hpp"
#include <catch2/catch_test_macros.hpp>

namespace
{
	using RtpPacketProcessResult = RTC::ProducerStreamManager::RtpPacketProcessResult;

	constexpr uint32_t MappedSsrc0 = 1000;
	constexpr uint32_t MappedSsrc1 = 2000;
	constexpr uint32_t MappedSsrc2 = 3000;

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
			this->keyFrameRequestedSsrcs.push_back(mappedSsrc);
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

	public:
		bool isActive{ true };
		int keyFrameRequestCount{ 0 };
		std::vector<uint32_t> keyFrameRequestedSsrcs;
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
			return true;
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
	};

	// NOLINTNEXTLINE(readability-identifier-naming)
	mocks::MockShared shared(/*getTimeMs*/
	                         []()
	                         {
		                         return DepLibUV::GetTimeMs();
	                         });

	std::unique_ptr<RTC::PipeProducerStreamManager> createManager(
	  MockListener* listener,
	  const std::vector<uint32_t>& ssrcs = { MappedSsrc0, MappedSsrc1, MappedSsrc2 },
	  bool keyFrameSupported             = true,
	  RTC::Media::Kind kind              = RTC::Media::Kind::VIDEO)
	{
		std::vector<RTC::RtpEncodingParameters> consumableRtpEncodings;

		for (auto ssrc : ssrcs)
		{
			RTC::RtpEncodingParameters encoding;
			encoding.ssrc = ssrc;
			consumableRtpEncodings.push_back(encoding);
		}

		const RTC::ConsumerTypes::VideoLayers preferredLayers;

		return std::make_unique<RTC::PipeProducerStreamManager>(
		  consumableRtpEncodings, preferredLayers, nullptr, kind, keyFrameSupported, listener, &shared);
	}
} // namespace

SCENARIO("PipeProducerStreamManager", "[rtp][producerstreammanager][pipe]")
{
	std::unique_ptr<RTC::RTP::Packet> packet(
	  RTC::RTP::Packet::Factory(rtpCommon::FactoryBuffer, sizeof(rtpCommon::FactoryBuffer)));

	packet->SetPayloadType(1);
	packet->SetSsrc(MappedSsrc0);
	packet->SetPayloadLength(40);

	SECTION("IsActive() returns listener state")
	{
		MockListener listener;
		auto manager = createManager(&listener);

		REQUIRE(manager->IsActive() == true);

		listener.isActive = false;

		REQUIRE(manager->IsActive() == false);
	}

	SECTION("ProcessRtpPacket() returns FORWARD with isSyncPacket on first packet after connect")
	{
		MockListener listener;
		auto manager = createManager(&listener, { MappedSsrc0 }, /*keyFrameSupported*/ false);

		manager->OnTransportConnected();

		packet->SetSsrc(MappedSsrc0);
		packet->SetSequenceNumber(10);

		auto result = manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ false, /*clockRate*/ 0, /*maxPacketTs*/ 0);

		REQUIRE(result.type == RtpPacketProcessResult::Type::FORWARD);
		REQUIRE(result.isSyncPacket == true);
		REQUIRE(result.syncSeqValue == 9);
		REQUIRE(result.tsOffset == 0u);

		// Second packet forwards normally, no sync.
		packet->SetSequenceNumber(11);

		result = manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ false, /*clockRate*/ 0, /*maxPacketTs*/ 0);

		REQUIRE(result.type == RtpPacketProcessResult::Type::FORWARD);
		REQUIRE(result.isSyncPacket == false);
	}

	SECTION("ProcessRtpPacket() returns BUFFER when sync required and packet is not a key frame")
	{
		MockListener listener;
		auto manager = createManager(&listener);

		manager->OnTransportConnected();

		packet->SetSsrc(MappedSsrc0);
		packet->SetSequenceNumber(10);

		auto result = manager->ProcessRtpPacket(
		  packet.get(), /*lastSentPacketHasMarker*/ false, /*clockRate*/ 0, /*maxPacketTs*/ 0);

		REQUIRE(result.type == RtpPacketProcessResult::Type::BUFFER);
	}

	SECTION("ProcessRtpPacket() syncs independently per stream")
	{
		MockListener listener;
		auto manager =
		  createManager(&listener, { MappedSsrc0, MappedSsrc1 }, /*keyFrameSupported*/ false);

		manager->OnTransportConnected();

		// Sync stream 0.
		packet->SetSsrc(MappedSsrc0);
		packet->SetSequenceNumber(100);

		auto result0 = manager->ProcessRtpPacket(packet.get(), false, 0, 0);

		REQUIRE(result0.type == RtpPacketProcessResult::Type::FORWARD);
		REQUIRE(result0.isSyncPacket == true);
		REQUIRE(result0.syncSeqValue == 99);

		// Stream 1 syncs independently.
		packet->SetSsrc(MappedSsrc1);
		packet->SetSequenceNumber(200);

		auto result1 = manager->ProcessRtpPacket(packet.get(), false, 0, 0);

		REQUIRE(result1.type == RtpPacketProcessResult::Type::FORWARD);
		REQUIRE(result1.isSyncPacket == true);
		REQUIRE(result1.syncSeqValue == 199);

		// Stream 0 next packet is no longer a sync packet.
		packet->SetSsrc(MappedSsrc0);
		packet->SetSequenceNumber(101);

		auto result2 = manager->ProcessRtpPacket(packet.get(), false, 0, 0);

		REQUIRE(result2.type == RtpPacketProcessResult::Type::FORWARD);
		REQUIRE(result2.isSyncPacket == false);
	}

	SECTION("ProcessRtpPacket() returns FORWARD with sendBufferedPackets on key frame sync")
	{
		MockListener listener;
		auto manager = createManager(&listener);

		manager->OnTransportConnected();

		packet->SetSsrc(MappedSsrc0);
		packet->SetSequenceNumber(50);

		auto* handler       = new MockPayloadDescriptorHandler();
		handler->isKeyFrame = true;
		packet->SetPayloadDescriptorHandler(handler);

		auto result = manager->ProcessRtpPacket(packet.get(), false, 0, 0);

		REQUIRE(result.type == RtpPacketProcessResult::Type::FORWARD);
		REQUIRE(result.isSyncPacket == true);
		REQUIRE(result.syncSeqValue == 49);
		REQUIRE(result.sendBufferedPackets == true);
	}

	SECTION("ProcessRtpPacket() returns DROP for empty payload packets")
	{
		MockListener listener;
		auto manager = createManager(&listener, { MappedSsrc0 }, /*keyFrameSupported*/ false);

		manager->OnTransportConnected();

		// Complete sync first.
		packet->SetSsrc(MappedSsrc0);
		packet->SetSequenceNumber(1);
		manager->ProcessRtpPacket(packet.get(), false, 0, 0);

		// Padding-only packet.
		packet->SetSequenceNumber(2);
		packet->RemovePayload();

		auto result = manager->ProcessRtpPacket(packet.get(), false, 0, 0);

		REQUIRE(result.type == RtpPacketProcessResult::Type::DROP);
	}

	SECTION("ProcessRtpPacket() preserves original marker bit")
	{
		MockListener listener;
		auto manager = createManager(&listener, { MappedSsrc0 }, /*keyFrameSupported*/ false);

		manager->OnTransportConnected();

		packet->SetSsrc(MappedSsrc0);
		packet->SetSequenceNumber(1);
		packet->SetMarker(true);

		auto result = manager->ProcessRtpPacket(packet.get(), false, 0, 0);

		REQUIRE(result.type == RtpPacketProcessResult::Type::FORWARD);
		REQUIRE(result.marker == true);

		packet->SetSequenceNumber(2);
		packet->SetMarker(false);

		result = manager->ProcessRtpPacket(packet.get(), false, 0, 0);

		REQUIRE(result.marker == false);
	}

	SECTION("OnTransportConnected() sets sync required for all streams and requests key frames")
	{
		MockListener listener;
		auto manager = createManager(&listener, { MappedSsrc0, MappedSsrc1, MappedSsrc2 });

		manager->OnTransportConnected();

		REQUIRE(listener.keyFrameRequestCount == 3);
		REQUIRE(
		  std::find(
		    listener.keyFrameRequestedSsrcs.begin(), listener.keyFrameRequestedSsrcs.end(), MappedSsrc0) !=
		  listener.keyFrameRequestedSsrcs.end());
		REQUIRE(
		  std::find(
		    listener.keyFrameRequestedSsrcs.begin(), listener.keyFrameRequestedSsrcs.end(), MappedSsrc1) !=
		  listener.keyFrameRequestedSsrcs.end());
		REQUIRE(
		  std::find(
		    listener.keyFrameRequestedSsrcs.begin(), listener.keyFrameRequestedSsrcs.end(), MappedSsrc2) !=
		  listener.keyFrameRequestedSsrcs.end());

		// All streams require a key frame before forwarding.
		packet->SetSequenceNumber(1);

		for (auto ssrc : { MappedSsrc0, MappedSsrc1, MappedSsrc2 })
		{
			packet->SetSsrc(ssrc);

			auto result = manager->ProcessRtpPacket(packet.get(), false, 0, 0);

			REQUIRE(result.type == RtpPacketProcessResult::Type::BUFFER);
		}
	}

	SECTION("OnTransportConnected() does not request key frames when not active")
	{
		MockListener listener;
		listener.isActive = false;
		auto manager      = createManager(&listener);

		manager->OnTransportConnected();

		REQUIRE(listener.keyFrameRequestCount == 0);
	}

	SECTION("OnTransportConnected() does not request key frames for audio")
	{
		MockListener listener;
		auto manager = createManager(
		  &listener, { MappedSsrc0 }, /*keyFrameSupported*/ false, RTC::Media::Kind::AUDIO);

		manager->OnTransportConnected();

		REQUIRE(listener.keyFrameRequestCount == 0);
	}

	SECTION("OnTransportDisconnected() clears sync required for all streams")
	{
		MockListener listener;
		auto manager =
		  createManager(&listener, { MappedSsrc0, MappedSsrc1 }, /*keyFrameSupported*/ false);

		manager->OnTransportConnected();
		manager->OnTransportDisconnected();

		// Sync cleared — packets forward immediately without waiting for key frame.
		packet->SetSequenceNumber(1);

		for (auto ssrc : { MappedSsrc0, MappedSsrc1 })
		{
			packet->SetSsrc(ssrc);

			auto result = manager->ProcessRtpPacket(packet.get(), false, 0, 0);

			REQUIRE(result.type == RtpPacketProcessResult::Type::FORWARD);
			REQUIRE(result.isSyncPacket == false);
		}
	}

	SECTION("OnPaused() clears sync required for all streams")
	{
		MockListener listener;
		auto manager = createManager(&listener, { MappedSsrc0 }, /*keyFrameSupported*/ false);

		manager->OnTransportConnected();
		manager->OnPaused();

		packet->SetSsrc(MappedSsrc0);
		packet->SetSequenceNumber(1);

		auto result = manager->ProcessRtpPacket(packet.get(), false, 0, 0);

		REQUIRE(result.type == RtpPacketProcessResult::Type::FORWARD);
		REQUIRE(result.isSyncPacket == false);
	}

	SECTION("OnResumed() sets sync required and requests key frames for all streams")
	{
		MockListener listener;
		auto manager = createManager(&listener, { MappedSsrc0, MappedSsrc1 });

		manager->OnTransportConnected();

		const int countBefore = listener.keyFrameRequestCount;

		manager->OnResumed();

		REQUIRE(listener.keyFrameRequestCount == countBefore + 2);

		// All streams need sync again.
		packet->SetSequenceNumber(1);

		for (auto ssrc : { MappedSsrc0, MappedSsrc1 })
		{
			packet->SetSsrc(ssrc);

			auto result = manager->ProcessRtpPacket(packet.get(), false, 0, 0);

			REQUIRE(result.type == RtpPacketProcessResult::Type::BUFFER);
		}
	}

	SECTION("RequestKeyFrame() requests for all video streams")
	{
		MockListener listener;
		auto manager = createManager(&listener, { MappedSsrc0, MappedSsrc1, MappedSsrc2 });

		manager->RequestKeyFrame();

		REQUIRE(listener.keyFrameRequestCount == 3);
	}

	SECTION("RequestKeyFrame() is no-op for audio")
	{
		MockListener listener;
		auto manager = createManager(
		  &listener, { MappedSsrc0 }, /*keyFrameSupported*/ false, RTC::Media::Kind::AUDIO);

		manager->RequestKeyFrame();

		REQUIRE(listener.keyFrameRequestCount == 0);
	}

	SECTION("RecalculateTargetLayers() always returns false")
	{
		MockListener listener;
		auto manager = createManager(&listener);

		RTC::ConsumerTypes::VideoLayers newTargetLayers;

		REQUIRE(manager->RecalculateTargetLayers(newTargetLayers) == false);
	}

	SECTION("IncreaseLayer() returns 0 (no BWE)")
	{
		MockListener listener;
		auto manager = createManager(&listener);

		manager->SetExternallyManagedBitrate();

		const auto used = manager->IncreaseLayer(1000000u, false, 0.0f, DepLibUV::GetTimeMs());

		REQUIRE(used == 0u);
	}

	SECTION("GetDesiredBitrate() returns 0 (no BWE)")
	{
		MockListener listener;
		auto manager = createManager(&listener);

		manager->SetExternallyManagedBitrate();

		REQUIRE(manager->GetDesiredBitrate(DepLibUV::GetTimeMs()) == 0u);
	}
}
