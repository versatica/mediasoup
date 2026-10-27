#include "common.hpp"
#include "RTC/RTP/Codecs/DependencyDescriptor.hpp"
#include "RTC/RTP/Packet.hpp"
#include "test/include/RTC/RTP/rtpCommon.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memcpy(), std::memset()
#include <vector>

namespace
{
	constexpr uint8_t DependencyDescriptorId{ static_cast<uint8_t>(
		RTC::RtpHeaderExtensionUri::Type::DEPENDENCY_DESCRIPTOR) };

	/**
	 * Creates a packet carrying the given descriptor in a Dependency Descriptor
	 * RTP header extension `reservedLen` bytes long, which is the room the
	 * Producer makes in it for the Consumers to rewrite the descriptor.
	 */
	std::unique_ptr<RTC::RTP::Packet> createPacket(const uint8_t* data, uint8_t len, uint8_t reservedLen)
	{
		std::unique_ptr<RTC::RTP::Packet> packet{ RTC::RTP::Packet::Factory(
			rtpCommon::FactoryBuffer, sizeof(rtpCommon::FactoryBuffer)) };

		REQUIRE(packet);

		uint8_t value[RTC::Consts::TwoBytesRtpExtensionMaxLength]{};

		std::memcpy(value, data, len);

		const std::vector<RTC::RTP::Packet::Extension> extensions{
			{ RTC::RtpHeaderExtensionUri::Type::DEPENDENCY_DESCRIPTOR, DependencyDescriptorId, reservedLen, value }
		};

		packet->SetExtensions(RTC::RTP::Packet::ExtensionsType::Auto, extensions);

		// As the Producer does, tell the length the descriptor really has and
		// leave the room made for it as padding behind it.
		REQUIRE(packet->UpdateDependencyDescriptor(data, len));

		return packet;
	}

	// The Dependency Descriptor the packet currently carries.
	std::vector<uint8_t> getDependencyDescriptor(const RTC::RTP::Packet* packet)
	{
		uint8_t len;
		const uint8_t* value = packet->GetExtensionValue(DependencyDescriptorId, len);

		REQUIRE(value);

		return { value, value + len };
	}
} // namespace

