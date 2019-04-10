#ifndef MS_RTC_PAYLOAD_DESCRIPTOR_HANDLER_HPP
#define MS_RTC_PAYLOAD_DESCRIPTOR_HANDLER_HPP

#include "common.hpp"
#include <limits> // std::numeric_limits

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

		// Encoding context used by PayloadDescriptorHandler to properly rewrite the PayloadDescriptor.
		class EncodingContext
		{
		public:
			struct Preferences
			{
				Preferences() = default;

				uint8_t qualityLayer{ std::numeric_limits<uint8_t>::max() };
				uint8_t spatialLayer{ std::numeric_limits<uint8_t>::max() };
				uint8_t temporalLayer{ std::numeric_limits<uint8_t>::max() };
			};

		public:
			virtual void SyncRequired() = 0;
			virtual void SetPreferences(Preferences preferences);

		public:
			virtual ~EncodingContext() = default;

		public:
			Preferences preferences;
		};

		class PayloadDescriptorHandler
		{
		public:
			virtual void Dump() const                                                 = 0;
			virtual bool Encode(RTC::Codecs::EncodingContext* context, uint8_t* data) = 0;
			virtual void Restore(uint8_t* data)                                       = 0;
			virtual uint8_t GetSpatialLayer() const                                   = 0;
			virtual uint8_t GetTemporalLayer() const                                  = 0;
			virtual bool IsKeyFrame() const                                           = 0;

		public:
			virtual ~PayloadDescriptorHandler() = default;
		};

		inline void EncodingContext::SetPreferences(Preferences preferences)
		{
			this->preferences = preferences;
		}
	} // namespace Codecs
} // namespace RTC

#endif
