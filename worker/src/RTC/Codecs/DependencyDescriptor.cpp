#define MS_CLASS "RTC::Codecs::DependencyDescriptor"
#define MS_LOG_DEV_LEVEL 3

#include "RTC/Codecs/DependencyDescriptor.hpp"
#include "Logger.hpp"
#include <vector>

namespace RTC
{
	namespace Codecs
	{
		/* Static members. */

		// clang-format off
		std::unordered_map<DependencyDescriptor::DecodeTargetIndication, std::string> DependencyDescriptor::DtiToString =
		{
			{ DependencyDescriptor::DecodeTargetIndication::NOT_PRESENT, "-" },
			{ DependencyDescriptor::DecodeTargetIndication::DISCARDABLE, "D" },
			{ DependencyDescriptor::DecodeTargetIndication::SWITCH,      "S" },
			{ DependencyDescriptor::DecodeTargetIndication::REQUIRED,    "R" },
		};
		// clang-format on

		/* Class methods. */

		DependencyDescriptor* DependencyDescriptor::Parse(
		  const uint8_t* data,
		  size_t len,
		  std::unique_ptr<TemplateDependencyStructure>& templateDependencyStructure)
		{
			MS_TRACE();

			// TODO: Remove.
			MS_DUMP_DATA(data, len);

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

			// TODO: Remove.
			dependencyDescriptor->Dump();

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

			// if (!this->isKeyFrame)
			// {
			// 	return;
			// }

			MS_DUMP_CLEAN(indentation, "<DependencyDescriptor>");
			MS_DUMP_CLEAN(indentation, "  startOfFrame: %s", this->startOfFrame ? "true" : "false");
			MS_DUMP_CLEAN(indentation, "  endOfFrame: %s", this->endOfFrame ? "true" : "false");
			MS_DUMP_CLEAN(indentation, "  frameDependencyTemplateId: %u", this->frameDependencyTemplateId);
			MS_DUMP_CLEAN(indentation, "  frameNumber: %u", this->frameNumber);
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
				MS_DUMP_CLEAN(
				  indentation + 1,
				  "  templateIdOffset: %u",
				  this->templateDependencyStructure->templateIdOffset);
				MS_DUMP_CLEAN(
				  indentation + 1,
				  "  decodeTargetCount: %u",
				  this->templateDependencyStructure->decodeTargetCount);
				MS_DUMP_CLEAN(indentation + 2, "<TemplateLayers>");
				for (const auto& layer : this->templateDependencyStructure->templateLayers)
				{
					MS_DUMP_CLEAN(indentation + 3, "<FrameDependencyTemplate>");
					MS_DUMP_CLEAN(indentation + 3, "  spatialLayerId: %u", layer.spatialLayer);
					MS_DUMP_CLEAN(indentation + 3, "  temporalLayerId: %u", layer.temporalLayer);
					std::string dtis;
					for (const auto& dti : layer.decodeTargetIndications)
					{
						dtis += DtiToString[dti];
					}
					MS_DUMP_CLEAN(
					  indentation + 3,
					  "  <DecodeTargetIndications> %s </DecodeTargetIndications>",
					  dtis.c_str());
					std::string fdiffs;
					for (const auto& fdiff : layer.frameDiffs)
					{
						if (!fdiffs.empty())
						{
							fdiffs += ",";
						}

						fdiffs += std::to_string(fdiff);
					}
					MS_DUMP_CLEAN(indentation + 3, "  <FrameDiffs> %s </FrameDiffs>", fdiffs.c_str());
					std::string fdiffChains;
					for (const auto& fdiffChain : layer.frameDiffChains)
					{
						if (!fdiffChains.empty())
						{
							fdiffChains += ",";
						}

						fdiffChains += std::to_string(fdiffChain);
					}
					MS_DUMP_CLEAN(
					  indentation + 3, "  <FrameDiffChains> %s </FrameDiffChains>", fdiffChains.c_str());
					MS_DUMP_CLEAN(indentation + 3, "<FrameDependencyTemplate>");
				}
				MS_DUMP_CLEAN(indentation + 2, "</TemplateLayers>");
				MS_DUMP_CLEAN(indentation + 1, "  </TemplateDependencyStructure>");
			}

			MS_DUMP_CLEAN(indentation, "</DependencyDescriptor>");
			MS_DUMP_CLEAN(indentation, "<ActiveDecodeTargets>");
			MS_DUMP_CLEAN(
			  indentation + 1, "%s", std::bitset<32>(this->activeDecodeTargetsBitmask).to_string().c_str());
			MS_DUMP_CLEAN(indentation, "</ActiveDecodeTargets>");
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
			auto activeDecodeTargetLayersPresentFlag    = this->bitStream.GetBit();

			// Advance 3 positios due to non interesting fields.
			bitStream.SkipBits(3);

			if (templateDependencyStructurePresentFlag)
			{
				if (!ReadTemplateDependencyStructure())
				{
					return false;
				}

				this->activeDecodeTargetsBitmask =
				  (1 << this->templateDependencyStructure->decodeTargetCount) - 1;
			}

