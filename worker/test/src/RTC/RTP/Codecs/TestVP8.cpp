#include "common.hpp"
#include "RTC/RTP/Codecs/VP8.hpp"
#include "RTC/RTP/Packet.hpp"
#include "RTC/RTP/rtpCommon.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memcmp(), std::memcpy()

using namespace RTC;

constexpr uint16_t MaxPictureId = (1 << 15) - 1;

SCENARIO("parse VP8 payload descriptor", "[rtp][codecs][vp8]")
{
	SECTION("parse payload descriptor")
	{
		/**
		 * VP8 Payload Descriptor
		 *
		 * 1 = X bit: Extended control bits present (I L T K)
		 * 1 = R bit: Reserved for future use (Error should be zero)
		 * 0 = N bit: Reference frame
		 * 1 = S bit: Start of VP8 partition
		 * Part Id: 0
		 * 1 = I bit: Picture ID byte present
		 * 0 = L bit: TL0PICIDX byte not present
		 * 0 = T bit: TID (temporal layer index) byte not present
		 * 0 = K bit: TID/KEYIDX byte not present
		 * 0000 = Reserved A: 0
		 * 0001 0001 = Picture Id: 17
		 */

		// clang-format off
		uint8_t originalBuffer[] =
		{
			0xd0, 0x80, 0x11, 0x00
		};
		// clang-format on

		// Keep a copy of the original buffer for comparing.
		uint8_t buffer[4] = { 0 };

		std::memcpy(buffer, originalBuffer, sizeof(buffer));

		std::unique_ptr<RTP::Codecs::VP8::PayloadDescriptor> payloadDescriptor{ RTP::Codecs::VP8::Parse(
			buffer, sizeof(buffer)) };

		REQUIRE(payloadDescriptor);

		REQUIRE(payloadDescriptor->extended == 1);
		REQUIRE(payloadDescriptor->nonReference == 0);
		REQUIRE(payloadDescriptor->start == 1);
		REQUIRE(payloadDescriptor->partitionIndex == 0);

		// Optional field flags.
		REQUIRE(payloadDescriptor->i == 1);
		REQUIRE(payloadDescriptor->l == 0);
		REQUIRE(payloadDescriptor->t == 0);
		REQUIRE(payloadDescriptor->k == 0);

		// Optional fields.
		REQUIRE(payloadDescriptor->pictureId == 17);
		REQUIRE(payloadDescriptor->tl0PictureIndex == 0);
		REQUIRE(payloadDescriptor->tlIndex == 0);
		REQUIRE(payloadDescriptor->y == 0);
		REQUIRE(payloadDescriptor->keyIndex == 0);

		REQUIRE(payloadDescriptor->isKeyFrame == true);
		REQUIRE(payloadDescriptor->hasPictureId == true);
		REQUIRE(payloadDescriptor->hasOneBytePictureId == true);
		REQUIRE(payloadDescriptor->hasTwoBytesPictureId == false);
		REQUIRE(payloadDescriptor->hasTl0PictureIndex == false);
		REQUIRE(payloadDescriptor->hasTlIndex == false);

		SECTION("encode payload descriptor")
		{
			payloadDescriptor->Encode(
			  buffer, payloadDescriptor->pictureId, payloadDescriptor->tl0PictureIndex);

			SECTION("compare encoded payloadDescriptor with original buffer")
			{
				REQUIRE(std::memcmp(buffer, originalBuffer, sizeof(buffer)) == 0);
			}
		}
	}

	SECTION("parse payload descriptor 2")
	{
		/**
		 * VP8 Payload Descriptor
		 *
		 * 1 = X bit: Extended control bits present (I L T K)
		 * 0 = R bit: Reserved for future use
		 * 0 = N bit: Reference frame
		 * 0 = S bit: Continuation of VP8 partition
		 * 000 = Part Id: 0
		 * 0 = I bit: No Picture byte ID
		 * 0 = L bit: TL0PICIDX byte not present
		 * 1 = T bit: TID (temporal layer index) byte present
		 * 1 = K bit: TID/KEYIDX byte present
		 * 1110 = Reserved A: 14
		 * 11 = Temporal layer Index (TID): 3
		 * 1 = 1 Lay Sync Bit (Y): True
		 * ...0 0100 = Temporal Key Frame Index (KEYIDX): 4
		 */

		// clang-format off
		uint8_t originalBuffer[] =
		{
		  0x88, 0x3e, 0xe4, 0x00
		};
		// clang-format on

		// Keep a copy of the original buffer for comparing.
		uint8_t buffer[4] = { 0 };

		std::memcpy(buffer, originalBuffer, sizeof(buffer));

		// Parse the buffer.
		std::unique_ptr<RTP::Codecs::VP8::PayloadDescriptor> payloadDescriptor{ RTP::Codecs::VP8::Parse(
			buffer, sizeof(buffer)) };

		REQUIRE(payloadDescriptor);

		REQUIRE(payloadDescriptor->extended == 1);
		REQUIRE(payloadDescriptor->nonReference == 0);
		REQUIRE(payloadDescriptor->start == 0);
		REQUIRE(payloadDescriptor->partitionIndex == 0);

		// Optional field flags.
		REQUIRE(payloadDescriptor->i == 0);
		REQUIRE(payloadDescriptor->l == 0);
		REQUIRE(payloadDescriptor->t == 1);
		REQUIRE(payloadDescriptor->k == 1);

		// Optional fields.
		REQUIRE(payloadDescriptor->pictureId == 0);
		REQUIRE(payloadDescriptor->tl0PictureIndex == 0);
		REQUIRE(payloadDescriptor->tlIndex == 3);
		REQUIRE(payloadDescriptor->y == 1);
		REQUIRE(payloadDescriptor->keyIndex == 4);

		REQUIRE(payloadDescriptor->isKeyFrame == false);
		REQUIRE(payloadDescriptor->hasPictureId == false);
		REQUIRE(payloadDescriptor->hasOneBytePictureId == false);
		REQUIRE(payloadDescriptor->hasTwoBytesPictureId == false);
		REQUIRE(payloadDescriptor->hasTl0PictureIndex == false);
		REQUIRE(payloadDescriptor->hasTlIndex == true);

		SECTION("encode payload descriptor")
		{
			payloadDescriptor->Encode(
			  buffer, payloadDescriptor->pictureId, payloadDescriptor->tl0PictureIndex);

			SECTION("compare encoded payloadDescriptor with original buffer")
			{
				REQUIRE(std::memcmp(buffer, originalBuffer, sizeof(buffer)) == 0);
			}
		}
	};

	SECTION("parse payload descriptor, encode")
	{
		/**
		 * VP8 Payload Descriptor
		 *
		 * 1 = X bit: Extended control bits present (I L T K)
		 * 1 = R bit: Reserved for future use (Error should be zero)
		 * 0 = N bit: Reference frame
		 * 1 = S bit: Start of VP8 partition
		 * Part Id: 0
		 * 1 = I bit: Picture ID byte present
		 * 1 = L bit: TL0PICIDX byte present
		 * 0 = T bit: TID (temporal layer index) byte not present
		 * 0 = K bit: TID/KEYIDX byte not present
		 * 0000 = Reserved A: 0
		 * 0001 0001 = Picture Id: 17
		 * 0000 0011 = TL0PICIDX: 3
		 */

		// clang-format off
		uint8_t originalBuffer[] =
		{
			0xd0, 0xc0, 0x11, 0x03
		};
		// clang-format on

		// Keep a copy of the original buffer for comparing.
		uint8_t buffer[4] = { 0 };

		std::memcpy(buffer, originalBuffer, sizeof(buffer));

		std::unique_ptr<RTP::Codecs::VP8::PayloadDescriptor> payloadDescriptor{ RTP::Codecs::VP8::Parse(
			buffer, sizeof(buffer)) };

		REQUIRE(payloadDescriptor);

		REQUIRE(payloadDescriptor->pictureId == 17);
		REQUIRE(payloadDescriptor->tl0PictureIndex == 3);

		SECTION("encode payload descriptor")
		{
			payloadDescriptor->Encode(buffer, 20, 1);

			std::unique_ptr<RTP::Codecs::VP8::PayloadDescriptor> payloadDescriptor{ RTP::Codecs::VP8::Parse(
				buffer, sizeof(buffer)) };

			REQUIRE(payloadDescriptor->pictureId == 20);
			REQUIRE(payloadDescriptor->tl0PictureIndex == 1);
		}

		SECTION("restore payload descriptor")
		{
			payloadDescriptor->Restore(buffer);

			std::unique_ptr<RTP::Codecs::VP8::PayloadDescriptor> payloadDescriptor{ RTP::Codecs::VP8::Parse(
				buffer, sizeof(buffer)) };

			REQUIRE(payloadDescriptor->pictureId == 17);
			REQUIRE(payloadDescriptor->tl0PictureIndex == 3);
		}
	}

	SECTION("parse payload descriptor. I flag set but no space for pictureId")
	{
		/**
		 * VP8 Payload Descriptor
		 *
		 * 1 = X bit: Extended control bits present (I L T K)
		 * 1 = R bit: Reserved for future use (Error should be zero)
		 * 0 = N bit: Reference frame
		 * 1 = S bit: Start of VP8 partition
		 * Part Id: 0
		 * 1 = I bit: Picture ID byte present
		 * 0 = L bit: TL0PICIDX byte not present
		 * 0 = T bit: TID (temporal layer index) byte not present
		 * 0 = K bit: TID/KEYIDX byte not present
		 * 0000 = Reserved A: 0
		 */

		// clang-format off
		uint8_t buffer[] =
		{
			0xd0, 0x80
		};
		// clang-format on

		auto payloadDescriptor = RTP::Codecs::VP8::Parse(buffer, sizeof(buffer));

		REQUIRE_FALSE(payloadDescriptor);
	}

	SECTION("parse payload descriptor. X flag is not set, no keyframe")
	{
		/**
		 * VP8 Payload Descriptor
		 *
		 * 0 = X bit: Extended control bits present (I L T K)
		 * 1 = R bit: Reserved for future use (Error should be zero)
		 * 0 = N bit: Reference frame
		 * 1 = S bit: Start of VP8 partition
		 * Part Id: 0
		 * 000000 = Size0 | H | VER
		 * 1 = P bit: Inverse Keyframe
		 */

		// clang-format off
		uint8_t buffer[] =
		{
			0x50, 0x01
		};
		// clang-format on

		auto* payloadDescriptor = RTP::Codecs::VP8::Parse(buffer, sizeof(buffer));

		REQUIRE(payloadDescriptor);

		REQUIRE(payloadDescriptor->extended == 0);
		REQUIRE(payloadDescriptor->nonReference == 0);
		REQUIRE(payloadDescriptor->start == 1);
		REQUIRE(payloadDescriptor->partitionIndex == 0);

		// Optional field flags.
		REQUIRE(payloadDescriptor->i == 0);
		REQUIRE(payloadDescriptor->l == 0);
		REQUIRE(payloadDescriptor->t == 0);
		REQUIRE(payloadDescriptor->k == 0);

		// Optional fields.
		REQUIRE(payloadDescriptor->pictureId == 0);
		REQUIRE(payloadDescriptor->tl0PictureIndex == 0);
		REQUIRE(payloadDescriptor->tlIndex == 0);
		REQUIRE(payloadDescriptor->y == 0);
		REQUIRE(payloadDescriptor->keyIndex == 0);

		REQUIRE(payloadDescriptor->isKeyFrame == false);
		REQUIRE(payloadDescriptor->hasPictureId == false);
		REQUIRE(payloadDescriptor->hasOneBytePictureId == false);
		REQUIRE(payloadDescriptor->hasTwoBytesPictureId == false);
		REQUIRE(payloadDescriptor->hasTl0PictureIndex == false);
		REQUIRE(payloadDescriptor->hasTlIndex == false);

		delete payloadDescriptor;
	}

	SECTION("parse payload descriptor. X flag is not set, keyframe")
	{
		/**
		 * VP8 Payload Descriptor
		 *
		 * 0 = X bit: Extended control bits present (I L T K)
		 * 1 = R bit: Reserved for future use (Error should be zero)
		 * 0 = N bit: Reference frame
		 * 1 = S bit: Start of VP8 partition
		 * Part Id: 0
		 * 000000 = Size0 | H | VER
		 * 0 = P bit: Inverse Keyframe
		 */

		// clang-format off
		uint8_t buffer[] =
		{
			0x50, 0x00
		};
		// clang-format on

		auto* payloadDescriptor = RTP::Codecs::VP8::Parse(buffer, sizeof(buffer));

		REQUIRE(payloadDescriptor);

		REQUIRE(payloadDescriptor->extended == 0);
		REQUIRE(payloadDescriptor->nonReference == 0);
		REQUIRE(payloadDescriptor->start == 1);
		REQUIRE(payloadDescriptor->partitionIndex == 0);

		// Optional field flags.
		REQUIRE(payloadDescriptor->i == 0);
		REQUIRE(payloadDescriptor->l == 0);
		REQUIRE(payloadDescriptor->t == 0);
		REQUIRE(payloadDescriptor->k == 0);

		// Optional fields.
		REQUIRE(payloadDescriptor->pictureId == 0);
		REQUIRE(payloadDescriptor->tl0PictureIndex == 0);
		REQUIRE(payloadDescriptor->tlIndex == 0);
		REQUIRE(payloadDescriptor->y == 0);
		REQUIRE(payloadDescriptor->keyIndex == 0);

		REQUIRE(payloadDescriptor->isKeyFrame == true);
		REQUIRE(payloadDescriptor->hasPictureId == false);
		REQUIRE(payloadDescriptor->hasOneBytePictureId == false);
		REQUIRE(payloadDescriptor->hasTwoBytesPictureId == false);
		REQUIRE(payloadDescriptor->hasTl0PictureIndex == false);
		REQUIRE(payloadDescriptor->hasTlIndex == false);

		delete payloadDescriptor;
	}
}

