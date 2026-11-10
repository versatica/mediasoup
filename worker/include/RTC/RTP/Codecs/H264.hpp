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
			private:
				/**
				 * Bits H264 numbers its frames with. It does not number them itself,
				 * so they are the ones the Dependency Descriptor gives.
				 */
				static constexpr uint8_t FrameIdLength{ DependencyDescriptor::FrameIdLength };

			public:
				struct PayloadDescriptor : public Codecs::PayloadDescriptor
				{
					/* Pure virtual methods inherited from Codecs::PayloadDescriptor. */
					~PayloadDescriptor() override = default;

					void Dump(int indentation = 0) const override;

					// Parsed values.
					/**
					 * Told by the Dependency Descriptor of the packet, since H264 packets
					 * do not carry it themselves. Left at 0 when there is none, so that
					 * nothing is filtered out.
					 */
					uint8_t temporalLayer{ 0 };
					bool isKeyFrame{ false };
				};

			public:
				static H264::PayloadDescriptor* Parse(const uint8_t* data, size_t len);
				static bool IsKeyFrame(const uint8_t* data, size_t len);
				static void ProcessRtpPacket(
				  RTP::Packet* packet, Codecs::DependencyDescriptor::Context& dependencyDescriptorContext);

			public:
				class EncodingContext : public Codecs::EncodingContext
				{
				public:
					explicit EncodingContext(Codecs::EncodingContext::Params& params)
					  : Codecs::EncodingContext(params, H264::FrameIdLength)
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
