#ifndef MS_RTC_RTP_CODECS_DEPENDENCY_DESCRIPTOR_PAYLOAD_DESCRIPTOR_HPP
#define MS_RTC_RTP_CODECS_DEPENDENCY_DESCRIPTOR_PAYLOAD_DESCRIPTOR_HPP

#include "common.hpp"
#include "RTC/RTP/Codecs/PayloadDescriptorHandler.hpp"
#include "RTC/RTP/Packet.hpp"

namespace RTC
{
	namespace RTP
	{
		namespace Codecs
		{
			/**
			 * Payload descriptor of a packet carrying a Dependency Descriptor RTP
			 * header extension, which tells everything needed to forward it: the
			 * layers of the frame, whether decoding can start at it, and which decode
			 * targets the endpoint must be told about.
			 *
			 * NOTE: Codec agnostic on purpose. The extension tells the same regardless
			 * of the codec that produced the frame, so every codec whose packets may
			 * carry it uses this rather than its own payload descriptor.
			 */
			struct DependencyDescriptorPayloadDescriptor : public Codecs::PayloadDescriptor
			{
				struct EncodingData
				{
					uint32_t maxSpatialLayer{ 0 };
					uint32_t maxTemporalLayer{ 0 };
				};

				struct Encoder : public Codecs::PayloadDescriptor::Encoder
				{
					~Encoder() override = default;
					explicit Encoder(EncodingData encodingData) : encodingData(encodingData)
					{
					}
					void Encode(DependencyDescriptorPayloadDescriptor* payloadDescriptor, RTP::Packet* packet) const;

					EncodingData encodingData;
				};

				static DependencyDescriptorPayloadDescriptor* Parse(
				  RTP::Packet* packet,
				  std::shared_ptr<Codecs::DependencyDescriptor::TemplateDependencyStructure>&
				    templateDependencyStructure);

				explicit DependencyDescriptorPayloadDescriptor(
				  std::unique_ptr<Codecs::DependencyDescriptor>& dependencyDescriptor);
				/* Pure virtual methods inherited from Codecs::PayloadDescriptor. */
				~DependencyDescriptorPayloadDescriptor() override = default;

				void Dump(int indentation = 0) const override;

				void Restore(RTP::Packet* packet) const;
				uint8_t GetDependencyDescriptorMaxLength() const
				{
					return this->dependencyDescriptor ? this->dependencyDescriptor->GetMaxLength() : 0;
				}
				// Highest spatial layer id of the stream this frame belongs to.
				uint32_t GetSpatialLayers() const
				{
					return this->dependencyDescriptor
					         ? this->dependencyDescriptor->templateDependencyStructure->spatialLayers
									 : 0;
				}
				bool IsPresentInDecodeTarget(uint8_t spatialLayer, uint8_t temporalLayer) const
				{
					return !this->dependencyDescriptor ||
					       this->dependencyDescriptor->IsPresentInDecodeTarget(spatialLayer, temporalLayer);
				}
				bool IsDecodeTargetActive(
				  uint8_t spatialLayer,
				  uint8_t temporalLayer,
				  uint8_t forwardedSpatialLayer,
				  uint8_t forwardedTemporalLayer) const
				{
					return this->dependencyDescriptor &&
					       this->dependencyDescriptor->IsDecodeTargetActive(
					         spatialLayer, temporalLayer, forwardedSpatialLayer, forwardedTemporalLayer);
				}
				bool IsSwitchPointForDecodeTarget(uint8_t spatialLayer, uint8_t temporalLayer) const
				{
					return this->dependencyDescriptor &&
					       this->dependencyDescriptor->IsSwitchPointForDecodeTarget(spatialLayer, temporalLayer);
				}
				bool IsSwitchPointForTemporalLayer(uint8_t temporalLayer) const
				{
					return this->dependencyDescriptor &&
					       this->dependencyDescriptor->IsSwitchPointForTemporalLayer(temporalLayer);
				}
				void UpdateActiveDecodeTargets(
				  uint16_t spatialLayer, uint16_t temporalLayer, RTP::Packet* packet);

				// Fields in the Dependency Descriptor extension.
				const bool startOfFrame;
				const bool endOfFrame;
				const uint16_t frameNumber;
				const uint8_t spatialLayer;
				const uint8_t temporalLayer;
				const std::unique_ptr<Codecs::DependencyDescriptor> dependencyDescriptor;
				// Parsed values.
				const bool isKeyFrame;
			};

			class DependencyDescriptorPayloadDescriptorHandler : public Codecs::PayloadDescriptorHandler
			{
			public:
				explicit DependencyDescriptorPayloadDescriptorHandler(
				  DependencyDescriptorPayloadDescriptor* payloadDescriptor);
				~DependencyDescriptorPayloadDescriptorHandler() override = default;

			public:
				void Dump(int indentation = 0) const override
				{
					this->payloadDescriptor->Dump(indentation);
				}
				bool Process(Codecs::EncodingContext* encodingContext, RTP::Packet* packet, bool& marker) override;

				void Encode(RTP::Packet* packet, Codecs::PayloadDescriptor::Encoder* encoder) override;
				void Restore(RTP::Packet* packet) override;
				uint8_t GetSpatialLayer() const override
				{
					return this->payloadDescriptor->spatialLayer;
				}
				uint8_t GetTemporalLayer() const override
				{
					return this->payloadDescriptor->temporalLayer;
				}
				bool IsKeyFrame() const override
				{
					return this->payloadDescriptor->isKeyFrame;
				}
				uint8_t GetDependencyDescriptorMaxLength() const override
				{
					return this->payloadDescriptor->GetDependencyDescriptorMaxLength();
				}

			private:
				std::unique_ptr<DependencyDescriptorPayloadDescriptor> payloadDescriptor;
			};
		} // namespace Codecs
	} // namespace RTP
} // namespace RTC

#endif