RTP::Codecs::VP8::PayloadDescriptor* CreateVP8PayloadDescriptor(
  uint8_t* buffer,
  size_t bufferLen,
  uint16_t pictureId,
  uint8_t tl0PictureIndex,
  uint8_t tlIndex,
  bool layerSync = true)
{
	uint16_t netPictureId = htons(pictureId);
	std::memcpy(buffer + 2, &netPictureId, 2);
	buffer[2] |= 0x80;
	buffer[4] = tl0PictureIndex;
	buffer[5] = tlIndex << 6;

	if (layerSync)
	{
		buffer[5] |= 0x20; // y bit
	}

	auto* payloadDescriptor = RTP::Codecs::VP8::Parse(buffer, bufferLen);

	REQUIRE(payloadDescriptor);

	return payloadDescriptor;
}

std::unique_ptr<RTP::Codecs::VP8::PayloadDescriptor> ProcessVP8Packet(
  RTP::Codecs::VP8::EncodingContext& context,
  uint16_t pictureId,
  uint8_t tl0PictureIndex,
  uint8_t tlIndex,
  bool layerSync = true)
{
	// clang-format off
	uint8_t payload[] =
	{
		0x90, 0xe0, 0x80, 0x00, 0x00, 0x00
	};
	// clang-format on

	std::unique_ptr<RTP::Packet> packet{ RTP::Packet::Factory(FactoryBuffer, sizeof(FactoryBuffer)) };

	packet->SetPayload(payload, sizeof(payload));

	bool marker;
	auto* payloadDescriptor = CreateVP8PayloadDescriptor(
	  packet->GetPayload(), packet->GetPayloadLength(), pictureId, tl0PictureIndex, tlIndex, layerSync);
	std::unique_ptr<RTP::Codecs::VP8::PayloadDescriptorHandler> payloadDescriptorHandler(
	  new RTP::Codecs::VP8::PayloadDescriptorHandler(payloadDescriptor));

	if (payloadDescriptorHandler->Process(&context, packet.get(), marker))
	{
		return std::unique_ptr<RTP::Codecs::VP8::PayloadDescriptor>(
		  RTP::Codecs::VP8::Parse(packet->GetPayload(), packet->GetPayloadLength()));
	}

	return nullptr;
}

