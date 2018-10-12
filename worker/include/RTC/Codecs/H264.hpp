#ifndef MS_RTC_CODECS_H264_HPP
#define MS_RTC_CODECS_H264_HPP

#include "common.hpp"
#include "RTC/Codecs/PayloadDescriptorHandler.hpp"
#include "RTC/RtpDictionaries.hpp"
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
				void Dump() const;

				bool isKeyFrame = { false };
			};

		public:
			static H264::PayloadDescriptor* Parse(uint8_t* data, size_t len);
			static void ProcessRtpPacket(RTC::RtpPacket* packet);

		public:
			class EncodingContext : public RTC::Codecs::EncodingContext
			{
				/* Pure virtual methods inherited from RTC::Codecs::EncodingContext. */
			public:
				void SyncRequired() override;
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

		inline void H264::EncodingContext::SyncRequired(){};

		/* Inline PayloadDescriptorHandler methods */

		inline bool H264::PayloadDescriptorHandler::Encode(
		  RTC::Codecs::EncodingContext* encodingContext, uint8_t* data)
		{
			(void)encodingContext;
			(void)data;

			return true;
		};

		inline void H264::PayloadDescriptorHandler::Restore(uint8_t* data)
		{
			(void)data;

			return;
		}

		inline bool H264::PayloadDescriptorHandler::IsKeyFrame() const
		{
			return this->payloadDescriptor->isKeyFrame;
		};

		inline void H264::PayloadDescriptorHandler::Dump() const
		{
			this->payloadDescriptor->Dump();
		}
	} // namespace Codecs
} // namespace RTC

#endif
