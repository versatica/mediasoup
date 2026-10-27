#include "common.hpp"
#include "RTC/RTP/Codecs/DependencyDescriptor.hpp"
#include <catch2/catch_test_macros.hpp>

SCENARIO("Dependency Descriptor", "[rtp][codecs][dependency-descriptor]")
{
	class Listener : public RTC::RTP::Codecs::DependencyDescriptor::Listener
	{
	public:
		bool OnDependencyDescriptorUpdated(const uint8_t* data, size_t len) override
		{
			return true;
		}
	};

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

		Listener listener;

		// clang-format on
		std::shared_ptr<RTC::RTP::Codecs::DependencyDescriptor::TemplateDependencyStructure>
		  templateDependencyStructure;
		auto dependencyDescriptor = std::unique_ptr<RTC::RTP::Codecs::DependencyDescriptor>(
		  RTC::RTP::Codecs::DependencyDescriptor::Parse(
		    data, sizeof(data), std::addressof(listener), templateDependencyStructure));

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

		Listener listener;

		std::shared_ptr<RTC::RTP::Codecs::DependencyDescriptor::TemplateDependencyStructure>
		  templateDependencyStructure;
		auto dependencyDescriptor = std::unique_ptr<RTC::RTP::Codecs::DependencyDescriptor>(
		  RTC::RTP::Codecs::DependencyDescriptor::Parse(
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
		dependencyDescriptor = std::unique_ptr<RTC::RTP::Codecs::DependencyDescriptor>(
		  RTC::RTP::Codecs::DependencyDescriptor::Parse(
		    data2, sizeof(data2), std::addressof(listener), templateDependencyStructure));

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
		  RTC::RTP::Codecs::DependencyDescriptor::Parse(
		    buffer, len, std::addressof(listener), templateDependencyStructure));

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
		/**
		 * Models the Dependency Descriptor RTP header extension, which only
		 * accepts a descriptor that fits in its current length, just like
		 * `Packet::UpdateDependencyDescriptor()` does.
		 */
		class ExtensionListener : public RTC::RTP::Codecs::DependencyDescriptor::Listener
		{
		public:
			bool OnDependencyDescriptorUpdated(const uint8_t* data, size_t len) override
			{
				if (len > this->length)
				{
					this->rejected = true;

					return false;
				}

				std::memcpy(this->value, data, len);

				this->length = len;

				return true;
			}

			uint8_t value[RTC::Consts::TwoBytesRtpExtensionMaxLength]{};
			uint8_t length{ 0 };
			bool rejected{ false };
		};

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

		ExtensionListener listener;

		std::shared_ptr<RTC::RTP::Codecs::DependencyDescriptor::TemplateDependencyStructure>
		  templateDependencyStructure;

		// NOTE: The key frame must be parsed first since it's what carries the
		// template dependency structure the delta frame below needs.
		auto keyFrame = std::unique_ptr<RTC::RTP::Codecs::DependencyDescriptor>(
		  RTC::RTP::Codecs::DependencyDescriptor::Parse(
		    keyFrameData, sizeof(keyFrameData), std::addressof(listener), templateDependencyStructure));

		REQUIRE(keyFrame);

		auto deltaFrame = std::unique_ptr<RTC::RTP::Codecs::DependencyDescriptor>(
		  RTC::RTP::Codecs::DependencyDescriptor::Parse(
		    deltaFrameData, sizeof(deltaFrameData), std::addressof(listener), templateDependencyStructure));

		REQUIRE(deltaFrame);

		// The descriptor grows once the active decode targets bitmask is written
		// into it, which is the room the Producer makes in the extension.
		const auto reservedLength = deltaFrame->GetMaxLength();

		REQUIRE(reservedLength > sizeof(deltaFrameData));

		listener.length = reservedLength;

		// First Consumer, which just wants decode target 0.
		REQUIRE(deltaFrame->UpdateActiveDecodeTargets(0, 0));
		REQUIRE(listener.length == reservedLength);

		// Once it's sent the packet is restored for the next Consumer.
		deltaFrame->Restore();

		// The descriptor is the received one again.
		REQUIRE(std::memcmp(listener.value, deltaFrameData, sizeof(deltaFrameData)) == 0);

		// But the extension keeps its length, so the next Consumer still fits in it.
		REQUIRE(listener.length == reservedLength);

		// Second Consumer, which wants every decode target.
		REQUIRE(deltaFrame->UpdateActiveDecodeTargets(0, 1));
		REQUIRE(!listener.rejected);

		// And it's told so rather than being given the descriptor of the first one.
		auto rewritten = std::unique_ptr<RTC::RTP::Codecs::DependencyDescriptor>(
		  RTC::RTP::Codecs::DependencyDescriptor::Parse(
		    listener.value, listener.length, std::addressof(listener), templateDependencyStructure));

		REQUIRE(rewritten);
		REQUIRE(rewritten->activeDecodeTargetsBitmask == 0b11);
	}

	/**
	 * The rewritten descriptor is longer than the received one, so it only fits
	 * if room was made for it in the RTP header extension.
	 */
	SECTION("a descriptor with no room for the bitmask is forwarded as received")
	{
		class ExtensionListener : public RTC::RTP::Codecs::DependencyDescriptor::Listener
		{
		public:
			bool OnDependencyDescriptorUpdated(const uint8_t* data, size_t len) override
			{
				if (len > this->length)
				{
					this->rejected = true;

					return false;
				}

				std::memcpy(this->value, data, len);

				this->length = len;

				return true;
			}

			uint8_t value[RTC::Consts::TwoBytesRtpExtensionMaxLength]{};
			uint8_t length{ 0 };
			bool rejected{ false };
		};

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

		ExtensionListener listener;

		std::shared_ptr<RTC::RTP::Codecs::DependencyDescriptor::TemplateDependencyStructure>
		  templateDependencyStructure;

		auto keyFrame = std::unique_ptr<RTC::RTP::Codecs::DependencyDescriptor>(
		  RTC::RTP::Codecs::DependencyDescriptor::Parse(
		    keyFrameData, sizeof(keyFrameData), std::addressof(listener), templateDependencyStructure));

		REQUIRE(keyFrame);

		auto deltaFrame = std::unique_ptr<RTC::RTP::Codecs::DependencyDescriptor>(
		  RTC::RTP::Codecs::DependencyDescriptor::Parse(
		    deltaFrameData, sizeof(deltaFrameData), std::addressof(listener), templateDependencyStructure));

		REQUIRE(deltaFrame);

		// No room was made, so the extension is just as long as it came.
		listener.length = sizeof(deltaFrameData);

		REQUIRE(deltaFrame->GetMaxLength() > sizeof(deltaFrameData));

		// So the rewritten descriptor doesn't fit and the received one is kept.
		REQUIRE(!deltaFrame->UpdateActiveDecodeTargets(0, 0));
		REQUIRE(listener.rejected);
		REQUIRE(listener.length == sizeof(deltaFrameData));

		// And since nothing was written there is nothing to undo either.
		listener.rejected = false;

		deltaFrame->Restore();

		REQUIRE(!listener.rejected);
		REQUIRE(listener.length == sizeof(deltaFrameData));
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

		Listener listener;

		std::shared_ptr<RTC::RTP::Codecs::DependencyDescriptor::TemplateDependencyStructure>
		  templateDependencyStructure;

		auto first = std::unique_ptr<RTC::RTP::Codecs::DependencyDescriptor>(
		  RTC::RTP::Codecs::DependencyDescriptor::Parse(
		    keyFrameData, sizeof(keyFrameData), std::addressof(listener), templateDependencyStructure));

		REQUIRE(first);

		const auto* firstStructure = first->templateDependencyStructure.get();

		REQUIRE(firstStructure == templateDependencyStructure.get());

		// A second key frame comes with its own structure.
		auto second = std::unique_ptr<RTC::RTP::Codecs::DependencyDescriptor>(
		  RTC::RTP::Codecs::DependencyDescriptor::Parse(
		    keyFrameData, sizeof(keyFrameData), std::addressof(listener), templateDependencyStructure));

		REQUIRE(second);

		// Which is a brand new one, told apart from the previous one.
		REQUIRE(second->templateDependencyStructure.get() != firstStructure);
		REQUIRE(templateDependencyStructure.get() == second->templateDependencyStructure.get());

		// And the first descriptor keeps the one it was parsed with alive.
		REQUIRE(first->templateDependencyStructure.get() == firstStructure);
		REQUIRE(first->templateDependencyStructure->decodeTargetCount == 2);
	}
}
