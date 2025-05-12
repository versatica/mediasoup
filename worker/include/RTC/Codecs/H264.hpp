#ifndef MS_RTC_CODECS_H264_HPP
#define MS_RTC_CODECS_H264_HPP

#include "common.hpp"
#include "RTC/Codecs/PayloadDescriptorHandler.hpp"
#include "RTC/RtpPacket.hpp"

namespace RTC
{
	namespace Codecs
	{
		class H264
		{
		public:
			struct PayloadDescriptor : public RTC::Codecs::PayloadDescriptor
			{
				/* Pure virtual methods inherited from RTC::Codecs::PayloadDescriptor. */
				~PayloadDescriptor() override = default;

				void Dump() const override;

				// Fields in Dependency Descriptor extension.
				bool startOfFrame{ false };
				bool endOfFrame{ false };
				uint8_t spatialLayer{ 0 };
				uint8_t temporalLayer{ 0 };

				// Parsed values.
				bool isKeyFrame{ false };
			};

		public:
			static H264::PayloadDescriptor* Parse(
			  const uint8_t* data, size_t len, RTC::Codecs::DependencyDescriptor* dependencyDescriptor);
			static bool IsKeyFrame(const uint8_t* data, size_t len);
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
				~EncodingContext() = default;

				/* Pure virtual methods inherited from RTC::Codecs::EncodingContext. */
			public:
				void SyncRequired() override
				{
				}
			};

		public:
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
					return nullptr;
				}
				void Encode(RtpPacket* packet, Codecs::PayloadDescriptor::Encoder* encoder) override
				{
				}
				void Restore(RtpPacket* packet) override
				{
				}
				uint8_t GetSpatialLayer() const override
				{
					return 0u;
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
