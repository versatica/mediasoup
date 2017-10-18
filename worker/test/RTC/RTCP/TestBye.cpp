#include "common.hpp"
#include "catch.hpp"
#include "RTC/RTCP/Bye.hpp"
#include <string>

using namespace RTC::RTCP;

SCENARIO("RTCP BYE parsing", "[parser][rtcp][bye]")
{
	SECTION("create ByePacket")
	{
		uint32_t ssrc1 = 1111;
		uint32_t ssrc2 = 2222;
		std::string reason("hasta la vista");
		// Create local Bye packet and check content.
		// ByePacket();
		ByePacket bye1;

		bye1.AddSsrc(ssrc1);
		bye1.AddSsrc(ssrc2);
		bye1.SetReason(reason);

		ByePacket::Iterator it = bye1.Begin();

		REQUIRE(*it == ssrc1);
		it++;
		REQUIRE(*it == ssrc2);
		REQUIRE(bye1.GetReason() == reason);

		// Locally store the content of the packet.
		uint8_t buffer[bye1.GetSize()];

		bye1.Serialize(buffer);

		// Parse the buffer of the previous packet and check content.
		ByePacket* bye2 = ByePacket::Parse(buffer, sizeof(buffer));

		it = bye2->Begin();

		REQUIRE(*it == ssrc1);
		it++;
		REQUIRE(*it == ssrc2);
		REQUIRE(bye2->GetReason() == reason);
	}
}
