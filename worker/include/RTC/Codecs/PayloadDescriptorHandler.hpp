#ifndef MS_RTC_PAYLOAD_DESCRIPTOR_HANDLER_HPP
#define MS_RTC_PAYLOAD_DESCRIPTOR_HANDLER_HPP

#include "common.hpp"

namespace RTC
{
	namespace Codecs
	{
		// Codec payload descriptor.
		struct PayloadDescriptor
		{
			virtual ~PayloadDescriptor() = default;
			virtual void Dump() const    = 0;
		};

		// Encoding context used by PayloadDescriptionHandler to properly rewrite the PayloadDescriptor.
		class EncodingContext
		{
		public:
			virtual void SyncRequired() = 0;
		};

		class PayloadDescriptorHandler
		{
		public:
			virtual void Dump() const                                                 = 0;
			virtual void Encode(RTC::Codecs::EncodingContext* context, uint8_t* data) = 0;
			virtual void Restore(uint8_t* data)                                       = 0;
			virtual bool IsKeyFrame() const                                           = 0;

		public:
			virtual ~PayloadDescriptorHandler() = default;
		};
	} // namespace Codecs
} // namespace RTC

#endif
