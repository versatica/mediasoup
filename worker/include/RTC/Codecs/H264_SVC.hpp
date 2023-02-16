#ifndef MS_RTC_CODECS_H264_SVC_HPP
#define MS_RTC_CODECS_H264_SVC_HPP

#include "common.hpp"
#include "RTC/Codecs/PayloadDescriptorHandler.hpp"
#include "RTC/RtpPacket.hpp"
#include "RTC/SeqManager.hpp"

namespace RTC
{
	namespace Codecs
	{
		class H264_SVC
		{
		public:
			struct PayloadDescriptor : public RTC::Codecs::PayloadDescriptor
			{
				~PayloadDescriptor() = default;

				void Dump() const override;

				// Fields in frame-marking extension.
				uint8_t s : 1;          // Start of Frame.
				uint8_t e : 1;          // End of Frame.
				uint8_t i : 1;          // Independent Frame.
				uint8_t d : 1;          // Discardable Frame.
				uint8_t b : 1;          // Base Layer Sync.
				uint8_t slIndex{ 0 };   // Temporal layer id.
				uint8_t tlIndex{ 0 };   // Spatial layer id.
				uint8_t tl0picidx{ 0 }; // TL0PICIDX

				// Parsed values.
				bool hasSlIndex{ false };
				bool hasTlIndex{ false };
				bool hasTl0picidx{ false };
				bool isKeyFrame{ false };

				// Extension fields.
				uint8_t idr{ 0 };
				uint8_t priorityId{ 0 };
				uint8_t noIntLayerPredFlag{ true };
			};

		public:
			static H264_SVC::PayloadDescriptor* Parse(
			  const uint8_t* data,
			  size_t len,
			  RTC::RtpPacket::FrameMarking* frameMarking = nullptr,
			  uint8_t frameMarkingLen                    = 0);
			static std::unique_ptr<H264_SVC::PayloadDescriptor> ParseSingleNalu(
			  const uint8_t* data,
			  size_t len,
			  std::unique_ptr<H264_SVC::PayloadDescriptor> payloadDescriptor,
			  bool isStartBit); // useful in FU packet to indicate first packet. Set to true for other packets
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
				}

			public:
				RTC::SeqManager<uint16_t, 15> pictureIdManager;
				bool syncRequired{ false };
			};

		public:
			class PayloadDescriptorHandler : public RTC::Codecs::PayloadDescriptorHandler
			{
			public:
				explicit PayloadDescriptorHandler(PayloadDescriptor* payloadDescriptor);
				~PayloadDescriptorHandler() = default;

			public:
				void Dump() const override
				{
					this->payloadDescriptor->Dump();
				}
				bool Process(RTC::Codecs::EncodingContext* encodingContext, uint8_t* data, bool& marker) override;
				void Restore(uint8_t* data) override;
				uint8_t GetSpatialLayer() const override
				{
					// return 0u;
					return this->payloadDescriptor->hasSlIndex ? this->payloadDescriptor->slIndex : 0u;
				}
				uint8_t GetTemporalLayer() const override
				{
					// return this->payloadDescriptor->tid;
					return this->payloadDescriptor->hasTlIndex ? this->payloadDescriptor->tlIndex : 0u;
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
