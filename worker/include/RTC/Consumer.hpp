#ifndef MS_RTC_CONSUMER_HPP
#define MS_RTC_CONSUMER_HPP

#include "common.hpp"
#include "Channel/ChannelRequest.hpp"
#include "Channel/ChannelSocket.hpp"
#include "FBS/consumer.h"
#include "FBS/transport.h"
#include "RTC/ConsumerTypes.hpp"
#include "RTC/ProducerStreamManager.hpp"
#include "RTC/RTCP/CompoundPacket.hpp"
#include "RTC/RTCP/FeedbackRtpNack.hpp"
#include "RTC/RTCP/ReceiverReport.hpp"
#include "RTC/RTP/HeaderExtensionIds.hpp"
#include "RTC/RTP/Packet.hpp"
#include "RTC/RTP/RtpStreamRecv.hpp"
#include "RTC/RTP/RtpStreamSend.hpp"
#include "RTC/RTP/SharedPacket.hpp"
#include "RTC/RtpDictionaries.hpp"
#include "RTC/SeqManager.hpp"
#include "Shared.hpp"
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
	                 public RTC::ProducerStreamManager::Listener
	{
		using RetransmissionBuffer =
		  std::map<uint16_t, RTC::RTP::SharedPacket, RTC::SeqManager<uint16_t>::SeqLowerThan>;

	public:
		class Listener
		{
		public:
			virtual ~Listener() = default;

		public:
			virtual void OnConsumerSendRtpPacket(RTC::Consumer* consumer, RTC::RTP::Packet* packet) = 0;
			virtual void OnConsumerRetransmitRtpPacket(RTC::Consumer* consumer, RTC::RTP::Packet* packet) = 0;
			virtual void OnConsumerKeyFrameRequested(RTC::Consumer* consumer, uint32_t mappedSsrc) = 0;
			virtual void OnConsumerNeedBitrateChange(RTC::Consumer* consumer)                      = 0;
			virtual void OnConsumerNeedZeroBitrate(RTC::Consumer* consumer)                        = 0;
			virtual void OnConsumerProducerClosed(RTC::Consumer* consumer)                         = 0;
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

	public:
		Consumer(
		  SharedInterface* shared,
		  const std::string& id,
		  const std::string& producerId,
		  RTC::Consumer::Listener* listener,
		  const FBS::Transport::ConsumeRequest* data);
		~Consumer() override;

	public:
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
		const struct RTC::RTP::HeaderExtensionIds& GetRtpHeaderExtensionIds() const
		{
			return this->rtpHeaderExtensionIds;
		}
		RTC::RtpParameters::Type GetType() const
		{
			return this->type;
		}
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
		void ProducerClosed();
		void SetExternallyManagedBitrate()
		{
			this->externallyManagedBitrate = true;
			this->producerStreamManager->SetExternallyManagedBitrate();
		}
		uint8_t GetBitratePriority() const;
		uint32_t IncreaseLayer(uint32_t bitrate, bool considerLoss);
		void ApplyLayers();
		uint32_t GetDesiredBitrate() const;
		void SendRtpPacket(RTC::RTP::Packet* packet, RTC::RTP::SharedPacket& sharedPacket);
		bool GetRtcp(RTC::RTCP::CompoundPacket* packet, uint64_t nowMs);
		void NeedWorstRemoteFractionLost(uint32_t mappedSsrc, uint8_t& worstRemoteFractionLost);
		void ReceiveNack(RTC::RTCP::FeedbackRtpNackPacket* nackPacket);
		void ReceiveKeyFrameRequest(RTC::RTCP::FeedbackPs::MessageType messageType, uint32_t ssrc);
		void ReceiveRtcpReceiverReport(RTC::RTCP::ReceiverReport* report);
		void ReceiveRtcpXrReceiverReferenceTime(RTC::RTCP::ReceiverReferenceTime* report);
		uint32_t GetTransmissionRate(uint64_t nowMs);
		float GetRtt() const;

		/* Methods inherited from Channel::ChannelSocket::RequestHandler. */
	public:
		void HandleRequest(Channel::ChannelRequest* request) override;

	protected:
		void EmitTraceEventRtpAndKeyFrameTypes(const RTC::RTP::Packet* packet, bool isRtx = false) const;
		void EmitTraceEventPliType(uint32_t ssrc) const;
		void EmitTraceEventFirType(uint32_t ssrc) const;
		void EmitTraceEventNackType() const;
		void EmitTraceEvent(flatbuffers::Offset<FBS::Consumer::TraceNotification>& notification) const;

	private:
		void EmitScore() const;
		void EmitLayersChange() const;

	private:
		void UserOnTransportConnected();
		void UserOnTransportDisconnected();
		void UserOnPaused();
		void UserOnResumed();

	private:
		void CreateRtpStreams();
		static void StorePacketInTargetLayerRetransmissionBuffer(
		  RetransmissionBuffer& targetLayerRetransmissionBuffer,
		  RTC::RTP::Packet* packet,
		  RTC::RTP::SharedPacket& sharedPacket);

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
		std::string id;
		std::string producerId;

	protected:
		// Passed by argument.
		SharedInterface* shared{ nullptr };
		RTC::Consumer::Listener* listener{ nullptr };
		RTC::Media::Kind kind;
		RTC::RtpParameters rtpParameters;
		RTC::RtpParameters::Type type;
		std::vector<RTC::RtpEncodingParameters> consumableRtpEncodings;
		struct RTC::RTP::HeaderExtensionIds rtpHeaderExtensionIds;
		const std::vector<uint8_t>* producerRtpStreamScores{ nullptr };
		// Others.
		std::bitset<128u> supportedCodecPayloadTypes;
		uint64_t lastRtcpSentTime{ 0u };
		uint16_t maxRtcpInterval{ 0u };
		bool externallyManagedBitrate{ false };
		uint8_t priority{ 1u };
		struct TraceEventTypes traceEventTypes;

	private:
		bool pipe{ false };
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
		bool paused{ false };
		bool producerPaused{ false };
		bool producerClosed{ false };
		bool lastSentPacketHasMarker{ false };
		std::unique_ptr<RTC::ProducerStreamManager> producerStreamManager;
	};
} // namespace RTC

#endif
