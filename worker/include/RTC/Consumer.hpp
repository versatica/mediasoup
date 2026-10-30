#ifndef MS_RTC_CONSUMER_HPP
#define MS_RTC_CONSUMER_HPP

#include "common.hpp"
#include "Channel/ChannelRequest.hpp"
#include "Channel/ChannelSocket.hpp"
#include "FBS/consumer.h"
#include "FBS/transport.h"
#include "RTC/BWE/BitrateAllocator.hpp"
#include "RTC/ConsumerTypes.hpp"
#include "RTC/ProducerStreamManager.hpp"
#include "RTC/RTCP/CompoundPacket.hpp"
#include "RTC/RTCP/Feedback.hpp"
#include "RTC/RTCP/FeedbackRtpNack.hpp"
#include "RTC/RTCP/ReceiverReport.hpp"
#include "RTC/RTP/HeaderExtensionIds.hpp"
#include "RTC/RTP/Packet.hpp"
#include "RTC/RTP/RtpStreamRecv.hpp"
#include "RTC/RTP/RtpStreamSend.hpp"
#include "RTC/RtpDictionaries.hpp"
#include "RTC/SeqManager.hpp"
#include "SharedInterface.hpp"
#include <ankerl/unordered_dense.h>
#include <bitset>
#include <map>
#include <string>
#include <vector>

