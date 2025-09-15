#include "common.hpp"
#include "RTC/Codecs/DependencyDescriptor.hpp"
#include <catch2/catch_test_macros.hpp>

using namespace RTC;

SCENARIO("parse Dependency Descriptor", "[codecs][DD]")
{
	SECTION("parse Dependency Descriptor")
	{
		/**
		 * Taken from https://issues.webrtc.org/issues/42225660.
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

		// clang-format on
		std::unique_ptr<Codecs::DependencyDescriptor::TemplateDependencyStructure> templateDependencyStructure;
		auto dependencyDescriptor = std::unique_ptr<Codecs::DependencyDescriptor>(
		  Codecs::DependencyDescriptor::Parse(data, sizeof(data), templateDependencyStructure));

		REQUIRE(dependencyDescriptor);
		REQUIRE(dependencyDescriptor->startOfFrame == true);
		REQUIRE(dependencyDescriptor->endOfFrame == false);
		REQUIRE(dependencyDescriptor->frameDependencyTemplateId == 0);
		REQUIRE(dependencyDescriptor->frameNumber == 303);

		auto* templateStructure = dependencyDescriptor->templateDependencyStructure;
		std::vector<Codecs::DependencyDescriptor::DecodeTargetIndication> dtis{};

		REQUIRE(templateStructure->templateLayers.size() == 5);
		REQUIRE(templateStructure->templateLayers[0].spatialLayer == 0);
		REQUIRE(templateStructure->templateLayers[0].temporalLayer == 0);
		dtis = {
			Codecs::DependencyDescriptor::DecodeTargetIndication::SWITCH,
			Codecs::DependencyDescriptor::DecodeTargetIndication::SWITCH,
			Codecs::DependencyDescriptor::DecodeTargetIndication::SWITCH,
		};
		REQUIRE(templateStructure->templateLayers[0].decodeTargetIndications == dtis);

		REQUIRE(templateStructure->templateLayers[1].spatialLayer == 0);
		REQUIRE(templateStructure->templateLayers[1].temporalLayer == 0);
		dtis = {
			Codecs::DependencyDescriptor::DecodeTargetIndication::SWITCH,
			Codecs::DependencyDescriptor::DecodeTargetIndication::SWITCH,
			Codecs::DependencyDescriptor::DecodeTargetIndication::SWITCH,
		};
		REQUIRE(templateStructure->templateLayers[1].decodeTargetIndications == dtis);

		REQUIRE(templateStructure->templateLayers[2].spatialLayer == 0);
		REQUIRE(templateStructure->templateLayers[2].temporalLayer == 1);
		dtis = {
			Codecs::DependencyDescriptor::DecodeTargetIndication::NOT_PRESENT,
			Codecs::DependencyDescriptor::DecodeTargetIndication::DISCARDABLE,
			Codecs::DependencyDescriptor::DecodeTargetIndication::SWITCH,
		};
		REQUIRE(templateStructure->templateLayers[2].decodeTargetIndications == dtis);

		REQUIRE(templateStructure->templateLayers[3].spatialLayer == 0);
		REQUIRE(templateStructure->templateLayers[3].temporalLayer == 2);
		dtis = {
			Codecs::DependencyDescriptor::DecodeTargetIndication::NOT_PRESENT,
			Codecs::DependencyDescriptor::DecodeTargetIndication::NOT_PRESENT,
			Codecs::DependencyDescriptor::DecodeTargetIndication::DISCARDABLE,
		};
		REQUIRE(templateStructure->templateLayers[3].decodeTargetIndications == dtis);

		REQUIRE(templateStructure->templateLayers[4].spatialLayer == 0);
		REQUIRE(templateStructure->templateLayers[4].temporalLayer == 2);
		dtis = {
			Codecs::DependencyDescriptor::DecodeTargetIndication::NOT_PRESENT,
			Codecs::DependencyDescriptor::DecodeTargetIndication::NOT_PRESENT,
			Codecs::DependencyDescriptor::DecodeTargetIndication::DISCARDABLE,
		};
		REQUIRE(templateStructure->templateLayers[4].decodeTargetIndications == dtis);
	}

	SECTION("Write Dependency Descriptor Frame Number")
	{
		/**
		 * Taken from https://issues.webrtc.org/issues/42225660.
		 * See previous test.
		 */

		// clang-format off
		uint8_t data[] =
		{
			0x80, 0x01, 0x2F, 0x80, 0x02, 0x14, 0xEA, 0xA8,
			0x60, 0x41, 0x4D, 0x14, 0x10, 0x20, 0x84, 0x26
		};

		// clang-format on
		std::unique_ptr<Codecs::DependencyDescriptor::TemplateDependencyStructure> templateDependencyStructure;
		auto dependencyDescriptor = std::unique_ptr<Codecs::DependencyDescriptor>(
		  Codecs::DependencyDescriptor::Parse(data, sizeof(data), templateDependencyStructure));

		REQUIRE(dependencyDescriptor);
		REQUIRE(dependencyDescriptor->frameNumber == 303);
	}
}
