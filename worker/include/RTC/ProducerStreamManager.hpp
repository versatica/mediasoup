#ifndef MS_RTC_PRODUCER_STREAM_MANAGER_HPP
#define MS_RTC_PRODUCER_STREAM_MANAGER_HPP

#include "common.hpp"
#include "RTC/ConsumerTypes.hpp"
#include "RTC/RTP/Codecs/PayloadDescriptorHandler.hpp"
#include "RTC/RTP/Packet.hpp"
#include "RTC/RTP/RtpStreamRecv.hpp"
#include "RTC/RtpDictionaries.hpp"
#include "SharedInterface.hpp"
#include <vector>

namespace RTC
{
	class ProducerStreamManager
	{
	public:
		class Listener
		{
		public:
			virtual ~Listener() = default;

		public:
			virtual bool IsActive() const                                              = 0;
			virtual void OnProducerStreamManagerKeyFrameRequested(uint32_t mappedSsrc) = 0;
			virtual void OnProducerStreamManagerNeedBitrateChange()                    = 0;
			virtual void OnProducerStreamManagerLayersChanged()                        = 0;
			virtual void OnProducerStreamManagerClearRetransmissionBuffer()            = 0;
			virtual void OnProducerStreamManagerScore()                                = 0;
		};

		struct RtpPacketProcessResult
		{
			enum class Type : uint8_t
			{
				FORWARD,
				DROP,
				SILENT_DROP,
				BUFFER
			};

			Type type{ Type::FORWARD };

			// Valid when type == FORWARD:
			uint32_t tsOffset{ 0u };
			bool isSyncPacket{ false };
			uint16_t syncSeqValue{ 0u };
			bool shouldSyncEncodingContext{ false };
			bool spatialLayerSwitched{ false };
			bool temporalLayerChanged{ false };
			bool marker{ false };
			bool sendBufferedPackets{ false };
		};

	public:
		ProducerStreamManager(
		  const std::vector<RTC::RtpEncodingParameters>& consumableRtpEncodings,
		  const RTC::ConsumerTypes::VideoLayers& preferredLayers,
		  std::unique_ptr<RTC::RTP::Codecs::EncodingContext> encodingContext,
		  RTC::Media::Kind kind,
		  bool keyFrameSupported,
		  Listener* listener,
		  SharedInterface* shared)
		  : listener(listener),
		    shared(shared),
		    keyFrameSupported(keyFrameSupported),
		    kind(kind),
		    consumableRtpEncodings(consumableRtpEncodings),
		    encodingContext(std::move(encodingContext)),
		    preferredLayers(preferredLayers)
		{
		}
		virtual ~ProducerStreamManager() = default;

	public:
		virtual RTC::ConsumerTypes::VideoLayers GetTargetLayers() const      = 0;
		virtual int16_t GetCurrentSpatialLayer() const                       = 0;
		virtual int16_t GetCurrentTemporalLayer() const                      = 0;
		virtual RTC::RTP::RtpStreamRecv* GetProducerCurrentRtpStream() const = 0;
		virtual RTC::RTP::RtpStreamRecv* GetProducerTargetRtpStream() const  = 0;
		// Returns true if the given packet belongs to the stream currently being
		// forwarded to the consumer. Used by Consumer to decide whether to account
		// a discarded packet in the RTP sequence manager.
		virtual bool IsPacketForCurrentStream(const RTC::RTP::Packet* packet) const = 0;
		RTC::RTP::Codecs::EncodingContext* GetEncodingContext() const
		{
			return this->encodingContext.get();
		}
		const RTC::ConsumerTypes::VideoLayers& GetPreferredLayers() const
		{
			return this->preferredLayers;
		}
		void SetPreferredLayers(const RTC::ConsumerTypes::VideoLayers& layers)
		{
			this->preferredLayers = layers;
		}

		virtual void ProducerRtpStream(RTC::RTP::RtpStreamRecv* rtpStream, uint32_t mappedSsrc)    = 0;
		virtual void ProducerNewRtpStream(RTC::RTP::RtpStreamRecv* rtpStream, uint32_t mappedSsrc) = 0;
		virtual void ProducerRtpStreamScore(
		  RTC::RTP::RtpStreamRecv* rtpStream, uint8_t score, uint8_t previousScore)           = 0;
		virtual void ProducerRtcpSenderReport(RTC::RTP::RtpStreamRecv* rtpStream, bool first) = 0;

		void SetExternallyManagedBitrate()
		{
			this->externallyManagedBitrate = true;
		}

		virtual uint32_t IncreaseLayer(
		  uint32_t bitrate, bool considerLoss, float lossPercentage, uint64_t nowMs) = 0;
		virtual void ApplyLayers(uint64_t rtpStreamActiveMs)                         = 0;
		virtual uint32_t GetDesiredBitrate(uint64_t nowMs) const                     = 0;

		virtual RtpPacketProcessResult ProcessRtpPacket(
		  RTC::RTP::Packet* packet,
		  bool lastSentPacketHasMarker,
		  uint32_t clockRate,
		  uint32_t maxPacketTs) = 0;

		virtual void RequestKeyFrame()                       = 0;
		virtual void RequestKeyFrameForTargetSpatialLayer()  = 0;
		virtual void RequestKeyFrameForCurrentSpatialLayer() = 0;

		virtual void UpdateTargetLayers(int16_t newTargetSpatialLayer, int16_t newTargetTemporalLayer) = 0;
		virtual bool RecalculateTargetLayers(RTC::ConsumerTypes::VideoLayers& newTargetLayers) const = 0;

		void MayChangeLayers(bool force)
		{
			RTC::ConsumerTypes::VideoLayers newTargetLayers;

			if (RecalculateTargetLayers(newTargetLayers))
			{
				// If bitrate externally managed, don't bother the transport unless
				// the newTargetSpatialLayer has changed (or force is true).
				// This is because, if bitrate is externally managed, the target temporal
				// layer is managed by the available given bitrate so the transport
				// will let us change it when it considers.
				if (this->externallyManagedBitrate)
				{
					if (newTargetLayers.spatial != GetTargetLayers().spatial || force)
					{
						this->listener->OnProducerStreamManagerNeedBitrateChange();
					}
				}
				else
				{
					UpdateTargetLayers(newTargetLayers.spatial, newTargetLayers.temporal);
				}
			}
		}
		virtual void OnTransportConnected()    = 0;
		virtual void OnTransportDisconnected() = 0;
		virtual void OnPaused()                = 0;
		virtual void OnResumed()               = 0;

	protected:
		virtual bool IsActive() const = 0;

		// Passed by argument.
		Listener* listener{ nullptr };
		SharedInterface* shared{ nullptr };
		bool keyFrameSupported{ false };
		RTC::Media::Kind kind{};
		std::vector<RTC::RtpEncodingParameters> consumableRtpEncodings;

		// Encoding context.
		std::unique_ptr<RTC::RTP::Codecs::EncodingContext> encodingContext;

		// Externally managed bitrate.
		bool externallyManagedBitrate{ false };

		// Layer preferences.
		RTC::ConsumerTypes::VideoLayers preferredLayers;
		RTC::ConsumerTypes::VideoLayers provisionalTargetLayers;

		// Sync state.
		bool syncRequired{ false };
	};
} // namespace RTC

#endif
