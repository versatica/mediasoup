#define MS_CLASS "RTC::Codecs::DependencyDescriptor"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/Codecs/DependencyDescriptor.hpp"
#include "Logger.hpp"
#include <vector>

namespace RTC
{
	namespace Codecs
	{
		/* Class methods. */

		DependencyDescriptor* DependencyDescriptor::Parse(
		  const uint8_t* data,
		  size_t len,
		  std::unique_ptr<TemplateDependencyStructure>& templateDependencyStructure)
		{
			MS_TRACE();

			if (len < 3)
			{
				MS_WARN_DEV("ignoring payload with length < 3");

				return nullptr;
			}

			if (templateDependencyStructure == nullptr)
			{
				templateDependencyStructure = std::make_unique<TemplateDependencyStructure>();
			}

			std::unique_ptr<DependencyDescriptor> dependencyDescriptor(
			  new DependencyDescriptor(data, len, templateDependencyStructure.get()));

			if (!dependencyDescriptor->ReadMandatoryDescriptorFields())
			{
				MS_WARN_DEV("failed to read mandatory fields");

				return nullptr;
			}

			if (len > 3)
			{
				if (!dependencyDescriptor->ReadExtendedDescriptorFields())
				{
					MS_WARN_DEV("failed to read extended fields");

					return nullptr;
				}
			}

			if (!dependencyDescriptor->ReadFrameDependencyDefinition())
			{
				MS_WARN_DEV("failed to read frame dependency definition");

				return nullptr;
			}

			return dependencyDescriptor.release();
		}

		/* Instance methods. */

		DependencyDescriptor::DependencyDescriptor(
		  const uint8_t* data, size_t len, TemplateDependencyStructure* templateDependencyStructure)
		  : templateDependencyStructure(templateDependencyStructure),
		    bitStream(const_cast<uint8_t*>(data), len)
		{
			MS_TRACE();
		}

		uint8_t DependencyDescriptor::GetSpatialLayer() const
		{
			return this->templateDependencyStructure->templateLayers[this->templateId].spatialLayer;
		}

		uint8_t DependencyDescriptor::GetTemporalLayer() const
		{
			return this->templateDependencyStructure->templateLayers[this->templateId].temporalLayer;
		}

		void DependencyDescriptor::Dump(int indentation) const
		{
			MS_TRACE();

			MS_DUMP_CLEAN(indentation, "<DependencyDescriptor>");
			MS_DUMP_CLEAN(indentation, "  startOfFrame: %s", this->startOfFrame ? "true" : "false");
			MS_DUMP_CLEAN(indentation, "  endOfFrame: %s", this->endOfFrame ? "true" : "false");
			MS_DUMP_CLEAN(indentation, "  frameDependencyTemplateId: %u", this->frameDependencyTemplateId);
			MS_DUMP_CLEAN(indentation, "  frameNumber: %u", this->frameNumber);
			MS_DUMP_CLEAN(indentation, "  templateIdOffset: %u", this->templateIdOffset);
			MS_DUMP_CLEAN(indentation, "  templateId: %u", this->templateId);
			MS_DUMP_CLEAN(indentation, "  temporalLayer: %u", this->temporalLayer);
			MS_DUMP_CLEAN(indentation, "  spatialLayer: %u", this->spatialLayer);

			if (this->isKeyFrame)
			{
				MS_DUMP_CLEAN(indentation + 1, "<TemplateDependencyStructure>");
				MS_DUMP_CLEAN(
				  indentation + 1, "  spatialLayers: %u", this->templateDependencyStructure->spatialLayers);
				MS_DUMP_CLEAN(
				  indentation + 1, "  temporalLayers: %u", this->templateDependencyStructure->temporalLayers);
				MS_DUMP_CLEAN(indentation + 2, "<TemplateLayers>");
				for (const auto& layer : this->templateDependencyStructure->templateLayers)
				{
					MS_DUMP_CLEAN(indentation + 3, "<FrameDependencyTemplate>");
					MS_DUMP_CLEAN(indentation + 3, "  spatialLayerId: %u", layer.spatialLayer);
					MS_DUMP_CLEAN(indentation + 3, "  temporalLayerId: %u", layer.temporalLayer);
					MS_DUMP_CLEAN(indentation + 3, "</FrameDependencyTemplate>");
				}
				MS_DUMP_CLEAN(indentation + 2, "</TemplateLayers>");
				MS_DUMP_CLEAN(indentation + 1, "  </TemplateDependencyStructure>");
			}

			MS_DUMP_CLEAN(indentation, "</DependencyDescriptor>");
		}

