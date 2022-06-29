#define MS_CLASS "RTC::Codecs::Opus"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/Codecs/Opus.hpp"
#include "Logger.hpp"

namespace RTC
{
	namespace Codecs
	{
		/* Class methods. */

		Opus::PayloadDescriptor* Opus::Parse(const uint8_t* data, size_t len)
		{
			MS_TRACE();

			std::unique_ptr<PayloadDescriptor> payloadDescriptor(new PayloadDescriptor());

			// libopus generates a single byte payload (TOC, no frames) to generate DTX.
			if (len == 1)
			{
				payloadDescriptor->isDtx = true;
			}

			return payloadDescriptor.release();
		}

		void Opus::ProcessRtpPacket(RTC::RtpPacket* packet)
		{
			MS_TRACE();

			auto* data = packet->GetPayload();
			auto len   = packet->GetPayloadLength();

			PayloadDescriptor* payloadDescriptor = Opus::Parse(data, len);

			auto* payloadDescriptorHandler = new PayloadDescriptorHandler(payloadDescriptor);

			packet->SetPayloadDescriptorHandler(payloadDescriptorHandler);
		}

		/* Instance methods. */

		void Opus::PayloadDescriptor::Dump() const
		{
			MS_TRACE();

			MS_DUMP("<PayloadDescriptor>");
			MS_DUMP("  isDtx : %s", this->isDtx ? "true" : "false");
			MS_DUMP("</PayloadDescriptor>");
		}

		Opus::PayloadDescriptorHandler::PayloadDescriptorHandler(Opus::PayloadDescriptor* payloadDescriptor)
		{
			MS_TRACE();

			this->payloadDescriptor.reset(payloadDescriptor);
		}

		bool Opus::PayloadDescriptorHandler::Process(
		  RTC::Codecs::EncodingContext* encodingContext, uint8_t* data, bool& /*marker*/)
		{
			MS_TRACE();

			auto* context = static_cast<RTC::Codecs::Opus::EncodingContext*>(encodingContext);

			if (this->payloadDescriptor->isDtx && context->GetIgnoreDtx())
			{
				return false;
			}
			else
			{
				return true;
			}
		};
	} // namespace Codecs
} // namespace RTC
