#ifndef MS_RTC_CODECS_VP9_HPP
#define MS_RTC_CODECS_VP9_HPP

#include "common.hpp"
#include "RTC/Codecs/PayloadDescriptorHandler.hpp"
#include "RTC/RtpPacket.hpp"
#include "RTC/SeqManager.hpp"

/* https://tools.ietf.org/html/draft-ietf-payload-vp9-06
 * VP9 Payload Descriptor
 *

   Flexible mode (with the F bit below set to 1)
   =============================================

      0 1 2 3 4 5 6 7
     +-+-+-+-+-+-+-+-+
     |I|P|L|F|B|E|V|-| (REQUIRED)
     +-+-+-+-+-+-+-+-+
I:   |M| PICTURE ID  | (REQUIRED)
     +-+-+-+-+-+-+-+-+
M:   | EXTENDED PID  | (RECOMMENDED)
     +-+-+-+-+-+-+-+-+
L:   | TID |U| SID |D| (CONDITIONALLY RECOMMENDED)
     +-+-+-+-+-+-+-+-+                             -\
P,F: | P_DIFF      |N| (CONDITIONALLY REQUIRED)    - up to 3 times
     +-+-+-+-+-+-+-+-+                             -/
V:   | SS            |
     | ..            |
     +-+-+-+-+-+-+-+-+

   Non-flexible mode (with the F bit below set to 0)
   =================================================

      0 1 2 3 4 5 6 7
     +-+-+-+-+-+-+-+-+
     |I|P|L|F|B|E|V|-| (REQUIRED)
     +-+-+-+-+-+-+-+-+
I:   |M| PICTURE ID  | (RECOMMENDED)
     +-+-+-+-+-+-+-+-+
M:   | EXTENDED PID  | (RECOMMENDED)
     +-+-+-+-+-+-+-+-+
L:   | TID |U| SID |D| (CONDITIONALLY RECOMMENDED)
     +-+-+-+-+-+-+-+-+
     |   TL0PICIDX   | (CONDITIONALLY REQUIRED)
     +-+-+-+-+-+-+-+-+
V:   | SS            |
     | ..            |
     +-+-+-+-+-+-+-+-+

*/

namespace RTC
{
	namespace Codecs
	{
		class VP9
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

				// Header.
				uint8_t i : 1; // I: Picture ID (PID) present.
				uint8_t p : 1; // P: Inter-picture predicted layer frame.
				uint8_t l : 1; // L: Layer indices present.
				uint8_t f : 1; // F: Flexible mode.
				uint8_t b : 1; // B: Start of a layer frame.
				uint8_t e : 1; // E: End of a layer frame.
				uint8_t v : 1; // V: Scalability structure (SS) data present.

				// Extension fields.
				uint16_t pictureId { 0 };
				uint8_t tlIndex { 0 };
				uint8_t slIndex { 0 };
				uint8_t tl0PictureIndex;
				uint8_t switchingUpPoint : 1;
				uint8_t interLayerDependency : 1;

				// Parsed values.
				bool isKeyFrame{ false };
				bool hasPictureId{ false };
				bool hasOneBytePictureId{ false };
				bool hasTwoBytesPictureId{ false };
				bool hasTl0PictureIndex{ false };
				bool hasTlIndex{ false };
				bool hasSlIndex{ false };
			};

		public:
			static VP9::PayloadDescriptor* Parse(
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

			public:
				RTC::SeqManager<uint16_t> pictureIdManager;
				RTC::SeqManager<uint8_t> tl0PictureIndexManager;
				bool syncRequired{ false };
				uint8_t currentTemporalLayer{ std::numeric_limits<uint8_t>::max() };
			};

			class PayloadDescriptorHandler : public RTC::Codecs::PayloadDescriptorHandler
			{
			public:
				explicit PayloadDescriptorHandler(PayloadDescriptor* payloadDescriptor);
				~PayloadDescriptorHandler() = default;

			public:
				void Dump() const override;
				bool Encode(RTC::Codecs::EncodingContext* encodingContext, uint8_t* data) override;
				void Restore(uint8_t* data) override;
				uint8_t GetSpatialLayer() const override;
				uint8_t GetTemporalLayer() const override;
				bool IsKeyFrame() const override;

			private:
				std::unique_ptr<PayloadDescriptor> payloadDescriptor;
			};
		};

		/* Inline EncondingContext methods */

		inline void VP9::EncodingContext::SyncRequired()
		{
			this->syncRequired = true;
		}

		/* Inline PayloadDescriptorHandler methods */

		inline uint8_t VP9::PayloadDescriptorHandler::GetSpatialLayer() const
		{
			return this->payloadDescriptor->hasSlIndex ? this->payloadDescriptor->slIndex : 0u;
		}

		inline uint8_t VP9::PayloadDescriptorHandler::GetTemporalLayer() const
		{
			return this->payloadDescriptor->hasTlIndex ? this->payloadDescriptor->tlIndex : 0u;
		}

		inline bool VP9::PayloadDescriptorHandler::IsKeyFrame() const
		{
			return this->payloadDescriptor->isKeyFrame;
		}

		inline void VP9::PayloadDescriptorHandler::Dump() const
		{
			this->payloadDescriptor->Dump();
		}
	} // namespace Codecs
} // namespace RTC

#endif
