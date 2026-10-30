#include "common.hpp"
#include "RTC/RTP/Codecs/VP9.hpp"
#include "RTC/RTP/Packet.hpp"
#include "test/include/RTC/RTP/rtpCommon.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memcmp(), std::memcpy()
#include <tuple>
#include <vector>

namespace
{
	constexpr uint8_t DependencyDescriptorId{ static_cast<uint8_t>(
		RTC::RtpHeaderExtensionUri::Type::DEPENDENCY_DESCRIPTOR) };
	RTC::RTP::Codecs::VP9::PayloadDescriptor* createVP9PayloadDescriptor(
	  uint8_t* buffer, size_t bufferLen, uint16_t pictureId, uint8_t tlIndex)
	{
		buffer[0]             = 0xAD; // I, L, B, E bits
		uint16_t netPictureId = htons(pictureId);
		std::memcpy(buffer + 1, &netPictureId, 2);
		buffer[1] |= 0x80;
		buffer[3] = (tlIndex << 5) | (1 << 4); // tlIndex, switchingUpPoint

		auto* payloadDescriptor = RTC::RTP::Codecs::VP9::Parse(buffer, bufferLen);

		REQUIRE(payloadDescriptor);

		return payloadDescriptor;
	}

	std::unique_ptr<RTC::RTP::Codecs::VP9::PayloadDescriptor> processVP9Packet(
	  RTC::RTP::Codecs::VP9::EncodingContext& context, uint16_t pictureId, uint8_t tlIndex)
	{
		// clang-format off
		uint8_t payload[] =
		{
			0xAD, 0x80, 0x00, 0x00, 0x00, 0x00
		};
		// clang-format on
		bool marker;
		auto* payloadDescriptor =
		  createVP9PayloadDescriptor(payload, sizeof(payload), pictureId, tlIndex);
		std::unique_ptr<RTC::RTP::Codecs::VP9::PayloadDescriptorHandler> payloadDescriptorHandler(
		  new RTC::RTP::Codecs::VP9::PayloadDescriptorHandler(payloadDescriptor));

		std::unique_ptr<RTC::RTP::Packet> packet{ RTC::RTP::Packet::Factory(
			rtpCommon::FactoryBuffer, sizeof(rtpCommon::FactoryBuffer)) };

		packet->SetPayload(payload, sizeof(payload));

		if (payloadDescriptorHandler->Process(&context, packet.get(), marker))
		{
			return std::unique_ptr<RTC::RTP::Codecs::VP9::PayloadDescriptor>(
			  RTC::RTP::Codecs::VP9::Parse(payload, sizeof(payload)));
		}

		return nullptr;
	}

	/**
	 * Creates a VP9 packet carrying the given Dependency Descriptor in an RTP
	 * header extension `reservedLen` bytes long, which is the room the Producer
	 * makes in it for the Consumers to rewrite the descriptor.
	 */
	std::unique_ptr<RTC::RTP::Packet> createPacketWithDependencyDescriptor(
	  const uint8_t* dependencyDescriptor,
	  uint8_t len,
	  uint8_t reservedLen,
	  uint8_t* payload,
	  size_t payloadLen)
	{
		std::unique_ptr<RTC::RTP::Packet> packet{ RTC::RTP::Packet::Factory(
			rtpCommon::FactoryBuffer, sizeof(rtpCommon::FactoryBuffer)) };

		REQUIRE(packet);

		uint8_t value[RTC::Consts::TwoBytesRtpExtensionMaxLength]{};

		std::memcpy(value, dependencyDescriptor, len);

		const std::vector<RTC::RTP::Packet::Extension> extensions{
			{ RTC::RtpHeaderExtensionUri::Type::DEPENDENCY_DESCRIPTOR, DependencyDescriptorId, reservedLen, value }
		};

		REQUIRE(packet->SetExtensions(RTC::RTP::Packet::ExtensionsType::Auto, extensions) == true);

		// As the Producer does, tell the length the descriptor really has.
		REQUIRE(packet->UpdateDependencyDescriptor(dependencyDescriptor, len));

		packet->SetPayload(payload, payloadLen);

		return packet;
	}
} // namespace

