#ifndef MS_RTC_CODECS_OPUS_HPP
#define MS_RTC_CODECS_OPUS_HPP

#include "common.hpp"
#include "RTC/Codecs/PayloadDescriptorHandler.hpp"
#include "RTC/RtpPacket.hpp"
#include "RTC/SeqManager.hpp"

namespace RTC
{
	namespace Codecs
	{
		class Opus
		{
		public:
			struct PayloadDescriptor : public RTC::Codecs::PayloadDescriptor
			{
				/* Pure virtual methods inherited from RTC::Codecs::PayloadDescriptor. */
				~PayloadDescriptor() override = default;

				void Dump() const override;

				// Mandatory fields.
				uint8_t stereo : 1;
				uint8_t code : 2;
				// Parsed values.
				bool isDtx{ false };
			};

		public:
			static Opus::PayloadDescriptor* Parse(const uint8_t* data, size_t len);
			static void ProcessRtpPacket(RTC::RtpPacket* packet);

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
					this->syncRequired = true;
				}

			public:
				bool syncRequired{ false };
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
} // namespace RTC

#endif
