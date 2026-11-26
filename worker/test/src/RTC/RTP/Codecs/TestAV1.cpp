#include "common.hpp"
#include "RTC/RTP/Codecs/AV1.hpp"
#include "RTC/RTP/Codecs/DependencyDescriptor.hpp"
#include "RTC/RTP/Codecs/Tools.hpp"
#include "RTC/RTP/Packet.hpp"
#include "test/include/RTC/RTP/rtpCommon.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memcpy()
#include <vector>

/**
 * How the Dependency Descriptor payload descriptor behaves once installed is
 * tested in TestDependencyDescriptorPayloadDescriptor.cpp, since every codec
 * shares it. What is tested here is only what AV1 itself does, this is, handing
 * its packets over to it.
 */

namespace
{
	constexpr uint8_t DependencyDescriptorId{ static_cast<uint8_t>(
		RTC::RtpHeaderExtensionUri::Type::DEPENDENCY_DESCRIPTOR) };

	/**
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

SCENARIO("AV1 payload descriptor", "[rtp][codecs][av1]")
{
	SECTION("parse the aggregation header")
	{
		/**
		 * AV1 Aggregation Header
		 *
		 * 0 = Z bit: The first OBU element does not continue one from the
		 *     previous packet
		 * 1 = Y bit: The last OBU element continues in the next packet
		 * 10 = W: Two OBU elements, the last one with no length field
		 * 1 = N bit: First packet of a coded video sequence
		 */

		// clang-format off
		const uint8_t payload[] =
		{
			0x68, 0x0a, 0x0b
		};
		// clang-format on

		const std::unique_ptr<RTC::RTP::Codecs::AV1::PayloadDescriptor> payloadDescriptor{
			RTC::RTP::Codecs::AV1::Parse(payload, sizeof(payload))
		};

		REQUIRE(payloadDescriptor);

		REQUIRE(payloadDescriptor->z == 0);
		REQUIRE(payloadDescriptor->y == 1);
		REQUIRE(payloadDescriptor->w == 2);
		REQUIRE(payloadDescriptor->n == 1);

		REQUIRE(payloadDescriptor->isKeyFrame == true);
	}

	SECTION("a payload with no aggregation header is not parsed")
	{
		// clang-format off
		const uint8_t payload[] =
		{
			0x68
		};
		// clang-format on

		const std::unique_ptr<RTC::RTP::Codecs::AV1::PayloadDescriptor> payloadDescriptor{
			RTC::RTP::Codecs::AV1::Parse(payload, 0)
		};

		REQUIRE(!payloadDescriptor);
	}

	// A coded video sequence starts with a key frame, which the N bit tells, and
	// it cannot start in the middle of an OBU, so a packet that continues one
	// does not start it however N is set.
	SECTION("a key frame is the start of a coded video sequence that continues no OBU")
	{
		const auto isKeyFrame = [](uint8_t aggregationHeader) -> bool
		{
			const uint8_t payload[] = { aggregationHeader, 0x0a };

			const std::unique_ptr<RTC::RTP::Codecs::AV1::PayloadDescriptor> payloadDescriptor{
				RTC::RTP::Codecs::AV1::Parse(payload, sizeof(payload))
			};

			REQUIRE(payloadDescriptor);

			return payloadDescriptor->isKeyFrame;
		};

		// N, with no Z.
		REQUIRE(isKeyFrame(0x08) == true);
		// N and Z.
		REQUIRE(isKeyFrame(0x88) == false);
		// Z alone.
		REQUIRE(isKeyFrame(0x80) == false);
		// Neither.
		REQUIRE(isKeyFrame(0x00) == false);
	}
}

