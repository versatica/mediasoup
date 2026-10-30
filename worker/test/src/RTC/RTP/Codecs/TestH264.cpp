#include "common.hpp"
#include "RTC/RTP/Codecs/H264.hpp"
#include "RTC/RTP/Packet.hpp"
#include "test/include/RTC/RTP/rtpCommon.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memcpy()
#include <vector>

namespace
{
	constexpr uint8_t DependencyDescriptorId{ static_cast<uint8_t>(
		RTC::RtpHeaderExtensionUri::Type::DEPENDENCY_DESCRIPTOR) };

	// Single NAL unit packet holding an IDR.
	// clang-format off
	uint8_t KeyFramePayload[] =
	{
		0x07, 0x80, 0x11, 0x00
	};
	// clang-format on

	/**
	 * Creates an H264 packet carrying the given Dependency Descriptor in an RTP
	 * header extension `reservedLen` bytes long, which is the room the Producer
	 * makes in it for the Consumers to rewrite the descriptor.
	 */
	std::unique_ptr<RTC::RTP::Packet> createPacket(
	  const uint8_t* dependencyDescriptor, uint8_t len, uint8_t reservedLen)
	{
		std::unique_ptr<RTC::RTP::Packet> packet{ RTC::RTP::Packet::Factory(
			rtpCommon::FactoryBuffer, sizeof(rtpCommon::FactoryBuffer)) };

		REQUIRE(packet);

		if (dependencyDescriptor)
		{
			uint8_t value[RTC::Consts::TwoBytesRtpExtensionMaxLength]{};

			std::memcpy(value, dependencyDescriptor, len);

			const std::vector<RTC::RTP::Packet::Extension> extensions{
				{ RTC::RtpHeaderExtensionUri::Type::DEPENDENCY_DESCRIPTOR,
				 DependencyDescriptorId, reservedLen,
				 value }
			};

			REQUIRE(packet->SetExtensions(RTC::RTP::Packet::ExtensionsType::Auto, extensions) == true);

			// As the Producer does, tell the length the descriptor really has.
			REQUIRE(packet->UpdateDependencyDescriptor(dependencyDescriptor, len));
		}

		packet->SetPayload(KeyFramePayload, sizeof(KeyFramePayload));

		return packet;
	}
} // namespace

