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

					// Parsed values.
					bool isKeyFrame{ false };
				};

			public:
				static H264::PayloadDescriptor* Parse(const uint8_t* data, size_t len);
				static bool IsKeyFrame(const uint8_t* data, size_t len);
				static void ProcessRtpPacket(
				  RTP::Packet* packet,
				  std::shared_ptr<Codecs::DependencyDescriptor::TemplateDependencyStructure>&
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
					  Codecs::EncodingContext* encodingContext, RTP::Packet* packet, bool& marker) override;
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
						return 0;
					}
					uint8_t GetTemporalLayer() const override
					{
						/*
						 * The temporal layer is not parsed out of H264 packets, and a packet
						 * carrying a Dependency Descriptor, which does tell it, is given its
						 * payload descriptor handler instead of this one.
						 */
						return 0;
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
