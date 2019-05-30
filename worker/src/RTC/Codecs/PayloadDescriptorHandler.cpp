#define MS_CLASS "RTC::Codecs::PayloadDescriptorHandler"
#define MS_LOG_DEV

#include "RTC/Codecs/PayloadDescriptorHandler.hpp"
#include "Logger.hpp"

namespace RTC
{
	namespace Codecs
	{
		void EncodingContext::SetTargetLayers(int16_t spatialLayer, int16_t temporalLayer)
		{
			MS_TRACE();

			MS_ERROR(
				"BEGIN [spatial:%d, temporal:%d, targetSpatial:%d, targetTemporal:%d, currentSpatial:%d, currentTemporal:%d]",
				spatialLayer,
				temporalLayer,
				this->targetSpatialLayer,
				this->targetTemporalLayer,
				this->currentSpatialLayer,
				this->currentTemporalLayer);

			// TODO: REMOVE
			// auto previousTargetSpatialLayer  = this->targetSpatialLayer;
			// auto previousCurrentSpatialLayer = this->currentSpatialLayer;

			this->targetSpatialLayer = spatialLayer;
			this->targetTemporalLayer = temporalLayer;

			if (this->targetSpatialLayer == -1 && this->targetTemporalLayer == -1)
			{
				MS_ERROR("--- 1");

				this->currentSpatialLayer  = -1;
				this->currentTemporalLayer = -1;
			}
			// Fix current spatial layer in case we downgrade the target spatial
			// layer.
			else if (this->targetSpatialLayer < this->currentSpatialLayer)
			{
				MS_ERROR("--- 2");

				this->currentSpatialLayer  = this->targetSpatialLayer;
				this->currentTemporalLayer = this->targetTemporalLayer;
			}
			// Fix current temporal layer if we stay in same spatial layer and
			// new target temporal layer is lower than current temporal layer.
			else if (
				this->currentSpatialLayer == this->targetSpatialLayer &&
				this->targetTemporalLayer < this->currentTemporalLayer
			)
			{
				MS_ERROR("--- 3");

				this->currentTemporalLayer = this->targetTemporalLayer;
			}
			else
			{
				MS_ERROR("--- nothing");
			}

			MS_ERROR(
				"END   [spatial:%d, temporal:%d, targetSpatial:%d, targetTemporal:%d, currentSpatial:%d, currentTemporal:%d]",
				spatialLayer,
				temporalLayer,
				this->targetSpatialLayer,
				this->targetTemporalLayer,
				this->currentSpatialLayer,
				this->currentTemporalLayer);
		}
	}
}