SCENARIO("process VP8 payload descriptor", "[rtp][codecs][vp8]")
{
	SECTION("do not drop TL0PICIDX from temporal layers higher than 0")
	{
		RTP::Codecs::EncodingContext::Params params;
		params.spatialLayers  = 0;
		params.temporalLayers = 2;
		RTP::Codecs::VP8::EncodingContext context(params);

		context.SetCurrentTemporalLayer(0);
		context.SetTargetTemporalLayer(0);

		// Frame 1.
		auto forwarded = ProcessVP8Packet(context, 0, 0, 0);
		REQUIRE(forwarded);
		REQUIRE(forwarded->pictureId == 0);
		REQUIRE(forwarded->tl0PictureIndex == 0);

		// Frame 2 gets lost.

		// Frame 3.
		forwarded = ProcessVP8Packet(context, 2, 1, 1);
		REQUIRE_FALSE(forwarded);

		// Frame 2 retransmitted.
		forwarded = ProcessVP8Packet(context, 1, 1, 0);
		REQUIRE(forwarded);
		REQUIRE(forwarded->pictureId == 1);
		REQUIRE(forwarded->tl0PictureIndex == 1);
	}

	SECTION("drop packets that belong to other temporal layers after rolling over pictureID")
	{
		RTP::Codecs::EncodingContext::Params params;
		params.spatialLayers  = 0;
		params.temporalLayers = 2;
		RTP::Codecs::VP8::EncodingContext context(params);
		context.SyncRequired();

		context.SetCurrentTemporalLayer(0);
		context.SetTargetTemporalLayer(0);

		// Frame 1.
		auto forwarded = ProcessVP8Packet(context, MaxPictureId, 0, 0);
		REQUIRE(forwarded);
		REQUIRE(forwarded->pictureId == 1);
		REQUIRE(forwarded->tl0PictureIndex == 1);

		// Frame 2.
		forwarded = ProcessVP8Packet(context, 0, 0, 0);
		REQUIRE(forwarded);
		REQUIRE(forwarded->pictureId == 2);
		REQUIRE(forwarded->tl0PictureIndex == 1);

		// Frame 3.
		forwarded = ProcessVP8Packet(context, 1, 0, 1);
		REQUIRE_FALSE(forwarded);
	}

	SECTION("old packets with higher temporal layer than current are dropped")
	{
		RTP::Codecs::EncodingContext::Params params;
		params.spatialLayers  = 0;
		params.temporalLayers = 2;
		RTP::Codecs::VP8::EncodingContext context(params);
		context.SyncRequired();

		context.SetCurrentTemporalLayer(0);
		context.SetTargetTemporalLayer(0);

		// Frame 1.
		auto forwarded = ProcessVP8Packet(context, 1, 0, 0);
		REQUIRE(forwarded);
		REQUIRE(forwarded->pictureId == 1);
		REQUIRE(forwarded->tlIndex == 0);
		REQUIRE(forwarded->tl0PictureIndex == 1);

		// Frame 2.
		forwarded = ProcessVP8Packet(context, 2, 0, 0);
		REQUIRE(forwarded);
		REQUIRE(forwarded->pictureId == 2);
		REQUIRE(forwarded->tlIndex == 0);
		REQUIRE(forwarded->tl0PictureIndex == 1);

		// Frame 3. Old packet with higher temporal layer than current.
		forwarded = ProcessVP8Packet(context, 0, 0, 1);
		REQUIRE_FALSE(forwarded);
		REQUIRE(context.GetCurrentTemporalLayer() == 0);
	}

	SECTION("packets with higher temporal layer than current are dropped")
	{
		RTP::Codecs::EncodingContext::Params params;
		params.spatialLayers  = 0;
		params.temporalLayers = 2;
		RTP::Codecs::VP8::EncodingContext context(params);
		context.SyncRequired();

		context.SetCurrentTemporalLayer(0);
		context.SetTargetTemporalLayer(0);

		// Frame 1.
		auto forwarded = ProcessVP8Packet(context, 1, 0, 0);
		REQUIRE(forwarded);
		REQUIRE(forwarded->pictureId == 1);
		REQUIRE(forwarded->tlIndex == 0);
		REQUIRE(forwarded->tl0PictureIndex == 1);

		// Frame 2.
		forwarded = ProcessVP8Packet(context, 2, 0, 0);
		REQUIRE(forwarded);
		REQUIRE(forwarded->pictureId == 2);
		REQUIRE(forwarded->tlIndex == 0);
		REQUIRE(forwarded->tl0PictureIndex == 1);

		context.SetTargetTemporalLayer(2);

		// Frame 3. Old packet with higher temporal layer than current.
		forwarded = ProcessVP8Packet(context, 3, 0, 1);
		REQUIRE_FALSE(forwarded);
		REQUIRE(context.GetCurrentTemporalLayer() == 0);
	}
}

