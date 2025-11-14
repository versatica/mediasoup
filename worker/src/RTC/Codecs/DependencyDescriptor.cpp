#define MS_CLASS "RTC::Codecs::DependencyDescriptor"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/Codecs/DependencyDescriptor.hpp"
#include "Logger.hpp"
#include <vector>

namespace RTC
{
	namespace Codecs
	{
		/* Static members. */

		// clang-format off
		std::unordered_map<DependencyDescriptor::DecodeTargetIndication, std::string> DependencyDescriptor::dtiToString =
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
		  DependencyDescriptor::Listener* listener,
		  std::unique_ptr<TemplateDependencyStructure>& templateDependencyStructure)
		{
			MS_TRACE();

			if (len < 3)
			{
				MS_WARN_DEV("ignoring payload with length < 3");

				return nullptr;
			}

			if (len > Consts::TwoBytesRtpExtensionMaxLength)
			{
				MS_WARN_DEV("ignoring payload with length > %" PRIu8, Consts::TwoBytesRtpExtensionMaxLength);

				return nullptr;
			}

			if (templateDependencyStructure == nullptr)
			{
				templateDependencyStructure = std::make_unique<TemplateDependencyStructure>();
			}

			std::unique_ptr<DependencyDescriptor> dependencyDescriptor(
			  new DependencyDescriptor(data, len, listener, templateDependencyStructure.get()));

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
		  const uint8_t* data,
		  size_t len,
		  DependencyDescriptor::Listener* listener,
		  TemplateDependencyStructure* templateDependencyStructure)
		  : templateDependencyStructure(templateDependencyStructure), listener(listener),
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
			MS_DUMP_CLEAN(indentation, "  templateId: %u", this->templateId);
			MS_DUMP_CLEAN(indentation, "  spatialLayer: %u", this->spatialLayer);
			MS_DUMP_CLEAN(indentation, "  temporalLayer: %u", this->temporalLayer);

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
						dtis += dtiToString[dti];
					}
					MS_DUMP_CLEAN(
					  indentation + 4, "<DecodeTargetIndications> %s </DecodeTargetIndications>", dtis.c_str());
					std::string fdiffs;
					for (const auto& fdiff : layer.frameDiffs)
					{
						if (!fdiffs.empty())
						{
							fdiffs += ",";
						}

						fdiffs += std::to_string(fdiff);
					}
					MS_DUMP_CLEAN(indentation + 4, "<FrameDiffs> %s </FrameDiffs>", fdiffs.c_str());
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
					  indentation + 4, "<FrameDiffChains> %s </FrameDiffChains>", fdiffChains.c_str());
					MS_DUMP_CLEAN(indentation + 3, "</FrameDependencyTemplate>");
				}
				MS_DUMP_CLEAN(indentation + 2, "</TemplateLayers>");
				MS_DUMP_CLEAN(indentation + 1, "</TemplateDependencyStructure>");
			}

			if (this->activeDecodeTargetsBitmask.has_value())
			{
				MS_DUMP_CLEAN(indentation + 1, "<ActiveDecodeTargets>");
				MS_DUMP_CLEAN(
				  indentation + 1,
				  "  %s",
				  std::bitset<32>(this->activeDecodeTargetsBitmask.value()).to_string().c_str());
				MS_DUMP_CLEAN(indentation + 1, "</ActiveDecodeTargets>");
			}
			MS_DUMP_CLEAN(indentation, "</DependencyDescriptor>");
		}

		void DependencyDescriptor::UpdateListener(DependencyDescriptor::Listener* listener)
		{
			this->listener = listener;
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
				this->templateDependencyStructure->templateLayers.emplace_back(spatialId, temporalId);

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

			for (size_t templateIndex = 0; templateIndex < templateCount; ++templateIndex)
			{
				for (uint8_t dtIndex = 0; dtIndex < this->templateDependencyStructure->decodeTargetCount;
				     ++dtIndex)
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

			for (size_t templateIndex = 0; templateIndex < templateCount; ++templateIndex)
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

					const uint8_t fdiff = this->bitStream.GetBits(4) + 1;

					this->templateDependencyStructure->templateLayers[templateIndex].frameDiffs.push_back(fdiff);

					followsFlag = this->bitStream.GetBit();
				}
			}

			return true;
		}

		bool DependencyDescriptor::ReadTemplateFrameDiffChains()
		{
			MS_TRACE();

			auto chainCount =
			  this->bitStream.ReadNs(this->templateDependencyStructure->decodeTargetCount + 1);

			if (!chainCount.has_value())
			{
				return false;
			}

			if (chainCount == 0)
			{
				return true;
			}

			for (uint8_t dtIndex = 0; dtIndex < this->templateDependencyStructure->decodeTargetCount;
			     ++dtIndex)
			{
				auto chain = this->bitStream.ReadNs(chainCount.value());

				if (!chain.has_value())
				{
					return false;
				}

				this->decodeTargetProtectedBy.push_back(chain.value());
			}

			auto templateCount = this->templateDependencyStructure->templateLayers.size();

			for (size_t templateIndex = 0; templateIndex < templateCount; ++templateIndex)
			{
				for (uint32_t chainIndex = 0; chainIndex < chainCount; ++chainIndex)
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

			const uint8_t templateIndex =
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

		const uint8_t* DependencyDescriptor::Serialize(uint8_t& len)
		{
			MS_TRACE();

			MS_ASSERT(!this->isKeyFrame, "serialization of key frames is not supported");

			this->bitStream.Reset();
			WriteMandatoryDescriptorFields();
			WriteExtendedDescriptorFields();

			len = std::ceil((this->bitStream.GetOffset() + 7) >> 3);

			return this->bitStream.GetData();
		}

		bool DependencyDescriptor::WriteMandatoryDescriptorFields()
		{
			MS_TRACE();

			this->bitStream.PutBit(this->startOfFrame ? 1 : 0);
			this->bitStream.PutBit(this->endOfFrame ? 1 : 0);
			this->bitStream.PutBits(6, this->frameDependencyTemplateId);
			this->bitStream.PutBits(16, this->frameNumber);

			return true;
		}

		bool DependencyDescriptor::WriteExtendedDescriptorFields()
		{
			MS_TRACE();

			// Template dependency structure present flag.
			this->bitStream.PutBit(0);
			// Active decode targets present flag.
			this->bitStream.PutBit(this->activeDecodeTargetsBitmask.has_value() ? 1 : 0);

			// Custom dtis flag.
			this->bitStream.PutBit(0);
			// Custom fdiffs flag.
			this->bitStream.PutBit(0);
			// Custom chains flag.
			this->bitStream.PutBit(0);

			// NOTE: Write template dependency structure if ever needed.

			// Active Decode Targets
			if (this->activeDecodeTargetsBitmask.has_value())
			{
				this->bitStream.PutBits(
				  this->templateDependencyStructure->decodeTargetCount,
				  this->activeDecodeTargetsBitmask.value());
			}

			return true;
		}

		bool DependencyDescriptor::UpdateActiveDecodeTargets(
		  uint32_t maxSpatialLayer, uint32_t maxTemporalLayer)
		{
			MS_TRACE();

			// We don't update active decode targets for key frames,
			// as by definition they enable all decode targets.
			if (this->isKeyFrame)
			{
				this->listener->OnDependencyDescriptorUpdated(
				  this->bitStream.GetData(), this->bitStream.GetLength());

				return true;
			}

			auto availableSpatialLayers  = this->templateDependencyStructure->spatialLayers;
			auto availableTemporalLayers = this->templateDependencyStructure->temporalLayers;

			MS_ASSERT(
			  maxSpatialLayer <= availableSpatialLayers,
			  "maxSpatialLayer must be less than or equal to availableSpatialLayers");
			MS_ASSERT(
			  maxTemporalLayer <= availableTemporalLayers,
			  "maxTemporalLayer must be less than or equal to availableTemporalLayers");

			uint32_t activeDecodeTargetsBitmask = 0;
			size_t bitIndex                     = 0;

			for (uint32_t spatialLayer = 0; spatialLayer <= maxSpatialLayer; ++spatialLayer)
			{
				for (uint32_t temporalLayer = 0; temporalLayer <= availableTemporalLayers; ++temporalLayer)
				{
					if (temporalLayer <= maxTemporalLayer)
					{
						// Set a 1.
						activeDecodeTargetsBitmask |= (1 << bitIndex);
					}
					else
					{
						// Set a 0.
						activeDecodeTargetsBitmask &= ~(1 << bitIndex);
					}

					bitIndex += 1;
				}
			}

			this->activeDecodeTargetsBitmask = activeDecodeTargetsBitmask;

			uint8_t len;
			const auto* data = this->Serialize(len);
			this->listener->OnDependencyDescriptorUpdated(data, len);

			return true;
		}
	} // namespace Codecs
} // namespace RTC
