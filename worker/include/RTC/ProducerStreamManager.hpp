#ifndef MS_RTC_PRODUCER_STREAM_MANAGER_HPP
#define MS_RTC_PRODUCER_STREAM_MANAGER_HPP

#include "common.hpp"
#include "RTC/BWE/BitrateAllocator.hpp"
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
			virtual bool IsActive() const = 0;

			virtual void OnProducerStreamManagerKeyFrameRequested(uint32_t mappedSsrc) = 0;

			virtual void OnProducerStreamManagerNeedBitrateChange() = 0;

			virtual void OnProducerStreamManagerLayersChanged() = 0;

			virtual void OnProducerStreamManagerClearRetransmissionBuffer() = 0;

			virtual void OnProducerStreamManagerScore() = 0;
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
			uint32_t tsOffset{ 0 };
			bool isSyncPacket{ false };
			uint16_t syncSeqValue{ 0 };
			bool shouldSyncEncodingContext{ false };
			bool spatialLayerSwitched{ false };
			bool temporalLayerChanged{ false };
			bool marker{ false };
			bool sendBufferedPackets{ false };
			/**
			 * How the payload descriptor handler rewrote the packet for this
			 * Consumer, so that its retransmissions are rewritten the very same way.
			 */
			std::unique_ptr<RTC::RTP::Codecs::PayloadDescriptor::Encoder> encoder{ nullptr };
		};

	public:
		ProducerStreamManager(
		  const std::vector<RTC::RtpEncodingParameters>& consumableRtpEncodings,
		  const RTC::ConsumerTypes::VideoLayers& preferredLayers,
		  std::unique_ptr<RTC::RTP::Codecs::EncodingContext> encodingContext,
		  RTC::Media::Kind kind,
		  bool keyFrameSupported,
		  Listener* listener,
		  SharedInterface* shared);

		virtual ~ProducerStreamManager() = default;

	public:
		virtual RTC::ConsumerTypes::VideoLayers GetTargetLayers() const = 0;

		virtual int16_t GetCurrentSpatialLayer() const = 0;

		virtual int16_t GetCurrentTemporalLayer() const = 0;

		/**
		 * RTP streams of the Producer that the Consumer is forwarding.
		 */
		virtual std::vector<RTC::RTP::RtpStreamRecv*> GetProducerRtpStreams() const = 0;

		virtual RTC::RTP::RtpStreamRecv* GetProducerCurrentRtpStream() const = 0;

		virtual RTC::RTP::RtpStreamRecv* GetProducerTargetRtpStream() const = 0;

		/**
		 * Returns true if the given packet belongs to the stream currently being
		 * forwarded to the consumer. Used by Consumer to decide whether to account
		 * a discarded packet in the RTP sequence manager.
		 */
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

		virtual void ProducerRtpStream(RTC::RTP::RtpStreamRecv* rtpStream, uint32_t mappedSsrc) = 0;

		virtual void ProducerNewRtpStream(RTC::RTP::RtpStreamRecv* rtpStream, uint32_t mappedSsrc) = 0;

		virtual void ProducerRtpStreamScore(
		  RTC::RTP::RtpStreamRecv* rtpStream, uint8_t score, uint8_t previousScore) = 0;

		virtual void ProducerRtcpSenderReport(RTC::RTP::RtpStreamRecv* rtpStream, bool first) = 0;

		/**
		 * Only spatial layers within a same RTP stream can start or stop separately,
		 * so just SVC cares about this.
		 */
		virtual void ProducerSpatialLayerActivityChanged(
		  RTC::RTP::RtpStreamRecv* rtpStream, uint8_t spatialLayer, bool isActive);

		/**
		 * Tell whether a bitrate allocation is running for this Transport, which is
		 * what decides who applies the layers worked out here.
		 *
		 * @remarks
		 * - While it is false there is nobody to ask for bitrate, so this one
		 *   applies what it works out and tells the Transport nothing.
		 */
		void SetBweManaged(bool bweManaged)
		{
			this->bweManaged = bweManaged;
		}

		/**
		 * The points this one may run at right now and which of them it is
		 * running at.
		 *
		 * @remarks
		 * - A manager with nothing to choose offers the single point it is
		 *   putting out, which the bitrate allocation takes off the top instead
		 *   of bidding for.
		 */
		virtual RTC::BWE::BitrateAllocator::Candidate::BitrateOptions GetBitrateOptions(
		  int64_t nowMs) const = 0;

		/**
		 * Run at the given point from now on.
		 */
		virtual void SetBitrateOption(const RTC::BWE::BitrateAllocator::Candidate::BitrateOption& option) = 0;

		virtual int64_t GetDesiredBitrate(int64_t nowMs) const = 0;

		virtual RtpPacketProcessResult ProcessRtpPacket(
		  RTC::RTP::Packet* packet,
		  bool lastSentPacketHasMarker,
		  uint32_t clockRate,
		  uint32_t maxPacketTs) = 0;

		virtual void RequestKeyFrame() = 0;

		virtual void RequestKeyFrameForTargetSpatialLayer() = 0;

		virtual void RequestKeyFrameForCurrentSpatialLayer() = 0;

		virtual void UpdateTargetLayers(int16_t newTargetSpatialLayer, int16_t newTargetTemporalLayer) = 0;
		virtual bool RecalculateTargetLayers(RTC::ConsumerTypes::VideoLayers& newTargetLayers) const = 0;

		void MayChangeLayers();

		virtual void OnTransportConnected() = 0;

		virtual void OnTransportDisconnected() = 0;

		virtual void OnPaused() = 0;

		virtual void OnResumed() = 0;

	protected:
		virtual bool IsActive() const = 0;

		/**
		 * Which of the given points is the one the bitrate allocation last
		 * settled on.
		 *
		 * @returns No value if it has settled on none yet or if the point it
		 *   settled on is not on offer anymore, which is what tells the
		 *   allocation that this one is not climbing back to anything.
		 */
		std::optional<size_t> GetCurrentBitrateOption(
		  const std::vector<RTC::BWE::BitrateAllocator::Candidate::BitrateOption>& options) const;

	protected:
		// Passed by argument.
		Listener* const listener;
		SharedInterface* const shared;
		const bool keyFrameSupported;
		const RTC::Media::Kind kind;
		const std::vector<RTC::RtpEncodingParameters> consumableRtpEncodings;
		// Encoding context.
		const std::unique_ptr<RTC::RTP::Codecs::EncodingContext> encodingContext;
		// Layer preferences.
		RTC::ConsumerTypes::VideoLayers preferredLayers;
		// Whether a bitrate allocation is running for this Transport. While it is
		// false nobody would answer a round, so this one applies the layers it works
		// out instead of asking.
		bool bweManaged{ false };
		// Whether the bitrate allocation has ever given this one a point to run at.
		// Telling that from "it was left sending nothing" is what makes the latter
		// pay the margin to climb back and the former not, since one that has never
		// been given anything is not climbing back to anything.
		bool bitrateAllocated{ false };
		// Sync state.
		bool syncRequired{ false };
	};
} // namespace RTC

#endif
