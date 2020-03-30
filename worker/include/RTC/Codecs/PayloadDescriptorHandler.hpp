#ifndef MS_RTC_CODECS_PAYLOAD_DESCRIPTOR_HANDLER_HPP
#define MS_RTC_CODECS_PAYLOAD_DESCRIPTOR_HANDLER_HPP

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

		// Encoding context used by PayloadDescriptorHandler to properly rewrite the
		// PayloadDescriptor.
		class EncodingContext
		{
		public:
			struct Params
			{
				uint8_t spatialLayers{ 1u };
				uint8_t temporalLayers{ 1u };
				bool ksvc{ false };
			};

		public:
			explicit EncodingContext(RTC::Codecs::EncodingContext::Params& params) : params(params)
			{
			}
			virtual ~EncodingContext() = default;

		public:
			uint8_t GetSpatialLayers() const
			{
				return this->params.spatialLayers;
			}
			uint8_t GetTemporalLayers() const
			{
				return this->params.temporalLayers;
			}
			bool IsKSvc() const
			{
				return this->params.ksvc;
			}
			int16_t GetTargetSpatialLayer() const
			{
				return this->targetSpatialLayer;
			}
			int16_t GetTargetTemporalLayer() const
			{
				return this->targetTemporalLayer;
			}
			int16_t GetCurrentSpatialLayer() const
			{
				return this->currentSpatialLayer;
			}
			int16_t GetCurrentTemporalLayer() const
			{
				return this->currentTemporalLayer;
			}
			void SetTargetSpatialLayer(int16_t spatialLayer)
			{
				this->targetSpatialLayer = spatialLayer;
			}
			void SetTargetTemporalLayer(int16_t temporalLayer)
			{
				this->targetTemporalLayer = temporalLayer;
			}
			void SetCurrentSpatialLayer(int16_t spatialLayer)
			{
				this->currentSpatialLayer = spatialLayer;
			}
			void SetCurrentTemporalLayer(int16_t temporalLayer)
			{
				this->currentTemporalLayer = temporalLayer;
			}
			virtual void SyncRequired() = 0;

		private:
			Params params;
			int16_t targetSpatialLayer{ -1 };
			int16_t targetTemporalLayer{ -1 };
			int16_t currentSpatialLayer{ -1 };
			int16_t currentTemporalLayer{ -1 };
		};

		class PayloadDescriptorHandler
		{
		public:
			virtual ~PayloadDescriptorHandler() = default;

		public:
			virtual void Dump() const                                                                = 0;
			virtual bool Process(RTC::Codecs::EncodingContext* context, uint8_t* data, bool& marker) = 0;
			virtual void Restore(uint8_t* data)                                                      = 0;
			virtual uint8_t GetSpatialLayer() const                                                  = 0;
			virtual uint8_t GetTemporalLayer() const                                                 = 0;
			virtual bool IsKeyFrame() const                                                          = 0;
		};
	} // namespace Codecs
} // namespace RTC

#endif
