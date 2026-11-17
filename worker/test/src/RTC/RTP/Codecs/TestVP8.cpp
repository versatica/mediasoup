#include "common.hpp"
#include "RTC/RTP/Codecs/VP8.hpp"
#include "RTC/RTP/Packet.hpp"
#include "test/include/RTC/RTP/rtpCommon.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memcmp(), std::memcpy()

SCENARIO("VP8 payload descriptor", "[rtp][codecs][vp8]")
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
		const uint8_t originalBuffer[] =
		{
			0xd0, 0x80, 0x11, 0x00,
		};
		// clang-format on

		// Keep a copy of the original buffer for comparing.
		uint8_t buffer[4] = { 0 };

		std::memcpy(buffer, originalBuffer, sizeof(buffer));

		const std::unique_ptr<RTC::RTP::Codecs::VP8::PayloadDescriptor> payloadDescriptor{
			RTC::RTP::Codecs::VP8::Parse(buffer, sizeof(buffer))
		};

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

			SECTION("compare encoded payload descriptor with original buffer")
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
		const uint8_t originalBuffer[] =
		{
		  0x88, 0x3e, 0xe4, 0x00,
		};
		// clang-format on

		// Keep a copy of the original buffer for comparing.
		uint8_t buffer[4] = { 0 };

		std::memcpy(buffer, originalBuffer, sizeof(buffer));

		// Parse the buffer.
		const std::unique_ptr<RTC::RTP::Codecs::VP8::PayloadDescriptor> payloadDescriptor{
			RTC::RTP::Codecs::VP8::Parse(buffer, sizeof(buffer))
		};

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
	}

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
		const uint8_t originalBuffer[] =
		{
			0xd0, 0xc0, 0x11, 0x03,
		};
		// clang-format on

		// Keep a copy of the original buffer for comparing.
		uint8_t buffer[4] = { 0 };

		std::memcpy(buffer, originalBuffer, sizeof(buffer));

		const std::unique_ptr<RTC::RTP::Codecs::VP8::PayloadDescriptor> payloadDescriptor{
			RTC::RTP::Codecs::VP8::Parse(buffer, sizeof(buffer))
		};

		REQUIRE(payloadDescriptor);

		REQUIRE(payloadDescriptor->pictureId == 17);
		REQUIRE(payloadDescriptor->tl0PictureIndex == 3);

		SECTION("encode payload descriptor")
		{
			payloadDescriptor->Encode(buffer, 20, 1);

			const std::unique_ptr<RTC::RTP::Codecs::VP8::PayloadDescriptor> encodedPayloadDescriptor{
				RTC::RTP::Codecs::VP8::Parse(buffer, sizeof(buffer))
			};

			REQUIRE(encodedPayloadDescriptor);
			REQUIRE(encodedPayloadDescriptor->pictureId == 20);
			REQUIRE(encodedPayloadDescriptor->tl0PictureIndex == 1);
		}

		SECTION("restore payload descriptor")
		{
			payloadDescriptor->Restore(buffer);

			const std::unique_ptr<RTC::RTP::Codecs::VP8::PayloadDescriptor> restoredPayloadDescriptor{
				RTC::RTP::Codecs::VP8::Parse(buffer, sizeof(buffer))
			};

			REQUIRE(restoredPayloadDescriptor);
			REQUIRE(restoredPayloadDescriptor->pictureId == 17);
			REQUIRE(restoredPayloadDescriptor->tl0PictureIndex == 3);
		}
	}

	SECTION("parse payload descriptor, I flag set but no space for pictureId")
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
		const uint8_t buffer[] =
		{
			0xd0, 0x80,
		};
		// clang-format on

		const std::unique_ptr<RTC::RTP::Codecs::VP8::PayloadDescriptor> payloadDescriptor{
			RTC::RTP::Codecs::VP8::Parse(buffer, sizeof(buffer))
		};

		REQUIRE(!payloadDescriptor);
	}

	SECTION("parse payload descriptor, X flag is not set, no keyframe")
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
		const uint8_t buffer[] =
		{
			0x50, 0x01,
		};
		// clang-format on

		const std::unique_ptr<RTC::RTP::Codecs::VP8::PayloadDescriptor> payloadDescriptor{
			RTC::RTP::Codecs::VP8::Parse(buffer, sizeof(buffer))
		};

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
	}

	SECTION("parse payload descriptor, X flag is not set, keyframe")
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
		const uint8_t buffer[] =
		{
			0x50, 0x00,
		};
		// clang-format on

		const std::unique_ptr<RTC::RTP::Codecs::VP8::PayloadDescriptor> payloadDescriptor{
			RTC::RTP::Codecs::VP8::Parse(buffer, sizeof(buffer))
		};

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
	}
}