SCENARIO("process AV1 RTP packet", "[rtp][codecs][av1]")
{
	RTC::RTP::Codecs::DependencyDescriptor::Context dependencyDescriptorContext;

	const std::unique_ptr<RTC::RTP::Packet> packet{ RTC::RTP::Packet::Factory(
		rtpCommon::FactoryBuffer, sizeof(rtpCommon::FactoryBuffer)) };

	REQUIRE(packet);

	SECTION("a packet carrying a Dependency Descriptor gets a payload descriptor")
	{
		uint8_t value[RTC::Consts::TwoByteRtpExtensionMaxLength]{};

		std::memcpy(value, KeyFrameData, sizeof(KeyFrameData));

		const std::vector<RTC::RTP::Packet::Extension> extensions{
			{ RTC::RtpHeaderExtensionUri::Type::DEPENDENCY_DESCRIPTOR,
			 DependencyDescriptorId, static_cast<uint8_t>(sizeof(KeyFrameData)),
			 value }
		};

		REQUIRE(packet->SetExtensions(RTC::RTP::Packet::ExtensionsForm::Auto, extensions) == true);

		REQUIRE(RTC::RTP::Codecs::AV1::ProcessRtpPacket(packet.get(), dependencyDescriptorContext));

		// A payload descriptor handler is installed, which is what tells the
		// length the rewritten descriptor may grow to.
		REQUIRE(packet->GetDependencyDescriptorMaxLength() > 0);

		// The template dependency structure of the stream is kept for the delta
		// frames that follow, which do not carry it.
		REQUIRE(dependencyDescriptorContext.templateDependencyStructure);
	}

	// Nothing but the Dependency Descriptor tells the layers of an AV1 packet, so
	// without it the packet is forwarded as it comes. Its aggregation header
	// still tells whether it starts a key frame, which is what a Consumer waits
	// for before forwarding anything to its endpoint.
	SECTION("a packet carrying no Dependency Descriptor is told a key frame by its payload")
	{
		// Z=0, W=1, N=1.
		// clang-format off
		const uint8_t payload[] =
		{
			0x18, 0x0a, 0x0b
		};
		// clang-format on

		packet->SetPayload(payload, sizeof(payload));

		REQUIRE(RTC::RTP::Codecs::AV1::ProcessRtpPacket(packet.get(), dependencyDescriptorContext));

		REQUIRE(packet->GetDependencyDescriptorMaxLength() == 0);
		REQUIRE(!dependencyDescriptorContext.templateDependencyStructure);

		REQUIRE(packet->IsKeyFrame() == true);

		// There is nothing to filter, so every packet is taken for the only layer
		// the Consumer can be given.
		REQUIRE(packet->GetSpatialLayer() == 0);
		REQUIRE(packet->GetTemporalLayer() == 0);
	}

	/**
	 * A simulcast Consumer is given one of several streams rather than the layers
	 * of a single one, so the stream the ProducerStreamManager picks is what
	 * chooses its spatial layer and its target spatial layer is never set.
	 */
	SECTION("a simulcast Consumer filters no spatial layer")
	{
		RTC::RtpCodecMimeType mimeType;

		mimeType.SetMimeType("video/AV1");

		REQUIRE(
		  RTC::RTP::Codecs::Tools::IsValidTypeForCodec(RTC::RtpParameters::Type::SIMULCAST, mimeType));

		uint8_t value[RTC::Consts::TwoByteRtpExtensionMaxLength]{};

		std::memcpy(value, KeyFrameData, sizeof(KeyFrameData));

		const std::vector<RTC::RTP::Packet::Extension> extensions{
			{ RTC::RtpHeaderExtensionUri::Type::DEPENDENCY_DESCRIPTOR,
			 DependencyDescriptorId, static_cast<uint8_t>(sizeof(KeyFrameData)),
			 value }
		};

		REQUIRE(packet->SetExtensions(RTC::RTP::Packet::ExtensionsForm::Auto, extensions) == true);

		REQUIRE(RTC::RTP::Codecs::AV1::ProcessRtpPacket(packet.get(), dependencyDescriptorContext));

		const RTC::RTP::Codecs::EncodingContext::Params params{ .spatialLayers  = 3,
		                                                        .temporalLayers = 3,
		                                                        .simulcast      = true };

		RTC::RTP::Codecs::AV1::EncodingContext context(params);

		context.SyncRequired();
		context.SetCurrentTemporalLayer(0);
		context.SetTargetTemporalLayer(0);

		// As the SimulcastProducerStreamManager leaves it.
		REQUIRE(context.GetTargetSpatialLayer() == -1);

		// The packet is given to the Dependency Descriptor handler, which AV1
		// installs as the handler of the codec, and no spatial layer filters it out.
		bool marker{ false };

		REQUIRE(packet->ProcessPayload(std::addressof(context), marker));
	}

	SECTION("a packet carrying no Dependency Descriptor and no key frame is told so")
	{
		// Z=0, W=1, N=0.
		// clang-format off
		const uint8_t payload[] =
		{
			0x10, 0x0a, 0x0b
		};
		// clang-format on

		packet->SetPayload(payload, sizeof(payload));

		REQUIRE(RTC::RTP::Codecs::AV1::ProcessRtpPacket(packet.get(), dependencyDescriptorContext));

		REQUIRE(packet->IsKeyFrame() == false);
	}
}
