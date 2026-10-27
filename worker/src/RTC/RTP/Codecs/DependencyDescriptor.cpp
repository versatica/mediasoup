#define MS_CLASS "RTC::Codecs::DependencyDescriptor"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/RTP/Codecs/DependencyDescriptor.hpp"
#include "Logger.hpp"
#include "RTC/RTP/Packet.hpp"
#include <algorithm> // std::max()
#include <bitset>
#include <cstring> // std::memset()
#include <string>
#include <vector>

namespace RTC
{
	namespace RTP
	{
		namespace Codecs
		{
			/* Static. */

			// Bit offset of the template dependency structure, this is, right after the
			// mandatory descriptor fields and the extended descriptor field flags.
			static constexpr uint32_t TemplateDependencyStructureOffset{ 29 };

			/* Static members. */

			// clang-format off
			ankerl::unordered_dense::map<DependencyDescriptor::DecodeTargetIndication, std::string> DependencyDescriptor::dtiToString =
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
			  std::shared_ptr<TemplateDependencyStructure>& templateDependencyStructure)
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
					templateDependencyStructure = std::make_shared<TemplateDependencyStructure>();
				}

				std::unique_ptr<DependencyDescriptor> dependencyDescriptor(
				  new DependencyDescriptor(data, len, templateDependencyStructure));

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

				// Hand over the structure this descriptor carried, if any, so that the
				// following ones are parsed with it.
				//
				// NOTE: Not done when parsing fails, so that a truncated structure never
				// replaces the one in use.
				templateDependencyStructure = dependencyDescriptor->templateDependencyStructure;

