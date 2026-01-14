#ifndef MS_RTC_RTP_CODECS_H264_HPP
#define MS_RTC_RTP_CODECS_H264_HPP

#include "common.hpp"
#include "RTC/RTP/Codecs/PayloadDescriptorHandler.hpp"
#include "RTC/RTP/Packet.hpp"

namespace RTC
{
	namespace RTP
	{
		namespace Codecs
		{
			class H264
			{
			public:
				struct PayloadDescriptor : public Codecs::PayloadDescriptor
				{
					/* Pure virtual methods inherited from Codecs::PayloadDescriptor. */
					~PayloadDescriptor() override = default;

					void Dump(int indentation = 0) const override;

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
				  const uint8_t* data, size_t len, Codecs::DependencyDescriptor* dependencyDescriptor);
				static bool IsKeyFrame(const uint8_t* data, size_t len);
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
					}
				};

			public:
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
					  Codecs::EncodingContext* encodingContext, RTC::RTP::Packet* packet, bool& marker) override;
					void RtpPacketChanged(RTC::RTP::Packet* packet) override {};
					std::unique_ptr<Codecs::PayloadDescriptor::Encoder> GetEncoder() const override
					{
						return nullptr;
					}
					void Encode(RTP::Packet* packet, Codecs::PayloadDescriptor::Encoder* encoder) override
					{
					}
					void Restore(RTP::Packet* packet) override
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
	} // namespace RTP
} // namespace RTC

#endif
