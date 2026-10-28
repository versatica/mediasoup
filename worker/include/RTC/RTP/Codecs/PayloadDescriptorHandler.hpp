#ifndef MS_RTC_RTP_CODECS_PAYLOAD_DESCRIPTOR_HANDLER_HPP
#define MS_RTC_RTP_CODECS_PAYLOAD_DESCRIPTOR_HANDLER_HPP

#include "common.hpp"
#include "RTC/ConsumerTypes.hpp"
#include "RTC/RTP/Codecs/DependencyDescriptor.hpp"
#include "RTC/SeqManager.hpp"
#include <deque>

namespace RTC
{
	namespace RTP
	{
		class Packet;

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

			class FrameIdList
			{
				static constexpr uint16_t MaxCurrentLayerFrameIdNum{ 1000 };

			public:
				explicit FrameIdList() = default;

				~FrameIdList()
				{
					this->layerChanges.clear();
				}

				void Push(uint16_t frameId, int16_t layer)
				{
					for (const auto& it : this->layerChanges)
					{
						// Layers can be changed only with ordered frameId values.
						// If frameId is lower than the previous one, then it has rolled over the max value.
						uint16_t diff = frameId > it.first
						                  ? frameId - it.first
						                  : frameId + RTC::SeqManager<uint16_t, 15>::MaxValue - it.first;

						if (diff > MaxCurrentLayerFrameIdNum)
						{
							this->layerChanges.pop_front();
						}
						else
						{
							break;
						}
					}

					this->layerChanges.emplace_back(frameId, layer);
				}

				int16_t GetLayer(uint16_t frameId) const
				{
					if (this->layerChanges.size() <= 1)
					{
						return -1;
					}

					for (auto it = std::next(this->layerChanges.begin()); it != this->layerChanges.end(); ++it)
					{
						if (RTC::SeqManager<uint16_t, 15>::IsSeqHigherThan(it->first, frameId))
						{
							return std::prev(it)->second;
						}
					}

					return -1;
				}

			private:
				// List populated with the spatial/temporal layer changes
				// indexed by the corresponding frameId.
				std::deque<std::pair<uint16_t, int16_t>> layerChanges;
			};

			// Encoding context used by PayloadDescriptorHandler to properly rewrite the
			// PayloadDescriptor.
			class EncodingContext
			{
			public:
				struct Params
				{
					uint8_t spatialLayers{ 1 };
					uint8_t temporalLayers{ 1 };
					bool ksvc{ false };
					/*
					 * Whether the Consumer is given one of several streams rather than
					 * layers of a single one, so that choosing the spatial layer means
					 * choosing the stream and is not up to the payload descriptor.
					 */
					bool simulcast{ false };
				};

			public:
				explicit EncodingContext(Codecs::EncodingContext::Params& params) : params(params)
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
				/*
				 * Whether the spatial layer is chosen by picking the stream to forward,
				 * in which case the target spatial layer of this context is never set
				 * and the payload descriptor must not filter on it.
				 */
				bool IsSimulcast() const
				{
					return this->params.simulcast;
				}
				int16_t GetTargetSpatialLayer() const
				{
					return this->targetLayers.spatial;
				}
				int16_t GetTargetTemporalLayer() const
				{
					return this->targetLayers.temporal;
				}
				const RTC::ConsumerTypes::VideoLayers& GetTargetLayers() const
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
				const RTC::ConsumerTypes::VideoLayers& GetCurrentLayers() const
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
				void SetCurrentSpatialLayer(int16_t spatialLayer, uint16_t frameId)
				{
					if (this->currentLayers.spatial == spatialLayer)
					{
						return;
					}

					this->spatialLayerFrameIdList.Push(frameId, spatialLayer);
					this->currentLayers.spatial = spatialLayer;
				}
				void SetCurrentTemporalLayer(int16_t temporalLayer, uint16_t frameId)
				{
					if (this->currentLayers.temporal == temporalLayer)
					{
						return;
					}

					this->temporalLayerFrameIdList.Push(frameId, temporalLayer);
					this->currentLayers.temporal = temporalLayer;
				}
				int16_t GetSpatialLayerForFrameId(uint16_t frameId) const
				{
					int16_t layer = this->spatialLayerFrameIdList.GetLayer(frameId);

					if (layer > -1)
					{
						return layer;
					}

					return this->currentLayers.spatial;
				}
				int16_t GetTemporalLayerForFrameId(uint16_t frameId) const
				{
					int16_t layer = this->temporalLayerFrameIdList.GetLayer(frameId);

					if (layer > -1)
					{
						return layer;
					}

					return this->currentLayers.temporal;
				}
				/**
				 * Whether the given frame id is older than the newest one seen, this is,
				 * whether the packet carrying it was reordered and the current layers are
				 * no longer the ones its frame was judged against.
				 *
				 * NOTE: Each codec gives the frame id it has. The Dependency Descriptor
				 * numbers every frame, so in SVC the spatial layers of a picture get one
				 * each, while VP8 and VP9 number the whole picture.
				 */
				bool IsOldFrameId(uint16_t frameId) const
				{
					return this->maxFrameId.has_value() &&
					       RTC::SeqManager<uint16_t, 15>::IsSeqLowerThan(frameId, this->maxFrameId.value());
				}
				void UpdateMaxFrameId(uint16_t frameId)
				{
					if (
					  !this->maxFrameId.has_value() ||
					  RTC::SeqManager<uint16_t, 15>::IsSeqHigherThan(frameId, this->maxFrameId.value()))
					{
						this->maxFrameId = frameId;
					}
				}

			private:
				const Params params;
				RTC::ConsumerTypes::VideoLayers targetLayers;
				RTC::ConsumerTypes::VideoLayers currentLayers;
				bool ignoreDtx{ false };
				// Newest frame number seen, to tell reordered packets apart.
				std::optional<uint16_t> maxFrameId{ std::nullopt };

			private:
				// List of spatial/temporal layer changes indexed by the corresponding frameId.
				FrameIdList spatialLayerFrameIdList;
				FrameIdList temporalLayerFrameIdList;
			};

			class PayloadDescriptorHandler
			{
			public:
				virtual ~PayloadDescriptorHandler() = default;

			public:
				virtual void Dump(int indentation = 0) const                                      = 0;
				virtual bool Process(EncodingContext* context, RTP::Packet* packet, bool& marker) = 0;
				virtual std::unique_ptr<PayloadDescriptor::Encoder> GetEncoder() const            = 0;
				virtual void Encode(RTP::Packet* packet, PayloadDescriptor::Encoder* encoder)     = 0;
				virtual void Restore(RTP::Packet* packet)                                         = 0;
				virtual uint8_t GetSpatialLayer() const                                           = 0;
				virtual uint8_t GetTemporalLayer() const                                          = 0;
				virtual bool IsKeyFrame() const                                                   = 0;
				/*
				 * Bytes the Dependency Descriptor of this packet may take once rewritten,
				 * or 0 if this codec never rewrites it.
				 */
				virtual uint8_t GetDependencyDescriptorMaxLength() const
				{
					return 0;
				}
			};
		} // namespace Codecs
	} // namespace RTP
} // namespace RTC

#endif
