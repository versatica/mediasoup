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
				~PayloadDescriptor() = default;

				void Dump() const override;

				// Fields in frame-marking extension.
				uint8_t s : 1;          // Start of Frame.
				uint8_t e : 1;          // End of Frame.
				uint8_t i : 1;          // Independent Frame.
				uint8_t d : 1;          // Discardable Frame.
				uint8_t b : 1;          // Base Layer Sync.
				uint8_t tid{ 0 };       // Temporal layer id.
				uint8_t lid{ 0 };       // Spatial layer id.
				uint8_t tl0picidx{ 0 }; // TL0PICIDX
				// Parsed values.
				bool hasLid{ false };
				bool hasTl0picidx{ false };
				bool isKeyFrame{ false };
			};

		public:
			static H264::PayloadDescriptor* Parse(
			  const uint8_t* data,
			  size_t len,
			  RTC::RtpPacket::FrameMarking* frameMarking = nullptr,
			  uint8_t frameMarkingLen                    = 0);
			static void ProcessRtpPacket(RTC::RtpPacket* packet);

		public:
			class EncodingContext : public RTC::Codecs::EncodingContext
			{
			public:
				~EncodingContext() = default;

				/* Pure virtual methods inherited from RTC::Codecs::EncodingContext. */
			public:
				void SyncRequired() override;
			};

		public:
			class PayloadDescriptorHandler : public RTC::Codecs::PayloadDescriptorHandler
			{
			public:
				explicit PayloadDescriptorHandler(PayloadDescriptor* payloadDescriptor);
				~PayloadDescriptorHandler() = default;

			public:
				void Dump() const override;
				bool Process(RTC::Codecs::EncodingContext* context, uint8_t* data) override;
				void Restore(uint8_t* data) override;
				uint8_t GetSpatialLayer() const override;
				uint8_t GetTemporalLayer() const override;
				bool IsKeyFrame() const override;

			private:
				std::unique_ptr<PayloadDescriptor> payloadDescriptor;
			};
		};

		/* Inline EncondingContext methods. */

		inline void H264::EncodingContext::SyncRequired()
		{
		}

		/* Inline PayloadDescriptorHandler methods. */

		inline void H264::PayloadDescriptorHandler::Dump() const
		{
			this->payloadDescriptor->Dump();
		}

		inline uint8_t H264::PayloadDescriptorHandler::GetSpatialLayer() const
		{
			return 0u;
		}

		inline uint8_t H264::PayloadDescriptorHandler::GetTemporalLayer() const
		{
			return this->payloadDescriptor->tid;
		}

		inline bool H264::PayloadDescriptorHandler::IsKeyFrame() const
		{
			return this->payloadDescriptor->isKeyFrame;
		}
	} // namespace Codecs
} // namespace RTC

#endif
