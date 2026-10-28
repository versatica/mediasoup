#ifndef MS_RTC_RTP_CODECS_AV1_HPP
#define MS_RTC_RTP_CODECS_AV1_HPP

#include "common.hpp"
#include "RTC/RTP/Codecs/PayloadDescriptorHandler.hpp"
#include "RTC/RTP/Packet.hpp"

namespace RTC
{
	namespace RTP
	{
		namespace Codecs
		{
			class AV1
			{
			public:
				struct PayloadDescriptor : public Codecs::PayloadDescriptor
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
						void Encode(AV1::PayloadDescriptor* payloadDescriptor) const;

						EncodingData encodingData;
					};

					explicit PayloadDescriptor(std::unique_ptr<Codecs::DependencyDescriptor>& dependencyDescriptor);
					/* Pure virtual methods inherited from Codecs::PayloadDescriptor. */
					~PayloadDescriptor() override = default;

					void Dump(int indentation = 0) const override;
					void UpdateListener(Codecs::DependencyDescriptor::Listener* listener) const
					{
						if (this->dependencyDescriptor)
						{
							this->dependencyDescriptor->UpdateListener(listener);
						}
					}
					void Encode();
					void Restore() const;
					void UpdateActiveDecodeTargets(uint16_t spatialLayer, uint16_t temporalLayer);
					std::unique_ptr<Codecs::PayloadDescriptor::Encoder> GetEncoder() const
					{
						if (this->encoder.has_value())
						{
							return std::make_unique<Encoder>(this->encoder.value());
						}
						else
						{
							return nullptr;
						}
					}

					void CreateEncoder(EncodingData encodingData)
					{
						this->encoder = Encoder(encodingData);
					}

					// Fields in Dependency Descriptor extension.
					const bool startOfFrame;
					const bool endOfFrame;
					const uint16_t frameNumber;
					const uint8_t spatialLayer;
					const uint8_t temporalLayer;
					std::unique_ptr<Codecs::DependencyDescriptor> dependencyDescriptor;
					// Parsed values.
					const bool isKeyFrame;
					std::optional<Encoder> encoder{ std::nullopt };
				};

			public:
				static AV1::PayloadDescriptor* Parse(
				  std::unique_ptr<Codecs::DependencyDescriptor>& dependencyDescriptor);
				static void ProcessRtpPacket(
				  RTP::Packet* packet,
				  std::unique_ptr<Codecs::DependencyDescriptor::TemplateDependencyStructure>&
				    templateDependencyStructure);

			public:
				class EncodingContext : public Codecs::EncodingContext
				{
				public:
					explicit EncodingContext(Codecs::EncodingContext::Params& params)
					  : Codecs::EncodingContext(params)
					{
					}
					~EncodingContext() override = default;

					/* Pure virtual methods inherited from Codecs::EncodingContext. */
				public:
					void SyncRequired() override
					{
						this->syncRequired = true;
					}

				public:
					bool syncRequired{ false };
				};

				class PayloadDescriptorHandler : public Codecs::PayloadDescriptorHandler
				{
				public:
					explicit PayloadDescriptorHandler(PayloadDescriptor* payloadDescriptor);
					~PayloadDescriptorHandler() override = default;

				public:
					void Dump(int indentation = 0) const override
					{
						this->payloadDescriptor->Dump(indentation);
					}
					bool Process(
					  Codecs::EncodingContext* encodingContext, RTP::Packet* packet, bool& marker) override;
					void RtpPacketChanged(RTP::Packet* packet) override
					{
						this->payloadDescriptor->UpdateListener(packet);
					};
					std::unique_ptr<Codecs::PayloadDescriptor::Encoder> GetEncoder() const override
					{
						return this->payloadDescriptor->GetEncoder();
					}
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

				private:
					std::unique_ptr<PayloadDescriptor> payloadDescriptor;
				};
			};
		} // namespace Codecs
	} // namespace RTP
} // namespace RTC

#endif
