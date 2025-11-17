#include "common.hpp"
#include "Utils.hpp" // BitStream.
#include <cstdint>
#include <optional>

namespace RTC
{
	namespace Codecs
	{
		struct DependencyDescriptor
		{
			class Listener
			{
			public:
				virtual ~Listener() = default;

			public:
				virtual void OnDependencyDescriptorUpdated(const uint8_t* data, size_t len) = 0;
			};

		public:
			enum class DecodeTargetIndication : uint8_t
			{
				NOT_PRESENT = 0,
				DISCARDABLE = 1,
				SWITCH      = 2,
				REQUIRED    = 3
			};

		private:
			static std::unordered_map<DecodeTargetIndication, std::string> dtiToString;

		private:
			struct FrameDependencyTemplate
			{
				FrameDependencyTemplate(uint32_t spatialLayer, uint32_t temporalLayer)
				  : spatialLayer(spatialLayer), temporalLayer(temporalLayer)
				{
				}

				uint32_t spatialLayer;
				uint32_t temporalLayer;
				std::vector<DecodeTargetIndication> decodeTargetIndications;
				std::vector<uint8_t> frameDiffs;
				std::vector<uint8_t> frameDiffChains;
			};

		public:
			struct TemplateDependencyStructure
			{
				uint32_t spatialLayers{ 0 };
				uint32_t temporalLayers{ 0 };
				uint8_t templateIdOffset{ 0 };
				uint8_t decodeTargetCount{ 0 };
				std::vector<FrameDependencyTemplate> templateLayers;
			};

		public:
			bool startOfFrame{ false };
			bool endOfFrame{ false };
			uint8_t frameDependencyTemplateId{ 0 };
			uint16_t frameNumber{ 0 };
			uint8_t templateId{ 0 };
			// Given by argument.
			TemplateDependencyStructure* templateDependencyStructure;
			std::vector<uint8_t> decodeTargetProtectedBy;
			std::optional<uint32_t> activeDecodeTargetsBitmask{ std::nullopt };
			// Calculated.
			uint8_t temporalLayer{ 0 };
			uint8_t spatialLayer{ 0 };
			// Whether the frame is a key frame. Set to true if the descriptor contains template layers.
			bool isKeyFrame{ false };

		private:
			DependencyDescriptor::Listener* listener{ nullptr };

		public:
			static DependencyDescriptor* Parse(
			  const uint8_t* data,
			  size_t len,
			  DependencyDescriptor::Listener* listener,
			  std::unique_ptr<TemplateDependencyStructure>& templateDependencyStructure);

			DependencyDescriptor(
			  const uint8_t* data,
			  size_t len,
			  DependencyDescriptor::Listener* listener,
			  TemplateDependencyStructure* templateDependencyStructure);

			void Dump(int indentation = 0) const;
			void UpdateListener(DependencyDescriptor::Listener* listener);
			const uint8_t* Serialize(uint8_t& len);
			bool UpdateActiveDecodeTargets(uint32_t maxSpatialLayer, uint32_t maxTemporalLayer);

		private:
			uint8_t GetSpatialLayer() const;
			uint8_t GetTemporalLayer() const;

			bool ReadMandatoryDescriptorFields();
			bool ReadExtendedDescriptorFields();
			bool ReadTemplateDependencyStructure();
			bool ReadTemplateLayers();
			bool ReadTemplateDecodeTargetIndications();
			bool ReadTemplateFrameDiffs();
			bool ReadTemplateFrameDiffChains();
			bool ReadFrameDependencyDefinition();
			bool WriteMandatoryDescriptorFields();
			bool WriteExtendedDescriptorFields();

		private:
			Utils::BitStream bitStream;
		};

	} // namespace Codecs
} // namespace RTC
