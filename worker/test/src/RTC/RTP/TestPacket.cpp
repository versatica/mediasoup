#include "common.hpp"
#include "MediaSoupErrors.hpp"
#include "RTC/RTP/Packet.hpp"
#include "RTC/RTP/common.hpp" // in worker/test/include/
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memset()

using namespace RTC::RTP;

// NOLINTNEXTLINE (clang-tidy readability-function-size)
SCENARIO("RTP Packet", "[rtp][serializable]")
{
	SECTION("Packet::Parse() succeeds")
	{
		// clang-format off
		uint8_t buffer[] =
		{
			0x80, 0x01, 0x00, 0x08,
			0x00, 0x00, 0x00, 0x04,
			0x00, 0x00, 0x00, 0x05
		};
		// clang-format on

		auto* packet = Packet::Parse(buffer, sizeof(buffer));

		CHECK_PACKET(
		  /*packet*/ packet,
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 12,
		  /*frozen*/ true,
		  /*payloadType*/ 1,
		  /*hasMarker*/ false,
		  /*seqNumber*/ 8,
		  /*timestamp*/ 4,
		  /*ssrc*/ 5,
		  /*hasCsrcs*/ false,
		  /*hasHeaderExtension*/ false);
	}
}
