#include "common.hpp"
#include "RTC/RTP/Codecs/PictureIdUnwrapper.hpp"
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
} // namespace

SCENARIO("process VP9 payload descriptor", "[rtp][codecs][vp9]")
{
	constexpr uint16_t MaxPictureId = (1 << 15) - 1;

	const auto createVP9PayloadDescriptor =
	  [](uint8_t* buffer, size_t bufferLen, uint16_t pictureId, uint8_t tlIndex)
	  -> RTC::RTP::Codecs::VP9::PayloadDescriptor*
	{
		const uint16_t netPictureId = htons(pictureId);

		buffer[0] = 0xAD; // I, L, B, E bits
		std::memcpy(buffer + 1, std::addressof(netPictureId), 2);
		buffer[1] |= 0x80;
		buffer[3] = (tlIndex << 5) | (1 << 4); // tlIndex, switchingUpPoint

		auto* const payloadDescriptor = RTC::RTP::Codecs::VP9::Parse(buffer, bufferLen);

		REQUIRE(payloadDescriptor);

		return payloadDescriptor;
	};

	const auto processVP9Packet =
	  [&createVP9PayloadDescriptor](
	    RTC::RTP::Codecs::VP9::EncodingContext& context,
	    uint16_t pictureId,
	    uint8_t tlIndex) -> std::unique_ptr<RTC::RTP::Codecs::VP9::PayloadDescriptor>
	{
		// clang-format off
		uint8_t payload[] =
		{
			0xAD, 0x80, 0x00, 0x00, 0x00, 0x00
		};
		// clang-format on

		auto* const payloadDescriptor =
		  createVP9PayloadDescriptor(payload, sizeof(payload), pictureId, tlIndex);

		const auto payloadDescriptorHandler =
		  std::make_unique<RTC::RTP::Codecs::VP9::PayloadDescriptorHandler>(payloadDescriptor);

		const std::unique_ptr<RTC::RTP::Packet> packet{
			RTC::RTP::Packet::Factory(rtpCommon::FactoryBuffer, sizeof(rtpCommon::FactoryBuffer)),
		};

		REQUIRE(packet);

		packet->SetPayload(payload, sizeof(payload));

		bool marker;

		if (payloadDescriptorHandler->Process(std::addressof(context), packet.get(), marker))
		{
			return std::unique_ptr<RTC::RTP::Codecs::VP9::PayloadDescriptor>(
			  RTC::RTP::Codecs::VP9::Parse(packet->GetPayload(), packet->GetPayloadLength()));
		}

		return nullptr;
	};

	// Creates a VP9 packet carrying the given Dependency Descriptor in an RTP
	// header extension `reservedLen` bytes long, which is the room the Producer
	// makes in it for the Consumers to rewrite the descriptor.
	const auto createPacketWithDependencyDescriptor =
	  [](
	    const uint8_t* dependencyDescriptor,
	    uint8_t len,
	    uint8_t reservedLen,
	    const uint8_t* payload,
	    size_t payloadLen) -> std::unique_ptr<RTC::RTP::Packet>
	{
		std::unique_ptr<RTC::RTP::Packet> packet{
			RTC::RTP::Packet::Factory(rtpCommon::FactoryBuffer, sizeof(rtpCommon::FactoryBuffer)),
		};

		REQUIRE(packet);

		uint8_t value[RTC::Consts::TwoByteRtpExtensionMaxLength]{};

		std::memcpy(value, dependencyDescriptor, len);

		const std::vector<RTC::RTP::Packet::Extension> extensions{
			{ RTC::RtpHeaderExtensionUri::Type::DEPENDENCY_DESCRIPTOR, DependencyDescriptorId, reservedLen, value }
		};

		REQUIRE(packet->SetExtensions(RTC::RTP::Packet::ExtensionsForm::Auto, extensions) == true);

		// As the Producer does, tell the length the descriptor really has.
		REQUIRE(packet->UpdateDependencyDescriptor(dependencyDescriptor, len));

		packet->SetPayload(payload, payloadLen);

		return packet;
	};

	SECTION("drop packets that belong to other temporal layers after rolling over pictureID")
	{
		const RTC::RTP::Codecs::EncodingContext::Params params{ .spatialLayers = 1, .temporalLayers = 3 };

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

	// The pictureIds of a 7 bits sender wrap every 128 pictures, which must not
	// make the Consumer take every packet that follows for a reordered one and
	// stop switching layers.
	SECTION("layers are still switched after a 7 bits pictureId wraps")
	{
		// Builds a VP9 payload whose pictureId takes a single byte, this is, 7
		// bits, which is what a sender that does not use the M bit gives.
		const auto processVP9PacketWithOneBytePictureId =
		  [](
		    RTC::RTP::Codecs::DependencyDescriptor::Context& dependencyDescriptorContext,
		    RTC::RTP::Codecs::PictureIdUnwrapper& pictureIdUnwrapper,
		    RTC::RTP::Codecs::VP9::EncodingContext& context,
		    uint8_t pictureId,
		    uint8_t tlIndex) -> bool
		{
			// clang-format off
			uint8_t payload[] =
			{
				0xAD, 0x00, 0x00, 0x00, 0x00, 0x00
			};
			// clang-format on

			// M unset, so the pictureId is the 7 bits that follow it.
			payload[1] = pictureId & 0x7F;
			// tlIndex and switchingUpPoint.
			payload[2] = static_cast<uint8_t>((uint32_t{ tlIndex } << 5) | (uint32_t{ 1 } << 4));

			const std::unique_ptr<RTC::RTP::Packet> packet{
				RTC::RTP::Packet::Factory(rtpCommon::FactoryBuffer, sizeof(rtpCommon::FactoryBuffer)),
			};

			REQUIRE(packet);

			packet->SetPayload(payload, sizeof(payload));

			// As the Producer does, which is what extends the pictureId.
			REQUIRE(
			  RTC::RTP::Codecs::VP9::ProcessRtpPacket(
			    packet.get(), dependencyDescriptorContext, pictureIdUnwrapper));

			bool marker;

			return packet->ProcessPayload(std::addressof(context), marker);
		};

		const RTC::RTP::Codecs::EncodingContext::Params params{ .spatialLayers = 1, .temporalLayers = 3 };
		const std::vector<uint8_t> pictureIds{ 126, 127, 0, 1 };

		// Held by the Producer stream, so every packet of it is extended with the
		// same unwrapper.
		RTC::RTP::Codecs::DependencyDescriptor::Context dependencyDescriptorContext;
		RTC::RTP::Codecs::PictureIdUnwrapper pictureIdUnwrapper;

		RTC::RTP::Codecs::VP9::EncodingContext context(params);

		context.SyncRequired();
		context.SetCurrentTemporalLayer(0);
		context.SetTargetTemporalLayer(0);

		context.SetCurrentSpatialLayer(0);
		context.SetTargetSpatialLayer(0);

		// Pictures 126 and 127, then the wrap to 0 and 1.
		for (const auto pictureId : pictureIds)
		{
			REQUIRE(processVP9PacketWithOneBytePictureId(
			  dependencyDescriptorContext, pictureIdUnwrapper, context, pictureId, 0));
		}

		// The Consumer is given a higher temporal layer now, and the next picture
		// is a switching up point, so it takes it there.
		context.SetTargetTemporalLayer(1);

		REQUIRE(processVP9PacketWithOneBytePictureId(
		  dependencyDescriptorContext, pictureIdUnwrapper, context, 2, 1));
		REQUIRE(context.GetCurrentTemporalLayer() == 1);
	}

	SECTION("PayloadDescriptorHandler")
	{
		const RTC::RTP::Codecs::EncodingContext::Params params{ .spatialLayers = 1, .temporalLayers = 3 };
		const uint16_t start = MaxPictureId - 2000;

		RTC::RTP::Codecs::VP9::EncodingContext context(params);

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

		context.SetCurrentTemporalLayer(0, 0); // This will drop items from start to start+1000.

		REQUIRE(context.GetTemporalLayerForFrameId(0) == 0);
		REQUIRE(context.GetTemporalLayerForFrameId(start + 0) == 2); // It will get the item at start+1001.
	}

	// A layer change is kept while it is no more than a thousand frames behind
	// the one being recorded, and frame ids wrap, so the distance between them is
	// taken in the space they are numbered in.
	SECTION("the frames a layer change is behind are counted across the wrap")
	{
		// Close enough to the 15 bits wrap that a change a thousand frames later
		// has a lower frame id than this one.
		constexpr uint16_t Start{ MaxPictureId - 767 };

		const RTC::RTP::Codecs::EncodingContext::Params params{ .spatialLayers = 1, .temporalLayers = 3 };

		// Records a layer change at `Start`, another at the next frame and a third
		// one the given number of frames past `Start`. Tells the layer the frame at
		// `Start` is given then, which is the one of its own change unless that
		// change was dropped for being too far behind.
		const auto layerAtStart = [&params](uint16_t frames) -> int16_t
		{
			RTC::RTP::Codecs::VP9::EncodingContext context(params);

			context.SetCurrentTemporalLayer(0, Start);
			context.SetCurrentTemporalLayer(1, Start + 1);
			context.SetCurrentTemporalLayer(2, static_cast<uint16_t>((Start + frames) & MaxPictureId));

			return context.GetTemporalLayerForFrameId(Start);
		};

		// No frames at all past it, which a target layer changed in the middle of a
		// frame brings about, and a thousand frames past it.
		REQUIRE(layerAtStart(0) == 0);
		REQUIRE(layerAtStart(1000) == 0);

		// Past that the change at `Start` is dropped, so the one at the next frame
		// is the oldest one left and tells the layer.
		REQUIRE(layerAtStart(1001) == 1);
	}

	// VP9 numbers its pictures with 15 bits, so that is where the frame ids of a
	// Consumer served by the VP9 payload descriptor wrap. A Consumer given the
	// Dependency Descriptor is served by its handler instead, which numbers every
	// frame itself with 16 bits.
	SECTION("frame ids wrap where the descriptor that numbered them says")
	{
		RTC::RTP::Codecs::EncodingContext::Params params{ .spatialLayers = 1, .temporalLayers = 3 };

		// Records a temporal layer change right before the 15 bits wrap and
		// another one eleven frames past it.
		const auto pushLayerChangesAroundWrap = [](RTC::RTP::Codecs::EncodingContext& context) -> void
		{
			context.SetCurrentTemporalLayer(0, MaxPictureId - 5);
			context.SetCurrentTemporalLayer(1, 5);
		};

		{
			RTC::RTP::Codecs::VP9::EncodingContext context(params);

			pushLayerChangesAroundWrap(context);

			// Eleven frames apart, so both changes are recent and the first one is
			// kept.
			REQUIRE(context.GetTemporalLayerForFrameId(MaxPictureId - 5) == 0);
		}

		{
			params.preferDependencyDescriptor = true;

			RTC::RTP::Codecs::VP9::EncodingContext context(params);

			pushLayerChangesAroundWrap(context);

			// 32757 frames apart in a 16 bits space, so the first change is too old
			// to be kept and the current layer is told instead.
			REQUIRE(context.GetTemporalLayerForFrameId(MaxPictureId - 5) == 1);
		}
	}

	// The Dependency Descriptor only reaches a Consumer that negotiated the
	// extension, so it is the one that serves it. The rest cannot be told the
	// layers any other way than by the payload descriptor of the codec, and
	// filtering them with the Dependency Descriptor would drop frames they are
	// never told not to wait for.
	SECTION("the Dependency Descriptor serves only the Consumers given it")
	{
		// Key frame carrying the template dependency structure of an L1T2 stream.
		// clang-format off
		const uint8_t keyFrameDescriptor[] =
		{
			0x80, 0x00, 0xE8, 0x80,
			0x01, 0x1E, 0xA8, 0x51,
			0x41, 0x01, 0x0C, 0x13,
			0xFC, 0x0B, 0x3C
		};

		// Mandatory fields only, so nothing tells about the active decode targets.
		const uint8_t deltaFrameDescriptor[] =
		{
			0x00, 0x00, 0xE8
		};

		const uint8_t payload[] =
		{
			0xAD, 0x80, 0x00, 0x00, 0x00, 0x00
		};
		// clang-format on

		RTC::RTP::Codecs::DependencyDescriptor::Context dependencyDescriptorContext;
		RTC::RTP::Codecs::PictureIdUnwrapper pictureIdUnwrapper;

		// NOTE: Taken apart from any packet, just to get the structure that the
		// delta frame below is parsed with.
		const std::unique_ptr<RTC::RTP::Codecs::DependencyDescriptor> keyFrame{
			RTC::RTP::Codecs::DependencyDescriptor::Parse(
			  keyFrameDescriptor, sizeof(keyFrameDescriptor), dependencyDescriptorContext)
		};

		REQUIRE(keyFrame);
		REQUIRE(dependencyDescriptorContext.templateDependencyStructure->decodeTargetCount == 2);

		const auto makeContext =
		  [](bool preferDependencyDescriptor) -> RTC::RTP::Codecs::EncodingContext::Params
		{
			return { .spatialLayers              = 1,
			         .temporalLayers             = 2,
			         .preferDependencyDescriptor = preferDependencyDescriptor };
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

			REQUIRE(
			  RTC::RTP::Codecs::VP9::ProcessRtpPacket(
			    packet.get(), dependencyDescriptorContext, pictureIdUnwrapper));

			const auto params = makeContext(/*preferDependencyDescriptor*/ true);

			RTC::RTP::Codecs::VP9::EncodingContext context(params);

			context.SetCurrentSpatialLayer(0);
			context.SetCurrentTemporalLayer(0);
			context.SetTargetSpatialLayer(0);
			context.SetTargetTemporalLayer(0);

			bool marker{ false };

			REQUIRE(packet->ProcessPayload(std::addressof(context), marker));

			std::unique_ptr<RTC::RTP::Codecs::DependencyDescriptor> forwarded;

			packet->ReadDependencyDescriptor(forwarded, dependencyDescriptorContext);

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

			REQUIRE(
			  RTC::RTP::Codecs::VP9::ProcessRtpPacket(
			    packet.get(), dependencyDescriptorContext, pictureIdUnwrapper));

			const auto params = makeContext(/*preferDependencyDescriptor*/ false);

			RTC::RTP::Codecs::VP9::EncodingContext context(params);

			context.SetCurrentSpatialLayer(0);
			context.SetCurrentTemporalLayer(0);
			context.SetTargetSpatialLayer(0);
			context.SetTargetTemporalLayer(0);

			bool marker{ false };

			REQUIRE(packet->ProcessPayload(std::addressof(context), marker));

			std::unique_ptr<RTC::RTP::Codecs::DependencyDescriptor> forwarded;

			packet->ReadDependencyDescriptor(forwarded, dependencyDescriptorContext);

			REQUIRE(forwarded);
			REQUIRE(!forwarded->activeDecodeTargetsBitmask.has_value());
		}
	}

	SECTION("drop packets that belong to other temporal layers with unordered pictureID")
	{
		const RTC::RTP::Codecs::EncodingContext::Params params{ .spatialLayers = 1, .temporalLayers = 3 };
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
			{ start + 16, 1, -1, true  }
		};

		RTC::RTP::Codecs::VP9::EncodingContext context(params);

		context.SyncRequired();
		context.SetCurrentSpatialLayer(0, 0);
		context.SetTargetSpatialLayer(0);

		for (const auto& [pictureId, tlIndex, targetTemporalLayer, shouldForward] : packets)
		{
			if (targetTemporalLayer >= 0)
			{
				context.SetTargetTemporalLayer(targetTemporalLayer);
			}

			const auto forwarded = processVP9Packet(context, pictureId, tlIndex);

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
