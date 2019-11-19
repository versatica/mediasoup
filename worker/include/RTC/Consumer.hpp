#ifndef MS_RTC_CONSUMER_HPP
#define MS_RTC_CONSUMER_HPP

#include "common.hpp"
#include "Channel/Request.hpp"
#include "RTC/RTCP/CompoundPacket.hpp"
#include "RTC/RTCP/FeedbackPs.hpp"
#include "RTC/RTCP/FeedbackPsFir.hpp"
#include "RTC/RTCP/FeedbackPsPli.hpp"
#include "RTC/RTCP/FeedbackRtpNack.hpp"
#include "RTC/RTCP/ReceiverReport.hpp"
#include "RTC/RtpDictionaries.hpp"
#include "RTC/RtpHeaderExtensionIds.hpp"
#include "RTC/RtpPacket.hpp"
#include "RTC/RtpStream.hpp"
#include "RTC/RtpStreamSend.hpp"
#include <json.hpp>
#include <string>
#include <unordered_set>
#include <vector>

using json = nlohmann::json;

namespace RTC
{
	class Consumer
	{
	public:
		class Listener
		{
		public:
			virtual void OnConsumerSendRtpPacket(RTC::Consumer* consumer, RTC::RtpPacket* packet) = 0;
			virtual void OnConsumerRetransmitRtpPacket(RTC::Consumer* consumer, RTC::RtpPacket* packet) = 0;
			virtual void OnConsumerKeyFrameRequested(RTC::Consumer* consumer, uint32_t mappedSsrc) = 0;
			virtual void OnConsumerNeedBitrateChange(RTC::Consumer* consumer)                      = 0;
			virtual void OnConsumerNeedZeroBitrate(RTC::Consumer* consumer)                        = 0;
			virtual void OnConsumerProducerClosed(RTC::Consumer* consumer)                         = 0;
		};

	public:
		struct Layers
		{
			int16_t spatial{ -1 };
			int16_t temporal{ -1 };
		};

	private:
		struct PacketEventTypes
		{
			bool rtp{ false };
			bool keyframe{ false };
			bool nack{ false };
			bool pli{ false };
			bool fir{ false };
		};

	public:
		Consumer(
		  const std::string& id,
		  RTC::Consumer::Listener* listener,
		  json& data,
		  RTC::RtpParameters::Type type);
		virtual ~Consumer();

	public:
		virtual void FillJson(json& jsonObject) const;
		virtual void FillJsonStats(json& jsonArray) const  = 0;
		virtual void FillJsonScore(json& jsonObject) const = 0;
		virtual void HandleRequest(Channel::Request* request);
		RTC::Media::Kind GetKind() const;
		const RTC::RtpParameters& GetRtpParameters() const;
		const struct RTC::RtpHeaderExtensionIds& GetRtpHeaderExtensionIds() const;
		RTC::RtpParameters::Type GetType() const;
		virtual Layers GetPreferredLayers() const;
		const std::vector<uint32_t>& GetMediaSsrcs() const;
		const std::vector<uint32_t>& GetRtxSsrcs() const;
		virtual bool IsActive() const;
		void TransportConnected();
		void TransportDisconnected();
		bool IsPaused() const;
		bool IsProducerPaused() const;
		void ProducerPaused();
		void ProducerResumed();
		virtual void ProducerRtpStream(RTC::RtpStream* rtpStream, uint32_t mappedSsrc)    = 0;
		virtual void ProducerNewRtpStream(RTC::RtpStream* rtpStream, uint32_t mappedSsrc) = 0;
		virtual void ProducerRtpStreamScore(
		  RTC::RtpStream* rtpStream, uint8_t score, uint8_t previousScore)           = 0;
		virtual void ProducerRtcpSenderReport(RTC::RtpStream* rtpStream, bool first) = 0;
		void ProducerClosed();
		void SetExternallyManagedBitrate();
		virtual uint8_t GetBitratePriority() const                          = 0;
		virtual uint32_t IncreaseLayer(uint32_t bitrate, bool considerLoss) = 0;
		virtual void ApplyLayers()                                          = 0;
		virtual uint32_t GetDesiredBitrate() const                          = 0;
		virtual void SendRtpPacket(RTC::RtpPacket* packet)                  = 0;
		virtual std::vector<RTC::RtpStreamSend*> GetRtpStreams()            = 0;
		virtual void GetRtcp(
		  RTC::RTCP::CompoundPacket* packet, RTC::RtpStreamSend* rtpStream, uint64_t nowMs) = 0;
		virtual void NeedWorstRemoteFractionLost(uint32_t mappedSsrc, uint8_t& worstRemoteFractionLost) = 0;
		virtual void ReceiveNack(RTC::RTCP::FeedbackRtpNackPacket* nackPacket) = 0;
		virtual void ReceiveKeyFrameRequest(
		  RTC::RTCP::FeedbackPs::MessageType messageType, uint32_t ssrc)          = 0;
		virtual void ReceiveRtcpReceiverReport(RTC::RTCP::ReceiverReport* report) = 0;
		virtual uint32_t GetTransmissionRate(uint64_t nowMs)                      = 0;
		virtual float GetRtt() const                                              = 0;

