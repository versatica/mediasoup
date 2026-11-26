#include "common.hpp"
#include "RTC/RTP/Codecs/PictureIdUnwrapper.hpp"
#include <catch2/catch_test_macros.hpp>

// A sender picks how wide every pictureId is, 7 or 15 bits, and may change it
// at any packet, while VP8 and VP9 number their pictures with 15 bits.
SCENARIO("pictureIds are unwrapped into the 15 bits", "[rtp][codecs][picture-id-unwrapper]")
{
	constexpr uint16_t MaxPictureId = (1 << 15) - 1;

	SECTION("a 7 bits sequence goes on growing across its wrap")
	{
		RTC::RTP::Codecs::PictureIdUnwrapper unwrapper;

		// The first pictureId is taken just as it comes.
		REQUIRE(unwrapper.Unwrap(126, false) == 126);
		REQUIRE(unwrapper.Unwrap(127, false) == 127);

		// Past the wrap the pictureIds go on growing rather than back to zero.
		REQUIRE(unwrapper.Unwrap(0, false) == 128);
		REQUIRE(unwrapper.Unwrap(1, false) == 129);
	}

	SECTION("a reordered pictureId lands before the highest one, which it does not move")
	{
		RTC::RTP::Codecs::PictureIdUnwrapper unwrapper;

		REQUIRE(unwrapper.Unwrap(126, false) == 126);
		REQUIRE(unwrapper.Unwrap(127, false) == 127);
		REQUIRE(unwrapper.Unwrap(0, false) == 128);

		// Reordered, so it is extended backwards and the highest one stays where
		// it was, as the next one shows.
		REQUIRE(unwrapper.Unwrap(127, false) == 127);
		REQUIRE(unwrapper.Unwrap(1, false) == 129);
	}

	SECTION("after a reset the next pictureId starts the sequence anew")
	{
		RTC::RTP::Codecs::PictureIdUnwrapper unwrapper;

		REQUIRE(unwrapper.Unwrap(126, false) == 126);
		REQUIRE(unwrapper.Unwrap(0, false) == 128);

		unwrapper.Reset();

		REQUIRE(unwrapper.Unwrap(5, false) == 5);
	}

	SECTION("a 15 bits sequence is left untouched")
	{
		RTC::RTP::Codecs::PictureIdUnwrapper unwrapper;

		// Already in the space the codecs number their pictures with, so nothing
		// is added to it, not even across its own wrap.
		REQUIRE(unwrapper.Unwrap(MaxPictureId - 1, true) == MaxPictureId - 1);
		REQUIRE(unwrapper.Unwrap(MaxPictureId, true) == MaxPictureId);
		REQUIRE(unwrapper.Unwrap(0, true) == 0);
		REQUIRE(unwrapper.Unwrap(1, true) == 1);
	}

	SECTION("the sequence goes on when the sender widens the pictureId to 15 bits")
	{
		RTC::RTP::Codecs::PictureIdUnwrapper unwrapper;

		REQUIRE(unwrapper.Unwrap(126, false) == 126);
		REQUIRE(unwrapper.Unwrap(127, false) == 127);

		// The sender takes 15 bits from here on, as it must to go past 127.
		REQUIRE(unwrapper.Unwrap(128, true) == 128);
		REQUIRE(unwrapper.Unwrap(129, true) == 129);
	}

	SECTION("the sequence goes on when the sender narrows the pictureId to 7 bits")
	{
		RTC::RTP::Codecs::PictureIdUnwrapper unwrapper;

		REQUIRE(unwrapper.Unwrap(300, true) == 300);

		// The sender restarts its numbering with 7 bits, which still carries the
		// low bits the sequence had.
		REQUIRE(unwrapper.Unwrap(45, false) == 301);
		REQUIRE(unwrapper.Unwrap(46, false) == 302);
	}

	SECTION("the unwrapped pictureIds wrap in turn at 15 bits")
	{
		RTC::RTP::Codecs::PictureIdUnwrapper unwrapper;

		uint16_t unwrapped{ 0 };

		// A whole 15 bits cycle of pictureIds numbered with 7 bits, so the last one
		// lands back at the pictureId the first one did.
		for (uint32_t pictureId{ 0 }; pictureId <= MaxPictureId + 1; ++pictureId)
		{
			unwrapped = unwrapper.Unwrap(static_cast<uint16_t>(pictureId % 128), false);
		}

		REQUIRE(unwrapped == 0);
	}
}
