#include "common.hpp"
#include "RTC/RTP/Codecs/AV1.hpp"
#include "RTC/RTP/Codecs/DependencyDescriptor.hpp"
#include "RTC/RTP/Packet.hpp"
#include "test/include/RTC/RTP/rtpCommon.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memcpy()
#include <vector>

/*
 * How the Dependency Descriptor payload descriptor behaves once installed is
 * tested in TestDependencyDescriptorPayloadDescriptor.cpp, since every codec
 * shares it. What is tested here is only what AV1 itself does, this is, handing
 * its packets over to it.
 */

namespace
{
	constexpr uint8_t DependencyDescriptorId{ static_cast<uint8_t>(
		RTC::RtpHeaderExtensionUri::Type::DEPENDENCY_DESCRIPTOR) };

	/*
	 * Key frame of an L3T3_KEY stream captured from Chrome. Its contents do not
	 * matter here beyond being a descriptor that parses.
	 */
	// clang-format off
	const uint8_t KeyFrameData[] =
	{
		0xc0, 0x00, 0x4f, 0x80, 0x08, 0x14, 0x85, 0x21, 0x4e, 0xaa, 0xaa, 0xa8,
		0x00, 0x06, 0x00, 0x00, 0x40, 0x00, 0x10, 0x00, 0x02, 0xaa, 0x80, 0xa8,
		0x00, 0x06, 0x00, 0x00, 0x40, 0x00, 0x10, 0x00, 0x02, 0xa0, 0x00, 0xa8,
		0x00, 0x06, 0x00, 0x00, 0x40, 0x00, 0x16, 0xd5, 0x49, 0x24, 0x1b, 0x55,
		0x24, 0x90, 0x6d, 0x54, 0x92, 0x31, 0x57, 0xe0, 0x01, 0x97, 0x4c, 0xa8,
		0x64, 0x33, 0x0e, 0x22, 0x23, 0x96, 0xec, 0xa8, 0x65, 0x53, 0x04, 0x22,
		0x43, 0x90, 0xec, 0xa8, 0x77, 0x53, 0x01, 0x3f, 0x00, 0xb3, 0x02, 0x7f,
		0x01, 0x67, 0x04, 0xff, 0x02, 0xcf
	};
	// clang-format on
} // namespace

SCENARIO("process AV1 RTP packet", "[rtp][codecs][av1]")
{
	std::shared_ptr<RTC::RTP::Codecs::DependencyDescriptor::TemplateDependencyStructure>
	  templateDependencyStructure;

	const std::unique_ptr<RTC::RTP::Packet> packet{ RTC::RTP::Packet::Factory(
		rtpCommon::FactoryBuffer, sizeof(rtpCommon::FactoryBuffer)) };

	REQUIRE(packet);

	SECTION("a packet carrying a Dependency Descriptor gets a payload descriptor")
	{
		uint8_t value[RTC::Consts::TwoBytesRtpExtensionMaxLength]{};

		std::memcpy(value, KeyFrameData, sizeof(KeyFrameData));

		const std::vector<RTC::RTP::Packet::Extension> extensions{
			{ RTC::RtpHeaderExtensionUri::Type::DEPENDENCY_DESCRIPTOR,
			 DependencyDescriptorId, static_cast<uint8_t>(sizeof(KeyFrameData)),
			 value }
		};

		packet->SetExtensions(RTC::RTP::Packet::ExtensionsType::Auto, extensions);

		RTC::RTP::Codecs::AV1::ProcessRtpPacket(packet.get(), templateDependencyStructure);

		// A payload descriptor handler is installed, which is what tells the
		// length the rewritten descriptor may grow to.
		REQUIRE(packet->GetDependencyDescriptorMaxLength() > 0);

		// The template dependency structure of the stream is kept for the delta
		// frames that follow, which do not carry it.
		REQUIRE(templateDependencyStructure);
	}

	/*
	 * Nothing but the Dependency Descriptor tells the layers of an AV1 packet,
	 * so without it there is nothing to process and the packet is forwarded just
	 * as it comes.
	 */
	SECTION("a packet carrying no Dependency Descriptor gets no payload descriptor")
	{
		RTC::RTP::Codecs::AV1::ProcessRtpPacket(packet.get(), templateDependencyStructure);

		REQUIRE(packet->GetDependencyDescriptorMaxLength() == 0);
		REQUIRE(!templateDependencyStructure);
	}
}
