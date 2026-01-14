#ifndef MS_RTC_RTP_CODECS_OPUS_HPP
#define MS_RTC_RTP_CODECS_OPUS_HPP

#include "common.hpp"
#include "RTC/RTP/Codecs/PayloadDescriptorHandler.hpp"
#include "RTC/RTP/Packet.hpp"

namespace RTC
{
	namespace RTP
	{
		namespace Codecs
		{
			class Opus
			{
			public:
				struct PayloadDescriptor : public Codecs::PayloadDescriptor
				{
					/* Pure virtual methods inherited from Codecs::PayloadDescriptor. */
					~PayloadDescriptor() override = default;

					void Dump(int indentation = 0) const override;

					// Mandatory fields.
					uint8_t stereo : 1;
					uint8_t code : 2;
					// Parsed values.
					bool isDtx{ false };
				};

			public:
				static Opus::PayloadDescriptor* Parse(const uint8_t* data, size_t len);
				static void ProcessRtpPacket(RTP::Packet* packet);

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
					void RtpPacketChanged(RTP::Packet* packet) override {};
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
						return 0u;
					}
					bool IsKeyFrame() const override
					{
						return false;
					}

				private:
					std::unique_ptr<PayloadDescriptor> payloadDescriptor;
				};
			};
		} // namespace Codecs
	} // namespace RTP
} // namespace RTC

#endif
