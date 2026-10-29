#define MS_CLASS "RTC::Codecs::DependencyDescriptor"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/RTP/Codecs/DependencyDescriptor.hpp"
#include "Logger.hpp"
#include "RTC/RTP/Packet.hpp"
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
			// Where a rewritten descriptor is built before being written into the packet.
			static thread_local uint8_t RewriteBuffer[Consts::TwoBytesRtpExtensionMaxLength];

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
			    bitReader(data, len),
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

				if (this->bitReader.GetLeftBits() < 24)
				{
					return false;
				}

				this->startOfFrame              = this->bitReader.GetBit();
				this->endOfFrame                = this->bitReader.GetBit();
				this->frameDependencyTemplateId = this->bitReader.GetBits(6);
				this->frameNumber               = this->bitReader.GetBits(16);

				return true;
			}

			bool DependencyDescriptor::ReadExtendedDescriptorFields()
			{
				MS_TRACE();

				if (this->bitReader.GetLeftBits() < 5)
				{
					return false;
				}

				this->extendedDescriptorFieldsPresent = true;

				const bool templateDependencyStructurePresentFlag = this->bitReader.GetBit();

				this->activeDecodeTargetsPresent           = this->bitReader.GetBit();
				this->customDecodeTargetIndicationsPresent = this->bitReader.GetBit();
				this->customFrameDiffsPresent              = this->bitReader.GetBit();
				this->customChainsPresent                  = this->bitReader.GetBit();
				this->templateDependencyStructureEndOffset = this->bitReader.GetOffset();

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

					this->templateDependencyStructureEndOffset = this->bitReader.GetOffset();

					this->activeDecodeTargetsBitmask =
					  (1 << this->templateDependencyStructure->decodeTargetCount) - 1;
				}

				if (this->activeDecodeTargetsPresent)
				{
					if (this->bitReader.GetLeftBits() < this->templateDependencyStructure->decodeTargetCount)
					{
						return false;
					}

					this->activeDecodeTargetsBitmask =
					  this->bitReader.GetBits(this->templateDependencyStructure->decodeTargetCount);
				}

				return true;
			}

			bool DependencyDescriptor::ReadTemplateDependencyStructure()
			{
				MS_TRACE();

				if (this->bitReader.GetLeftBits() < 11)
				{
					return false;
				}

				this->templateDependencyStructure->templateIdOffset  = this->bitReader.GetBits(6);
				this->templateDependencyStructure->decodeTargetCount = this->bitReader.GetBits(5) + 1;

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

				if (this->bitReader.GetLeftBits() < 1)
				{
					return false;
				}

				if (!this->bitReader.GetBit())
				{
					return true;
				}

				// Render width and height, 16 bits each, of every spatial layer.
				const uint32_t bits = (this->templateDependencyStructure->spatialLayers + 1) * 32;

				if (this->bitReader.GetLeftBits() < bits)
				{
					return false;
				}

				this->bitReader.SkipBits(bits);

				return true;
			}

			/*
			 * Tells the spatial and temporal layer of every decode target, this is, the
			 * highest ones among the templates that are part of it.
			 */
			void DependencyDescriptor::ComputeDecodeTargetLayers() const
			{
				MS_TRACE();

				const auto& templateLayers = this->templateDependencyStructure->templateLayers;

				auto& decodeTargetLayers = this->templateDependencyStructure->decodeTargetLayers;

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

					if (this->bitReader.GetLeftBits() < 2)
					{
						return false;
					}

					nextLayerIdc = this->bitReader.GetBits(2);

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
						if (this->bitReader.GetLeftBits() < 2)
						{
							return false;
						}

						this->templateDependencyStructure->templateLayers[templateIndex]
						  .decodeTargetIndications.push_back(
						    static_cast<DecodeTargetIndication>(this->bitReader.GetBits(2)));
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
					if (this->bitReader.GetLeftBits() < 1)
					{
						return false;
					}

					bool followsFlag = this->bitReader.GetBit();

					while (followsFlag)
					{
						if (this->bitReader.GetLeftBits() < 5)
						{
							return false;
						}

						const uint8_t fdiff = this->bitReader.GetBits(4) + 1;

						this->templateDependencyStructure->templateLayers[templateIndex].frameDiffs.push_back(
						  fdiff);

						followsFlag = this->bitReader.GetBit();
					}
				}

				return true;
			}

			bool DependencyDescriptor::ReadTemplateFrameDiffChains()
			{
				MS_TRACE();

				auto chainCount =
				  this->bitReader.ReadNs(this->templateDependencyStructure->decodeTargetCount + 1);

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
					auto chain = this->bitReader.ReadNs(chainCount.value());

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
						if (this->bitReader.GetLeftBits() < 4)
						{
							return false;
						}

						this->templateDependencyStructure->templateLayers[templateIndex].frameDiffChains.push_back(
						  this->bitReader.GetBits(4));
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
				this->frameDependencyDefinitionOffset = this->bitReader.GetOffset();

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

				this->frameDependencyDefinitionEndOffset = this->bitReader.GetOffset();

				return true;
			}

			bool DependencyDescriptor::ReadFrameDecodeTargetIndications()
			{
				MS_TRACE();

				const uint32_t bits = this->templateDependencyStructure->decodeTargetCount * 2;

				if (this->bitReader.GetLeftBits() < bits)
				{
					return false;
				}

				this->bitReader.SkipBits(bits);

				return true;
			}

			bool DependencyDescriptor::ReadFrameDiffs()
			{
				MS_TRACE();

				while (true)
				{
					if (this->bitReader.GetLeftBits() < 2)
					{
						return false;
					}

					const uint32_t nextFrameDiffSize = this->bitReader.GetBits(2);

					if (nextFrameDiffSize == 0)
					{
						return true;
					}

					if (this->bitReader.GetLeftBits() < 4 * nextFrameDiffSize)
					{
						return false;
					}

					this->bitReader.SkipBits(4 * nextFrameDiffSize);
				}
			}

			bool DependencyDescriptor::ReadFrameDiffChains()
			{
				MS_TRACE();

				const uint32_t bits = this->templateDependencyStructure->chainCount * 8;

				if (this->bitReader.GetLeftBits() < bits)
				{
					return false;
				}

				this->bitReader.SkipBits(bits);

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
					  codec,
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

				const auto* data    = this->bitReader.GetData();
				const uint32_t bits = GetRewrittenBits();

				if (bits > Consts::TwoBytesRtpExtensionMaxLength * 8)
				{
					MS_WARN_DEV("rewritten descriptor does not fit [bits:%" PRIu32 "]", bits);

					return 0;
				}

				const auto len = static_cast<uint8_t>((bits + 7) / 8);

				std::memset(buffer, 0, len);

				Utils::BitWriter writer(buffer, len);

				// Mandatory descriptor fields, unchanged.
				writer.PutBitRange(data, this->length, 0, 24);

				// Extended descriptor field flags, with the active decode targets one set.
				writer.PutBit(this->isKeyFrame ? 1 : 0);
				writer.PutBit(1);
				writer.PutBit(this->customDecodeTargetIndicationsPresent ? 1 : 0);
				writer.PutBit(this->customFrameDiffsPresent ? 1 : 0);
				writer.PutBit(this->customChainsPresent ? 1 : 0);

				// Template dependency structure, unchanged.
				if (this->isKeyFrame)
				{
					writer.PutBitRange(
					  data,
					  this->length,
					  TemplateDependencyStructureOffset,
					  this->templateDependencyStructureEndOffset);
				}

				writer.PutBits(
				  this->templateDependencyStructure->decodeTargetCount, activeDecodeTargetsBitmask);

				// Custom frame dependency fields, unchanged.
				writer.PutBitRange(
				  data,
				  this->length,
				  this->frameDependencyDefinitionOffset,
				  this->frameDependencyDefinitionEndOffset);

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
				 * The RTP header extension is shrunk back to the received length, which
				 * leaves the room the Producer made for the rewrite as padding behind
				 * it. The next Consumer sharing this packet grows into that padding
				 * again when it writes its own, possibly longer, descriptor.
				 */

				// NOTE: It stays marked as rewritten if it could not be written back,
				// since that's what the packet still carries.
				if (!packet->UpdateDependencyDescriptor(this->bitReader.GetData(), this->length))
				{
					return;
				}

				this->rewritten = false;
			}

			uint32_t DependencyDescriptor::GetDecodeTargetsBitmask(
			  uint8_t maxSpatialLayer, uint8_t maxTemporalLayer) const
			{
				MS_TRACE();

				const auto& decodeTargetLayers   = this->templateDependencyStructure->decodeTargetLayers;
				const auto forwardedDecodeTarget = GetDecodeTarget(maxSpatialLayer, maxTemporalLayer);

				uint32_t bitmask{ 0 };

				for (size_t decodeTarget{ 0 }; decodeTarget < decodeTargetLayers.size(); ++decodeTarget)
				{
					const auto& decodeTargetLayer = decodeTargetLayers[decodeTarget];

					if (decodeTargetLayer.spatialLayer > maxSpatialLayer || decodeTargetLayer.temporalLayer > maxTemporalLayer)
					{
						continue;
					}

					/*
					 * NOTE: In full SVC every lower decode target needs a subset of the
					 * frames of the higher ones, so all of them stay active. In K-SVC the
					 * lower spatial layers are not needed and hence not sent, so their
					 * decode targets are dropped from the bitmask.
					 */
					if (!IsDecodeTargetActive(decodeTarget, forwardedDecodeTarget))
					{
						continue;
					}

					bitmask |= (uint32_t{ 1 } << decodeTarget);
				}

				return bitmask;
			}

			/*
			 * Index of the decode target formed by the given spatial and temporal
			 * layer, or the decode target count if the stream has no such one.
			 */
			size_t DependencyDescriptor::GetDecodeTarget(uint8_t spatialLayer, uint8_t temporalLayer) const
			{
				MS_TRACE();

				const auto& decodeTargetLayers = this->templateDependencyStructure->decodeTargetLayers;

				for (size_t decodeTarget{ 0 }; decodeTarget < decodeTargetLayers.size(); ++decodeTarget)
				{
					const auto& decodeTargetLayer = decodeTargetLayers[decodeTarget];

					if (decodeTargetLayer.spatialLayer == spatialLayer && decodeTargetLayer.temporalLayer == temporalLayer)
					{
						return decodeTarget;
					}
				}

				return decodeTargetLayers.size();
			}

			/*
			 * Whether the given decode target is still being sent when only the frames
			 * of the forwarded one are, this is, whether every frame it needs is also
			 * needed by that one.
			 */
			bool DependencyDescriptor::IsDecodeTargetActive(
			  size_t decodeTarget, size_t forwardedDecodeTarget) const
			{
				MS_TRACE();

				const auto& templateLayers = this->templateDependencyStructure->templateLayers;

				// Nothing is known about a decode target the stream does not have, so no
				// decode target is dropped from the bitmask.
				if (forwardedDecodeTarget >= this->templateDependencyStructure->decodeTargetCount)
				{
					return true;
				}

				return std::ranges::all_of(
				  templateLayers,
				  [decodeTarget, forwardedDecodeTarget](const auto& templateLayer) -> bool
				  {
					  const auto& decodeTargetIndications = templateLayer.decodeTargetIndications;

					  // Nothing is known about a decode target this template does not tell
					  // about, so it does not rule the given one out.
					  if (
					    decodeTarget >= decodeTargetIndications.size() ||
					    forwardedDecodeTarget >= decodeTargetIndications.size())
					  {
						  return true;
					  }

					  const bool needed =
					    decodeTargetIndications[decodeTarget] != DecodeTargetIndication::NOT_PRESENT;
					  const bool sent =
					    decodeTargetIndications[forwardedDecodeTarget] != DecodeTargetIndication::NOT_PRESENT;

					  return !needed || sent;
				  });
			}

			bool DependencyDescriptor::IsPresentInDecodeTarget(uint8_t spatialLayer, uint8_t temporalLayer) const
			{
				MS_TRACE();

				const auto decodeTarget = GetDecodeTarget(spatialLayer, temporalLayer);
				const auto& decodeTargetIndications =
				  this->templateDependencyStructure->templateLayers[this->templateId].decodeTargetIndications;

				// The stream has no such decode target, so nothing can be told about this
				// frame and it is kept.
				if (decodeTarget >= decodeTargetIndications.size())
				{
					return true;
				}

				return decodeTargetIndications[decodeTarget] != DecodeTargetIndication::NOT_PRESENT;
			}

			bool DependencyDescriptor::IsSwitchPointForDecodeTarget(
			  uint8_t spatialLayer, uint8_t temporalLayer) const
			{
				MS_TRACE();

				const auto decodeTarget = GetDecodeTarget(spatialLayer, temporalLayer);
				const auto& decodeTargetIndications =
				  this->templateDependencyStructure->templateLayers[this->templateId].decodeTargetIndications;

				// The stream has no such decode target, so there is nothing to switch
				// into.
				if (decodeTarget >= decodeTargetIndications.size())
				{
					return false;
				}

				return decodeTargetIndications[decodeTarget] == DecodeTargetIndication::SWITCH;
			}

			bool DependencyDescriptor::IsSwitchPointForTemporalLayer(uint8_t temporalLayer) const
			{
				MS_TRACE();

				return IsSwitchPointForDecodeTarget(static_cast<uint8_t>(this->spatialLayer), temporalLayer);
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

				/*
				 * NOTE: The buffer is reused across calls. `Write()` zeroes the bytes it
				 * fills and only those are read back, so nothing is left over from a
				 * previous one.
				 */
				const auto len = Write(activeDecodeTargetsBitmask, RewriteBuffer);

				if (len == 0)
				{
					return false;
				}

				// NOTE: Only once it's written, since the packet refuses a descriptor
				// that doesn't fit and then keeps the one it had.
				if (!packet->UpdateDependencyDescriptor(RewriteBuffer, len))
				{
					return false;
				}

				this->rewritten = true;

				return true;
			}
		} // namespace Codecs
	} // namespace RTP
} // namespace RTC
