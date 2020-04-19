#ifndef MS_RTC_CODECS_VP8_HPP
#define MS_RTC_CODECS_VP8_HPP

#include "common.hpp"
#include "RTC/Codecs/PayloadDescriptorHandler.hpp"
#include "RTC/RtpPacket.hpp"
#include "RTC/SeqManager.hpp"

/* RFC 7741
 * VP8 Payload Descriptor

  Single octet PictureID (M = 0)        Dual octet PictureID (M = 1)
  ==============================        ============================

      0 1 2 3 4 5 6 7                       0 1 2 3 4 5 6 7
     +-+-+-+-+-+-+-+-+                     +-+-+-+-+-+-+-+-+
     |X|R|N|S|R| PID | (REQUIRED)          |X|R|N|S|R| PID | (REQUIRED)
     +-+-+-+-+-+-+-+-+                     +-+-+-+-+-+-+-+-+
X:   |I|L|T|K| RSV   | (OPTIONAL)       X: |I|L|T|K| RSV   | (OPTIONAL)
     +-+-+-+-+-+-+-+-+                     +-+-+-+-+-+-+-+-+
I:   |M| PictureID   | (OPTIONAL)       I: |M| PictureID   | (OPTIONAL)
     +-+-+-+-+-+-+-+-+                     +-+-+-+-+-+-+-+-+
L:   |   TL0PICIDX   | (OPTIONAL)          |   PictureID   |
     +-+-+-+-+-+-+-+-+                     +-+-+-+-+-+-+-+-+
T/K: |TID|Y| KEYIDX  | (OPTIONAL)       L: |   TL0PICIDX   | (OPTIONAL)
     +-+-+-+-+-+-+-+-+                     +-+-+-+-+-+-+-+-+
                                      T/K: |TID|Y| KEYIDX  | (OPTIONAL)
                                           +-+-+-+-+-+-+-+-+
 */

namespace RTC
{
	namespace Codecs
	{
		class VP8
		{
		public:
			struct PayloadDescriptor : public RTC::Codecs::PayloadDescriptor
			{
				/* Pure virtual methods inherited from RTC::Codecs::PayloadDescriptor. */
				~PayloadDescriptor() = default;

				void Dump() const override;
				// Rewrite the buffer with the given pictureId and tl0PictureIndex values.
				void Encode(uint8_t* data, uint16_t pictureId, uint8_t tl0PictureIndex) const;
				void Restore(uint8_t* data) const;

				// Mandatory fields.
				uint8_t extended : 1;
				uint8_t nonReference : 1;
				uint8_t start : 1;
				uint8_t partitionIndex : 4;
				// Optional field flags.
				uint8_t i : 1; // PictureID present.
				uint8_t l : 1; // TL0PICIDX present.
				uint8_t t : 1; // TID present.
				uint8_t k : 1; // KEYIDX present.
				// Optional fields.
				uint16_t pictureId;
				uint8_t tl0PictureIndex;
				uint8_t tlIndex : 2;
				uint8_t y : 1;
				uint8_t keyIndex : 5;
				// Parsed values.
				bool isKeyFrame{ false };
				bool hasPictureId{ false };
				bool hasOneBytePictureId{ false };
				bool hasTwoBytesPictureId{ false };
				bool hasTl0PictureIndex{ false };
				bool hasTlIndex{ false };
			};

		public:
			static VP8::PayloadDescriptor* Parse(
			  const uint8_t* data,
			  size_t len,
			  RTC::RtpPacket::FrameMarking* frameMarking = nullptr,
			  uint8_t frameMarkingLen                    = 0);
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
				RTC::SeqManager<uint16_t> pictureIdManager;
				RTC::SeqManager<uint8_t> tl0PictureIndexManager;
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
					return 0u;
				}
				uint8_t GetTemporalLayer() const override
				{
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
