#include "common.hpp"
#include "Utils.hpp" // BitStream.
#include <cstdint>

namespace RTC
{
	namespace Codecs
	{
		struct DependencyDescriptor
		{
			struct FameDependencyTemplate
			{
				uint32_t spatialLayer;
				uint32_t temporalLayer;
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
			bool ReadFrameDependencyDefinition();

		private:
			Utils::BitStream bitStream;
		};

	} // namespace Codecs
} // namespace RTC