	protected:
		void EmitPacketEventRtpAndKeyFrameTypes(RTC::RtpPacket* packet, bool isRtx = false) const;
		void EmitPacketEventKeyFrameType(RTC::RtpPacket* packet, bool isRtx = false) const;
		void EmitPacketEventPliType(uint32_t ssrc) const;
		void EmitPacketEventFirType(uint32_t ssrc) const;
		void EmitPacketEventNackType() const;

	private:
		virtual void UserOnTransportConnected()    = 0;
		virtual void UserOnTransportDisconnected() = 0;
		virtual void UserOnPaused()                = 0;
		virtual void UserOnResumed()               = 0;

	public:
		// Passed by argument.
		const std::string id;

	protected:
		// Passed by argument.
		RTC::Consumer::Listener* listener{ nullptr };
		RTC::Media::Kind kind;
		RTC::RtpParameters rtpParameters;
		RTC::RtpParameters::Type type{ RTC::RtpParameters::Type::NONE };
		std::vector<RTC::RtpEncodingParameters> consumableRtpEncodings;
		struct RTC::RtpHeaderExtensionIds rtpHeaderExtensionIds;
		// Others.
		std::unordered_set<uint8_t> supportedCodecPayloadTypes;
		uint64_t lastRtcpSentTime{ 0u };
		uint16_t maxRtcpInterval{ 0u };
		bool externallyManagedBitrate{ false };
		uint8_t priority{ 1u };
		struct PacketEventTypes packetEventTypes;

	private:
		// Others.
		std::vector<uint32_t> mediaSsrcs;
		std::vector<uint32_t> rtxSsrcs;
		bool transportConnected{ false };
		bool paused{ false };
		bool producerPaused{ false };
		bool producerClosed{ false };
	};

	/* Inline methods. */

	inline RTC::Media::Kind Consumer::GetKind() const
	{
		return this->kind;
	}

	inline const RTC::RtpParameters& Consumer::GetRtpParameters() const
	{
		return this->rtpParameters;
	}

	inline const struct RTC::RtpHeaderExtensionIds& Consumer::GetRtpHeaderExtensionIds() const
	{
		return this->rtpHeaderExtensionIds;
	}

	inline RTC::RtpParameters::Type Consumer::GetType() const
	{
		return this->type;
	}

	inline Consumer::Layers Consumer::GetPreferredLayers() const
	{
		// By default return 1:1.
		Consumer::Layers layers;

		return layers;
	}

	inline const std::vector<uint32_t>& Consumer::GetMediaSsrcs() const
	{
		return this->mediaSsrcs;
	}

	inline const std::vector<uint32_t>& Consumer::GetRtxSsrcs() const
	{
		return this->rtxSsrcs;
	}

	inline bool Consumer::IsActive() const
	{
		// The parent Consumer just checks whether Consumer and Producer are
		// not paused and the transport connected.
		// clang-format off
		return (
			this->transportConnected &&
			!this->paused &&
			!this->producerPaused &&
			!this->producerClosed
		);
		// clang-format on
	}

	inline bool Consumer::IsPaused() const
	{
		return this->paused;
	}

	inline bool Consumer::IsProducerPaused() const
	{
		return this->producerPaused;
	}

	inline void Consumer::SetExternallyManagedBitrate()
	{
		this->externallyManagedBitrate = true;
	}
} // namespace RTC

#endif
