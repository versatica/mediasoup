#include "common.hpp"
#include "RTC/RTP/Codecs/VP9.hpp"
#include "RTC/RTP/Packet.hpp"
#include "RTC/RTP/rtpCommon.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memcmp()

namespace
{
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

		REQUIRE(context.GetTemporalLayerForPictureId(start + 0) == 0);
		REQUIRE(context.GetTemporalLayerForPictureId(start + 1) == 1);
		REQUIRE(context.GetTemporalLayerForPictureId(start + 2) == 1);
		REQUIRE(context.GetTemporalLayerForPictureId(start + 5) == 2);
		REQUIRE(context.GetTemporalLayerForPictureId(start + 6) == 0);

		context.SetCurrentTemporalLayer(1, start + 1000);
		context.SetCurrentTemporalLayer(2, start + 1001); // This will drop the first item.

		REQUIRE(context.GetTemporalLayerForPictureId(start + 1000) == 1);
		REQUIRE(context.GetTemporalLayerForPictureId(start + 0) == 1); // It will get the item at start+1.

		context.SetCurrentTemporalLayer(0, 0); // This will drop items from start to start+999.

		REQUIRE(context.GetTemporalLayerForPictureId(0) == 0);
		REQUIRE(
		  context.GetTemporalLayerForPictureId(start + 0) == 1); // It will get the item at start+1000.
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
