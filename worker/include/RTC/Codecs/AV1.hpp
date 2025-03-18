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
				/* Pure virtual methods inherited from RTC::Codecs::PayloadDescriptor. */
				~PayloadDescriptor() override = default;

				void Dump() const override;

				bool startOfFrame{ false };
				bool endOfFrame{ false };
				uint8_t spatialLayer{ 0 };
				uint8_t temporalLayer{ 0 };
				bool isKeyFrame{ false };
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
				}
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
				bool Process(RTC::Codecs::EncodingContext* encodingContext, uint8_t* data, bool& marker) override;
				void Restore(uint8_t* data) override;
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
