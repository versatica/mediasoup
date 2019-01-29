#ifndef MS_RTC_PRODUCER_HPP
#define MS_RTC_PRODUCER_HPP

#include "common.hpp"
#include "json.hpp"
#include "Channel/Request.hpp"
#include "RTC/KeyFrameRequestManager.hpp"
#include "RTC/RTCP/CompoundPacket.hpp"
#include "RTC/RTCP/Feedback.hpp"
#include "RTC/RTCP/ReceiverReport.hpp"
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
	class Producer : public RtpStreamRecv::Listener, public KeyFrameRequestManager::Listener
	{
	public:
		class Listener
		{
		public:
			virtual void OnProducerPaused(RTC::Producer* producer)  = 0;
			virtual void OnProducerResumed(RTC::Producer* producer) = 0;
			virtual void OnProducerRtpStreamHealthy(
			  RTC::Producer* producer, RTC::RtpStream* rtpStream, uint32_t mappedSsrc) = 0;
			virtual void OnProducerRtpStreamUnhealthy(
			  RTC::Producer* producer, RTC::RtpStream* rtpStream, uint32_t mappedSsrc)                = 0;
			virtual void OnProducerRtpPacketReceived(RTC::Producer* producer, RTC::RtpPacket* packet) = 0;
			virtual void OnProducerSendRtcpPacket(RTC::Producer* producer, RTC::RTCP::Packet* packet) = 0;
		};

	private:
		struct RtpEncodingMapping
		{
			std::string rid;
			uint32_t ssrc{ 0 };
			uint32_t rtxSsrc{ 0 };
			uint32_t mappedSsrc{ 0 };
		};

	private:
		struct RtpMapping
		{
			std::map<uint8_t, uint8_t> codecs;
			std::map<uint8_t, uint8_t> headerExtensions;
			std::vector<RtpEncodingMapping> encodings;
		};

	public:
		Producer(const std::string& id, Listener* listener, json& data);
		virtual ~Producer();

	public:
		void FillJson(json& jsonObject) const;
		void FillJsonStats(json& jsonArray) const;
		void HandleRequest(Channel::Request* request);
		const RTC::RtpParameters& GetRtpParameters() const;
		const struct RTC::RtpHeaderExtensionIds& GetRtpHeaderExtensionIds() const;
		bool IsPaused() const;
		std::map<RTC::RtpStreamRecv*, uint32_t>& GetHealthyRtpStreams();
		void ReceiveRtpPacket(RTC::RtpPacket* packet);
		void ReceiveRtcpSenderReport(RTC::RTCP::SenderReport* report);
		void GetRtcp(RTC::RTCP::CompoundPacket* packet, uint64_t now);
		void RequestKeyFrame(uint32_t mappedSsrc);

	private:
		RTC::RtpStreamRecv* GetRtpStream(RTC::RtpPacket* packet);
		RTC::RtpStreamRecv* CreateRtpStream(
		  uint32_t ssrc, const RTC::RtpCodecParameters& codec, size_t encodingIdx);
		void SetHealthyRtpStream(RTC::RtpStreamRecv* rtpStream);
		void SetUnhealthyRtpStream(RTC::RtpStreamRecv* rtpStream);
		bool MangleRtpPacket(RTC::RtpPacket* packet, RTC::RtpStreamRecv* rtpStream) const;

		/* Pure virtual methods inherited from RTC::RtpStreamRecv::Listener. */
	public:
		void OnRtpStreamRecvNackRequired(
		  RTC::RtpStreamRecv* rtpStream, const std::vector<uint16_t>& seqNumbers) override;
		void OnRtpStreamRecvPliRequired(RTC::RtpStreamRecv* rtpStream) override;
		void OnRtpStreamRecvFirRequired(RTC::RtpStreamRecv* rtpStream) override;
		void OnRtpStreamHealthy(RTC::RtpStream* rtpStream) override;
		void OnRtpStreamUnhealthy(RTC::RtpStream* rtpStream) override;

		/* Pure virtual methods inherited from RTC::KeyFrameRequestManager::Listener. */
	public:
		void OnKeyFrameNeeded(KeyFrameRequestManager* keyFrameRequestManager, uint32_t ssrc) override;

	public:
		// Passed by argument.
		const std::string id;

	private:
		// Passed by argument.
		Listener* listener{ nullptr };
		// Allocated by this.
		std::map<uint32_t, RTC::RtpStreamRecv*> mapSsrcRtpStream;
		RTC::KeyFrameRequestManager* keyFrameRequestManager{ nullptr };
		// Others.
		RTC::Media::Kind kind;
		RTC::RtpParameters rtpParameters;
		struct RtpMapping rtpMapping;
		std::map<uint32_t, RTC::RtpStreamRecv*> mapRtxSsrcRtpStream;
		std::map<RTC::RtpStreamRecv*, uint32_t> mapRtpStreamMappedSsrc;
		std::map<uint32_t, uint32_t> mapMappedSsrcSsrc;
		std::map<RTC::RtpStreamRecv*, uint32_t> mapHealthyRtpStreamMappedSsrc;
		struct RTC::RtpHeaderExtensionIds rtpHeaderExtensionIds;
		struct RTC::RtpHeaderExtensionIds mappedRtpHeaderExtensionIds;
		bool paused{ false };
		// Timestamp when last RTCP was sent.
		uint64_t lastRtcpSentTime{ 0 };
		uint16_t maxRtcpInterval{ 0 };
	};

	/* Inline methods. */

	inline const RTC::RtpParameters& Producer::GetRtpParameters() const
	{
		return this->rtpParameters;
	}

	inline const struct RTC::RtpHeaderExtensionIds& Producer::GetRtpHeaderExtensionIds() const
	{
		return this->rtpHeaderExtensionIds;
	}

	inline bool Producer::IsPaused() const
	{
		return this->paused;
	}

	inline std::map<RTC::RtpStreamRecv*, uint32_t>& Producer::GetHealthyRtpStreams()
	{
		return this->mapHealthyRtpStreamMappedSsrc;
	}
} // namespace RTC

#endif