SCENARIO("Dependency Descriptor", "[rtp][codecs][dependency-descriptor]")
{
	SECTION("parse")
	{
		/**
		 * Taken from https://issues.webrtc.org/issues/42225660.
		 *
		 * {
		 *     "startOfFrame" : true,
		 *     "endOfFrame" : false,
		 *     "frameDependencyTemplateId" : 0,
		 *     "frameNumber" : 303,
		 *     "templateStructure": {
		 *         "templateIdOffset" : 0,
		 *         "templateInfo" : {
		 *           "0" : {
		 *             "spatialId" : 0,
		 *             "temporalId" : 0,
		 *             "dti" : [ "SWITCH", "SWITCH", "SWITCH" ],
		 *             "fdiff" : [],
		 *               "chains" : [0]
		 *           },
		 *           "1" : {
		 *             "spatialId" : 0,
		 *             "temporalId" : 0,
		 *             "dti" : [ "SWITCH", "SWITCH", "SWITCH" ],
		 *             "fdiff" : [4],
		 *             "chains" : [4]
		 *           },
		 *           "2" : {
		 *             "spatialId" : 0,
		 *             "temporalId" : 1,
		 *             "dti" : [ "NOT_PRESENT", "DISCARDABLE", "SWITCH" ],
		 *             "fdiff" : [2],
		 *             "chains" : [2]
		 *           },
		 *           "3" : {
		 *             "spatialId" : 0,
		 *             "temporalId" : 2,
		 *             "dti" : [ "NOT_PRESENT", "NOT_PRESENT", "DISCARDABLE" ],
		 *             "fdiff" : [1],
		 *               "chains" : [1]
		 *           },
		 *           "4" : {
		 *             "spatialId" : 0,
		 *             "temporalId" : 2,
		 *             "dti" : [ "NOT_PRESENT", "NOT_PRESENT", "DISCARDABLE" ],
		 *             "fdiff" : [1],
		 *             "chains" : [3]
		 *           }
		 *         },
		 *         "decodeTargetInfo" : {
		 *           "0" : { "protectedBy" : 0, "spatialId" : 0, "temporalId" : 0 },
		 *           "1" : { "protectedBy" : 0, "spatialId" : 0, "temporalId" : 1 },
		 *           "2" : { "protectedBy" : 0, "spatialId" : 0, "temporalId" : 2 }
		 *         },
		 *         "maxSpatialId" : 0,
		 *         "maxTemporalId" : 2
		 *         }
		 * }
		 */

		// clang-format off
		uint8_t data[] =
		{
			0x80, 0x01, 0x2F, 0x80, 0x02, 0x14, 0xEA, 0xA8,
			0x60, 0x41, 0x4D, 0x14, 0x10, 0x20, 0x84, 0x26
		};

		std::shared_ptr<RTC::RTP::Codecs::DependencyDescriptor::TemplateDependencyStructure>
		  templateDependencyStructure;
		auto dependencyDescriptor = std::unique_ptr<RTC::RTP::Codecs::DependencyDescriptor>(
		  RTC::RTP::Codecs::DependencyDescriptor::Parse(
		    data, sizeof(data), templateDependencyStructure));

		REQUIRE(dependencyDescriptor);
		REQUIRE(dependencyDescriptor->startOfFrame == true);
		REQUIRE(dependencyDescriptor->endOfFrame == false);
		REQUIRE(dependencyDescriptor->frameDependencyTemplateId == 0);
		REQUIRE(dependencyDescriptor->frameNumber == 303);

		const auto& templateStructure = dependencyDescriptor->templateDependencyStructure;
		std::vector<RTC::RTP::Codecs::DependencyDescriptor::DecodeTargetIndication> dtis{};
		std::vector<uint8_t> fdiffs{};
		std::vector<uint8_t> fdiffChains{};

		REQUIRE(templateStructure->templateLayers.size() == 5);
		REQUIRE(templateStructure->templateLayers[0].spatialLayer == 0);
		REQUIRE(templateStructure->templateLayers[0].temporalLayer == 0);
		dtis = {
			RTC::RTP::Codecs::DependencyDescriptor::DecodeTargetIndication::SWITCH,
			RTC::RTP::Codecs::DependencyDescriptor::DecodeTargetIndication::SWITCH,
			RTC::RTP::Codecs::DependencyDescriptor::DecodeTargetIndication::SWITCH,
		};
		REQUIRE(templateStructure->templateLayers[0].decodeTargetIndications == dtis);
		fdiffs = {};
		REQUIRE(templateStructure->templateLayers[0].frameDiffs == fdiffs);
		fdiffChains = { 0 };
		REQUIRE(templateStructure->templateLayers[0].frameDiffChains == fdiffChains);

		REQUIRE(templateStructure->templateLayers[1].spatialLayer == 0);
		REQUIRE(templateStructure->templateLayers[1].temporalLayer == 0);
		dtis = {
			RTC::RTP::Codecs::DependencyDescriptor::DecodeTargetIndication::SWITCH,
			RTC::RTP::Codecs::DependencyDescriptor::DecodeTargetIndication::SWITCH,
			RTC::RTP::Codecs::DependencyDescriptor::DecodeTargetIndication::SWITCH,
		};
		REQUIRE(templateStructure->templateLayers[1].decodeTargetIndications == dtis);
		fdiffs = { 4 };
		REQUIRE(templateStructure->templateLayers[1].frameDiffs == fdiffs);
		fdiffChains = { 4 };
		REQUIRE(templateStructure->templateLayers[1].frameDiffChains == fdiffChains);

		REQUIRE(templateStructure->templateLayers[2].spatialLayer == 0);
		REQUIRE(templateStructure->templateLayers[2].temporalLayer == 1);
		dtis = {
			RTC::RTP::Codecs::DependencyDescriptor::DecodeTargetIndication::NOT_PRESENT,
			RTC::RTP::Codecs::DependencyDescriptor::DecodeTargetIndication::DISCARDABLE,
			RTC::RTP::Codecs::DependencyDescriptor::DecodeTargetIndication::SWITCH,
		};
		REQUIRE(templateStructure->templateLayers[2].decodeTargetIndications == dtis);
		fdiffs = { 2 };
		REQUIRE(templateStructure->templateLayers[2].frameDiffs == fdiffs);
		fdiffChains = { 2 };
		REQUIRE(templateStructure->templateLayers[2].frameDiffChains == fdiffChains);

		REQUIRE(templateStructure->templateLayers[3].spatialLayer == 0);
		REQUIRE(templateStructure->templateLayers[3].temporalLayer == 2);
		dtis = {
			RTC::RTP::Codecs::DependencyDescriptor::DecodeTargetIndication::NOT_PRESENT,
			RTC::RTP::Codecs::DependencyDescriptor::DecodeTargetIndication::NOT_PRESENT,
			RTC::RTP::Codecs::DependencyDescriptor::DecodeTargetIndication::DISCARDABLE,
		};
		REQUIRE(templateStructure->templateLayers[3].decodeTargetIndications == dtis);
		fdiffs = { 1 };
		REQUIRE(templateStructure->templateLayers[3].frameDiffs == fdiffs);
		fdiffChains = { 1 };
		REQUIRE(templateStructure->templateLayers[3].frameDiffChains == fdiffChains);

		REQUIRE(templateStructure->templateLayers[4].spatialLayer == 0);
		REQUIRE(templateStructure->templateLayers[4].temporalLayer == 2);
		dtis = {
			RTC::RTP::Codecs::DependencyDescriptor::DecodeTargetIndication::NOT_PRESENT,
			RTC::RTP::Codecs::DependencyDescriptor::DecodeTargetIndication::NOT_PRESENT,
			RTC::RTP::Codecs::DependencyDescriptor::DecodeTargetIndication::DISCARDABLE,
		};
		REQUIRE(templateStructure->templateLayers[4].decodeTargetIndications == dtis);
		fdiffs = { 1 };
		REQUIRE(templateStructure->templateLayers[4].frameDiffs == fdiffs);
		fdiffChains = { 3 };
		REQUIRE(templateStructure->templateLayers[4].frameDiffChains == fdiffChains);
	}

	SECTION("serialize")
	{
		/**
		 * <DependencyDescriptor>
		 * 	startOfFrame: true
		 * 	endOfFrame: false
		 * 	frameDependencyTemplateId: 0
		 * 	frameNumber: 232
		 * 	templateId: 0
		 * 	temporalLayer: 0
		 * 	spatialLayer: 0
		 * 	<TemplateDependencyStructure>
		 * 		spatialLayers: 0
		 * 		temporalLayers: 1
		 * 		templateIdOffset: 0
		 * 		decodeTargetCount: 2
		 * 		<TemplateLayers>
		 * 		  <FrameDependencyTemplate>
		 * 		    spatialLayerId: 0
		 * 		    temporalLayerId: 0
		 * 		    <DecodeTargetIndications> SS </DecodeTargetIndications>
		 * 		    <FrameDiffs>  </FrameDiffs>
		 * 		    <FrameDiffChains> 0 </FrameDiffChains>
		 * 		  <FrameDependencyTemplate>
		 * 		  <FrameDependencyTemplate>
		 * 		    spatialLayerId: 0
		 * 		    temporalLayerId: 0
		 * 		    <DecodeTargetIndications> SS </DecodeTargetIndications>
		 * 		    <FrameDiffs> 2 </FrameDiffs>
		 * 		    <FrameDiffChains> 2 </FrameDiffChains>
		 * 		  <FrameDependencyTemplate>
		 * 		  <FrameDependencyTemplate>
		 * 		    spatialLayerId: 0
		 * 		    temporalLayerId: 1
		 * 		    <DecodeTargetIndications> -D </DecodeTargetIndications>
		 * 		    <FrameDiffs> 1 </FrameDiffs>
		 * 		    <FrameDiffChains> 1 </FrameDiffChains>
		 * 		  <FrameDependencyTemplate>
		 * 		</TemplateLayers>
		 * 		</TemplateDependencyStructure>
		 * </DependencyDescriptor>
		 */
		// clang-format off
		uint8_t data1[] =
		{
			0x80, 0x00, 0xE8, 0x80,
			0x01, 0x1E, 0xA8, 0x51,
			0x41, 0x01, 0x0C, 0x13,
			0xFC, 0x0B, 0x3C,
		};
		// clang-format on

		std::shared_ptr<RTC::RTP::Codecs::DependencyDescriptor::TemplateDependencyStructure>
		  templateDependencyStructure;
		auto dependencyDescriptor = std::unique_ptr<RTC::RTP::Codecs::DependencyDescriptor>(
		  RTC::RTP::Codecs::DependencyDescriptor::Parse(
		    data1, sizeof(data1), templateDependencyStructure));

		REQUIRE(dependencyDescriptor);
		REQUIRE(dependencyDescriptor->frameNumber == 232);

		// clang-format off
		//  000000 4D 00 D8
		uint8_t data2[] =
		{
			0x00, 0x00, 0xE8,
		};

		// clang-format on
		dependencyDescriptor = std::unique_ptr<RTC::RTP::Codecs::DependencyDescriptor>(
		  RTC::RTP::Codecs::DependencyDescriptor::Parse(
		    data2, sizeof(data2), templateDependencyStructure));

		REQUIRE(dependencyDescriptor);
		REQUIRE(dependencyDescriptor->frameNumber == 232);

		// Only decode targets 0 and 1 are kept, which is all of them here.
		const auto bitmask = dependencyDescriptor->GetDecodeTargetsBitmask(0, 1);

		// activeDecodeTargetsBitmask == 00000011
		REQUIRE(bitmask == 3);

		uint8_t buffer[RTC::Consts::TwoBytesRtpExtensionMaxLength];

		const auto len = dependencyDescriptor->Write(bitmask, buffer);

		REQUIRE(len > 0);

		// The descriptor as received must not have been touched.
		REQUIRE(dependencyDescriptor->frameNumber == 232);

		dependencyDescriptor = std::unique_ptr<RTC::RTP::Codecs::DependencyDescriptor>(
		  RTC::RTP::Codecs::DependencyDescriptor::Parse(buffer, len, templateDependencyStructure));

		REQUIRE(dependencyDescriptor);
		REQUIRE(dependencyDescriptor->frameNumber == 232);
		REQUIRE(dependencyDescriptor->activeDecodeTargetsBitmask == 3);
	}

	/**
	 * A single packet is shared by every Consumer of a Producer, so each of them
	 * rewrites the descriptor, sends the packet and restores it for the next one.
	 *
	 * The RTP header extension holding it is sized once by the Producer, so
	 * restoring it must leave the room the next Consumer needs.
	 */
	SECTION("restoring a rewritten descriptor leaves room for the next Consumer")
	{
		// Key frame carrying the template dependency structure of an L1T2 stream.
		// clang-format off
		uint8_t keyFrameData[] =
		{
			0x80, 0x00, 0xE8, 0x80,
			0x01, 0x1E, 0xA8, 0x51,
			0x41, 0x01, 0x0C, 0x13,
			0xFC, 0x0B, 0x3C,
		};

		// Mandatory fields only, so nothing tells about the active decode targets.
		uint8_t deltaFrameData[] =
		{
			0x00, 0x00, 0xE8,
		};
		// clang-format on

		std::shared_ptr<RTC::RTP::Codecs::DependencyDescriptor::TemplateDependencyStructure>
		  templateDependencyStructure;

		// NOTE: The key frame must be parsed first since it's what carries the
		// template dependency structure the delta frame below needs.
		auto keyFrame = std::unique_ptr<RTC::RTP::Codecs::DependencyDescriptor>(
		  RTC::RTP::Codecs::DependencyDescriptor::Parse(
		    keyFrameData, sizeof(keyFrameData), templateDependencyStructure));

		REQUIRE(keyFrame);

		auto deltaFrame = std::unique_ptr<RTC::RTP::Codecs::DependencyDescriptor>(
		  RTC::RTP::Codecs::DependencyDescriptor::Parse(
		    deltaFrameData, sizeof(deltaFrameData), templateDependencyStructure));

		REQUIRE(deltaFrame);

		// The descriptor grows once the active decode targets bitmask is written
		// into it, which is the room the Producer makes in the extension.
		const auto reservedLength = deltaFrame->GetMaxLength();

		REQUIRE(reservedLength > sizeof(deltaFrameData));

		const auto packet = createPacket(deltaFrameData, sizeof(deltaFrameData), reservedLength);

		// The packet tells the length the descriptor really has.
		REQUIRE(getDependencyDescriptor(packet.get()).size() == sizeof(deltaFrameData));

		// First Consumer, which just wants decode target 0, grows into the room.
		REQUIRE(deltaFrame->UpdateActiveDecodeTargets(0, 0, packet.get()));
		REQUIRE(getDependencyDescriptor(packet.get()).size() == reservedLength);

		// Once it's sent the packet is restored for the next Consumer.
		deltaFrame->Restore(packet.get());

		auto restored = getDependencyDescriptor(packet.get());

		// The descriptor is the received one again, at the length it really has.
		REQUIRE(restored.size() == sizeof(deltaFrameData));
		REQUIRE(std::memcmp(restored.data(), deltaFrameData, sizeof(deltaFrameData)) == 0);

		// And the next Consumer still grows into the room left behind it.
		REQUIRE(deltaFrame->UpdateActiveDecodeTargets(0, 1, packet.get()));
		REQUIRE(getDependencyDescriptor(packet.get()).size() == reservedLength);

		const auto value = getDependencyDescriptor(packet.get());

		// And it's told so rather than being given the descriptor of the first one.
		auto rewritten = std::unique_ptr<RTC::RTP::Codecs::DependencyDescriptor>(
		  RTC::RTP::Codecs::DependencyDescriptor::Parse(
		    value.data(), value.size(), templateDependencyStructure));

		REQUIRE(rewritten);
		REQUIRE(rewritten->activeDecodeTargetsBitmask == 0b11);
	}

	/**
	 * The rewritten descriptor is longer than the received one, so it only fits
	 * if room was made for it in the RTP header extension.
	 */
	SECTION("a descriptor with no room for the bitmask is forwarded as received")
	{
		// clang-format off
		uint8_t keyFrameData[] =
		{
			0x80, 0x00, 0xE8, 0x80,
			0x01, 0x1E, 0xA8, 0x51,
			0x41, 0x01, 0x0C, 0x13,
			0xFC, 0x0B, 0x3C,
		};

		uint8_t deltaFrameData[] =
		{
			0x00, 0x00, 0xE8,
		};
		// clang-format on

		std::shared_ptr<RTC::RTP::Codecs::DependencyDescriptor::TemplateDependencyStructure>
		  templateDependencyStructure;

		auto keyFrame = std::unique_ptr<RTC::RTP::Codecs::DependencyDescriptor>(
		  RTC::RTP::Codecs::DependencyDescriptor::Parse(
		    keyFrameData, sizeof(keyFrameData), templateDependencyStructure));

		REQUIRE(keyFrame);

		auto deltaFrame = std::unique_ptr<RTC::RTP::Codecs::DependencyDescriptor>(
		  RTC::RTP::Codecs::DependencyDescriptor::Parse(
		    deltaFrameData, sizeof(deltaFrameData), templateDependencyStructure));

		REQUIRE(deltaFrame);

		REQUIRE(deltaFrame->GetMaxLength() > sizeof(deltaFrameData));

		// No room was made, so the extension is just as long as it came.
		const auto packet = createPacket(deltaFrameData, sizeof(deltaFrameData), sizeof(deltaFrameData));

		// So the rewritten descriptor doesn't fit and the received one is kept.
		REQUIRE(!deltaFrame->UpdateActiveDecodeTargets(0, 0, packet.get()));
		REQUIRE(getDependencyDescriptor(packet.get()).size() == sizeof(deltaFrameData));

		// And since nothing was written there is nothing to undo either.
		deltaFrame->Restore(packet.get());

		const auto value = getDependencyDescriptor(packet.get());

		REQUIRE(value.size() == sizeof(deltaFrameData));
		REQUIRE(std::memcmp(value.data(), deltaFrameData, sizeof(deltaFrameData)) == 0);
	}

	/**
	 * Descriptors are rewritten long after being parsed, for instance when
	 * retransmitted, and how they are written depends on the template dependency
	 * structure they were parsed with.
	 */
	SECTION("a new template dependency structure leaves the previous one alone")
	{
		// Key frame carrying the template dependency structure of an L1T2 stream.
		// clang-format off
		uint8_t keyFrameData[] =
		{
			0x80, 0x00, 0xE8, 0x80,
			0x01, 0x1E, 0xA8, 0x51,
			0x41, 0x01, 0x0C, 0x13,
			0xFC, 0x0B, 0x3C,
		};
		// clang-format on

		std::shared_ptr<RTC::RTP::Codecs::DependencyDescriptor::TemplateDependencyStructure>
		  templateDependencyStructure;

		auto first = std::unique_ptr<RTC::RTP::Codecs::DependencyDescriptor>(
		  RTC::RTP::Codecs::DependencyDescriptor::Parse(
		    keyFrameData, sizeof(keyFrameData), templateDependencyStructure));

		REQUIRE(first);

		const auto* firstStructure = first->templateDependencyStructure.get();

		REQUIRE(firstStructure == templateDependencyStructure.get());

		// A second key frame comes with its own structure.
		auto second = std::unique_ptr<RTC::RTP::Codecs::DependencyDescriptor>(
		  RTC::RTP::Codecs::DependencyDescriptor::Parse(
		    keyFrameData, sizeof(keyFrameData), templateDependencyStructure));

		REQUIRE(second);

		// Which is a brand new one, told apart from the previous one.
		REQUIRE(second->templateDependencyStructure.get() != firstStructure);
		REQUIRE(templateDependencyStructure.get() == second->templateDependencyStructure.get());

		// And the first descriptor keeps the one it was parsed with alive.
		REQUIRE(first->templateDependencyStructure.get() == firstStructure);
		REQUIRE(first->templateDependencyStructure->decodeTargetCount == 2);
	}

	/**
	 * The custom frame dependency fields are not parsed into any member of the
	 * descriptor, just delimited, so rewriting it must copy them over verbatim.
	 */
	SECTION("custom decode target indications, frame diffs and chains are preserved")
	{
		// Key frame carrying the template dependency structure of an L1T2 stream,
		// which has 2 decode targets and 1 chain.
		// clang-format off
		uint8_t keyFrameData[] =
		{
			0x80, 0x00, 0xE8, 0x80,
			0x01, 0x1E, 0xA8, 0x51,
			0x41, 0x01, 0x0C, 0x13,
			0xFC, 0x0B, 0x3C,
		};
		// clang-format on

		/**
		 * Delta frame of that stream carrying every custom frame dependency field,
		 * which no capture at hand has:
		 *
		 *   start_of_frame:                1
		 *   end_of_frame:                  1
		 *   frame_dependency_template_id:  0
		 *   frame_number:                  232
		 *   extended field flags:          structure:0 bitmask:1 dtis:1 fdiffs:1 chains:1
		 *   active_decode_targets_bitmask: 0b11
		 *   custom dtis:                   switch, required
		 *   custom frame diffs:            a single 4 bit one, 5
		 *   custom chains:                 0xA5
		 *
		 * Which takes 51 bits, so the last 5 are zero padding.
		 */
		// clang-format off
		uint8_t deltaFrameData[] =
		{
			0xC0, 0x00, 0xE8, 0x7F,
			0x6A, 0x94, 0xA0,
		};
		// clang-format on

		std::shared_ptr<RTC::RTP::Codecs::DependencyDescriptor::TemplateDependencyStructure>
		  templateDependencyStructure;

		auto keyFrame = std::unique_ptr<RTC::RTP::Codecs::DependencyDescriptor>(
		  RTC::RTP::Codecs::DependencyDescriptor::Parse(
		    keyFrameData, sizeof(keyFrameData), templateDependencyStructure));

		REQUIRE(keyFrame);
		REQUIRE(templateDependencyStructure->decodeTargetCount == 2);
		REQUIRE(templateDependencyStructure->chainCount == 1);

		auto deltaFrame = std::unique_ptr<RTC::RTP::Codecs::DependencyDescriptor>(
		  RTC::RTP::Codecs::DependencyDescriptor::Parse(
		    deltaFrameData, sizeof(deltaFrameData), templateDependencyStructure));

		REQUIRE(deltaFrame);
		REQUIRE(deltaFrame->frameNumber == 232);
		REQUIRE(deltaFrame->activeDecodeTargetsBitmask == 0b11);

		// Nothing grows, since the bitmask is already in there.
		REQUIRE(deltaFrame->GetMaxLength() == sizeof(deltaFrameData));

		uint8_t buffer[RTC::Consts::TwoBytesRtpExtensionMaxLength];

		// Writing the very bitmask it came with must give the very same descriptor.
		auto len = deltaFrame->Write(0b11, buffer);

		REQUIRE(len == sizeof(deltaFrameData));
		REQUIRE(std::memcmp(buffer, deltaFrameData, len) == 0);

		/*
		 * And dropping a decode target must only change the bitmask, which lives
		 * in the two bits that tell both descriptors apart.
		 */
		// clang-format off
		uint8_t rewrittenData[] =
		{
			0xC0, 0x00, 0xE8, 0x7B,
			0x6A, 0x94, 0xA0,
		};
		// clang-format on

		len = deltaFrame->Write(0b01, buffer);

		REQUIRE(len == sizeof(rewrittenData));
		REQUIRE(std::memcmp(buffer, rewrittenData, len) == 0);

		// The rewritten descriptor is still a valid one.
		auto rewritten = std::unique_ptr<RTC::RTP::Codecs::DependencyDescriptor>(
		  RTC::RTP::Codecs::DependencyDescriptor::Parse(buffer, len, templateDependencyStructure));

		REQUIRE(rewritten);
		REQUIRE(rewritten->frameNumber == 232);
		REQUIRE(rewritten->activeDecodeTargetsBitmask == 0b01);
	}

	SECTION("rewriting a key frame keeps the template dependency structure")
	{
		// Key frame carrying the template dependency structure of an L1T2 stream.
		// clang-format off
		uint8_t keyFrameData[] =
		{
			0x80, 0x00, 0xE8, 0x80,
			0x01, 0x1E, 0xA8, 0x51,
			0x41, 0x01, 0x0C, 0x13,
			0xFC, 0x0B, 0x3C,
		};
		// clang-format on

		std::shared_ptr<RTC::RTP::Codecs::DependencyDescriptor::TemplateDependencyStructure>
		  templateDependencyStructure;

		auto keyFrame = std::unique_ptr<RTC::RTP::Codecs::DependencyDescriptor>(
		  RTC::RTP::Codecs::DependencyDescriptor::Parse(
		    keyFrameData, sizeof(keyFrameData), templateDependencyStructure));

		REQUIRE(keyFrame);
		REQUIRE(keyFrame->isKeyFrame);

		const auto structure = keyFrame->templateDependencyStructure;

		uint8_t buffer[RTC::Consts::TwoBytesRtpExtensionMaxLength];

		// Only decode target 0 is kept.
		const auto len = keyFrame->Write(0b01, buffer);

		REQUIRE(len != 0);
		REQUIRE(len == keyFrame->GetMaxLength());

		/*
		 * The rewritten key frame must carry the very same structure, so that the
		 * endpoint goes on resolving the templates of the frames that follow.
		 */
		std::shared_ptr<RTC::RTP::Codecs::DependencyDescriptor::TemplateDependencyStructure> rewrittenStructure;

		auto rewritten = std::unique_ptr<RTC::RTP::Codecs::DependencyDescriptor>(
		  RTC::RTP::Codecs::DependencyDescriptor::Parse(buffer, len, rewrittenStructure));

		REQUIRE(rewritten);
		REQUIRE(rewritten->isKeyFrame);
		REQUIRE(rewritten->frameNumber == keyFrame->frameNumber);
		REQUIRE(rewritten->frameDependencyTemplateId == keyFrame->frameDependencyTemplateId);
		REQUIRE(rewritten->activeDecodeTargetsBitmask == 0b01);
		REQUIRE(rewritten->decodeTargetProtectedBy == keyFrame->decodeTargetProtectedBy);

		REQUIRE(rewrittenStructure->spatialLayers == structure->spatialLayers);
		REQUIRE(rewrittenStructure->temporalLayers == structure->temporalLayers);
		REQUIRE(rewrittenStructure->templateIdOffset == structure->templateIdOffset);
		REQUIRE(rewrittenStructure->decodeTargetCount == structure->decodeTargetCount);
		REQUIRE(rewrittenStructure->chainCount == structure->chainCount);
		REQUIRE(rewrittenStructure->templateLayers.size() == structure->templateLayers.size());

		for (size_t idx{ 0 }; idx < structure->templateLayers.size(); ++idx)
		{
			const auto& templateLayer          = structure->templateLayers[idx];
			const auto& rewrittenTemplateLayer = rewrittenStructure->templateLayers[idx];

			REQUIRE(rewrittenTemplateLayer.spatialLayer == templateLayer.spatialLayer);
			REQUIRE(rewrittenTemplateLayer.temporalLayer == templateLayer.temporalLayer);
			REQUIRE(rewrittenTemplateLayer.decodeTargetIndications == templateLayer.decodeTargetIndications);
			REQUIRE(rewrittenTemplateLayer.frameDiffs == templateLayer.frameDiffs);
			REQUIRE(rewrittenTemplateLayer.frameDiffChains == templateLayer.frameDiffChains);
		}
	}
}