SCENARIO("process VP9 payload descriptor", "[rtp][codecs][vp9]")
{
	constexpr uint16_t MaxPictureId = (1 << 15) - 1;

	SECTION("drop packets that belong to other temporal layers after rolling over pictureID")
	{
		RTC::RTP::Codecs::EncodingContext::Params params;
		params.spatialLayers  = 1;
		params.temporalLayers = 3;

		RTC::RTP::Codecs::VP9::EncodingContext context(params);
		context.SyncRequired();
		context.SetCurrentTemporalLayer(0);
		context.SetTargetTemporalLayer(0);

		context.SetCurrentSpatialLayer(0);
		context.SetTargetSpatialLayer(0);

		// Frame 1.
		auto forwarded = processVP9Packet(context, MaxPictureId, 0);
		REQUIRE(forwarded);
		REQUIRE(forwarded->pictureId == MaxPictureId);

		// Frame 2.
		forwarded = processVP9Packet(context, 0, 0);
		REQUIRE(forwarded);
		REQUIRE(forwarded->pictureId == 0);

		// Frame 3.
		forwarded = processVP9Packet(context, 1, 1);
		REQUIRE(!forwarded);
	}

	SECTION("PayloadDescriptorHandler")
	{
		RTC::RTP::Codecs::EncodingContext::Params params;
		params.spatialLayers  = 1;
		params.temporalLayers = 3;

		RTC::RTP::Codecs::VP9::EncodingContext context(params);

		const uint16_t start = MaxPictureId - 2000;

		context.SetCurrentTemporalLayer(0, start + 0);
		context.SetCurrentTemporalLayer(1, start + 1);
		context.SetCurrentTemporalLayer(2, start + 5);
		context.SetCurrentTemporalLayer(0, start + 6);

		REQUIRE(context.GetTemporalLayerForFrameId(start + 0) == 0);
		REQUIRE(context.GetTemporalLayerForFrameId(start + 1) == 1);
		REQUIRE(context.GetTemporalLayerForFrameId(start + 2) == 1);
		REQUIRE(context.GetTemporalLayerForFrameId(start + 5) == 2);
		REQUIRE(context.GetTemporalLayerForFrameId(start + 6) == 0);

		context.SetCurrentTemporalLayer(1, start + 1000);
		context.SetCurrentTemporalLayer(2, start + 1001); // This will drop the first item.

		REQUIRE(context.GetTemporalLayerForFrameId(start + 1000) == 1);
		REQUIRE(context.GetTemporalLayerForFrameId(start + 0) == 1); // It will get the item at start+1.

		context.SetCurrentTemporalLayer(0, 0); // This will drop items from start to start+999.

		REQUIRE(context.GetTemporalLayerForFrameId(0) == 0);
		REQUIRE(context.GetTemporalLayerForFrameId(start + 0) == 1); // It will get the item at start+1000.
	}

	/*
	 * The Dependency Descriptor only reaches a Consumer that negotiated the
	 * extension, so it is the one that serves it. The rest cannot be told the
	 * layers any other way than by the payload descriptor of the codec, and
	 * filtering them with the Dependency Descriptor would drop frames they are
	 * never told not to wait for.
	 */
	SECTION("the Dependency Descriptor serves only the Consumers given it")
	{
		// Key frame carrying the template dependency structure of an L1T2 stream.
		// clang-format off
		uint8_t keyFrameDescriptor[] =
		{
			0x80, 0x00, 0xE8, 0x80,
			0x01, 0x1E, 0xA8, 0x51,
			0x41, 0x01, 0x0C, 0x13,
			0xFC, 0x0B, 0x3C,
		};

		// Mandatory fields only, so nothing tells about the active decode targets.
		uint8_t deltaFrameDescriptor[] =
		{
			0x00, 0x00, 0xE8,
		};

		uint8_t payload[] =
		{
			0xAD, 0x80, 0x00, 0x00, 0x00, 0x00
		};
		// clang-format on

		std::shared_ptr<RTC::RTP::Codecs::DependencyDescriptor::TemplateDependencyStructure>
		  templateDependencyStructure;

		// NOTE: Taken apart from any packet, just to get the structure that the
		// delta frame below is parsed with.
		const std::unique_ptr<RTC::RTP::Codecs::DependencyDescriptor> keyFrame{
			RTC::RTP::Codecs::DependencyDescriptor::Parse(
			  keyFrameDescriptor, sizeof(keyFrameDescriptor), templateDependencyStructure)
		};

		REQUIRE(keyFrame);
		REQUIRE(templateDependencyStructure->decodeTargetCount == 2);

		auto makeContext = [](bool preferDependencyDescriptor)
		{
			RTC::RTP::Codecs::EncodingContext::Params params;

			params.spatialLayers              = 1;
			params.temporalLayers             = 2;
			params.preferDependencyDescriptor = preferDependencyDescriptor;

			return params;
		};

		// A Consumer that was given the extension is told which decode target it
		// is being sent.
		{
			const auto packet = createPacketWithDependencyDescriptor(
			  deltaFrameDescriptor,
			  sizeof(deltaFrameDescriptor),
			  /*reservedLen*/ 4,
			  payload,
			  sizeof(payload));

			RTC::RTP::Codecs::VP9::ProcessRtpPacket(packet.get(), templateDependencyStructure);

			auto params = makeContext(/*preferDependencyDescriptor*/ true);
			RTC::RTP::Codecs::VP9::EncodingContext context(params);

			context.SetCurrentSpatialLayer(0);
			context.SetCurrentTemporalLayer(0);
			context.SetTargetSpatialLayer(0);
			context.SetTargetTemporalLayer(0);

			bool marker{ false };

			REQUIRE(packet->ProcessPayload(std::addressof(context), marker));

			std::unique_ptr<RTC::RTP::Codecs::DependencyDescriptor> forwarded;

			packet->ReadDependencyDescriptor(forwarded, templateDependencyStructure);

			REQUIRE(forwarded);
			REQUIRE(forwarded->activeDecodeTargetsBitmask == 0b01);
		}

		// A Consumer that was not is served by the VP9 payload descriptor, which
		// writes no active decode targets bitmask at all.
		{
			const auto packet = createPacketWithDependencyDescriptor(
			  deltaFrameDescriptor,
			  sizeof(deltaFrameDescriptor),
			  /*reservedLen*/ 4,
			  payload,
			  sizeof(payload));

			RTC::RTP::Codecs::VP9::ProcessRtpPacket(packet.get(), templateDependencyStructure);

			auto params = makeContext(/*preferDependencyDescriptor*/ false);
			RTC::RTP::Codecs::VP9::EncodingContext context(params);

			context.SetCurrentSpatialLayer(0);
			context.SetCurrentTemporalLayer(0);
			context.SetTargetSpatialLayer(0);
			context.SetTargetTemporalLayer(0);

			bool marker{ false };

			REQUIRE(packet->ProcessPayload(std::addressof(context), marker));

			std::unique_ptr<RTC::RTP::Codecs::DependencyDescriptor> forwarded;

			packet->ReadDependencyDescriptor(forwarded, templateDependencyStructure);

			REQUIRE(forwarded);
			REQUIRE(!forwarded->activeDecodeTargetsBitmask.has_value());
		}
	}

	SECTION("drop packets that belong to other temporal layers with unordered pictureID")
	{
		RTC::RTP::Codecs::EncodingContext::Params params;
		params.spatialLayers  = 1;
		params.temporalLayers = 3;

		RTC::RTP::Codecs::VP9::EncodingContext context(params);
		context.SyncRequired();
		context.SetCurrentSpatialLayer(0, 0);
		context.SetTargetSpatialLayer(0);

		const uint16_t start                                                     = MaxPictureId - 20;
		const std::vector<std::tuple<uint16_t, uint16_t, int16_t, bool>> packets = {
			// targetTemporalLayer=0
			{ start,      0, 0,  true  },
			{ start,      1, -1, false },
			{ start + 1,  0, -1, true  },
			{ start + 1,  1, -1, false },
			{ start + 2,  0, -1, true  },
			{ start + 2,  1, -1, false },
			// targetTemporalLayer=1
			{ start + 10, 0, 1,  true  },
			{ start + 10, 1, -1, true  },
			{ start + 11, 0, -1, true  },
			{ start + 11, 1, -1, true  },
			{ start + 3,  0, -1, true  }, // old packet
			{ start + 3,  1, -1, false },
			{ start + 12, 0, -1, true  },
			{ start + 12, 1, -1, true  },
			// targetTemporalLayer=0
			{ start + 14, 0, 0,  true  },
			{ start + 14, 1, -1, false },
			{ start + 13, 0, -1, true  }, // old packet
			{ start + 13, 1, -1, true  },
			// targetTemporalLayer=1
			{ start + 15, 0, 1,  true  },
			{ start + 15, 1, -1, true  },
			// targetTemporalLayer=0
			{ 0,          0, 0,  true  },
			{ 0,          1, -1, false },
			{ 1,          0, -1, true  },
			{ 1,          1, -1, false },
			{ start + 16, 0, -1, true  }, // old packet
			{ start + 16, 1, -1, true  },
		};

		for (const auto& packet : packets)
		{
			auto pictureId           = std::get<0>(packet);
			auto tlIndex             = std::get<1>(packet);
			auto targetTemporalLayer = std::get<2>(packet);
			auto shouldForward       = std::get<3>(packet);

			if (targetTemporalLayer >= 0)
			{
				context.SetTargetTemporalLayer(targetTemporalLayer);
			}

			auto forwarded = processVP9Packet(context, pictureId, tlIndex);

			if (shouldForward)
			{
				REQUIRE(forwarded);
				REQUIRE(forwarded->pictureId == pictureId);
			}
			else
			{
				REQUIRE(!forwarded);
			}
		}
	}
}
