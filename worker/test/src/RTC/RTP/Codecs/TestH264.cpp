#include "common.hpp"
#include "RTC/RTP/Codecs/H264.hpp"
#include <catch2/catch_test_macros.hpp>

using namespace RTC;

SCENARIO("parse H264 payload descriptor", "[codecs][h264]")
{
	SECTION("parse payload descriptor")
	{
		// clang-format off
		uint8_t originalBuffer[] =
		{
			0x07, 0x80, 0x11, 0x00
		};
		// clang-format on
		//
		// Keep a copy of the original buffer for comparing.
		uint8_t buffer[4] = { 0 };

		std::memcpy(buffer, originalBuffer, sizeof(buffer));

		RTP::Codecs::DependencyDescriptor* dependencyDescriptor{ nullptr };

		std::unique_ptr<RTP::Codecs::H264::PayloadDescriptor> payloadDescriptor{ RTP::Codecs::H264::Parse(
			buffer, sizeof(buffer), dependencyDescriptor) };

		REQUIRE(payloadDescriptor);
	}
}
