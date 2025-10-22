#include "common.hpp"
#include "RTC/Codecs/DependencyDescriptor.hpp"
#include <catch2/catch_test_macros.hpp>

using namespace RTC;

class Listener : public ::RTC::Codecs::DependencyDescriptor::Listener
{
public:
	void OnDependencyDescriptorUpdated(const uint8_t* data, size_t len) override
	{
	}
};

SCENARIO("parse Dependency Descriptor", "[codecs][DD]")
{
	SECTION("parse Dependency Descriptor")
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

		Listener listener;

		// clang-format on
		std::unique_ptr<Codecs::DependencyDescriptor::TemplateDependencyStructure> templateDependencyStructure;
		auto dependencyDescriptor =
		  std::unique_ptr<Codecs::DependencyDescriptor>(Codecs::DependencyDescriptor::Parse(
		    data, sizeof(data), std::addressof(listener), templateDependencyStructure));

		REQUIRE(dependencyDescriptor);
		REQUIRE(dependencyDescriptor->startOfFrame == true);
		REQUIRE(dependencyDescriptor->endOfFrame == false);
		REQUIRE(dependencyDescriptor->frameDependencyTemplateId == 0);
		REQUIRE(dependencyDescriptor->frameNumber == 303);

		auto* templateStructure = dependencyDescriptor->templateDependencyStructure;
		std::vector<Codecs::DependencyDescriptor::DecodeTargetIndication> dtis{};
		std::vector<uint8_t> fdiffs{};
		std::vector<uint8_t> fdiffChains{};

		REQUIRE(templateStructure->templateLayers.size() == 5);
		REQUIRE(templateStructure->templateLayers[0].spatialLayer == 0);
		REQUIRE(templateStructure->templateLayers[0].temporalLayer == 0);
		dtis = {
			Codecs::DependencyDescriptor::DecodeTargetIndication::SWITCH,
			Codecs::DependencyDescriptor::DecodeTargetIndication::SWITCH,
			Codecs::DependencyDescriptor::DecodeTargetIndication::SWITCH,
		};
		REQUIRE(templateStructure->templateLayers[0].decodeTargetIndications == dtis);
		fdiffs = {};
		REQUIRE(templateStructure->templateLayers[0].frameDiffs == fdiffs);
		fdiffChains = { 0 };
		REQUIRE(templateStructure->templateLayers[0].frameDiffChains == fdiffChains);

		REQUIRE(templateStructure->templateLayers[1].spatialLayer == 0);
		REQUIRE(templateStructure->templateLayers[1].temporalLayer == 0);
		dtis = {
			Codecs::DependencyDescriptor::DecodeTargetIndication::SWITCH,
			Codecs::DependencyDescriptor::DecodeTargetIndication::SWITCH,
			Codecs::DependencyDescriptor::DecodeTargetIndication::SWITCH,
		};
		REQUIRE(templateStructure->templateLayers[1].decodeTargetIndications == dtis);
		fdiffs = { 4 };
		REQUIRE(templateStructure->templateLayers[1].frameDiffs == fdiffs);
		fdiffChains = { 4 };
		REQUIRE(templateStructure->templateLayers[1].frameDiffChains == fdiffChains);

		REQUIRE(templateStructure->templateLayers[2].spatialLayer == 0);
		REQUIRE(templateStructure->templateLayers[2].temporalLayer == 1);
		dtis = {
			Codecs::DependencyDescriptor::DecodeTargetIndication::NOT_PRESENT,
			Codecs::DependencyDescriptor::DecodeTargetIndication::DISCARDABLE,
			Codecs::DependencyDescriptor::DecodeTargetIndication::SWITCH,
		};
		REQUIRE(templateStructure->templateLayers[2].decodeTargetIndications == dtis);
		fdiffs = { 2 };
		REQUIRE(templateStructure->templateLayers[2].frameDiffs == fdiffs);
		fdiffChains = { 2 };
		REQUIRE(templateStructure->templateLayers[2].frameDiffChains == fdiffChains);

		REQUIRE(templateStructure->templateLayers[3].spatialLayer == 0);
		REQUIRE(templateStructure->templateLayers[3].temporalLayer == 2);
		dtis = {
			Codecs::DependencyDescriptor::DecodeTargetIndication::NOT_PRESENT,
			Codecs::DependencyDescriptor::DecodeTargetIndication::NOT_PRESENT,
			Codecs::DependencyDescriptor::DecodeTargetIndication::DISCARDABLE,
		};
		REQUIRE(templateStructure->templateLayers[3].decodeTargetIndications == dtis);
		fdiffs = { 1 };
		REQUIRE(templateStructure->templateLayers[3].frameDiffs == fdiffs);
		fdiffChains = { 1 };
		REQUIRE(templateStructure->templateLayers[3].frameDiffChains == fdiffChains);

		REQUIRE(templateStructure->templateLayers[4].spatialLayer == 0);
		REQUIRE(templateStructure->templateLayers[4].temporalLayer == 2);
		dtis = {
			Codecs::DependencyDescriptor::DecodeTargetIndication::NOT_PRESENT,
			Codecs::DependencyDescriptor::DecodeTargetIndication::NOT_PRESENT,
			Codecs::DependencyDescriptor::DecodeTargetIndication::DISCARDABLE,
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

		Listener listener;

		std::unique_ptr<Codecs::DependencyDescriptor::TemplateDependencyStructure> templateDependencyStructure;
		auto dependencyDescriptor =
		  std::unique_ptr<Codecs::DependencyDescriptor>(Codecs::DependencyDescriptor::Parse(
		    data1, sizeof(data1), std::addressof(listener), templateDependencyStructure));

		REQUIRE(dependencyDescriptor);
		REQUIRE(dependencyDescriptor->frameNumber == 232);

		// clang-format off
		//  000000 4D 00 D8
		uint8_t data2[] =
		{
			0x00, 0x00, 0xE8,
		};

		// clang-format on
		dependencyDescriptor =
		  std::unique_ptr<Codecs::DependencyDescriptor>(Codecs::DependencyDescriptor::Parse(
		    data2, sizeof(data2), std::addressof(listener), templateDependencyStructure));

		REQUIRE(dependencyDescriptor);
		REQUIRE(dependencyDescriptor->frameNumber == 232);

		uint8_t len;

		auto data = dependencyDescriptor->Serialize(len);

		// clang-format on
		dependencyDescriptor =
		  std::unique_ptr<Codecs::DependencyDescriptor>(Codecs::DependencyDescriptor::Parse(
		    data, sizeof(data), std::addressof(listener), templateDependencyStructure));

		REQUIRE(dependencyDescriptor);
		REQUIRE(dependencyDescriptor->frameNumber == 232);

		dependencyDescriptor->UpdateActiveDecodeTargets(0, 1);

		data = dependencyDescriptor->Serialize(len);

		// clang-format on
		dependencyDescriptor =
		  std::unique_ptr<Codecs::DependencyDescriptor>(Codecs::DependencyDescriptor::Parse(
		    data, sizeof(data), std::addressof(listener), templateDependencyStructure));

		REQUIRE(dependencyDescriptor);
		REQUIRE(dependencyDescriptor->frameNumber == 232);
		// activeDecodeTargetsBitmask == 00000011
		REQUIRE(dependencyDescriptor->activeDecodeTargetsBitmask == 3);
	}
}
