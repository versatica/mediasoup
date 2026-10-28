#ifndef MS_RTC_RTP_CODECS_DEPENDENCY_DESCRIPTOR_HPP
#define MS_RTC_RTP_CODECS_DEPENDENCY_DESCRIPTOR_HPP

#include "common.hpp"
#include "Utils.hpp" // BitReader.
#include <ankerl/unordered_dense.h>

namespace RTC
{
	namespace RTP
	{
		class Packet;

		namespace Codecs
		{
			struct DependencyDescriptor
			{
			public:
				enum class DecodeTargetIndication : uint8_t
				{
					NOT_PRESENT = 0,
					DISCARDABLE = 1,
					SWITCH      = 2,
					REQUIRED    = 3
				};

			private:
				static ankerl::unordered_dense::map<DecodeTargetIndication, std::string> dtiToString;

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

				// Spatial and temporal layer a decode target corresponds to.
				struct DecodeTargetLayer
				{
					uint8_t spatialLayer{ 0 };
					uint8_t temporalLayer{ 0 };
				};

			public:
				struct TemplateDependencyStructure
				{
					uint32_t spatialLayers{ 0 };
					uint32_t temporalLayers{ 0 };
					uint8_t templateIdOffset{ 0 };
					uint8_t decodeTargetCount{ 0 };
					uint32_t chainCount{ 0 };
					std::vector<FrameDependencyTemplate> templateLayers;
					// Which spatial and temporal layer each decode target corresponds to,
					// derived from the decode target indications of the templates.
					std::vector<DecodeTargetLayer> decodeTargetLayers;
				};

			public:
				bool startOfFrame{ false };
				bool endOfFrame{ false };
				uint8_t frameDependencyTemplateId{ 0 };
				uint16_t frameNumber{ 0 };
				uint8_t templateId{ 0 };
				/*
				 * Given by argument, and replaced by a new one whenever this descriptor
				 * carries a template dependency structure.
				 *
				 * NOTE: Shared so that descriptors parsed with a previous structure (which
				 * may still sit in a retransmission buffer) keep seeing it unchanged.
				 */
				std::shared_ptr<TemplateDependencyStructure> templateDependencyStructure;
				std::vector<uint8_t> decodeTargetProtectedBy;
				std::optional<uint32_t> activeDecodeTargetsBitmask{ std::nullopt };
				// Calculated.
				uint8_t temporalLayer{ 0 };
				uint8_t spatialLayer{ 0 };
				// Whether the frame is a key frame. Set to true if the descriptor contains template layers.
				bool isKeyFrame{ false };

			public:
				static DependencyDescriptor* Parse(
				  const uint8_t* data,
				  size_t len,
				  std::shared_ptr<TemplateDependencyStructure>& templateDependencyStructure);

				DependencyDescriptor(
				  const uint8_t* data,
				  size_t len,
				  std::shared_ptr<TemplateDependencyStructure> templateDependencyStructure);

				void Dump(int indentation = 0) const;
				/*
				 * Rewrites the descriptor of the given packet with the decode targets
				 * that the given maximum layers leave active.
				 *
				 * NOTE: The packet is given on every call rather than remembered, since
				 * a packet shares its payload descriptor handler with its clones and
				 * every Consumer of its Producer.
				 */
				bool UpdateActiveDecodeTargets(
				  uint32_t maxSpatialLayer, uint32_t maxTemporalLayer, RTP::Packet* packet);
				// Writes the descriptor back as it was received, undoing any rewrite.
				void Restore(RTP::Packet* packet);
				/*
				 * Bitmask of the decode targets given the maximum spatial and temporal layers.
				 *
				 * Bit i corresponds to decode target i, so the returned value is what
				 * `active_decode_targets_bitmask` must be set to.
				 */
				uint32_t GetDecodeTargetsBitmask(uint8_t maxSpatialLayer, uint8_t maxTemporalLayer) const;
				/*
				 * Bytes the descriptor takes once rewritten by `Write()`, which may be
				 * more than it came in since the active decode targets bitmask is always
				 * written even if it was not present.
				 *
				 * It's a maximum because when layers are not filtered the descriptor stays
				 * untouched. The Producer uses it to reserve room in the RTP header
				 * extension for whatever its Consumers may write there.
				 */
				uint8_t GetMaxLength() const;
				/*
				 * Writes the descriptor into the given buffer with the given active decode
				 * targets bitmask, and returns how many bytes it took.
				 *
				 * Everything else is copied over from the descriptor as received,
				 * including the template dependency structure and the custom frame
				 * dependency fields, which are not parsed into any field of this class.
				 */
				uint8_t Write(uint32_t activeDecodeTargetsBitmask, uint8_t* buffer) const;
				bool IsSwitchPointForTemporalLayer(uint8_t temporalLayer) const;

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
				bool ReadRenderResolutions();
				bool ReadFrameDependencyDefinition();
				bool ReadFrameDecodeTargetIndications();
				bool ReadFrameDiffs();
				bool ReadFrameDiffChains();
				void ComputeDecodeTargetLayers() const;
				uint32_t GetRewrittenBits() const;

			private:
				Utils::BitReader bitReader;
				// Length of the descriptor as received.
				uint8_t length{ 0 };
				// Whether the descriptor in the packet no longer is the one received.
				bool rewritten{ false };
				/*
				 * Where things are within the descriptor as received, so that the parts
				 * that are not parsed can be copied over verbatim when rewriting.
				 */
				bool extendedDescriptorFieldsPresent{ false };
				bool activeDecodeTargetsPresent{ false };
				bool customDecodeTargetIndicationsPresent{ false };
				bool customFrameDiffsPresent{ false };
				bool customChainsPresent{ false };
				uint32_t templateDependencyStructureEndOffset{ 0 };
				uint32_t frameDependencyDefinitionOffset{ 0 };
				uint32_t frameDependencyDefinitionEndOffset{ 0 };
			};
		} // namespace Codecs
	} // namespace RTP
} // namespace RTC

#endif
