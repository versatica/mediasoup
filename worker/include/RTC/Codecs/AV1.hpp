#ifndef MS_RTC_CODECS_AV1_HPP
#define MS_RTC_CODECS_AV1_HPP

#include "common.hpp"
#include "RTC/Codecs/PayloadDescriptorHandler.hpp"
#include "RTC/RtpPacket.hpp"

namespace RTC
{
	namespace Codecs
	{
		class AV1
		{
		public:
			struct PayloadDescriptor : public RTC::Codecs::PayloadDescriptor
			{
				struct EncodingData
				{
					uint32_t frameNumber{ 0 };
				};

				struct Encoder : public RTC::Codecs::PayloadDescriptor::Encoder
				{
					~Encoder() override = default;
					explicit Encoder(EncodingData encodingData) : encodingData(encodingData)
					{
					}
					void Encode(uint8_t* data, const AV1::PayloadDescriptor* payloadDescriptor) const;

					EncodingData encodingData;
				};

				PayloadDescriptor(Codecs::DependencyDescriptor* dependencyDescriptor);
				/* Pure virtual methods inherited from RTC::Codecs::PayloadDescriptor. */
				~PayloadDescriptor() override = default;

				void Dump() const override;
				// Rewrite the buffer with the given frameNumber value.
				void Encode(uint8_t* data, uint16_t frameNumber) const;
				// Rewrite the buffer with the frameNumber value of the encoder.
				void Encode(uint8_t* data) const;
				void Restore(uint8_t* data) const;

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
				bool startOfFrame{ false };
				bool endOfFrame{ false };
				uint16_t frameNumber{ 0 };
				uint8_t spatialLayer{ 0 };
				uint8_t temporalLayer{ 0 };

				// Parsed values.
				bool isKeyFrame{ false };

				std::optional<Encoder> encoder{ std::nullopt };
			};

		public:
			static AV1::PayloadDescriptor* Parse(Codecs::DependencyDescriptor* dependencyDescriptor);
			static void ProcessRtpPacket(
			  RTC::RtpPacket* packet,
			  std::unique_ptr<RTC::Codecs::DependencyDescriptor::TemplateDependencyStructure>&
			    templateDependencyStructure);

		public:
			class EncodingContext : public RTC::Codecs::EncodingContext
			{
			public:
				explicit EncodingContext(RTC::Codecs::EncodingContext::Params& params)
				  : RTC::Codecs::EncodingContext(params)
				{
				}
				~EncodingContext() override = default;

				/* Pure virtual methods inherited from RTC::Codecs::EncodingContext. */
			public:
				void SyncRequired() override
				{
					this->syncRequired = true;
				}

			public:
				RTC::SeqManager<uint16_t> frameNumberManager;
				bool syncRequired{ false };
			};

			class PayloadDescriptorHandler : public RTC::Codecs::PayloadDescriptorHandler
			{
			public:
				explicit PayloadDescriptorHandler(PayloadDescriptor* payloadDescriptor);
				~PayloadDescriptorHandler() override = default;

			public:
				void Dump() const override
				{
					this->payloadDescriptor->Dump();
				}
				bool Process(
				  RTC::Codecs::EncodingContext* encodingContext, RTC::RtpPacket* packet, bool& marker) override;
				std::unique_ptr<RTC::Codecs::PayloadDescriptor::Encoder> GetEncoder() const override
				{
					return this->payloadDescriptor->GetEncoder();
				}
				void Encode(RtpPacket* packet, Codecs::PayloadDescriptor::Encoder* encoder) override;
				void Restore(RtpPacket* packet) override;
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
} // namespace RTC

#endif