namespace RTC
{
	class Consumer : public Channel::ChannelSocket::RequestHandler,
	                 public RTC::RTP::RtpStreamSend::Listener,
	                 public RTC::BWE::BitrateAllocator::Candidate,
	                 public RTC::ProducerStreamManager::Listener
	{
		using RetransmissionBuffer =
		  std::map<uint16_t, std::shared_ptr<RTC::RTP::Packet>, RTC::SeqManager<uint16_t>::SeqLowerThan>;

	public:
		class Listener
		{
		public:
			virtual ~Listener() = default;

		public:
			virtual void OnConsumerSendRtpPacket(RTC::Consumer* consumer, RTC::RTP::Packet* packet) = 0;

			virtual void OnConsumerRetransmitRtpPacket(RTC::Consumer* consumer, RTC::RTP::Packet* packet) = 0;

			virtual void OnConsumerKeyFrameRequested(RTC::Consumer* consumer, uint32_t mappedSsrc) = 0;

			virtual void OnConsumerNeedBitrateChange(RTC::Consumer* consumer) = 0;

			virtual void OnConsumerProducerClosed(RTC::Consumer* consumer) = 0;
		};

	private:
		struct TraceEventTypes
		{
			bool rtp{ false };
			bool keyframe{ false };
			bool nack{ false };
			bool pli{ false };
			bool fir{ false };
		};

	private:
		static void StorePacketInTargetLayerRetransmissionBuffer(
		  RetransmissionBuffer& targetLayerRetransmissionBuffer,
		  RTC::RTP::Packet* packet,
		  std::shared_ptr<RTC::RTP::Packet>& sharedPacket);

	public:
		Consumer(
		  SharedInterface* shared,
		  const std::string& id,
		  const std::string& producerId,
		  RTC::Consumer::Listener* listener,
		  const FBS::Transport::ConsumeRequest* data);
		~Consumer() override;

		flatbuffers::Offset<FBS::Consumer::DumpResponse> FillBuffer(
		  flatbuffers::FlatBufferBuilder& builder) const;

		flatbuffers::Offset<FBS::Consumer::BaseConsumerDump> FillBufferBase(
		  flatbuffers::FlatBufferBuilder& builder) const;

		flatbuffers::Offset<FBS::Consumer::GetStatsResponse> FillBufferStats(
		  flatbuffers::FlatBufferBuilder& builder);

		flatbuffers::Offset<FBS::Consumer::ConsumerScore> FillBufferScore(
		  flatbuffers::FlatBufferBuilder& builder) const;

		RTC::Media::Kind GetKind() const
		{
			return this->kind;
		}

		const RTC::RtpParameters& GetRtpParameters() const
		{
			return this->rtpParameters;
		}

		const RTC::RTP::HeaderExtensionIds& GetRtpHeaderExtensionIds() const
		{
			return this->rtpHeaderExtensionIds;
		}

		/**
		 * Whether this Consumer negotiated the Dependency Descriptor header
		 * extension. Only then is the payload descriptor made out of it
		 * used for this Consumer.
		 */
		bool PrefersDependencyDescriptor() const
		{
			return this->rtpHeaderExtensionIds.dependencyDescriptor != 0;
		}

		RTC::RtpParameters::Type GetType() const
		{
			return this->type;
		}

		/**
		 * Whether this Consumer was negotiated with transport-cc, which takes both
		 * the RTP header extension and the RTCP feedback.
		 */
		bool SupportsTransportCc() const;
		/**
		 * Whether this Consumer was negotiated with REMB, whose RTP header extension
		 * is abs-send-time.
		 */

		bool SupportsRemb() const;

		RTC::ConsumerTypes::VideoLayers GetPreferredLayers() const
		{
			return this->producerStreamManager->GetPreferredLayers();
		}

		const std::vector<uint32_t>& GetMediaSsrcs() const
		{
			return this->mediaSsrcs;
		}

		const std::vector<uint32_t>& GetRtxSsrcs() const
		{
			return this->rtxSsrcs;
		}

		bool IsActive() const override
		{
			// clang-format off
			return (
				this->transportConnected &&
				!this->paused &&
				!this->producerPaused &&
				!this->producerClosed
			);
			// clang-format on
		}

		void TransportConnected();

		void TransportDisconnected();

		bool IsPaused() const
		{
			return this->paused;
		}

		bool IsProducerPaused() const
		{
			return this->producerPaused;
		}

		void ProducerPaused();

		void ProducerResumed();

		void ProducerRtpStream(RTC::RTP::RtpStreamRecv* rtpStream, uint32_t mappedSsrc);

		void ProducerNewRtpStream(RTC::RTP::RtpStreamRecv* rtpStream, uint32_t mappedSsrc);

		void ProducerRtpStreamScores(const std::vector<uint8_t>* scores);

		void ProducerRtpStreamScore(RTC::RTP::RtpStreamRecv* rtpStream, uint8_t score, uint8_t previousScore);

		void ProducerRtcpSenderReport(RTC::RTP::RtpStreamRecv* rtpStream, bool first);

		void ProducerSpatialLayerActivityChanged(
		  RTC::RTP::RtpStreamRecv* rtpStream, uint8_t spatialLayer, bool isActive);

		void ProducerClosed();

		/**
		 * Tell whether a bitrate allocation is running for this Transport, which is
		 * what decides whether this one asks for bitrate or helps itself.
		 *
		 * @remarks
		 * - While it is false there is nobody to answer a round, so nothing about
		 *   bitrate or layers is told to the Transport.
		 */
		void SetBweManaged(bool bweManaged)
		{
			this->bweManaged = bweManaged;
			this->producerStreamManager->SetBweManaged(bweManaged);
		}

		const std::string& GetBitrateAllocationId() const override;

		uint8_t GetBitrateWeight() const override;

		RTC::BWE::BitrateAllocator::Candidate::BitrateOptions GetBitrateOptions() const override;

		void SetBitrateOption(const RTC::BWE::BitrateAllocator::Candidate::BitrateOption& option) override;

		int64_t GetDesiredBitrate() const;

		void SendRtpPacket(RTC::RTP::Packet* packet, std::shared_ptr<RTC::RTP::Packet>& sharedPacket);

		bool GetRtcp(RTC::RTCP::CompoundPacket* packet, int64_t nowUs);

		/**
		 * Worst remote fraction lost among the RTP streams of this Consumer.
		 */
		uint8_t GetWorstRemoteFractionLost(uint32_t mappedSsrc) const;

		void ReceiveNack(RTC::RTCP::FeedbackRtpNackPacket* nackPacket);

		void ReceiveKeyFrameRequest(RTC::RTCP::FeedbackPs::MessageType messageType, uint32_t ssrc);

		/**
		 * @returns What this Receiver Report says about the link to the remote
		 *   endpoint.
		 */
		RTC::RTP::RtpStreamSend::ReceiverReportResult ReceiveRtcpReceiverReport(
		  RTC::RTCP::ReceiverReport* report, int64_t receivedAtUs);

		void ReceiveRtcpXrReceiverReferenceTime(
		  RTC::RTCP::ReceiverReferenceTime* report, int64_t receivedAtUs);

		int64_t GetTransmissionRate(int64_t nowMs);

		/**
		 * Longest round trip time measured on the RTP streams of this Consumer, or
		 * no value while none of them has measured any.
		 */
		std::optional<float> GetRttMs() const;

		/* Methods inherited from Channel::ChannelSocket::RequestHandler. */
	public:
		void HandleRequest(Channel::ChannelRequest* request) override;

	private:
		void EmitTraceEventRtpAndKeyFrameTypes(const RTC::RTP::Packet* packet, bool isRtx = false) const;

		void EmitTraceEventPliType(uint32_t ssrc) const;

		void EmitTraceEventFirType(uint32_t ssrc) const;

		void EmitTraceEventNackType() const;

		void EmitTraceEvent(flatbuffers::Offset<FBS::Consumer::TraceNotification>& notification) const;

		void EmitScore() const;

		void EmitLayersChange() const;

	private:
		void UserOnTransportConnected();

		void UserOnTransportDisconnected();

		void UserOnPaused();

		void UserOnResumed();

	private:
		void CreateRtpStreams();

		/* Pure virtual methods inherited from RtpStreamSend::Listener. */
	public:
		void OnRtpStreamScore(RTC::RTP::RtpStream* rtpStream, uint8_t score, uint8_t previousScore) override;

		void OnRtpStreamRetransmitRtpPacket(
		  RTC::RTP::RtpStreamSend* rtpStream, RTC::RTP::Packet* packet) override;

		/* Pure virtual methods inherited from ProducerStreamManager::Listener. */
	public:
		void OnProducerStreamManagerKeyFrameRequested(uint32_t mappedSsrc) override;

		void OnProducerStreamManagerNeedBitrateChange() override;

		void OnProducerStreamManagerLayersChanged() override;

		void OnProducerStreamManagerClearRetransmissionBuffer() override;

		void OnProducerStreamManagerScore() override;

	public:
		// Passed by argument.
		const std::string id;
		const std::string producerId;

	private:
		// Passed by argument.
		SharedInterface* const shared;
		RTC::Consumer::Listener* const listener;
		const RTC::Media::Kind kind;
		RTC::RtpParameters rtpParameters;
		RTC::RtpParameters::Type type;
		std::vector<RTC::RtpEncodingParameters> consumableRtpEncodings;
		RTC::RTP::HeaderExtensionIds rtpHeaderExtensionIds;
		const std::vector<uint8_t>* producerRtpStreamScores{ nullptr };
		// Others.
		std::bitset<128> supportedCodecPayloadTypes;
		int64_t lastRtcpSentAtUs{ 0 };
		int64_t maxRtcpIntervalMs{ 0 };
		// Whether a bitrate allocation is running for this Transport. While it is
		// false nobody would answer a round, so this one is told nothing about
		// bitrate and its manager applies the layers it works out.
		bool bweManaged{ false };
		uint8_t priority{ 1 };
		TraceEventTypes traceEventTypes;
		const bool pipe;
		// Others.
		std::vector<RTC::RTP::RtpStreamSend*> rtpStreams;
		ankerl::unordered_dense::map<uint32_t, uint32_t> mapMappedSsrcSsrc;
		ankerl::unordered_dense::map<uint32_t, RTC::RTP::RtpStreamSend*> mapSsrcRtpStream;
		ankerl::unordered_dense::map<RTC::RTP::RtpStreamSend*, RTC::SeqManager<uint16_t>> mapRtpStreamRtpSeqManager;
		// Buffers to store packets that arrive earlier than the first packet of the
		// video key frame.
		ankerl::unordered_dense::map<RTC::RTP::RtpStreamSend*, RetransmissionBuffer>
		  mapRtpStreamTargetLayerRetransmissionBuffer;
		std::vector<uint32_t> mediaSsrcs;
		std::vector<uint32_t> rtxSsrcs;
		bool transportConnected{ false };
		bool paused;
		bool producerPaused{ false };
		bool producerClosed{ false };
		bool lastSentPacketHasMarker{ false };
		std::unique_ptr<RTC::ProducerStreamManager> producerStreamManager;
	};
} // namespace RTC

#endif