SCENARIO("H264 payload descriptor", "[rtp][codecs][h264]")
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

		std::unique_ptr<RTC::RTP::Codecs::H264::PayloadDescriptor> payloadDescriptor{
			RTC::RTP::Codecs::H264::Parse(buffer, sizeof(buffer))
		};

		REQUIRE(payloadDescriptor);
		REQUIRE(payloadDescriptor->isKeyFrame);
	}

	/**
	 * The Dependency Descriptor tells the layers of the frame regardless of the
	 * codec, and it must be rewritten for the layers each Consumer is given, so
	 * it takes over whenever the packet carries one.
	 */
	SECTION("the Dependency Descriptor takes over when the packet carries one")
	{
		// Key frame carrying the template dependency structure of an L1T2 stream.
		// clang-format off
		uint8_t keyFrameDescriptor[] =
		{
			0x80, 0x00, 0xE8, 0x80,
			0x01, 0x1E, 0xA8, 0x51,
			0x41, 0x01, 0x0C, 0x13,
			0xFC, 0x0B, 0x3C,
		};

		// Mandatory fields only, so nothing tells about the active decode targets.
		uint8_t deltaFrameDescriptor[] =
		{
			0x00, 0x00, 0xE8,
		};
		// clang-format on

		std::shared_ptr<RTC::RTP::Codecs::DependencyDescriptor::TemplateDependencyStructure>
		  templateDependencyStructure;

		// NOTE: Taken apart from any packet, just to get the structure that the
		// delta frame below is parsed with.
		const std::unique_ptr<RTC::RTP::Codecs::DependencyDescriptor> keyFrame{
			RTC::RTP::Codecs::DependencyDescriptor::Parse(
			  keyFrameDescriptor, sizeof(keyFrameDescriptor), templateDependencyStructure)
		};

		REQUIRE(keyFrame);
		REQUIRE(templateDependencyStructure->decodeTargetCount == 2);

		// Room for the active decode targets bitmask the Consumer writes.
		const auto packet =
		  createPacket(deltaFrameDescriptor, sizeof(deltaFrameDescriptor), /*reservedLen*/ 4);

		RTC::RTP::Codecs::H264::ProcessRtpPacket(packet.get(), templateDependencyStructure);

		/*
		 * The H264 payload descriptor handler never reserves room for a rewritten
		 * Dependency Descriptor, so a non zero length tells that the Dependency
		 * Descriptor one took over.
		 */
		REQUIRE(packet->GetDependencyDescriptorMaxLength() > 0);
		REQUIRE(packet->GetSpatialLayer() == 0);
		REQUIRE(packet->GetTemporalLayer() == 0);

		// A Consumer that only wants the lowest temporal layer.
		RTC::RTP::Codecs::EncodingContext::Params params;

		params.spatialLayers              = 1;
		params.temporalLayers             = 2;
		params.preferDependencyDescriptor = true;

		RTC::RTP::Codecs::H264::EncodingContext context(params);

		context.SetCurrentSpatialLayer(0);
		context.SetCurrentTemporalLayer(0);
		context.SetTargetSpatialLayer(0);
		context.SetTargetTemporalLayer(0);

		bool marker{ false };

		REQUIRE(packet->ProcessPayload(std::addressof(context), marker));

		// And it's told that only decode target 0 is active.
		std::unique_ptr<RTC::RTP::Codecs::DependencyDescriptor> forwarded;

		packet->ReadDependencyDescriptor(forwarded, templateDependencyStructure);

		REQUIRE(forwarded);
		REQUIRE(forwarded->activeDecodeTargetsBitmask == 0b01);
	}

	/**
	 * A simulcast Consumer is given one of several streams, so its spatial layer
	 * is chosen by the ProducerStreamManager when it picks the stream and the
	 * target spatial layer of its encoding context is never set.
	 */
	SECTION("the Dependency Descriptor of a simulcast Consumer filters temporal layers only")
	{
		// clang-format off
		uint8_t keyFrameDescriptor[] =
		{
			0x80, 0x00, 0xE8, 0x80,
			0x01, 0x1E, 0xA8, 0x51,
			0x41, 0x01, 0x0C, 0x13,
			0xFC, 0x0B, 0x3C,
		};

		uint8_t deltaFrameDescriptor[] =
		{
			0x00, 0x00, 0xE8,
		};
		// clang-format on

		std::shared_ptr<RTC::RTP::Codecs::DependencyDescriptor::TemplateDependencyStructure>
		  templateDependencyStructure;

		const std::unique_ptr<RTC::RTP::Codecs::DependencyDescriptor> keyFrame{
			RTC::RTP::Codecs::DependencyDescriptor::Parse(
			  keyFrameDescriptor, sizeof(keyFrameDescriptor), templateDependencyStructure)
		};

		REQUIRE(keyFrame);

		const auto packet =
		  createPacket(deltaFrameDescriptor, sizeof(deltaFrameDescriptor), /*reservedLen*/ 4);

		RTC::RTP::Codecs::H264::ProcessRtpPacket(packet.get(), templateDependencyStructure);

		RTC::RTP::Codecs::EncodingContext::Params params;

		params.spatialLayers              = 1;
		params.temporalLayers             = 2;
		params.simulcast                  = true;
		params.preferDependencyDescriptor = true;

		RTC::RTP::Codecs::H264::EncodingContext context(params);

		/*
		 * Just as SimulcastProducerStreamManager does: only the temporal layers
		 * are given to the context, so the spatial ones are left at -1.
		 */
		context.SetTargetTemporalLayer(1);
		context.SetCurrentTemporalLayer(1);

		REQUIRE(context.GetTargetSpatialLayer() == -1);

		bool marker{ false };

		// It must be forwarded rather than aborting on the unset spatial layer.
		REQUIRE(packet->ProcessPayload(std::addressof(context), marker));

		// And every decode target of the stream is told active, since no spatial
		// layer is being filtered out.
		std::unique_ptr<RTC::RTP::Codecs::DependencyDescriptor> forwarded;

		packet->ReadDependencyDescriptor(forwarded, templateDependencyStructure);

		REQUIRE(forwarded);
		REQUIRE(forwarded->activeDecodeTargetsBitmask == 0b11);
	}

	SECTION("the H264 payload descriptor is used when there is no Dependency Descriptor")
	{
		const auto packet = createPacket(nullptr, 0, 0);

		std::shared_ptr<RTC::RTP::Codecs::DependencyDescriptor::TemplateDependencyStructure>
		  templateDependencyStructure;

		RTC::RTP::Codecs::H264::ProcessRtpPacket(packet.get(), templateDependencyStructure);

		// No Dependency Descriptor to rewrite, and the key frame is told by the
		// NAL unit of the payload.
		REQUIRE(packet->GetDependencyDescriptorMaxLength() == 0);
		REQUIRE(packet->IsKeyFrame());
	}
}