SCENARIO("process VP8 payload descriptor", "[rtp][codecs][vp8]")
{
	constexpr uint16_t MaxPictureId = (1 << 15) - 1;

	const auto createVP8PayloadDescriptor = [](
	                                          uint8_t* buffer,
	                                          size_t bufferLen,
	                                          uint16_t pictureId,
	                                          uint8_t tl0PictureIndex,
	                                          uint8_t tlIndex,
	                                          bool layerSync =
	                                            true) -> RTC::RTP::Codecs::VP8::PayloadDescriptor*
	{
		const uint16_t netPictureId = htons(pictureId);

		std::memcpy(buffer + 2, std::addressof(netPictureId), 2);
		buffer[2] |= 0x80;
		buffer[4] = tl0PictureIndex;
		buffer[5] = tlIndex << 6;

		if (layerSync)
		{
			buffer[5] |= 0x20; // y bit
		}

		auto* const payloadDescriptor = RTC::RTP::Codecs::VP8::Parse(buffer, bufferLen);

		REQUIRE(payloadDescriptor);

		return payloadDescriptor;
	};

	const auto processVP8Packet =
	  [&createVP8PayloadDescriptor](
	    RTC::RTP::Codecs::VP8::EncodingContext& context,
	    uint16_t pictureId,
	    uint8_t tl0PictureIndex,
	    uint8_t tlIndex,
	    bool layerSync = true) -> std::unique_ptr<RTC::RTP::Codecs::VP8::PayloadDescriptor>
	{
		// clang-format off
		const uint8_t payload[] =
		{
			0x90, 0xe0, 0x80, 0x00, 0x00, 0x00,
		};
		// clang-format on

		const std::unique_ptr<RTC::RTP::Packet> packet{ RTC::RTP::Packet::Factory(
			rtpCommon::FactoryBuffer, sizeof(rtpCommon::FactoryBuffer)) };

		REQUIRE(packet);

		packet->SetPayload(payload, sizeof(payload));

		auto* const payloadDescriptor = createVP8PayloadDescriptor(
		  packet->GetPayload(), packet->GetPayloadLength(), pictureId, tl0PictureIndex, tlIndex, layerSync);

		const auto payloadDescriptorHandler =
		  std::make_unique<RTC::RTP::Codecs::VP8::PayloadDescriptorHandler>(payloadDescriptor);

		bool marker;

		if (payloadDescriptorHandler->Process(std::addressof(context), packet.get(), marker))
		{
			return std::unique_ptr<RTC::RTP::Codecs::VP8::PayloadDescriptor>(
			  RTC::RTP::Codecs::VP8::Parse(packet->GetPayload(), packet->GetPayloadLength()));
		}

		return nullptr;
	};

	SECTION("do not drop TL0PICIDX from temporal layers higher than 0")
	{
		const RTC::RTP::Codecs::EncodingContext::Params params{
			.spatialLayers  = 0,
			.temporalLayers = 2,
		};

		RTC::RTP::Codecs::VP8::EncodingContext context(params);

		context.SetCurrentTemporalLayer(0);
		context.SetTargetTemporalLayer(0);

		// Frame 1.
		auto forwarded = processVP8Packet(context, 0, 0, 0);

		REQUIRE(forwarded);
		REQUIRE(forwarded->pictureId == 0);
		REQUIRE(forwarded->tl0PictureIndex == 0);

		// Frame 2 gets lost.

		// Frame 3.
		forwarded = processVP8Packet(context, 2, 1, 1);

		REQUIRE(!forwarded);

		// Frame 2 retransmitted.
		forwarded = processVP8Packet(context, 1, 1, 0);

		REQUIRE(forwarded);
		REQUIRE(forwarded->pictureId == 1);
		REQUIRE(forwarded->tl0PictureIndex == 1);
	}

	SECTION("drop packets that belong to other temporal layers after rolling over pictureID")
	{
		const RTC::RTP::Codecs::EncodingContext::Params params{
			.spatialLayers  = 0,
			.temporalLayers = 2,
		};

		RTC::RTP::Codecs::VP8::EncodingContext context(params);

		context.SyncRequired();

		context.SetCurrentTemporalLayer(0);
		context.SetTargetTemporalLayer(0);

		// Frame 1.
		auto forwarded = processVP8Packet(context, MaxPictureId, 0, 0);

		REQUIRE(forwarded);
		REQUIRE(forwarded->pictureId == 1);
		REQUIRE(forwarded->tl0PictureIndex == 1);

		// Frame 2.
		forwarded = processVP8Packet(context, 0, 0, 0);

		REQUIRE(forwarded);
		REQUIRE(forwarded->pictureId == 2);
		REQUIRE(forwarded->tl0PictureIndex == 1);

		// Frame 3.
		forwarded = processVP8Packet(context, 1, 0, 1);

		REQUIRE(!forwarded);
	}

	SECTION("old packets with higher temporal layer than current are dropped")
	{
		const RTC::RTP::Codecs::EncodingContext::Params params{
			.spatialLayers  = 0,
			.temporalLayers = 2,
		};

		RTC::RTP::Codecs::VP8::EncodingContext context(params);

		context.SyncRequired();

		context.SetCurrentTemporalLayer(0);
		context.SetTargetTemporalLayer(0);

		// Frame 1.
		auto forwarded = processVP8Packet(context, 1, 0, 0);

		REQUIRE(forwarded);
		REQUIRE(forwarded->pictureId == 1);
		REQUIRE(forwarded->tlIndex == 0);
		REQUIRE(forwarded->tl0PictureIndex == 1);

		// Frame 2.
		forwarded = processVP8Packet(context, 2, 0, 0);

		REQUIRE(forwarded);
		REQUIRE(forwarded->pictureId == 2);
		REQUIRE(forwarded->tlIndex == 0);
		REQUIRE(forwarded->tl0PictureIndex == 1);

		// Frame 3. Old packet with higher temporal layer than current.
		forwarded = processVP8Packet(context, 0, 0, 1);

		REQUIRE(!forwarded);
		REQUIRE(context.GetCurrentTemporalLayer() == 0);
	}

	SECTION("packets with higher temporal layer than current are dropped")
	{
		const RTC::RTP::Codecs::EncodingContext::Params params{
			.spatialLayers  = 0,
			.temporalLayers = 2,
		};

		RTC::RTP::Codecs::VP8::EncodingContext context(params);

		context.SyncRequired();

		context.SetCurrentTemporalLayer(0);
		context.SetTargetTemporalLayer(0);

		// Frame 1.
		auto forwarded = processVP8Packet(context, 1, 0, 0);

		REQUIRE(forwarded);
		REQUIRE(forwarded->pictureId == 1);
		REQUIRE(forwarded->tlIndex == 0);
		REQUIRE(forwarded->tl0PictureIndex == 1);

		// Frame 2.
		forwarded = processVP8Packet(context, 2, 0, 0);

		REQUIRE(forwarded);
		REQUIRE(forwarded->pictureId == 2);
		REQUIRE(forwarded->tlIndex == 0);
		REQUIRE(forwarded->tl0PictureIndex == 1);

		context.SetTargetTemporalLayer(2);

		// Frame 3. Old packet with higher temporal layer than current.
		forwarded = processVP8Packet(context, 3, 0, 1);

		REQUIRE(!forwarded);
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
	const uint8_t payload[] =
	{
		0x80, 0xe0, 0x01, 0x01,
		0xe8, 0x40, 0x7a, 0xd8,
	};
	// clang-format on

	bool marker;

	SECTION("encode based on specific encoder")
	{
		auto* const payloadDescriptor = RTC::RTP::Codecs::VP8::Parse(payload, sizeof(payload));

		REQUIRE(payloadDescriptor);

		const RTC::RTP::Codecs::EncodingContext::Params params{
			.spatialLayers  = 0,
			.temporalLayers = 3,
		};

		RTC::RTP::Codecs::VP8::EncodingContext context(params);

		context.SetCurrentTemporalLayer(3);
		context.SetTargetTemporalLayer(3);

		REQUIRE(payloadDescriptor->pictureId == 1);

		const auto payloadDescriptorHandler =
		  std::make_unique<RTC::RTP::Codecs::VP8::PayloadDescriptorHandler>(payloadDescriptor);

		std::unique_ptr<RTC::RTP::Packet> packet{ RTC::RTP::Packet::Factory(
			rtpCommon::FactoryBuffer, sizeof(rtpCommon::FactoryBuffer)) };

		packet->SetPayload(payload, sizeof(payload));

		auto forwarded = payloadDescriptorHandler->Process(std::addressof(context), packet.get(), marker);

		REQUIRE(forwarded);

		const auto encoder1 = std::move(context.encoder);

		REQUIRE(encoder1);

		// Update pictureId.
		payloadDescriptor->pictureId = 2;

		packet.reset(
		  RTC::RTP::Packet::Factory(rtpCommon::FactoryBuffer, sizeof(rtpCommon::FactoryBuffer)));

		packet->SetPayload(payload, sizeof(payload));

		forwarded = payloadDescriptorHandler->Process(std::addressof(context), packet.get(), marker);

		REQUIRE(forwarded);
		REQUIRE(payloadDescriptor->pictureId == 2);

		// encoder2 contains the pictureId value 2.
		const auto encoder2 = std::move(context.encoder);

		REQUIRE(encoder2);

		// Encode with encoder1.
		packet.reset(
		  RTC::RTP::Packet::Factory(rtpCommon::FactoryBuffer, sizeof(rtpCommon::FactoryBuffer)));

		packet->SetPayload(payload, sizeof(payload));

		payloadDescriptorHandler->Encode(packet.get(), encoder1.get());

		// Parse the payload.
		const std::unique_ptr<RTC::RTP::Codecs::VP8::PayloadDescriptor> payloadDescriptor2{
			RTC::RTP::Codecs::VP8::Parse(packet->GetPayload(), packet->GetPayloadLength())
		};

		REQUIRE(payloadDescriptor2);
		REQUIRE(payloadDescriptor2->pictureId == 1);

		// Encode with encoder2.
		packet.reset(
		  RTC::RTP::Packet::Factory(rtpCommon::FactoryBuffer, sizeof(rtpCommon::FactoryBuffer)));

		packet->SetPayload(payload, sizeof(payload));

		payloadDescriptorHandler->Encode(packet.get(), encoder2.get());

		// Parse the payload.
		const std::unique_ptr<RTC::RTP::Codecs::VP8::PayloadDescriptor> payloadDescriptor3{
			RTC::RTP::Codecs::VP8::Parse(packet->GetPayload(), packet->GetPayloadLength())
		};

		REQUIRE(payloadDescriptor3);
		REQUIRE(payloadDescriptor3->pictureId == 2);
	}
}
