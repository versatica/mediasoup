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
			EncodingContext(RTC::Codecs::EncodingContext::Params& params);
			virtual ~EncodingContext() = default;

		public:
			uint8_t GetSpatialLayers() const;
			uint8_t GetTemporalLayers() const;
			bool IsKSvc() const;
			int16_t GetTargetSpatialLayer() const;
			int16_t GetTargetTemporalLayer() const;
			int16_t GetCurrentSpatialLayer() const;
			int16_t GetCurrentTemporalLayer() const;
			void SetTargetSpatialLayer(int16_t spatialLayer);
			void SetTargetTemporalLayer(int16_t temporalLayer);
			void SetCurrentSpatialLayer(int16_t spatialLayer);
			void SetCurrentTemporalLayer(int16_t temporalLayer);
			virtual void SyncRequired() = 0;

		private:
			Params params;
			int16_t targetSpatialLayer{ -1 };
			int16_t targetTemporalLayer{ -1 };
			int16_t currentSpatialLayer{ -1 };
			int16_t currentTemporalLayer{ -1 };
		};

		/* Inline instance methods. */

		inline EncodingContext::EncodingContext(RTC::Codecs::EncodingContext::Params& params)
		  : params(params)
		{
		}

		inline uint8_t EncodingContext::GetSpatialLayers() const
		{
			return this->params.spatialLayers;
		}

		inline uint8_t EncodingContext::GetTemporalLayers() const
		{
			return this->params.temporalLayers;
		}

		inline bool EncodingContext::IsKSvc() const
		{
			return this->params.ksvc;
		}

		inline int16_t EncodingContext::GetTargetSpatialLayer() const
		{
			return this->targetSpatialLayer;
		}

		inline int16_t EncodingContext::GetTargetTemporalLayer() const
		{
			return this->targetTemporalLayer;
		}

		inline int16_t EncodingContext::GetCurrentSpatialLayer() const
		{
			return this->currentSpatialLayer;
		}

		inline int16_t EncodingContext::GetCurrentTemporalLayer() const
		{
			return this->currentTemporalLayer;
		}

		inline void EncodingContext::SetTargetSpatialLayer(int16_t spatialLayer)
		{
			this->targetSpatialLayer = spatialLayer;
		}

		inline void EncodingContext::SetTargetTemporalLayer(int16_t temporalLayer)
		{
			this->targetTemporalLayer = temporalLayer;
		}

		inline void EncodingContext::SetCurrentSpatialLayer(int16_t spatialLayer)
		{
			this->currentSpatialLayer = spatialLayer;
		}

		inline void EncodingContext::SetCurrentTemporalLayer(int16_t temporalLayer)
		{
			this->currentTemporalLayer = temporalLayer;
		}

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
