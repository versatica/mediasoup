#include "common.hpp"
#include "RTC/Codecs/VP8.hpp"
#include <catch.hpp>
#include <cstring> // std::memcmp()

using namespace RTC;

SCENARIO("parse VP8 payload descriptor", "[codecs][vp8]")
{
	SECTION("parse payload descriptor")
	{
		/** VP8 Payload Descriptor
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

		const auto* payloadDescriptor = Codecs::VP8::Parse(buffer, sizeof(buffer));

		REQUIRE(payloadDescriptor);

		REQUIRE(payloadDescriptor->extended == 1);
		REQUIRE(payloadDescriptor->nonReference == 0);
		REQUIRE(payloadDescriptor->start == 1);
		REQUIRE(payloadDescriptor->partitionIndex == 0);

		// optional field flags.
		REQUIRE(payloadDescriptor->i == 1);
		REQUIRE(payloadDescriptor->l == 0);
		REQUIRE(payloadDescriptor->t == 0);
		REQUIRE(payloadDescriptor->k == 0);

		// optional fields.
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

		delete payloadDescriptor;
	}

	SECTION("parse payload descriptor 2")
	{
		/** VP8 Payload Descriptor
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
		const auto* payloadDescriptor = Codecs::VP8::Parse(buffer, sizeof(buffer));

		REQUIRE(payloadDescriptor);

		REQUIRE(payloadDescriptor->extended == 1);
		REQUIRE(payloadDescriptor->nonReference == 0);
		REQUIRE(payloadDescriptor->start == 0);
		REQUIRE(payloadDescriptor->partitionIndex == 0);

		// optional field flags.
		REQUIRE(payloadDescriptor->i == 0);
		REQUIRE(payloadDescriptor->l == 0);
		REQUIRE(payloadDescriptor->t == 1);
		REQUIRE(payloadDescriptor->k == 1);

		// optional fields.
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

		delete payloadDescriptor;
	};

	SECTION("parse payload descriptor. I flag set but no space for pictureId")
	{
		/** VP8 Payload Descriptor
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

		auto payloadDescriptor = Codecs::VP8::Parse(buffer, sizeof(buffer));

		REQUIRE_FALSE(payloadDescriptor);
	}

	SECTION("parse payload descriptor. X flag is not set")
	{
		/** VP8 Payload Descriptor
		 *
		 * 0 = X bit: Extended control bits present (I L T K)
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
		uint8_t buffer[] =
		{
			0x50, 0x80, 0x11
		};
		// clang-format on

		auto payloadDescriptor = Codecs::VP8::Parse(buffer, sizeof(buffer));

		REQUIRE_FALSE(payloadDescriptor);
	}
}
