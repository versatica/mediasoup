#include "common.hpp"
#include "Utils.hpp" // BitStream.
#include <cstdint>

namespace RTC
{
	namespace Codecs
	{
		struct DependencyDescriptor
		{
			enum class DecodeTargetIndication : uint8_t
			{
				NOT_PRESENT = 0,
				DISCARDABLE = 1,
				SWITCH      = 2,
				REQUIRED    = 3
			};

			// clang-format off
			static std::unordered_map<DecodeTargetIndication, std::string> DtiToString;

			struct FameDependencyTemplate
			{
				uint32_t spatialLayer;
				uint32_t temporalLayer;
				std::vector<DecodeTargetIndication> decodeTargetIndications;
				std::vector<uint8_t> frameDiffs;
			};

			struct TemplateDependencyStructure
			{
				uint32_t spatialLayers{ 0 };
				uint32_t temporalLayers{ 0 };
				std::vector<FameDependencyTemplate> templateLayers;
			};

			bool startOfFrame{ false };
			bool endOfFrame{ false };
			uint8_t frameDependencyTemplateId{ 0 };
			uint16_t frameNumber{ 0 };
			uint8_t templateIdOffset{ 0 };
			uint8_t templateId{ 0 };
			// Given by argument.
			TemplateDependencyStructure* templateDependencyStructure;
			uint8_t decodeTargetCount{ 0 };
			// Calculated.
			uint8_t temporalLayer{ 0 };
			uint8_t spatialLayer{ 0 };
			// Whether the frame is a key frame. Set to true if the descriptor contains template layers.
			bool isKeyFrame{ false };

			static DependencyDescriptor* Parse(
			  const uint8_t* data,
			  size_t len,
			  std::unique_ptr<TemplateDependencyStructure>& templateDependencyStructure);

			DependencyDescriptor(
			  const uint8_t* data, size_t len, TemplateDependencyStructure* templateDependencyStructure);

			void Dump(int indentation = 0) const;

		private:
			uint8_t GetSpatialLayer() const;
			uint8_t GetTemporalLayer() const;

			bool ReadMandatoryDescriptorFields();
			bool ReadExtendedDescriptorFields();
			bool ReadTemplateDependencyStructure();
			bool ReadTemplateLayers();
			bool ReadTemplateDecodeTargetIndications();
			bool ReadTemplateFrameDiffs();
			bool ReadFrameDependencyDefinition();

		private:
			Utils::BitStream bitStream;
		};

	} // namespace Codecs
} // namespace RTC
