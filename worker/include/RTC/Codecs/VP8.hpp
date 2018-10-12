#ifndef MS_RTC_CODECS_VP8_HPP
#define MS_RTC_CODECS_VP8_HPP

#include "common.hpp"
#include "RTC/Codecs/PayloadDescriptorHandler.hpp"
#include "RTC/RtpDictionaries.hpp"
#include "RTC/RtpPacket.hpp"
#include "RTC/SeqManager.hpp"

/* RFC 7741
 * VP8 Payload Descriptor
 *

  Single octet PictureID (M = 0)        Dual octet PictureID (M = 1)
 ===============================        ============================

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
				void Dump() const;

				// Rewrite the buffer with the given pictureId and tl0PictureIndex values.
				void Encode(uint8_t* data, uint16_t pictureId, uint8_t tl0PictureIndex) const;
				void Restore(uint8_t* data) const;

				// mandatory fields.
				uint8_t extended : 1;
				uint8_t nonReference : 1;
				uint8_t start : 1;
				uint8_t partitionIndex : 4;
				// optional field flags.
				uint8_t i : 1; // PictureID present.
				uint8_t l : 1; // TL0PICIDX present.
				uint8_t t : 1; // TID present.
				uint8_t k : 1; // KEYIDX present.
				// optional fields.
				uint16_t pictureId;
				uint8_t tl0PictureIndex;
				uint8_t tlIndex : 2;
				uint8_t y : 1;
				uint8_t keyIndex : 5;

				bool isKeyFrame           = { false };
				bool hasPictureId         = { false };
				bool hasOneBytePictureId  = { false };
				bool hasTwoBytesPictureId = { false };
				bool hasTl0PictureIndex   = { false };
				bool hasTlIndex           = { false };
			};

		public:
			static VP8::PayloadDescriptor* Parse(uint8_t* data, size_t len);
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
				SeqManager<uint16_t> pictureIdManager;
				SeqManager<uint8_t> tl0PictureIndexManager;
				bool syncRequired{ false };
			};

			class PayloadDescriptorHandler : public RTC::Codecs::PayloadDescriptorHandler
			{
			public:
				explicit PayloadDescriptorHandler(PayloadDescriptor* payloadDescriptor);
				~PayloadDescriptorHandler() = default;

			public:
				void Dump() const;
				bool Encode(RTC::Codecs::EncodingContext* context, uint8_t* data);
				void Restore(uint8_t* data);
				bool IsKeyFrame() const;

			private:
				std::unique_ptr<PayloadDescriptor> payloadDescriptor;
			};
		};

		/* Inline EncondingContext methods */

		inline void VP8::EncodingContext::SyncRequired()
		{
			this->syncRequired = true;
		};

		/* Inline PayloadDescriptorHandler methods */

		inline bool VP8::PayloadDescriptorHandler::IsKeyFrame() const
		{
			return this->payloadDescriptor->isKeyFrame;
		};

		inline void VP8::PayloadDescriptorHandler::Dump() const
		{
			this->payloadDescriptor->Dump();
		}
	} // namespace Codecs
} // namespace RTC

#endif
