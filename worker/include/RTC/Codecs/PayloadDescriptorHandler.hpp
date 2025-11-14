#ifndef MS_RTC_CODECS_PAYLOAD_DESCRIPTOR_HANDLER_HPP
#define MS_RTC_CODECS_PAYLOAD_DESCRIPTOR_HANDLER_HPP

#include "common.hpp"
#include "DependencyDescriptor.hpp"
#include "RTC/ConsumerTypes.hpp"
#include "RTC/SeqManager.hpp"
#include <deque>

namespace RTC
{
	class RtpPacket;

	using namespace ConsumerTypes;

	namespace Codecs
	{
		// Codec payload descriptor.
		struct PayloadDescriptor
		{
			struct Encoder
			{
				virtual ~Encoder() = default;
			};

			virtual ~PayloadDescriptor()                 = default;
			virtual void Dump(int indentation = 0) const = 0;
		};

		class PictureIdList
		{
			static constexpr uint16_t MaxCurrentLayerPictureIdNum{ 1000u };

		public:
			explicit PictureIdList() = default;

			~PictureIdList()
			{
				this->layerChanges.clear();
			}

			void Push(uint16_t pictureId, int16_t layer)
			{
				for (const auto& it : this->layerChanges)
				{
					// Layers can be changed only with ordered pictureId values.
					// If pictureId is lower than the previous one, then it has rolled over the max value.
					uint16_t diff = pictureId > it.first
					                  ? pictureId - it.first
					                  : pictureId + RTC::SeqManager<uint16_t, 15>::MaxValue - it.first;

					if (diff > MaxCurrentLayerPictureIdNum)
					{
						this->layerChanges.pop_front();
					}
					else
					{
						break;
					}
				}

				this->layerChanges.emplace_back(pictureId, layer);
			}

			int16_t GetLayer(uint16_t pictureId) const
			{
				if (this->layerChanges.size() <= 1)
				{
					return -1;
				}

				for (auto it = std::next(this->layerChanges.begin()); it != this->layerChanges.end(); ++it)
				{
					if (RTC::SeqManager<uint16_t, 15>::IsSeqHigherThan(it->first, pictureId))
					{
						return std::prev(it)->second;
					}
				}

				return -1;
			}

		private:
			// List populated with the spatial/temporal layer changes
			// indexed by the corresponding pictureId.
			std::deque<std::pair<uint16_t, int16_t>> layerChanges;
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
				return this->targetLayers.spatial;
			}
			int16_t GetTargetTemporalLayer() const
			{
				return this->targetLayers.temporal;
			}
			const VideoLayers& GetTargetLayers() const
			{
				return this->targetLayers;
			}
			int16_t GetCurrentSpatialLayer() const
			{
				return this->currentLayers.spatial;
			}
			int16_t GetCurrentTemporalLayer() const
			{
				return this->currentLayers.temporal;
			}
			const VideoLayers& GetCurrentLayers() const
			{
				return this->currentLayers;
			}
			bool GetIgnoreDtx() const
			{
				return this->ignoreDtx;
			}
			void SetTargetSpatialLayer(int16_t spatialLayer)
			{
				this->targetLayers.spatial = spatialLayer;
			}
			void SetTargetTemporalLayer(int16_t temporalLayer)
			{
				this->targetLayers.temporal = temporalLayer;
			}
			void SetCurrentSpatialLayer(int16_t spatialLayer)
			{
				this->currentLayers.spatial = spatialLayer;
			}
			void SetCurrentTemporalLayer(int16_t temporalLayer)
			{
				this->currentLayers.temporal = temporalLayer;
			}
			void SetIgnoreDtx(bool ignoreDtx)
			{
				this->ignoreDtx = ignoreDtx;
			}
			virtual void SyncRequired() = 0;
			void SetCurrentSpatialLayer(int16_t spatialLayer, uint16_t pictureId)
			{
				if (this->currentLayers.spatial == spatialLayer)
				{
					return;
				}

				this->spatialLayerPictureIdList.Push(pictureId, spatialLayer);
				this->currentLayers.spatial = spatialLayer;
			}
			void SetCurrentTemporalLayer(int16_t temporalLayer, uint16_t pictureId)
			{
				if (this->currentLayers.temporal == temporalLayer)
				{
					return;
				}

				this->temporalLayerPictureIdList.Push(pictureId, temporalLayer);
				this->currentLayers.temporal = temporalLayer;
			}
			int16_t GetSpatialLayerForPictureId(uint16_t pictureId) const
			{
				int16_t layer = this->spatialLayerPictureIdList.GetLayer(pictureId);

				if (layer > -1)
				{
					return layer;
				}

				return this->currentLayers.spatial;
			}
			int16_t GetTemporalLayerForPictureId(uint16_t pictureId) const
			{
				int16_t layer = this->temporalLayerPictureIdList.GetLayer(pictureId);

				if (layer > -1)
				{
					return layer;
				}

				return this->currentLayers.temporal;
			}

		private:
			Params params;
			VideoLayers targetLayers;
			VideoLayers currentLayers;
			bool ignoreDtx{ false };

		private:
			// List of spatial/temporal layer changes indexed by the corresponding pictureId.
			PictureIdList spatialLayerPictureIdList;
			PictureIdList temporalLayerPictureIdList;
		};

		class PayloadDescriptorHandler
		{
		public:
			virtual ~PayloadDescriptorHandler() = default;

		public:
			virtual void Dump(int indentation = 0) const = 0;
			virtual bool Process(
			  RTC::Codecs::EncodingContext* context, RTC::RtpPacket* packet, bool& marker) = 0;
			virtual void RtpPacketCloned(RtpPacket* packet)                                = 0;
			virtual std::unique_ptr<PayloadDescriptor::Encoder> GetEncoder() const         = 0;
			virtual void Encode(RtpPacket* packet, PayloadDescriptor::Encoder* encoder)    = 0;
			virtual void Restore(RtpPacket* packet)                                        = 0;
			virtual uint8_t GetSpatialLayer() const                                        = 0;
			virtual uint8_t GetTemporalLayer() const                                       = 0;
			virtual bool IsKeyFrame() const                                                = 0;
		};
	} // namespace Codecs
} // namespace RTC

#endif
