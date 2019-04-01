#ifndef MS_RTC_PRODUCER_HPP
#define MS_RTC_PRODUCER_HPP

#include "common.hpp"
#include "json.hpp"
#include "Channel/Request.hpp"
#include "RTC/KeyFrameRequestManager.hpp"
#include "RTC/RTCP/CompoundPacket.hpp"
#include "RTC/RTCP/Packet.hpp"
#include "RTC/RTCP/SenderReport.hpp"
#include "RTC/RtpDictionaries.hpp"
#include "RTC/RtpHeaderExtensionIds.hpp"
#include "RTC/RtpPacket.hpp"
#include "RTC/RtpStream.hpp"
#include "RTC/RtpStreamRecv.hpp"
#include <map>
#include <string>
#include <vector>

namespace RTC
{
	class Producer : public RTC::RtpStreamRecv::Listener, public RTC::KeyFrameRequestManager::Listener
	{
	public:
		class Listener
		{
		public:
			virtual void OnProducerPaused(RTC::Producer* producer)  = 0;
			virtual void OnProducerResumed(RTC::Producer* producer) = 0;
			virtual void OnProducerNewRtpStream(
			  RTC::Producer* producer, RTC::RtpStream* rtpStream, uint32_t mappedSsrc) = 0;
			virtual void OnProducerRtpStreamScore(
			  RTC::Producer* producer, RTC::RtpStream* rtpStream, uint8_t score)                      = 0;
			virtual void OnProducerRtpPacketReceived(RTC::Producer* producer, RTC::RtpPacket* packet) = 0;
			virtual void OnProducerSendRtcpPacket(RTC::Producer* producer, RTC::RTCP::Packet* packet) = 0;
			virtual void OnProducerNeedWorstRemoteFractionLost(
			  RTC::Producer* producer, uint32_t mappedSsrc, uint8_t& worstRemoteFractionLost) = 0;
		};

	private:
		struct RtpEncodingMapping
		{
			std::string rid;
			uint32_t ssrc{ 0 };
			uint32_t mappedSsrc{ 0 };
		};

	private:
		struct RtpMapping
		{
			std::map<uint8_t, uint8_t> codecs;
			std::map<uint8_t, uint8_t> headerExtensions;
			std::vector<RtpEncodingMapping> encodings;
		};

	private:
		struct VideoOrientation
		{
			bool camera{ false };
			bool flip{ false };
			uint16_t rotation{ 0 };
		};

	public:
		Producer(const std::string& id, RTC::Producer::Listener* listener, json& data);
		virtual ~Producer();

	public:
		void FillJson(json& jsonObject) const;
		void FillJsonStats(json& jsonArray) const;
		void HandleRequest(Channel::Request* request);
		RTC::Media::Kind GetKind() const;
		const RTC::RtpParameters& GetRtpParameters() const;
		const struct RTC::RtpHeaderExtensionIds& GetRtpHeaderExtensionIds() const;
		RTC::RtpParameters::Type GetType() const;
		bool IsPaused() const;
		std::map<RTC::RtpStreamRecv*, uint32_t>& GetRtpStreams();
		void ReceiveRtpPacket(RTC::RtpPacket* packet);
		void ReceiveRtcpSenderReport(RTC::RTCP::SenderReport* report);
		void GetRtcp(RTC::RTCP::CompoundPacket* packet, uint64_t now);
		void RequestKeyFrame(uint32_t mappedSsrc);

	private:
		RTC::RtpStreamRecv* GetRtpStream(RTC::RtpPacket* packet);
		RTC::RtpStreamRecv* CreateRtpStream(
		  uint32_t ssrc, const RTC::RtpCodecParameters& mediaCodec, size_t encodingIdx);
		bool MangleRtpPacket(RTC::RtpPacket* packet, RTC::RtpStreamRecv* rtpStream) const;
		void PostProcessRtpPacket(RTC::RtpPacket* packet);
		void EmitScore() const;

		/* Pure virtual methods inherited from RTC::RtpStreamRecv::Listener. */
	public:
		void OnRtpStreamScore(RTC::RtpStream* rtpStream, uint8_t score) override;
		void OnRtpStreamSendRtcpPacket(RTC::RtpStreamRecv* rtpStream, RTC::RTCP::Packet* packet) override;
		void OnRtpStreamNeedWorstRemoteFractionLost(
		  RTC::RtpStreamRecv* rtpStream, uint8_t& worstRemoteFractionLost) override;

		/* Pure virtual methods inherited from RTC::KeyFrameRequestManager::Listener. */
	public:
		void OnKeyFrameNeeded(RTC::KeyFrameRequestManager* keyFrameRequestManager, uint32_t ssrc) override;

	public:
		// Passed by argument.
		const std::string id;

	private:
		// Passed by argument.
		RTC::Producer::Listener* listener{ nullptr };
		// Allocated by this.
		std::map<uint32_t, RTC::RtpStreamRecv*> mapSsrcRtpStream;
		RTC::KeyFrameRequestManager* keyFrameRequestManager{ nullptr };
		// Others.
		RTC::Media::Kind kind;
		RTC::RtpParameters rtpParameters;
		RTC::RtpParameters::Type type{ RTC::RtpParameters::Type::NONE };
		struct RtpMapping rtpMapping;
		std::map<uint32_t, RTC::RtpStreamRecv*> mapRtxSsrcRtpStream;
		std::map<RTC::RtpStreamRecv*, uint32_t> mapRtpStreamMappedSsrc;
		std::map<uint32_t, uint32_t> mapMappedSsrcSsrc;
		struct RTC::RtpHeaderExtensionIds rtpHeaderExtensionIds;
		struct RTC::RtpHeaderExtensionIds mappedRtpHeaderExtensionIds;
		bool paused{ false };
		// Timestamp when last RTCP was sent.
		uint64_t lastRtcpSentTime{ 0 };
		uint16_t maxRtcpInterval{ 0 };
		// Video orientation.
		bool videoOrientationDetected{ false };
		struct VideoOrientation videoOrientation;
	};

	/* Inline methods. */

	inline RTC::Media::Kind Producer::GetKind() const
	{
		return this->kind;
	}

	inline const RTC::RtpParameters& Producer::GetRtpParameters() const
	{
		return this->rtpParameters;
	}

	inline const struct RTC::RtpHeaderExtensionIds& Producer::GetRtpHeaderExtensionIds() const
	{
		return this->rtpHeaderExtensionIds;
	}

	inline RTC::RtpParameters::Type Producer::GetType() const
	{
		return this->type;
	}

	inline bool Producer::IsPaused() const
	{
		return this->paused;
	}

	inline std::map<RTC::RtpStreamRecv*, uint32_t>& Producer::GetRtpStreams()
	{
		return this->mapRtpStreamMappedSsrc;
	}
} // namespace RTC

#endif