SCENARIO("encode VP8 payload descriptor", "[rtp][codecs][vp8]")
{
	/**
	 * VP8 Payload Descriptor
	 *
	 * 1 = X bit: Extended control bits present (I L T K)
	 * 0 = R bit: Reserved for future use
	 * 0 = N bit: Reference frame
	 * 0 = S bit: Continuation of VP8 partition
	 * 000 = Part Id: 0
	 * 1 = I bit: Picture byte ID
	 * 1 = L bit: TL0PICIDX byte present
	 * 1 = T bit: TID (temporal layer index) byte present
	 * 0 = K bit: TID/KEYIDX byte present
	 * 0000 = Reserved A: 14
	 * 0000000000000001 = PictureId
	 */

	// clang-format off
	uint8_t payload[] =
	{
		0x80, 0xe0, 0x01, 0x01,
		0xe8, 0x40, 0x7a, 0xd8
	};
	// clang-format on

	bool marker;

	SECTION("encode based on specific encoder")
	{
		auto* payloadDescriptor = RTP::Codecs::VP8::Parse(payload, sizeof(payload));

		REQUIRE(payloadDescriptor);

		RTP::Codecs::EncodingContext::Params params;
		params.spatialLayers  = 0;
		params.temporalLayers = 3;
		RTP::Codecs::VP8::EncodingContext context(params);

		context.SetCurrentTemporalLayer(3);
		context.SetTargetTemporalLayer(3);

		REQUIRE(payloadDescriptor->pictureId == 1);

		auto* payloadDescriptorHandler =
		  new RTP::Codecs::VP8::PayloadDescriptorHandler(payloadDescriptor);

		std::unique_ptr<RTP::Packet> packet{ RTP::Packet::Factory(FactoryBuffer, sizeof(FactoryBuffer)) };

		packet->SetPayload(payload, sizeof(payload));

		auto forwarded = payloadDescriptorHandler->Process(&context, packet.get(), marker);
		REQUIRE(forwarded);

		auto encoder1 = payloadDescriptorHandler->GetEncoder();
		REQUIRE(encoder1);

		// Update pictureId.
		payloadDescriptor->pictureId = 2;

		packet.reset(RTP::Packet::Factory(FactoryBuffer, sizeof(FactoryBuffer)));

		packet->SetPayload(payload, sizeof(payload));

		forwarded = payloadDescriptorHandler->Process(&context, packet.get(), marker);
		REQUIRE(forwarded);
		REQUIRE(payloadDescriptor->pictureId == 2);

		// encoder2 contains the pictureId value 2.
		auto encoder2 = payloadDescriptorHandler->GetEncoder();
		REQUIRE(encoder2);

		// Encode with encoder1.
		packet.reset(RTP::Packet::Factory(FactoryBuffer, sizeof(FactoryBuffer)));

		packet->SetPayload(payload, sizeof(payload));

		payloadDescriptorHandler->Encode(packet.get(), encoder1.get());

		// Parse the payload.
		auto* payloadDescriptor2 = RTP::Codecs::VP8::Parse(payload, sizeof(payload));
		REQUIRE(payloadDescriptor2);
		REQUIRE(payloadDescriptor2->pictureId == 1);

		// Encode with encoder2.
		packet.reset(RTP::Packet::Factory(FactoryBuffer, sizeof(FactoryBuffer)));

		packet->SetPayload(payload, sizeof(payload));

		payloadDescriptorHandler->Encode(packet.get(), encoder2.get());

		// Parse the payload.
		auto* payloadDescriptor3 =
		  RTP::Codecs::VP8::Parse(packet->GetPayload(), packet->GetPayloadLength());
		REQUIRE(payloadDescriptor3);
		REQUIRE(payloadDescriptor3->pictureId == 2);

		delete payloadDescriptor3;
		delete payloadDescriptor2;
		delete payloadDescriptorHandler;
	}
}