				return dependencyDescriptor.release();
			}

			/* Instance methods. */

			DependencyDescriptor::DependencyDescriptor(
			  const uint8_t* data,
			  size_t len,
			  std::shared_ptr<TemplateDependencyStructure> templateDependencyStructure)
			  : templateDependencyStructure(std::move(templateDependencyStructure)),
			    bitStream(const_cast<uint8_t*>(data), len),
			    length(static_cast<uint8_t>(len))
			{
				MS_TRACE();
			}

			uint8_t DependencyDescriptor::GetSpatialLayer() const
			{
				MS_TRACE();

				return this->templateDependencyStructure->templateLayers[this->templateId].spatialLayer;
			}

			uint8_t DependencyDescriptor::GetTemporalLayer() const
			{
				MS_TRACE();

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
						MS_DUMP_CLEAN(indentation + 3, "  decodeTargetIndications: %s", dtis.c_str());
						std::string fdiffs;
						for (const auto& fdiff : layer.frameDiffs)
						{
							if (!fdiffs.empty())
							{
								fdiffs += ",";
							}

							fdiffs += std::to_string(fdiff);
						}
						MS_DUMP_CLEAN(indentation + 3, "  frameDiffs: %s", fdiffs.c_str());
						std::string fdiffChains;
						for (const auto& fdiffChain : layer.frameDiffChains)
						{
							if (!fdiffChains.empty())
							{
								fdiffChains += ",";
							}

							fdiffChains += std::to_string(fdiffChain);
						}
						MS_DUMP_CLEAN(indentation + 3, "  frameDiffChains: %s", fdiffChains.c_str());
						MS_DUMP_CLEAN(indentation + 3, "</FrameDependencyTemplate>");
					}
					MS_DUMP_CLEAN(indentation + 2, "</TemplateLayers>");
					MS_DUMP_CLEAN(indentation + 1, "</TemplateDependencyStructure>");
				}

				if (this->activeDecodeTargetsBitmask.has_value())
				{
					MS_DUMP_CLEAN(
					  indentation + 1,
					  "activeDecodeTargetsBitmask: %s",
					  std::bitset<32>(this->activeDecodeTargetsBitmask.value()).to_string().c_str());
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

				this->extendedDescriptorFieldsPresent = true;

				const bool templateDependencyStructurePresentFlag = this->bitStream.GetBit();

				this->activeDecodeTargetsPresent           = this->bitStream.GetBit();
				this->customDecodeTargetIndicationsPresent = this->bitStream.GetBit();
				this->customFrameDiffsPresent              = this->bitStream.GetBit();
				this->customChainsPresent                  = this->bitStream.GetBit();
				this->templateDependencyStructureEndOffset = this->bitStream.GetOffset();

				if (templateDependencyStructurePresentFlag)
				{
					/*
					 * Take a brand new structure rather than overwriting the one in use,
					 * which descriptors already parsed with it keep referencing.
					 *
					 * Those may still be rewritten long after this point, for instance when
					 * retransmitted, and the number of decode targets they write depends on
					 * the structure they were parsed with.
					 */
					this->templateDependencyStructure = std::make_shared<TemplateDependencyStructure>();

					if (!ReadTemplateDependencyStructure())
					{
						return false;
					}

					this->templateDependencyStructureEndOffset = this->bitStream.GetOffset();

					this->activeDecodeTargetsBitmask =
					  (1 << this->templateDependencyStructure->decodeTargetCount) - 1;
				}

				if (this->activeDecodeTargetsPresent)
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

				ComputeDecodeTargetLayers();

				if (!ReadRenderResolutions())
				{
					return false;
				}

				return true;
			}

			/*
			 * The render resolutions are not used, but they must be consumed so that
			 * whatever follows the template dependency structure is read at the right
			 * bit offset.
			 */
			bool DependencyDescriptor::ReadRenderResolutions()
			{
				MS_TRACE();

				if (this->bitStream.GetLeftBits() < 1)
				{
					return false;
				}

				if (!this->bitStream.GetBit())
				{
					return true;
				}

				// Render width and height, 16 bits each, of every spatial layer.
				const uint32_t bits = (this->templateDependencyStructure->spatialLayers + 1) * 32;

				if (this->bitStream.GetLeftBits() < bits)
				{
					return false;
				}

				this->bitStream.SkipBits(bits);

				return true;
			}

			/*
			 * Tells the spatial and temporal layer of every decode target, this is, the
			 * highest ones among the templates that are part of it.
			 */
			void DependencyDescriptor::ComputeDecodeTargetLayers() const
			{
				MS_TRACE();

				auto& decodeTargetLayers   = this->templateDependencyStructure->decodeTargetLayers;
				const auto& templateLayers = this->templateDependencyStructure->templateLayers;

				decodeTargetLayers.clear();

				for (uint8_t dtIndex{ 0 }; dtIndex < this->templateDependencyStructure->decodeTargetCount;
				     ++dtIndex)
				{
					DecodeTargetLayer decodeTargetLayer;

					for (const auto& templateLayer : templateLayers)
					{
						if (templateLayer.decodeTargetIndications[dtIndex] != DecodeTargetIndication::NOT_PRESENT)
						{
							decodeTargetLayer.spatialLayer = std::max<uint8_t>(
							  decodeTargetLayer.spatialLayer, static_cast<uint8_t>(templateLayer.spatialLayer));
							decodeTargetLayer.temporalLayer = std::max<uint8_t>(
							  decodeTargetLayer.temporalLayer, static_cast<uint8_t>(templateLayer.temporalLayer));
						}
					}

					decodeTargetLayers.push_back(decodeTargetLayer);
				}
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

						this->templateDependencyStructure->templateLayers[templateIndex]
						  .decodeTargetIndications.push_back(
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

						this->templateDependencyStructure->templateLayers[templateIndex].frameDiffs.push_back(
						  fdiff);

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

				this->templateDependencyStructure->chainCount = chainCount.value();

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

				const uint8_t templateIndex = (this->frameDependencyTemplateId + 64 -
				                               this->templateDependencyStructure->templateIdOffset) %
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

				// The custom frame dependency fields are not used, but they must be
				// delimited so that they can be copied over verbatim when rewriting.
				this->frameDependencyDefinitionOffset = this->bitStream.GetOffset();

				if (this->customDecodeTargetIndicationsPresent && !ReadFrameDecodeTargetIndications())
				{
					return false;
				}

				if (this->customFrameDiffsPresent && !ReadFrameDiffs())
				{
					return false;
				}

				if (this->customChainsPresent && !ReadFrameDiffChains())
				{
					return false;
				}

				this->frameDependencyDefinitionEndOffset = this->bitStream.GetOffset();

				return true;
			}

			bool DependencyDescriptor::ReadFrameDecodeTargetIndications()
			{
				MS_TRACE();

				const uint32_t bits = this->templateDependencyStructure->decodeTargetCount * 2;

				if (this->bitStream.GetLeftBits() < bits)
				{
					return false;
				}

				this->bitStream.SkipBits(bits);

				return true;
			}

			bool DependencyDescriptor::ReadFrameDiffs()
			{
				MS_TRACE();

				while (true)
				{
					if (this->bitStream.GetLeftBits() < 2)
					{
						return false;
					}

					const uint32_t nextFrameDiffSize = this->bitStream.GetBits(2);

					if (nextFrameDiffSize == 0)
					{
						return true;
					}

					if (this->bitStream.GetLeftBits() < 4 * nextFrameDiffSize)
					{
						return false;
					}

					this->bitStream.SkipBits(4 * nextFrameDiffSize);
				}
			}

			bool DependencyDescriptor::ReadFrameDiffChains()
			{
				MS_TRACE();

				const uint32_t bits = this->templateDependencyStructure->chainCount * 8;

				if (this->bitStream.GetLeftBits() < bits)
				{
					return false;
				}

				this->bitStream.SkipBits(bits);

				return true;
			}

			uint32_t DependencyDescriptor::GetRewrittenBits() const
			{
				MS_TRACE();

				// Mandatory descriptor fields.
				uint32_t bits{ 24 };

				// Extended descriptor field flags.
				bits += 5;

				if (this->isKeyFrame)
				{
					bits += this->templateDependencyStructureEndOffset - TemplateDependencyStructureOffset;
				}

				// Active decode targets bitmask, which we always write.
				bits += this->templateDependencyStructure->decodeTargetCount;

				// Custom frame dependency fields.
				bits += this->frameDependencyDefinitionEndOffset - this->frameDependencyDefinitionOffset;

				return bits;
			}

			uint8_t DependencyDescriptor::GetMaxLength() const
			{
				MS_TRACE();

				const auto bits = GetRewrittenBits();

				// The rewritten descriptor would not fit in the RTP header extension, so
				// `Write()` will refuse to produce it and the descriptor will be forwarded
				// just as it was received, telling the endpoint that decode targets being
				// filtered out are active.
				if (bits > Consts::TwoBytesRtpExtensionMaxLength * 8)
				{
					MS_WARN_TAG(
					  rtp,
					  "rewritten descriptor does not fit so it will be forwarded as received [bits:%" PRIu32
					  "]",
					  bits);

					return this->length;
				}

				// NOTE: The descriptor is bit packed and zero padded up to the next byte
				// boundary, hence the rounding up.
				return static_cast<uint8_t>((bits + 7) / 8);
			}

			uint8_t DependencyDescriptor::Write(uint32_t activeDecodeTargetsBitmask, uint8_t* buffer) const
			{
				MS_TRACE();

				const auto* data    = this->bitStream.GetData();
				const uint32_t bits = GetRewrittenBits();

				if (bits > Consts::TwoBytesRtpExtensionMaxLength * 8)
				{
					MS_WARN_DEV("rewritten descriptor does not fit [bits:%" PRIu32 "]", bits);

					return 0;
				}

				const auto len = static_cast<uint8_t>((bits + 7) / 8);

				std::memset(buffer, 0, len);

				Utils::BitWriter writer(buffer);

				// Mandatory descriptor fields, unchanged.
				writer.CopyBits(data, 0, 24);

				// Extended descriptor field flags, with the active decode targets one set.
				writer.PutBit(this->isKeyFrame ? 1 : 0);
				writer.PutBit(1);
				writer.PutBit(this->customDecodeTargetIndicationsPresent ? 1 : 0);
				writer.PutBit(this->customFrameDiffsPresent ? 1 : 0);
				writer.PutBit(this->customChainsPresent ? 1 : 0);

				// Template dependency structure, unchanged.
				if (this->isKeyFrame)
				{
					writer.CopyBits(
					  data, TemplateDependencyStructureOffset, this->templateDependencyStructureEndOffset);
				}

				writer.PutBits(
				  this->templateDependencyStructure->decodeTargetCount, activeDecodeTargetsBitmask);

				// Custom frame dependency fields, unchanged.
				writer.CopyBits(
				  data, this->frameDependencyDefinitionOffset, this->frameDependencyDefinitionEndOffset);

				MS_ASSERT(writer.GetOffset() == bits, "wrong number of written bits");

				return len;
			}

			void DependencyDescriptor::Restore(RTP::Packet* packet)
			{
				MS_TRACE();

				// Never rewritten, so there is nothing to undo.
				if (!this->rewritten)
				{
					return;
				}

				/*
				 * The descriptor is restored as the Producer left it rather than as it was
				 * received, this is, followed by the zero padding that makes room for the
				 * active decode targets bitmask.
				 *
				 * Otherwise the RTP header extension would be shrunk to the received
				 * length and the next Consumer sharing this packet could no longer write
				 * its own longer descriptor into it.
				 *
				 * NOTE: Trailing zeroes are just zero padding for Dependency Descriptor
				 * parsers.
				 */
				const auto len = std::max<uint8_t>(GetMaxLength(), this->length);

				uint8_t buffer[Consts::TwoBytesRtpExtensionMaxLength];

				std::memcpy(buffer, this->bitStream.GetData(), this->length);
				std::memset(buffer + this->length, 0, len - this->length);

				// NOTE: It stays marked as rewritten if it could not be written back,
				// since that's what the packet still carries.
				if (!packet->UpdateDependencyDescriptor(buffer, len))
				{
					return;
				}

				this->rewritten = false;
			}

			uint32_t DependencyDescriptor::GetDecodeTargetsBitmask(
			  uint8_t maxSpatialLayer, uint8_t maxTemporalLayer) const
			{
				MS_TRACE();

				const auto& decodeTargetLayers = this->templateDependencyStructure->decodeTargetLayers;
				uint32_t bitmask{ 0 };

				for (size_t decodeTarget{ 0 }; decodeTarget < decodeTargetLayers.size(); ++decodeTarget)
				{
					const auto& decodeTargetLayer = decodeTargetLayers[decodeTarget];

					if (decodeTargetLayer.spatialLayer <= maxSpatialLayer && decodeTargetLayer.temporalLayer <= maxTemporalLayer)
					{
						bitmask |= (uint32_t{ 1 } << decodeTarget);
					}
				}

				return bitmask;
			}

			/*
			 * Whether decoding can start at this frame for the decode target formed by
			 * the spatial layer of this frame and the given temporal layer.
			 *
			 * That's what a "switch" indication means: this frame and every later frame
			 * of that decode target can be decoded without any older frame.
			 */
			bool DependencyDescriptor::IsSwitchPointForTemporalLayer(uint8_t temporalLayer) const
			{
				MS_TRACE();

				const auto& decodeTargetIndications =
				  this->templateDependencyStructure->templateLayers[this->templateId].decodeTargetIndications;

				// NOTE: Decode targets are ordered by spatial layer and, within each one,
				// by temporal layer. And `temporalLayers` holds the highest temporal layer
				// id, hence the + 1.
				const auto temporalLayerCount = this->templateDependencyStructure->temporalLayers + 1;
				const size_t decodeTarget     = (this->spatialLayer * temporalLayerCount) + temporalLayer;

				if (decodeTarget >= decodeTargetIndications.size())
				{
					return false;
				}

				return decodeTargetIndications[decodeTarget] == DecodeTargetIndication::SWITCH;
			}

			bool DependencyDescriptor::UpdateActiveDecodeTargets(
			  uint32_t maxSpatialLayer, uint32_t maxTemporalLayer, RTP::Packet* packet)
			{
				MS_TRACE();

				const auto activeDecodeTargetsBitmask = GetDecodeTargetsBitmask(
				  static_cast<uint8_t>(maxSpatialLayer), static_cast<uint8_t>(maxTemporalLayer));

				// Nothing to do if the endpoint is already told exactly this.
				if (!this->rewritten && this->activeDecodeTargetsPresent && this->activeDecodeTargetsBitmask == activeDecodeTargetsBitmask)
				{
					return true;
				}

				uint8_t buffer[Consts::TwoBytesRtpExtensionMaxLength];

				const auto len = Write(activeDecodeTargetsBitmask, buffer);

				if (len == 0)
				{
					return false;
				}

				// NOTE: Only once it's written, since the packet refuses a descriptor
				// that doesn't fit and then keeps the one it had.
				if (!packet->UpdateDependencyDescriptor(buffer, len))
				{
					return false;
				}

				this->rewritten = true;

				return true;
			}
		} // namespace Codecs
	} // namespace RTP
} // namespace RTC