			if (activeDecodeTargetLayersPresentFlag)
			{
				if (this->bitStream.GetLeftBits() < this->templateDependencyStructure->decodeTargetCount)
				{
					return false;
				}

				this->activeDecodeTargetsBitmask =
				  this->bitStream.GetBits(this->templateDependencyStructure->decodeTargetCount);
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

			this->templateDependencyStructure->templateIdOffset  = this->bitStream.GetBits(6);
			this->templateDependencyStructure->decodeTargetCount = this->bitStream.GetBits(5) + 1;

			if (!ReadTemplateLayers())
			{
				return false;
			}

			if (!ReadTemplateDecodeTargetIndications())
			{
				return false;
			}

			if (!ReadTemplateFrameDiffs())
			{
				return false;
			}

			if (!ReadTemplateFrameDiffChains())
			{
				return false;
			}

			return true;
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

		bool DependencyDescriptor::ReadTemplateDecodeTargetIndications()
		{
			MS_TRACE();

			auto templateCount = this->templateDependencyStructure->templateLayers.size();

			for (size_t templateIndex = 0; templateIndex < templateCount; templateIndex++)
			{
				for (uint8_t dtIndex = 0; dtIndex < this->templateDependencyStructure->decodeTargetCount;
				     dtIndex++)
				{
					if (this->bitStream.GetLeftBits() < 2)
					{
						return false;
					}

					this->templateDependencyStructure->templateLayers[templateIndex].decodeTargetIndications.push_back(
					  static_cast<DecodeTargetIndication>(this->bitStream.GetBits(2)));
				}
			}

			return true;
		}

		bool DependencyDescriptor::ReadTemplateFrameDiffs()
		{
			MS_TRACE();

			auto templateCount = this->templateDependencyStructure->templateLayers.size();

			for (size_t templateIndex = 0; templateIndex < templateCount; templateIndex++)
			{
				if (this->bitStream.GetLeftBits() < 1)
				{
					return false;
				}

				bool followsFlag = this->bitStream.GetBit();

				while (followsFlag)
				{
					if (this->bitStream.GetLeftBits() < 5)
					{
						return false;
					}

					uint8_t fdiff = this->bitStream.GetBits(4) + 1;

					this->templateDependencyStructure->templateLayers[templateIndex].frameDiffs.push_back(fdiff);

					followsFlag = this->bitStream.GetBit();
				}
			}

			return true;
		}

		bool DependencyDescriptor::ReadTemplateFrameDiffChains()
		{
			MS_TRACE();

			// TODO: Check left bits.
			auto chainCount =
			  this->bitStream.ReadNs(this->templateDependencyStructure->decodeTargetCount + 1);

			if (chainCount == 0)
			{
				return true;
			}

			for (uint8_t dtIndex = 0; dtIndex < this->templateDependencyStructure->decodeTargetCount;
			     dtIndex++)
			{
				uint8_t chain = this->bitStream.ReadNs(chainCount);
				this->decodeTargetProtectedBy.push_back(chain);
			}

			auto templateCount = this->templateDependencyStructure->templateLayers.size();

			for (size_t templateIndex = 0; templateIndex < templateCount; templateIndex++)
			{
				for (uint32_t chainIndex = 0; chainIndex < chainCount; chainIndex++)
				{
					if (this->bitStream.GetLeftBits() < 4)
					{
						return false;
					}

					this->templateDependencyStructure->templateLayers[templateIndex].frameDiffChains.push_back(
					  this->bitStream.GetBits(4));
				}
			}

			return true;
		}

		bool DependencyDescriptor::ReadFrameDependencyDefinition()
		{
			MS_TRACE();

			uint8_t templateIndex =
			  (this->frameDependencyTemplateId + 64 - this->templateDependencyStructure->templateIdOffset) %
			  64;

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

		// TODO: Hardcoded to only set spatial 0 and temporal 0.
		bool DependencyDescriptor::UpdateActiveDecodeTargets()
		{
			MS_TRACE();

			if (this->isKeyFrame)
			{
				return true;
			}

			this->bitStream.Reset();

			// Bits required for mandatory fields.
			if (this->bitStream.GetLeftBits() < 24)
			{
				MS_WARN_DEV("not enough space for mandatory fields");

				return false;
			}

			this->bitStream.SkipBits(24);

			// Bits required for extended fields.
			if (this->bitStream.GetLeftBits() < 5)
			{
				MS_WARN_DEV("not enough space for extended fields");

				return false;
			}

			// Skip dependency structure present flag.
			this->bitStream.SkipBits(1);
			// Set the active decode targets present flag.
			this->bitStream.PutBit(1);

			// Advance 3 positions due to non interesting fields.
			bitStream.SkipBits(3);

			if (this->bitStream.GetLeftBits() < this->templateDependencyStructure->decodeTargetCount)
			{
				MS_WARN_DEV("not enough space for active decode targets");

				return false;
			}

			// Write the active decode targets bitmask.
			this->bitStream.PutBits(this->templateDependencyStructure->decodeTargetCount, 1);

			return true;
		}
	} // namespace Codecs
} // namespace RTC
