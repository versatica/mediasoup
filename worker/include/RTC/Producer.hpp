#ifndef MS_RTC_PRODUCER_HPP
#define MS_RTC_PRODUCER_HPP

#include "common.hpp"
#include "json.hpp"
#include "Channel/Request.hpp"
#include "RTC/RTCP/CompoundPacket.hpp"
#include "RTC/RTCP/Feedback.hpp"
#include "RTC/RTCP/ReceiverReport.hpp"
#include "RTC/RtpDictionaries.hpp"
#include "RTC/RtpHeaderExtensionIds.hpp"
#include "RTC/RtpPacket.hpp"
#include "RTC/RtpStream.hpp"
#include "RTC/RtpStreamRecv.hpp"
#include "handles/Timer.hpp"
#include <map>
#include <string>
#include <vector>

namespace RTC
{
	class Producer : public RtpStreamRecv::Listener, public Timer::Listener
	{
	public:
		class Listener
		{
		public:
			virtual void OnProducerPaused(RTC::Producer* producer)                            = 0;
			virtual void OnProducerResumed(RTC::Producer* producer)                           = 0;
			virtual void OnProducerRtpPacket(RTC::Producer* producer, RTC::RtpPacket* packet) = 0;
			virtual void OnProducerStreamEnabled(
			  RTC::Producer* producer, const RTC::RtpStream* rtpStream, uint32_t mappedSsrc) = 0;
			virtual void OnProducerStreamDisabled(
			  RTC::Producer* producer, const RTC::RtpStream* rtpStream, uint32_t mappedSsrc) = 0;
		};

	public:
		struct RtpMapping
		{
			std::map<uint8_t, uint8_t> codecPayloadTypes;
			std::map<uint8_t, uint8_t> headerExtensionIds;
		};

	private:
		struct RtpStreamInfo
		{
			RTC::RtpStreamRecv* rtpStream{ nullptr };
			std::string rid{};
			RTC::RtpEncodingParameters::Profile profile{ RTC::RtpEncodingParameters::Profile::NONE };
			uint32_t rtxSsrc{ 0 };
			bool active{ false };
		};

	public:
		Producer(
		  std::string& id,
		  Listener* listener,
		  RTC::Media::Kind kind,
		  RTC::RtpParameters& rtpParameters,
		  struct RtpMapping& rtpMapping);
		virtual ~Producer();

	public:
		void FillJson(json& jsonObject) const;
		void FillJsonStats(json& jsonObject) const = 0;
		void Pause();
		void Resume();
		const RTC::RtpParameters& GetRtpParameters() const;
		const struct RTC::RtpHeaderExtensionIds& GetRtpHeaderExtensionIds() const;
		bool IsPaused() const;
		void ReceiveRtpPacket(RTC::RtpPacket* packet);
		void ReceiveRtcpSenderReport(RTC::RTCP::SenderReport* report);
		void GetRtcp(RTC::RTCP::CompoundPacket* packet, uint64_t now);
		void RequestKeyFrame(bool force = false);
		const std::map<RTC::RtpEncodingParameters::Profile, const RTC::RtpStream*>& GetActiveProfiles() const;

	private:
		void FillRtpHeaderExtensionIds();
		void MayNeedNewStream(RTC::RtpPacket* packet);
		void CreateRtpStream(RTC::RtpEncodingParameters& encoding, uint32_t ssrc);
		void ApplyRtpMapping(RTC::RtpPacket* packet) const;
		void ActivateStream(RTC::RtpStreamRecv* rtpStream);
		void DeactivateStream(RTC::RtpStreamRecv* rtpStream);
		// TODO: Remove?
		bool IsStreamActive(const RTC::RtpStream* rtpStream) const;

		/* Pure virtual methods inherited from RTC::RtpStreamRecv::Listener. */
	public:
		void OnRtpStreamRecvNackRequired(
		  RTC::RtpStreamRecv* rtpStream, const std::vector<uint16_t>& seqNumbers) override;
		void OnRtpStreamRecvPliRequired(RTC::RtpStreamRecv* rtpStream) override;
		void OnRtpStreamRecvFirRequired(RTC::RtpStreamRecv* rtpStream) override;
		void OnRtpStreamInactive(RTC::RtpStream* rtpStream) override;
		void OnRtpStreamActive(RTC::RtpStream* rtpStream) override;

		/* Pure virtual methods inherited from Timer::Listener. */
	public:
		void OnTimer(Timer* timer) override;

	public:
		// Passed by argument.
		const std::string id;
		Listener* listener{ nullptr };
		RTC::Media::Kind kind;

	private:
		// Passed by argument.
		RTC::RtpParameters rtpParameters;
		struct RtpMapping rtpMapping;
		// Allocated by this.
		std::map<uint32_t, RtpStreamInfo> mapSsrcRtpStreamInfo;
		std::map<RTC::RtpEncodingParameters::Profile, const RTC::RtpStream*> mapActiveProfiles;
		Timer* keyFrameRequestBlockTimer{ nullptr };
		// Others.
		std::vector<RtpEncodingParameters> outputEncodings;
		struct RTC::RtpHeaderExtensionIds rtpHeaderExtensionIds;
		bool paused{ false };
		bool isKeyFrameRequested{ false };
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

	inline void Producer::ReceiveRtcpSenderReport(RTC::RTCP::SenderReport* report)
	{
		auto it = this->mapSsrcRtpStreamInfo.find(report->GetSsrc());
		if (it != this->mapSsrcRtpStreamInfo.end())
		{
			auto& info      = it->second;
			auto* rtpStream = info.rtpStream;

			rtpStream->ReceiveRtcpSenderReport(report);
		}
	}

	inline const std::map<RTC::RtpEncodingParameters::Profile, const RTC::RtpStream*>& Producer::
	  GetActiveProfiles() const
	{
		return this->mapActiveProfiles;
	}
} // namespace RTC

#endif