		bool DependencyDescriptor::ReadMandatoryDescriptorFields()
		{
			MS_TRACE();

			if (this->bitStream.GetLeftBits() < 24)
			{
				return false;
			}

			this->startOfFrame              = this->bitStream.GetBit();
			this->endOfFrame                = this->bitStream.GetBit();
			this->frameDependencyTemplateId = this->bitStream.GetBits(6);
			this->frameNumber               = this->bitStream.GetBits(16);

			return true;
		}

		bool DependencyDescriptor::ReadExtendedDescriptorFields()
		{
			MS_TRACE();

			if (this->bitStream.GetLeftBits() < 5)
			{
				return false;
			}

			auto templateDependencyStructurePresentFlag = this->bitStream.GetBit();

			// Advance 4 positions due to non interesting fields.
			bitStream.SkipBits(4);

			if (templateDependencyStructurePresentFlag)
			{
				if (!ReadTemplateDependencyStructure())
				{
					return false;
				}
			}

			return true;
		}

		bool DependencyDescriptor::ReadTemplateDependencyStructure()
		{
			MS_TRACE();

			if (this->bitStream.GetLeftBits() < 11)
			{
				return false;
			}

			this->templateIdOffset = this->bitStream.GetBits(6);

			this->bitStream.SkipBits(5);

			return ReadTemplateLayers();
		}

		bool DependencyDescriptor::ReadTemplateLayers()
		{
			MS_TRACE();

			uint8_t temporalId    = 0;
			uint8_t spatialId     = 0;
			uint32_t nextLayerIdc = 0;

			this->templateDependencyStructure->templateLayers.clear();

			// Set the key frame flag.
			this->isKeyFrame = true;

			do
			{
				this->templateDependencyStructure->templateLayers.emplace_back(
				  FameDependencyTemplate{ spatialId, temporalId });

				if (this->bitStream.GetLeftBits() < 2)
				{
					return false;
				}

				nextLayerIdc = this->bitStream.GetBits(2);

				// nextLayerIdc == 0, same spatialId and temporalId.
				if (nextLayerIdc == 1)
				{
					temporalId++;
				}
				else if (nextLayerIdc == 2)
				{
					temporalId = 0;
					spatialId++;
				}
			} while (nextLayerIdc != 3);

			this->templateDependencyStructure->spatialLayers  = spatialId;
			this->templateDependencyStructure->temporalLayers = temporalId;

			return true;
		}

		bool DependencyDescriptor::ReadFrameDependencyDefinition()
		{
			MS_TRACE();

			uint8_t templateIndex = (this->frameDependencyTemplateId + 64 - this->templateIdOffset) % 64;

			if (this->templateDependencyStructure->templateLayers.size() <= templateIndex)
			{
				MS_WARN_DEV("invalid template index %u", templateIndex);

				return false;
			}

			this->templateId = templateIndex;

			// Retrieve spatial and temporal layers.
			this->spatialLayer =
			  this->templateDependencyStructure->templateLayers[this->templateId].spatialLayer;
			this->temporalLayer =
			  this->templateDependencyStructure->templateLayers[this->templateId].temporalLayer;

			return true;
		}
	} // namespace Codecs
} // namespace RTC
