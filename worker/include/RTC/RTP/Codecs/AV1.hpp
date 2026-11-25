#ifndef MS_RTC_RTP_CODECS_AV1_HPP
#define MS_RTC_RTP_CODECS_AV1_HPP

#include "common.hpp"
#include "RTC/RTP/Codecs/DependencyDescriptorPayloadDescriptor.hpp"
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
			private:
				/**
				 * Bits AV1 numbers its frames with. It does not number them itself, so
				 * they are the ones the Dependency Descriptor gives.
				 */
				static constexpr uint8_t FrameIdLength{ DependencyDescriptor::FrameIdLength };

			public:
				/**
				 * Aggregation header of the packet, which is all that an AV1 payload
				 * tells about itself. The layers of the frame are only told by the
				 * Dependency Descriptor.
				 */
				struct PayloadDescriptor : public Codecs::PayloadDescriptor
				{
					/* Pure virtual methods inherited from Codecs::PayloadDescriptor. */
					~PayloadDescriptor() override = default;

					void Dump(int indentation = 0) const override;

					// Aggregation header.
					/**
					 * Z: Whether the first OBU element continues one from the previous
					 * packet.
					 */
					uint8_t z : 1;
					/**
					 * Y: Whether the last OBU element continues in the next packet.
					 */
					uint8_t y : 1;
					/**
					 * W: OBU elements in the packet, or 0 if every one of them carries its
					 * length.
					 */
					uint8_t w : 2;
					/**
					 * N: Whether the packet is the first one of a coded video sequence.
					 */
					uint8_t n : 1;
					// Parsed values.
					bool isKeyFrame{ false };
				};

			public:
				static AV1::PayloadDescriptor* Parse(const uint8_t* data, size_t len);
				static void ProcessRtpPacket(
				  RTP::Packet* packet, Codecs::DependencyDescriptor::Context& dependencyDescriptorContext);

			public:
				class EncodingContext : public Codecs::EncodingContext
				{
				public:
					explicit EncodingContext(const Codecs::EncodingContext::Params& params)
					  : Codecs::EncodingContext(params, AV1::FrameIdLength)
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
